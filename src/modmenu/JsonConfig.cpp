#include "modmenu/internal/JsonConfig.hpp"

#include "nlohmann/json.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <memory>
#include <optional>
#include <type_traits>

namespace modmenu::detail {
namespace {

using json = nlohmann::ordered_json; // keeps the mod's own key order, both in the page and when the file is rewritten
namespace fs = std::filesystem;

struct File {
    fs::path path;
    json doc = json::object();
    fs::file_time_type stamp{};
    json hints = json::object(); // optional config/modmenu.json
    Notify notify;

    void refresh() {
        std::error_code ec;
        const auto t = fs::last_write_time(path, ec);
        if (ec || t == stamp) return;
        std::ifstream in(path);
        json j = json::parse(in, nullptr, false);
        if (!j.is_object()) return;
        doc = std::move(j);
        stamp = t;
    }

    const json* get(const json::json_pointer& p) {
        refresh();
        return doc.contains(p) ? &doc[p] : nullptr;
    }

    // Re read first so a save the mod made in the meantime is not clobbered.
    void set(const json::json_pointer& p, json v) {
        stamp = {};
        refresh();
        doc[p] = std::move(v);
        std::ofstream(path) << doc.dump(2);
        std::error_code ec;
        stamp = fs::last_write_time(path, ec);
        if (notify) notify(p.to_string().substr(1));
    }
};

// The value at p if it has the C++ type's JSON shape, otherwise def.
template <class T>
T Read(const std::shared_ptr<File>& f, const json::json_pointer& p, T def = {}) {
    const json* x = f->get(p);
    if (!x) return def;
    bool shaped;
    if constexpr (std::is_same_v<T, bool>) shaped = x->is_boolean();
    else if constexpr (std::is_integral_v<T>) shaped = x->is_number_integer();
    else if constexpr (std::is_floating_point_v<T>) shaped = x->is_number();
    else shaped = x->is_string();
    return shaped ? x->get<T>() : def;
}

json ReadHints(const fs::path& config) {
    std::ifstream in(config.parent_path() / "modmenu.json");
    json h = json::parse(in, nullptr, false);
    return h.is_object() ? h : json::object();
}

std::string Label(std::string key) {
    std::replace(key.begin(), key.end(), '_', ' ');
    if (!key.empty()) key[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(key[0])));
    return key;
}

// Hint readers: a hint of the wrong type is ignored instead of throwing.
std::string Str(const json& h, const char* k, std::string def = {}) {
    return h.contains(k) && h[k].is_string() ? h[k].get<std::string>() : def;
}
float Num(const json& h, const char* k, float def) { return h.contains(k) && h[k].is_number() ? h[k].get<float>() : def; }
bool Flag(const json& h, const char* k, bool def) { return h.contains(k) && h[k].is_boolean() ? h[k].get<bool>() : def; }
bool IsNum(const json& h, const char* k) { return h.contains(k) && h[k].is_number(); }

std::optional<json::json_pointer> Pointer(const std::string& slashPath) {
    try {
        return json::json_pointer("/" + slashPath);
    } catch (const json::exception&) {
        return std::nullopt;
    }
}

bool IsHexColor(const json& v) {
    if (!v.is_string()) return false;
    const auto& s = v.get_ref<const std::string&>();
    return s.size() == 7 && s[0] == '#' && std::all_of(s.begin() + 1, s.end(), [](unsigned char c) { return std::isxdigit(c); });
}

// 0xRRGGBB from an int or a "#rrggbb" string.
int ColorOf(const json& x) {
    if (IsHexColor(x)) return std::stoi(x.get_ref<const std::string&>().substr(1), nullptr, 16);
    return static_cast<int>(x.get<std::int64_t>() & 0xFFFFFF);
}

// The picker works in 0xRRGGBB. Int values keep any alpha byte they had; "#rrggbb" strings keep their letter case.
Setting ColorRow(const std::shared_ptr<File>& f, const json::json_pointer& p, const std::string& label, const json& v,
                 const json& hint) {
    const bool hex = IsHexColor(v);
    const bool upper = hex && std::any_of(v.get_ref<const std::string&>().begin(), v.get_ref<const std::string&>().end(),
                                          [](unsigned char c) { return std::isupper(c); });
    const bool declared = hint.contains("default") && (IsHexColor(hint["default"]) || hint["default"].is_number_integer());
    return Setting::Color(
        label,
        [f, p, hex] {
            const json* x = f->get(p);
            return x && (hex ? IsHexColor(*x) : x->is_number_integer()) ? ColorOf(*x) : 0;
        },
        [f, p, hex, upper](int rgb) {
            rgb &= 0xFFFFFF;
            if (hex) {
                char buf[8];
                std::snprintf(buf, sizeof buf, upper ? "#%06X" : "#%06x", rgb);
                f->set(p, std::string(buf));
                return;
            }
            const json* cur = f->get(p);
            const std::int64_t hi = cur && cur->is_number_integer() ? (cur->get<std::int64_t>() & ~std::int64_t{0xFFFFFF}) : 0;
            f->set(p, hi | rgb);
        },
        ColorOf(declared ? hint["default"] : v));
}

// Hints every row type shares: "tip", and "shown_if": "some/bool/path" (prefix "!" to invert).
void ApplyCommon(const std::shared_ptr<File>& f, const json& hint, Setting& row) {
    if (const std::string tip = Str(hint, "tip"); !tip.empty()) row.tooltip = tip;
    std::string cond = Str(hint, "shown_if");
    if (cond.empty()) return;
    const bool invert = cond[0] == '!';
    if (invert) cond.erase(0, 1);
    const auto at = Pointer(cond);
    if (!at) return;
    row.shownIf = [f, at = *at, invert] {
        const json* x = f->get(at);
        return (x && x->is_boolean() && x->get<bool>()) != invert;
    };
}

void Build(const std::shared_ptr<File>& f, const json& node, const json::json_pointer& ptr, std::vector<Setting>& out) {
    static const json none = json::object();
    for (auto it = node.begin(); it != node.end(); ++it) {
        const std::string& key = it.key();
        const auto p = ptr / key;
        const json& v = it.value();

        // Everything about a row beyond its JSON shape comes from the hints file, keyed by this path.
        const auto hit = f->hints.find(p.to_string().substr(1));
        const json& hint = hit != f->hints.end() && hit->is_object() ? *hit : none;
        if (Flag(hint, "hidden", false)) continue;
        const std::string label = Str(hint, "label", Label(key));
        const std::string type = Str(hint, "type");

        std::vector<std::string> opts;
        if (hint.contains("choices") && hint["choices"].is_array())
            for (const auto& o : hint["choices"])
                if (o.is_string()) opts.push_back(o.get<std::string>());

        if (!opts.empty() && (v.is_string() || v.is_number_integer())) {
            const bool str = v.is_string();
            const auto toIndex = [opts, str](const json* x) -> int {
                if (!x) return 0;
                if (!str) return x->is_number_integer() ? std::clamp(x->get<int>(), 0, static_cast<int>(opts.size()) - 1) : 0;
                const auto at = x->is_string() ? std::find(opts.begin(), opts.end(), x->get<std::string>()) : opts.end();
                return at == opts.end() ? 0 : static_cast<int>(at - opts.begin());
            };
            out.push_back(Setting::Choice(
                label, opts, [f, p, toIndex] { return toIndex(f->get(p)); },
                [f, p, opts, str](int i) {
                    i = std::clamp(i, 0, static_cast<int>(opts.size()) - 1);
                    f->set(p, str ? json(opts[static_cast<size_t>(i)]) : json(i));
                },
                toIndex(hint.contains("default") ? &hint["default"] : &v)));
        } else if (IsHexColor(v) || (type == "color" && v.is_number_integer())) {
            out.push_back(ColorRow(f, p, label, v, hint));
        } else if (type == "keybind" && v.is_number_integer()) {
            out.push_back(Setting::Key(
                label,
                [f, p] { return Read<int>(f, p); },
                [f, p](int code) { f->set(p, code); }, static_cast<int>(Num(hint, "default", v.get<float>()))));
        } else if (v.is_boolean()) {
            out.push_back(Setting::Toggle(
                label,
                [f, p] { return Read<bool>(f, p); },
                [f, p](bool b) { f->set(p, b); }, Flag(hint, "default", v.get<bool>())));
        } else if (v.is_number() && IsNum(hint, "min") && IsNum(hint, "max")) {
            // A slider only exists when the mod declares both limits.
            const bool isInt = v.is_number_integer();
            out.push_back(Setting::Slider(
                label,
                [f, p] { return Read<float>(f, p); },
                [f, p, isInt](float x) {
                    f->set(p, isInt ? json(std::llround(x)) : json(std::round(static_cast<double>(x) * 100) / 100));
                },
                Num(hint, "min", 0.0f), Num(hint, "max", 1.0f), Num(hint, "default", v.get<float>()),
                Num(hint, "step", isInt ? 1.0f : 0.01f), static_cast<int>(Num(hint, "decimals", isInt ? 0.0f : 2.0f)),
                Str(hint, "unit")));
        } else if (v.is_number()) {
            // No limits declared: a typed number, so nothing is guessed.
            const bool isInt = v.is_number_integer();
            out.push_back(Setting::Text(
                label,
                [f, p] { const json* x = f->get(p); return x && x->is_number() ? x->dump() : std::string(); },
                [f, p, isInt](const std::string& text) {
                    char* end = nullptr;
                    const double d = std::strtod(text.c_str(), &end);
                    if (end == text.c_str() || !std::isfinite(d)) return;
                    while (*end == ' ') ++end;
                    if (*end != '\0') return;
                    f->set(p, isInt ? json(std::llround(d)) : json(d));
                },
                IsNum(hint, "default") ? hint["default"].dump() : v.dump()));
        } else if (v.is_string()) {
            out.push_back(Setting::Text(
                label,
                [f, p] { return Read<std::string>(f, p); },
                [f, p](const std::string& s) { f->set(p, s); }, Str(hint, "default", v.get<std::string>())));
        } else if (v.is_object()) {
            std::vector<Setting> kids;
            Build(f, v, p, kids);
            if (kids.empty()) continue;
            out.push_back(Setting::Category(label, std::move(kids)));
        } else {
            continue;
        }
        ApplyCommon(f, hint, out.back());
    }
}

}

std::vector<Setting> JsonConfigSettings(const fs::path& file, Notify notify) {
    auto f = std::make_shared<File>();
    f->path = file;
    f->refresh();
    f->hints = ReadHints(file);
    std::vector<Setting> out;
    f->notify = std::move(notify);
    Build(f, f->doc, json::json_pointer{}, out);
    // "type": "action" hints have no value in config.json; they become buttons that call notify(path).
    for (const auto& [path, hint] : f->hints.items()) {
        if (!hint.is_object() || Str(hint, "type") != "action" || Flag(hint, "hidden", false)) continue;
        const auto at = Pointer(path);
        if (!at || f->doc.contains(*at)) continue; // a real config value at that path wins
        out.push_back(Setting::Action(
            Str(hint, "label", Label(path.substr(path.rfind('/') == std::string::npos ? 0 : path.rfind('/') + 1))),
            [f, path = path] { if (f->notify) f->notify(path); }, Flag(hint, "confirm", false)));
        ApplyCommon(f, hint, out.back());
    }
    return out;
}

}
