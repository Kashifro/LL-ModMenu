#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace modmenu {

struct Setting {
    enum class Kind { Toggle, Key, Slider, Section, Color, Category, Action, Choice, Text };
    Kind kind = Kind::Toggle;
    std::string label;
    std::string tooltip;
    std::function<bool()> getBool;
    std::function<void(bool)> setBool;
    bool defBool = false;
    std::function<int()> getKey;
    std::function<void(int)> setKey;
    int defKey = 0;
    std::function<float()> getFloat;
    std::function<void(float)> setFloat;
    float min = 0, max = 1, step = 0;
    float defFloat = 0;
    int decimals = 0;
    std::string unit;
    std::vector<Setting> children;
    std::function<void()> onAction;
    bool confirm = false;
    std::function<bool()> shownIf;
    std::function<int()> getInt;
    std::function<void(int)> setInt;
    int defInt = 0;
    std::vector<std::string> options;
    std::function<std::string()> getText;
    std::function<void(const std::string&)> setText;
    std::string defText;
    std::string placeholder;

    Setting Tip(std::string text) && { tooltip = std::move(text); return std::move(*this); }
    Setting ShownIf(std::function<bool()> fn) && { shownIf = std::move(fn); return std::move(*this); }

    static Setting Toggle(std::string label, std::function<bool()> get, std::function<void(bool)> set, bool def) {
        return {.label = std::move(label), .getBool = std::move(get), .setBool = std::move(set), .defBool = def};
    }

    static Setting Key(std::string label, std::function<int()> get, std::function<void(int)> set, int def) {
        return {.kind = Kind::Key, .label = std::move(label), .getKey = std::move(get), .setKey = std::move(set),
                .defKey = def};
    }

    static Setting Slider(std::string label, std::function<float()> get, std::function<void(float)> set,
                          float min, float max, float def, float step = 0, int decimals = 0, std::string unit = {}) {
        return {.kind = Kind::Slider, .label = std::move(label), .getFloat = std::move(get),
                .setFloat = std::move(set), .min = min, .max = max, .step = step, .defFloat = def,
                .decimals = decimals, .unit = std::move(unit)};
    }

    static Setting Color(std::string label, std::function<int()> get, std::function<void(int)> set, int def) {
        return {.kind = Kind::Color, .label = std::move(label), .getInt = std::move(get), .setInt = std::move(set),
                .defInt = def};
    }

    static Setting Choice(std::string label, std::vector<std::string> options, std::function<int()> get,
                          std::function<void(int)> set, int def = 0) {
        return {.kind = Kind::Choice, .label = std::move(label), .getInt = std::move(get), .setInt = std::move(set),
                .defInt = def, .options = std::move(options)};
    }

    static Setting Text(std::string label, std::function<std::string()> get,
                        std::function<void(const std::string&)> set, std::string def = {},
                        std::string placeholder = {}) {
        return {.kind = Kind::Text, .label = std::move(label), .getText = std::move(get), .setText = std::move(set),
                .defText = std::move(def), .placeholder = std::move(placeholder)};
    }

    static Setting Category(std::string label, std::vector<Setting> children) {
        return {.kind = Kind::Category, .label = std::move(label), .children = std::move(children)};
    }

    static Setting Action(std::string label, std::function<void()> run, bool confirm = false) {
        return {.kind = Kind::Action, .label = std::move(label), .onAction = std::move(run), .confirm = confirm};
    }

    static Setting Section(std::string label) { return {.kind = Kind::Section, .label = std::move(label)}; }

    void Reset() const {
        switch (kind) {
            case Kind::Toggle: if (setBool) setBool(defBool); break;
            case Kind::Key: if (setKey) setKey(defKey); break;
            case Kind::Color:
            case Kind::Choice: if (setInt) setInt(defInt); break;
            case Kind::Text: if (setText) setText(defText); break;
            case Kind::Category: for (const auto& child : children) child.Reset(); break;
            case Kind::Slider: if (setFloat) setFloat(defFloat); break;
            case Kind::Section:
            case Kind::Action: break;
        }
    }
};

}
