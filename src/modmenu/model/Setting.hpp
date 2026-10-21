#pragma once

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace nugget::modmenu {

struct Setting {
    enum class Kind { Toggle, Key, Slider, Header, Color, Category, Action, Choice, Text };
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
    float minV = 0, maxV = 1, step = 0;
    float defFloat = 0;
    int decimals = 0;
    std::string suffix;
    std::vector<Setting> children;
    std::function<void()> onAction;
    std::string glyph;
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
    Setting Glyph(std::string name) && { glyph = std::move(name); return std::move(*this); }

    static Setting Toggle(std::string label, std::function<bool()> get, std::function<void(bool)> set, bool def) {
        Setting row;
        row.label = std::move(label);
        row.getBool = std::move(get);
        row.setBool = std::move(set);
        row.defBool = def;
        return row;
    }

    static Setting KeyBind(std::string label, std::function<int()> get, std::function<void(int)> set, int def) {
        Setting row;
        row.kind = Kind::Key;
        row.label = std::move(label);
        row.getKey = std::move(get);
        row.setKey = std::move(set);
        row.defKey = def;
        return row;
    }

    static Setting Slider(std::string label, std::function<float()> get, std::function<void(float)> set,
                          float min, float max, float def, float step = 0, int decimals = 0,
                          std::string suffix = {}) {
        Setting row;
        row.kind = Kind::Slider;
        row.label = std::move(label);
        row.getFloat = std::move(get);
        row.setFloat = std::move(set);
        row.minV = min;
        row.maxV = max;
        row.defFloat = def;
        row.step = step;
        row.decimals = decimals;
        row.suffix = std::move(suffix);
        return row;
    }

    static Setting Color(std::string label, std::function<int()> get, std::function<void(int)> set, int def) {
        Setting row;
        row.kind = Kind::Color;
        row.label = std::move(label);
        row.getInt = std::move(get);
        row.setInt = std::move(set);
        row.defInt = def;
        return row;
    }

    static Setting Choice(std::string label, std::vector<std::string> options, std::function<int()> get,
                          std::function<void(int)> set, int def = 0) {
        Setting row;
        row.kind = Kind::Choice;
        row.label = std::move(label);
        row.options = std::move(options);
        row.getInt = std::move(get);
        row.setInt = std::move(set);
        row.defInt = def;
        return row;
    }

    static Setting Text(std::string label, std::function<std::string()> get,
                        std::function<void(const std::string&)> set, std::string def = {},
                        std::string placeholder = {}) {
        Setting row;
        row.kind = Kind::Text;
        row.label = std::move(label);
        row.getText = std::move(get);
        row.setText = std::move(set);
        row.defText = std::move(def);
        row.placeholder = std::move(placeholder);
        return row;
    }

    static Setting Category(std::string label, std::vector<Setting> children) {
        Setting row;
        row.kind = Kind::Category;
        row.label = std::move(label);
        row.children = std::move(children);
        return row;
    }

    static Setting Action(std::string label, std::function<void()> run, bool confirm = false) {
        Setting row;
        row.kind = Kind::Action;
        row.label = std::move(label);
        row.onAction = std::move(run);
        row.confirm = confirm;
        return row;
    }

    static Setting Section(std::string label) {
        Setting row;
        row.kind = Kind::Header;
        row.label = std::move(label);
        return row;
    }

    void Reset() const {
        switch (kind) {
            case Kind::Toggle: if (setBool) setBool(defBool); break;
            case Kind::Key: if (setKey) setKey(defKey); break;
            case Kind::Color:
            case Kind::Choice: if (setInt) setInt(defInt); break;
            case Kind::Text: if (setText) setText(defText); break;
            case Kind::Category: for (const auto& child : children) child.Reset(); break;
            case Kind::Slider: if (setFloat) setFloat(defFloat); break;
            case Kind::Header:
            case Kind::Action: break;
        }
    }
};

}
