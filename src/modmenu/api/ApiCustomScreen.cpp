#include "modmenu/api/ApiInternal.hpp"

#include "MossUI/MossUI.hpp"

#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace nugget::modmenu::api {
namespace {

struct BuilderContext {
    MossUI::Container* root;
    ActiveToken active;
};

MossUI::Rect ConvertRect(ModMenuRectV2 rect) { return {rect.x0, rect.x1, rect.y0, rect.y1}; }

class DynamicLabelWidget : public MossUI::Widget {
public:
    DynamicLabelWidget(ActiveToken active, ModMenuGetTextV1 get, void* userData, MossUI::Align align)
        : active(std::move(active)), get(get), userData(userData), align(align) {}

    void Render(MossUI::UI& ui) override {
        if (!active->load() || !get) return;
        ui.Text(bounds.x0, bounds.x1, bounds.y0 + 11, CopyText(get(userData)), MossUI::col::WHITE, align, 1.0f);
    }

private:
    ActiveToken active;
    ModMenuGetTextV1 get;
    void* userData;
    MossUI::Align align;
};

void AddLabel(void* context, ModMenuRectV2 bounds, const char* text, int32_t alignment) {
    auto& builder = *static_cast<BuilderContext*>(context);
    auto& widget = builder.root->Add<MossUI::LabelWidget>(CopyText(text),
        alignment >= 0 && alignment <= 2 ? static_cast<MossUI::Align>(alignment) : MossUI::Align::Left);
    widget.bounds = ConvertRect(bounds);
}

void AddDynamicLabel(void* context, ModMenuRectV2 bounds, ModMenuGetTextV1 get,
                     void* userData, int32_t alignment) {
    if (!get) return;
    auto& builder = *static_cast<BuilderContext*>(context);
    auto& widget = builder.root->Add<DynamicLabelWidget>(builder.active, get, userData,
        alignment >= 0 && alignment <= 2 ? static_cast<MossUI::Align>(alignment) : MossUI::Align::Left);
    widget.bounds = ConvertRect(bounds);
}

void AddButton(void* context, ModMenuRectV2 bounds, const char* label, ModMenuActionV1 action, void* userData) {
    if (!action) return;
    auto& builder = *static_cast<BuilderContext*>(context);
    auto& widget = builder.root->Add<MossUI::ButtonWidget>(CopyText(label), [active = builder.active, action, userData] {
        if (active->load()) action(userData);
    });
    widget.bounds = ConvertRect(bounds);
}

void AddToggle(void* context, ModMenuRectV2 bounds, const char* label,
               ModMenuGetBoolV1 get, ModMenuSetBoolV1 set, void* userData) {
    if (!get || !set) return;
    auto& builder = *static_cast<BuilderContext*>(context);
    auto active = builder.active;
    auto& widget = builder.root->Add<MossUI::ToggleOptionWidget>(CopyText(label),
        [active, get, userData] { return active->load() && get(userData) != 0; },
        [active, set, userData](bool value) { if (active->load()) set(userData, value ? 1 : 0); });
    widget.bounds = ConvertRect(bounds);
}

void AddSlider(void* context, ModMenuRectV2 bounds, const char* label,
               ModMenuGetFloatV1 get, ModMenuSetFloatV1 set, void* userData,
               float minValue, float maxValue, float step, int32_t decimals, const char* suffix) {
    if (!get || !set || !std::isfinite(minValue) || !std::isfinite(maxValue) || minValue > maxValue ||
        !std::isfinite(step) || step < 0 || decimals < 0) return;
    auto& builder = *static_cast<BuilderContext*>(context);
    auto active = builder.active;
    auto& widget = builder.root->Add<MossUI::SliderOptionWidget>(CopyText(label),
        [active, get, userData] { return active->load() ? get(userData) : 0.0f; },
        [active, set, userData](float value) { if (active->load()) set(userData, value); },
        minValue, maxValue, step, decimals, CopyText(suffix));
    widget.bounds = ConvertRect(bounds);
}

void AddChoice(void* context, ModMenuRectV2 bounds, const char* label,
               const char* const* options, size_t optionCount, ModMenuGetIntV1 get,
               ModMenuSetIntV1 set, void* userData) {
    if (!options || !optionCount || optionCount > 128 || !get || !set) return;
    auto& builder = *static_cast<BuilderContext*>(context);
    std::vector<std::string> copiedOptions;
    copiedOptions.reserve(optionCount);
    for (size_t i = 0; i < optionCount; ++i) copiedOptions.push_back(CopyText(options[i]));
    auto active = builder.active;
    auto& widget = builder.root->Add<MossUI::ChoiceOptionWidget>(CopyText(label), std::move(copiedOptions),
        [active, get, userData] { return active->load() ? get(userData) : 0; },
        [active, set, userData](int value) { if (active->load()) set(userData, value); });
    widget.bounds = ConvertRect(bounds);
}

void AddColor(void* context, ModMenuRectV2 bounds, const char* label,
              ModMenuGetIntV1 get, ModMenuSetIntV1 set, void* userData) {
    if (!get || !set) return;
    auto& builder = *static_cast<BuilderContext*>(context);
    auto active = builder.active;
    auto& widget = builder.root->Add<MossUI::ColorOptionWidget>(CopyText(label),
        [active, get, userData] { return active->load() ? get(userData) : 0; },
        [active, set, userData](int value) { if (active->load()) set(userData, value); });
    widget.bounds = ConvertRect(bounds);
}

void AddText(void* context, ModMenuRectV2 bounds, const char* label,
             ModMenuGetTextV1 get, ModMenuSetTextV1 set, void* userData, const char* placeholder) {
    if (!get || !set) return;
    auto& builder = *static_cast<BuilderContext*>(context);
    auto active = builder.active;
    auto& widget = builder.root->Add<MossUI::TextOptionWidget>(CopyText(label),
        [active, get, userData] { return active->load() ? CopyText(get(userData)) : std::string(); },
        [active, set, userData](const std::string& value) { if (active->load()) set(userData, value.c_str()); },
        CopyText(placeholder));
    widget.bounds = ConvertRect(bounds);
}

const ModMenuScreenBuilderV2 screenBuilder = {
    sizeof(ModMenuScreenBuilderV2), nullptr, AddLabel, AddButton, AddToggle, AddSlider, AddChoice, AddColor,
    AddText, AddDynamicLabel
};

class CustomScreen : public MossUI::Screen {
public:
    explicit CustomScreen(Registration registration)
        : registration(std::move(registration)) {}

