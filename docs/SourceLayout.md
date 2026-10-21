# Source layout

- `api/` contains the versioned C ABI, its exports, and registration and screen-builder implementation.
- `settings/` contains built-in LL-ModMenu settings, settings-factory caching, and key conflict lookup.
- `view/` contains menu screens and shared drawing helpers. `view/internal/` holds private view declarations.
- `internal/` contains LL-ModMenu registry and settings declarations used by implementation files.
- `Registry.cpp`, `ModDiscovery.cpp`, `ModMenu.cpp`, and `ModMenu.hpp` are the top-level LL-ModMenu lifecycle and registry entry points.
- `src/MossMC/` adapts MossUI to Minecraft and LeviLamina.
- `src/nugget/` contains the platform-facing settings, keybind, logging, and client-context support.
- `src/hooks/` and `src/mod/` contain LeviLamina hooks and mod lifecycle code.
