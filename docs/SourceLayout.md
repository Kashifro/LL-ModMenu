# Source layout

- `modmenu/` is LL-ModMenu itself. `ModMenu.*` is the lifecycle, `Registry.cpp` the mod list, `ModDiscovery.cpp` finds installed mods from their `manifest.json`, `Launch.cpp` opens URLs and folders, and `JsonConfig.cpp` builds a mod's settings page from its `config/config.json` and `config/modmenu.json`.
- `modmenu/settings/` holds the built-in LL-ModMenu settings and the per-mod settings cache.
- `modmenu/view/` holds the menu screens and shared drawing helpers; `view/internal/` holds private view declarations.
- `modmenu/internal/` holds declarations shared by implementation files only (registry, settings, JSON config, launching).
- `platform/` is game-facing support: the key-value preference store (`Prefs`), the open-menu keybind, logging, and the client-context lookup.
- `MossMC/` adapts MossUI to Minecraft and LeviLamina.
- `hooks/` and `mod/` hold the LeviLamina hooks and the mod lifecycle.