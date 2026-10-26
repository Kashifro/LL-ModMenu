#include "platform/Prefs.hpp"

#include "ll/api/io/FileUtils.h"
#include "platform/Log.hpp"
#include "nlohmann/json.hpp"

#include <mutex>

namespace modmenu::prefs {
namespace {

struct Store {
    std::mutex mutex;
    nlohmann::json data = nlohmann::json::object();
    bool loaded = false;
};

Store& S() {
    static Store s;
    return s;
}

std::filesystem::path File() { return Self().getDataDir() / "settings.json"; }

void Load(Store& s) {
    if (s.loaded) return;
    s.loaded = true;
    if (auto text = ll::file_utils::readFile(File())) {
        auto j = nlohmann::json::parse(*text, nullptr, false);
        if (j.is_object()) s.data = std::move(j);
    }
}

template <class T, class Check>
T Get(const std::string& key, T def, Check is) {
    auto& s = S();
    std::lock_guard lock(s.mutex);
    Load(s);
    const auto it = s.data.find(key);
    return it != s.data.end() && is(*it) ? it->get<T>() : def;
}

template <class T>
void Set(const std::string& key, T value) {
    auto& s = S();
    std::lock_guard lock(s.mutex);
    Load(s);
    if (const auto it = s.data.find(key); it != s.data.end() && *it == value) return;
    s.data[key] = value;
    ll::file_utils::writeFile(File(), s.data.dump(2) + "\n");
}

}

bool GetBool(const std::string& key, bool def) {
    return Get(key, def, [](const nlohmann::json& j) { return j.is_boolean(); });
}
void SetBool(const std::string& key, bool value) { Set(key, value); }

int GetInt(const std::string& key, int def) {
    return Get(key, def, [](const nlohmann::json& j) { return j.is_number_integer(); });
}
void SetInt(const std::string& key, int value) { Set(key, value); }

std::string GetString(const std::string& key, const std::string& def) {
    return Get(key, def, [](const nlohmann::json& j) { return j.is_string(); });
}
void SetString(const std::string& key, const std::string& value) { Set(key, value); }

}