    void Render(MossUI::UI& ui) override {
        ui.Backdrop(0.95f);
        ui.Panel({0, layoutW, 32, layoutH});
        root.Render(ui);
    }

    void Init(float width, float height) override {
        layoutW = width;
        layoutH = height;
        root.Clear();
        root.Add<MossUI::LabelWidget>(registration.name, MossUI::Align::Center)
            .bounds = {8, width - 8, 8, 28};
        if (registration.active->load() && registration.buildCustomScreen) {
            BuilderContext context{&root, registration.active};
            auto builder = screenBuilder;
            builder.context = &context;
            try {
                registration.buildCustomScreen(registration.customScreenUserData, &builder, width, height);
            } catch (...) {
                root.Add<MossUI::LabelWidget>("Could not build this custom screen", MossUI::Align::Center)
                    .bounds = {8, width - 8, height * 0.5f, height * 0.5f + 24};
            }
        }
        root.Add<MossUI::ButtonWidget>("Done", [] { MossUI::Close(); })
            .bounds = {width * 0.5f - 70, width * 0.5f + 70, height - 32, height - 10};
    }

    bool OnKey(int key) override {
        if (key != MossUI::Key(MossUI::KeyCode::ESCAPE)) return false;
        MossUI::Close();
        return true;
    }

private:
    Registration registration;
};

}

bool OpenCustomScreen(const char* id) {
    if (!id || !*id) return false;
    auto registration = FindRegistration(id);
    if (!registration.active || !registration.active->load() || !registration.buildCustomScreen) return false;
    MossUI::PushScreen(std::make_shared<CustomScreen>(std::move(registration)));
    return true;
}

}
