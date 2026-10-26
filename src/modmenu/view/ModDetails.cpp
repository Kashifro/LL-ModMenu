#include "modmenu/view/internal/ModDetails.hpp"

namespace modmenu::view {

void DrawModHeader(UI& ui, const ModInfo& mod, float rightX, float paneW) {
    constexpr float headerY = MOD_DETAILS_HEADER_Y;
    const float x = rightX, tx = x + 36.0f;
    DrawModIcon(ui, {x, x + MOD_ICON_SIZE, headerY, headerY + MOD_ICON_SIZE}, mod);
    const std::string name = ui.Trunc(mod.name, static_cast<float>(ui.Width()) - tx);
    ui.Text(tx, ui.Width(), headerY + 1, name, col::WHITE, Align::Left, 1.0f);
    float badgeX = tx + ui.TextWidth(name) + 2.0f;
    if (Opt(OPT_BADGES)) {
        for (const auto& badge : mod.badges) {
            const float need = ui.TextWidth(badge) + 6.0f;
            if (badgeX + need >= ui.Width() - 28.0f) break;
            badgeX += DrawBadge(ui, badgeX, headerY, badge);
        }
    }
    ui.Text(tx, ui.Width(), headerY + 12, "v" + mod.version, TEXT_GRAY, Align::Left, 1.0f);
    if (Opt(OPT_SHOW_CREDITS) && !mod.author.empty()) {
        auto authors = SplitAuthors(mod.author);
        std::string joined;
        for (size_t i = 0; i < authors.size(); ++i) joined += (i ? ", " : "") + authors[i];
        const auto wrapped = ui.Wrap("By " + joined, paneW - 40.0f);
        if (!wrapped.empty())
            ui.Text(tx, ui.Width(), headerY + 22, wrapped.size() > 1 ? wrapped[0] + "..." : wrapped[0],
                    TEXT_GRAY, Align::Left, 1.0f);
    }
}

void BuildModDescription(UI& ui, const ModInfo* mod, DescView& desc) {
    desc.lines.clear();
    desc.ResetScroll();
    if (!mod) return;
    const float wrap = desc.RowWidth() - 5.0f;
    auto push = [&](const std::string& text, float indent, float width, const std::string& url = {}) {
        bool first = true;
        for (const auto& line : ui.Wrap(text, width)) {
            desc.lines.push_back({line, first ? indent : indent + 8.0f, url});
            first = false;
        }
    };
    auto blank = [&] { desc.lines.push_back({}); };

    if (!mod->description.empty()) push(mod->description, 0, wrap);
    if (Opt(OPT_SHOW_LINKS) && (!mod->website.empty() || !mod->issues.empty() || !mod->source.empty() || !mod->links.empty())) {
        if (!desc.lines.empty()) blank();
        push("Links:", 0, wrap);
        if (!mod->website.empty()) push("Website", 8, wrap - 16, mod->website);
        if (!mod->issues.empty()) push("Issues", 8, wrap - 16, mod->issues);
        if (!mod->source.empty()) push("Source", 8, wrap - 16, mod->source);
        for (const auto& [label, url] : mod->links) push(label, 8, wrap - 16, url);
    }
    if (Opt(OPT_SHOW_LICENSE) && !mod->license.empty()) {
        if (!desc.lines.empty()) blank();
        push("License:", 0, wrap);
        push(mod->license, 8, wrap - 16);
    }
    if (Opt(OPT_SHOW_CREDITS) && !mod->author.empty()) {
        if (!desc.lines.empty()) blank();
        push("Credits:", 0, wrap);
        push("Authors:", 8, wrap - 16);
        for (const auto& author : SplitAuthors(mod->author)) push(author, 16, wrap - 24);
    }
}

}
