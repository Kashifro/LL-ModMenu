
#include "hooks/Hooks.hpp"

#include "MossMC/MossMC.hpp"
#include "MossUI/MossUI.hpp"
#include "modmenu/ModMenu.hpp"
#include "nugget/ClientContext.hpp"
#include "nugget/Keybinds.hpp"
#include "nugget/Log.hpp"

#include "ll/api/event/EventBus.h"
#include "ll/api/event/input/KeyInputEvent.h"
#include "ll/api/event/input/MouseInputEvent.h"
#include "ll/api/event/render/UIRenderEvent.h"

#include "mc/client/gui/screens/ScreenView.h"

#include <windows.h>
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

int CurrentModifiers(int key, bool down) {
    bool shift = AsyncKeyDown(VK_SHIFT) || AsyncKeyDown(VK_LSHIFT) || AsyncKeyDown(VK_RSHIFT);
    bool ctrl = AsyncKeyDown(VK_CONTROL) || AsyncKeyDown(VK_LCONTROL) || AsyncKeyDown(VK_RCONTROL);
    bool alt = AsyncKeyDown(VK_MENU) || AsyncKeyDown(VK_LMENU) || AsyncKeyDown(VK_RMENU);

    
    if (key == VK_SHIFT || key == VK_LSHIFT || key == VK_RSHIFT) shift = down;
    if (key == VK_CONTROL || key == VK_LCONTROL || key == VK_RCONTROL) ctrl = down;
    if (key == VK_MENU || key == VK_LMENU || key == VK_RMENU) alt = down;

    return (shift ? MossUI::MOD_SHIFT : 0) | (ctrl ? MossUI::MOD_CTRL : 0) | (alt ? MossUI::MOD_ALT : 0);
}

void OnKey(KeyInputEvent& ev) {
    const int key = ev.keyCode();
    const bool down = ev.isDown();
    const int mods = CurrentModifiers(key, down);

    if (MossUI::IsOpen()) {
        MossUI::OnKey(key, down, mods & MossUI::MOD_SHIFT, mods & MossUI::MOD_CTRL, mods & MossUI::MOD_ALT);
        ev.cancel();
    } else if (down && nugget::clientctx::IsOnScreen("hud_screen") &&
               nugget::modmenu::InGameEnabled() && nugget::keybinds::Dispatch(key)) {
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
    nugget::clientctx::SetScreenName(name);
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
            nugget::Log().error("render CRASH: code 0x{:08X} addr 0x{:X} (ModMenu.dll+0x{:X})",
                                sehCode, addr, addr - base);
        }
    }
}

} 

namespace nugget::hooks {

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
