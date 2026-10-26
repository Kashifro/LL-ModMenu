#include "hooks/Hooks.hpp"

#include "MossMC/MossMC.hpp"
#include "MossUI/MossUI.hpp"
#include "modmenu/ModMenu.hpp"
#include "platform/ClientContext.hpp"
#include "platform/Keybinds.hpp"
#include "platform/Log.hpp"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/input/KeyInputEvent.h"
#include "ll/api/event/input/MouseInputEvent.h"
#include "ll/api/event/render/UIRenderEvent.h"

#include "mc/client/gui/screens/ScreenView.h"

#include <Windows.h>
#undef MOD_ALT
#undef MOD_SHIFT
#undef MOD_CONTROL
#undef MOD_WIN

#include <cstdint>
#include <string>
#include <vector>

namespace {

using namespace ll::event;

std::vector<ListenerPtr> listeners;

bool AsyncKeyDown(int key) { return (GetAsyncKeyState(key) & 0x8000) != 0; }

// The generic VK_SHIFT / VK_CONTROL / VK_MENU already cover both sides. The key being handled wins over the
// async state, which can lag the event.
bool Held(int any, int left, int right, int key, bool down) {
    return key == any || key == left || key == right ? down : AsyncKeyDown(any);
}

int CurrentModifiers(int key, bool down) {
    return (Held(VK_SHIFT, VK_LSHIFT, VK_RSHIFT, key, down) ? MossUI::MOD_SHIFT : 0) |
           (Held(VK_CONTROL, VK_LCONTROL, VK_RCONTROL, key, down) ? MossUI::MOD_CTRL : 0) |
           (Held(VK_MENU, VK_LMENU, VK_RMENU, key, down) ? MossUI::MOD_ALT : 0);
}

void OnKey(KeyInputEvent& ev) {
    const int key = ev.keyCode();
    const bool down = ev.isDown();
    const int mods = CurrentModifiers(key, down);

    if (MossUI::IsOpen()) {
        MossUI::OnKey(key, down, mods & MossUI::MOD_SHIFT, mods & MossUI::MOD_CTRL, mods & MossUI::MOD_ALT);
        ev.cancel();
    } else if (down && modmenu::clientctx::IsOnScreen("hud_screen") &&
               modmenu::InGameEnabled() && modmenu::keybinds::IsOpenKey(key)) {
        modmenu::Open();
        ev.cancel();
    }
}

void OnMouse(MouseInputEvent& ev) {
    if (!MossUI::IsOpen()) return;
    MossUI::OnMouse(ev.actionButtonId(), ev.buttonData(), ev.x(), ev.y());
    ev.cancel();
}

int errors = 0;
unsigned sehCode = 0;
void* sehAddr = nullptr;

int SehFilter(EXCEPTION_POINTERS* p) {
    sehCode = p->ExceptionRecord->ExceptionCode;
    sehAddr = p->ExceptionRecord->ExceptionAddress;
    return EXCEPTION_EXECUTE_HANDLER;
}

void RenderBody(AfterUIRenderEvent& ev) {
    const std::string name = ev.screenView().getScreenName();
    modmenu::clientctx::SetScreenName(name);
    if (MossUI::IsOpen() && (name == "start_screen" || name == "hud_screen"))
        MossMC::Render(ev.uiRenderContext());
}

void OnRender(AfterUIRenderEvent& ev) {
    __try {
        RenderBody(ev);
    } __except (SehFilter(GetExceptionInformation())) {
        if (errors++ < 5) {
            const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"ModMenu.dll"));
            const auto addr = reinterpret_cast<uintptr_t>(sehAddr);
            modmenu::Log().error("render CRASH: code 0x{:08X} addr 0x{:X} (ModMenu.dll+0x{:X})",
                                sehCode, addr, addr - base);
        }
    }
}

}

namespace modmenu::hooks {

void InstallEvents() {
    if (!listeners.empty()) return;
    auto& bus = EventBus::getInstance();
    listeners.push_back(bus.emplaceListener<KeyInputEvent>(OnKey));
    listeners.push_back(bus.emplaceListener<MouseInputEvent>(OnMouse));
    listeners.push_back(bus.emplaceListener<AfterUIRenderEvent>(OnRender));
}

void RemoveEvents() {
    auto& bus = EventBus::getInstance();
    for (auto& l : listeners) bus.removeListener(l);
    listeners.clear();
}

}
