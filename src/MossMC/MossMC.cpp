#include "MossMC/MossMC.hpp"

#include "ll/api/service/TargetedBedrock.h"
#include "ll/api/utils/StringUtils.h"
#include "mc/deps/application/AppPlatform.h"
#include "nugget/ClientContext.hpp"
#include "nugget/Log.hpp"
#include "nugget/Settings.hpp"

#include "mc/client/game/ClientInstance.h"
#include "mc/client/gui/CaretMeasureData.h"
#include "mc/client/gui/Font.h"
#include "mc/client/gui/FontHandle.h"
#include "mc/client/gui/TextAlignment.h"
#include "mc/client/gui/TextMeasureData.h"
#include "mc/client/gui/controls/UIRenderContext.h"
#include "mc/client/sound/SoundEngine.h"
#include "mc/deps/core/file/PathView.h"
#include "mc/deps/core/resource/ResourceFileSystem.h"
#include "mc/deps/core/resource/ResourceLocation.h"
#include "mc/deps/core/string/HashedString.h"
#include "mc/deps/input/RectangleArea.h"
#include "mc/deps/minecraft_renderer/renderer/BedrockTextureData.h"
#include "mc/deps/minecraft_renderer/renderer/IsMissingTexture.h"
#include "mc/deps/minecraft_renderer/renderer/TexturePtr.h"

#include <glm/vec2.hpp>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <map>
#include <set>
#include <vector>

namespace MossMC {

namespace {

using MossUI::Align;
using MossUI::Backend;
using MossUI::Color;
using MossUI::Host;
using MossUI::Rect;
using MossUI::Skin;
using MossUI::TextRun;

constexpr mce::Color WHITE{255, 255, 255, 255};


RectangleArea ToArea(const Rect& r) {
    RectangleArea out;
    out._x0 = r.x0;
    out._x1 = r.x1;
    out._y0 = r.y0;
    out._y1 = r.y1;
    return out;
}

mce::Color ToColor(const Color& c) { return mce::Color{c.r, c.g, c.b, c.a}; }

ui::TextAlignment ToAlign(Align a) {
    switch (a) {
        case Align::Center : return ui::TextAlignment::Center;
        case Align::Right : return ui::TextAlignment::Right;
        default : return ui::TextAlignment::Left;
    }
}

TextMeasureData Measure(bool shadow, float scale) {
    return {scale, 0.0f, shadow, false, false, ui::TextAlignment::Left};
}

const CaretMeasureData& NoCaret() {
    static const CaretMeasureData c{0, false};
    return c;
}

const HashedString& FlushMaterial() {
    static const HashedString m("ui_flush");
    return m;
}


void DrawNinePatch(UIRenderContext& ctx, const mce::ClientTexture& texture, const Rect& r, float tw, float th,
                   float sw, float sh) {
    const float w = std::max(0.0f, r.x1 - r.x0), h = std::max(0.0f, r.y1 - r.y0);
    const float midW = std::max(0.0f, w - 2 * sw), midH = std::max(0.0f, h - 2 * sh);
    const float srcW = std::max(0.0f, tw - 2 * sw), srcH = std::max(0.0f, th - 2 * sh);

    const float px[3] = {r.x0, r.x0 + sw, r.x0 + w - sw}, sx[3] = {sw, midW, sw};
    const float py[3] = {r.y0, r.y0 + sh, r.y0 + h - sh}, sy[3] = {sh, midH, sh};
    const float u[3] = {0, sw / tw, (tw - sw) / tw}, uw[3] = {sw / tw, srcW / tw, sw / tw};
    const float v[3] = {0, sh / th, (th - sh) / th}, vh[3] = {sh / th, srcH / th, sh / th};

    for (int j = 0; j < 3; ++j)
        for (int i = 0; i < 3; ++i) {
            if (sx[i] <= 0 || sy[j] <= 0) continue;
            ctx.drawImage(texture, glm::vec2{px[i], py[j]}, glm::vec2{sx[i], sy[j]}, glm::vec2{u[i], v[j]},
                          glm::vec2{uw[i], vh[j]}, false);
        }
}


mce::TexturePtr LoadTex(UIRenderContext& ctx, const std::string& path) {
    const std::filesystem::path fsPath(path);
    const bool rawFile = fsPath.is_absolute() || fsPath.extension() == ".png";
    const std::string resourcePath = rawFile ? fsPath.lexically_normal().generic_string() : path;
    ResourceLocation loc{Core::PathView{resourcePath}, rawFile ? ResourceFileSystem::Raw : ResourceFileSystem::UserPackage};
    return ctx.getTexture(loc, rawFile);
}

const mce::ClientTexture* TexOf(const mce::TexturePtr& t) {
    const auto& data = t.mClientTexture;
    if (!data || data->mIsMissingTexture == IsMissingTexture::Yes) return nullptr;
    return &data->mClientTexture.get();
}

struct SkinSpec {
    const char* name;
    float w, sw, h, sh;
};

constexpr SkinSpec SKIN_SPECS[4] = {
    {"dialog_background_opaque", 16, 4, 16, 4},
    {"ScrollHandle", 3, 1, 5, 2},
    {"slider_button_default", 6, 2, 6, 2},
    {"slider_button_hover", 6, 2, 6, 2},
};

class McBackend : public Backend, public Host {
public:
    void Begin(UIRenderContext& c) {
        ctx = &c;
        clips.clear();
        ++frame;
        font = DefaultFont();
        LoadSkins();
    }

    void Fill(const Rect& r, const Color& c, float alpha) override { ctx->fillRectangle(ToArea(r), ToColor(c), alpha); }

    bool Image(const Rect& r, const std::string& path) override {
        const mce::ClientTexture* tex = Cached(path);
        if (!tex) return false;
        ctx->drawImage(*tex, glm::vec2{r.x0, r.y0}, glm::vec2{r.x1 - r.x0, r.y1 - r.y0}, glm::vec2{0, 0}, glm::vec2{1, 1},
                       false);
        ctx->flushImages(WHITE, 1.0f, FlushMaterial());
        return true;
    }

    void Preload(const std::string& path) override { Cached(path); }

    bool ImageTiled(const Rect& r, const std::string& path, float tile) override {
        const mce::ClientTexture* tex = Cached(path);
        if (!tex) return false;
        for (float y = r.y0; y < r.y1; y += tile) {
            const float h = std::min(tile, r.y1 - y);
            for (float x = r.x0; x < r.x1; x += tile) {
                const float w = std::min(tile, r.x1 - x);
                ctx->drawImage(*tex, glm::vec2{x, y}, glm::vec2{w, h}, glm::vec2{0, 0}, glm::vec2{w / tile, h / tile}, false);
            }
        }
        ctx->flushImages(WHITE, 1.0f, FlushMaterial());
        return true;
    }

    bool Skinned(const Rect& r, Skin skin) override {
        const size_t i = static_cast<size_t>(skin);
        const mce::TexturePtr& handle = skins[i];
        const SkinSpec& spec = SKIN_SPECS[i];
        const mce::ClientTexture* tex = TexOf(handle);
        if (!tex) {
            if (frame - skinBorn[i] >= SETTLE_FRAMES) {
                const std::string reason = handle.mClientTexture ? "game resource texture is flagged missing"
                                                                 : "game resource texture handle unavailable";
                Report(std::string("textures/ui/") + spec.name, reason);
            }
            return false;
        }
        DrawNinePatch(*ctx, *tex, r, spec.w, spec.h, spec.sw, spec.sh);
        ctx->flushImages(WHITE, 1.0f, FlushMaterial());
        return true;
    }

    void PushClip(const Rect& r) override {
        clips.push_back(ToArea(r));
        ApplyClip();
    }

    void PopClip() override {
        if (!clips.empty()) clips.pop_back();
        ApplyClip();
    }

    void Text(const TextRun& t) override {
        const mce::Color color = ToColor(t.color);
        const RectangleArea area = ToArea(t.rect);
        const auto measure = Measure(t.shadow, t.scale);
        if (font) ctx->drawText(*font, area, std::string(t.text), color, t.alpha, ToAlign(t.align), measure, NoCaret());
        else      ctx->drawDebugText(area, std::string(t.text), color, t.alpha, ToAlign(t.align), measure, NoCaret());
    }

    void FlushText() override { ctx->flushText(0.0f, std::nullopt); }

    float TextWidth(const std::string& s) override {
        if (!font) return 0;
        return static_cast<float>(font->getLineLength(std::string_view(s), 1.0f, false));
    }

    std::string ClipboardGet() override {
        auto platform = ll::service::getAppPlatform();
        if (!platform || !platform->supportsClipboardGet()) return {};
        return ll::string_utils::wstr2str(platform->getClipboardText());
    }

    void ClipboardSet(const std::string& text) override {
        if (auto platform = ll::service::getAppPlatform()) platform->setClipboard(text);
    }

    bool ToGui(int rx, int ry, float& x, float& y) override {
        auto* ci = nugget::clientctx::Client();
        const float scale = ci ? ci->getGuiScale() : 0.0f;
        if (scale < 1.0f) return false;
        x = static_cast<float>(rx) / scale;
        y = static_cast<float>(ry) / scale;
        return true;
    }

    bool OverHud() override { return nugget::clientctx::IsOnScreen("hud_screen"); }

    void GrabMouse() override {
        if (auto* ci = nugget::clientctx::Client()) ci->grabMouse();
    }
    void ReleaseMouse() override {
        if (auto* ci = nugget::clientctx::Client()) ci->releaseMouse();
    }
    bool MouseGrabbed() override {
        auto* ci = nugget::clientctx::Client();
        return ci && ci->getMouseGrabbed();
    }

    void Click() override {
        if (!nugget::settings::GetBool("modmenu.click", true)) return;
        const float volume = static_cast<float>(nugget::settings::GetInt("modmenu.click_volume", 100)) / 100.0f;
        if (volume <= 0.0f) return;
        if (auto* ci = nugget::clientctx::Client()) ci->getSoundEngine()->playUI("random.click", volume, 1.0f);
    }

    void ScreenOpened() override {
        ResetTextures();
    }

    UIRenderContext* ctx = nullptr;
    std::vector<RectangleArea> clips;

    
    void ApplyClip() {
        if (clips.empty()) {
            ctx->setFullClippingRectangle();
            ctx->disableScissorTest();
        } else {
            ctx->setClippingRectangle(clips.back());
            ctx->enableScissorTest(clips.back());
        }
    }

private:
    static Font* DefaultFont() {
        try {
            auto* client = nugget::clientctx::Client();
            if (!client) return nullptr;
            FontHandle handle = client->getFontHandle();
            return handle.mDefaultFont ? &handle.getFont() : nullptr;
        } catch (...) {
            return nullptr;
        }
    }

    
    void LoadSkins() {
        if (owner == ctx || skins[0].mClientTexture) return;
        owner = ctx;
        for (size_t i = 0; i < skins.size(); ++i) {
            skins[i] = LoadTex(*ctx, std::string("textures/ui/") + SKIN_SPECS[i].name);
            skinBorn[i] = frame;
        }
    }

    
    const mce::ClientTexture* Cached(const std::string& path) {
        auto it = cache.find(path);
        if (it == cache.end()) it = cache.emplace(path, Entry{LoadTex(*ctx, path), frame}).first;
        Entry& e = it->second;
        if (frame - e.born < SETTLE_FRAMES) return nullptr;
        const mce::ClientTexture* tex = TexOf(e.tex);
        if (!tex) {
            const auto& data = e.tex.mClientTexture;
            std::string reason = data ? "texture loader flagged the handle missing" : "texture handle unavailable";
            if (e.tex.mResourceLocationPtr)
                reason += std::string("; resolved path: ") + e.tex.mResourceLocationPtr->mPath->value;
            Report(path, reason);
        }
        return tex;
    }

    void Report(const std::string& path, const std::string& reason) {
        if (reported.insert(path).second)
            nugget::Log().warn("texture '{}' unavailable: {}", path, reason);
    }

    
    void ResetTextures() {
        cache.clear();
        owner = nullptr;
        for (auto& s : skins) s.mClientTexture.reset();
        skinBorn.fill(frame);
    }

    Font* font = nullptr;
    UIRenderContext* owner = nullptr;
    std::array<mce::TexturePtr, 4> skins;
    std::array<int, 4> skinBorn{};
    struct Entry { mce::TexturePtr tex; int born; };
    static constexpr int SETTLE_FRAMES = 4;
    int frame = 0;
    std::map<std::string, Entry> cache;
    std::set<std::string> reported;
};

McBackend& Mc() {
    static McBackend* mc = new McBackend();   
    return *mc;
}

void FrameSize(UIRenderContext& ctx, float& w, float& h) {
    const auto clip = ctx.getFullClippingRectangle();
    w = clip._x1 - clip._x0;
    h = clip._y1 - clip._y0;
}

} 

void Install() {
    MossUI::SetBackend(&Mc());
    MossUI::SetHost(&Mc());
}

void Render(UIRenderContext& ctx) {
    float w, h;
    FrameSize(ctx, w, h);
    Mc().Begin(ctx);
    MossUI::Render(w, h);
}

} 
