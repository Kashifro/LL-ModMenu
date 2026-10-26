# LL-ModMenu

A LeviLamina client mod for browsing installed mods, viewing their details, and opening their settings.

- A searchable, sortable list of every loaded mod, with icon, badges, links, and license from its `manifest.json`.
- A settings page for any mod that has a `config/config.json`, with no code on the mod's side. An optional `config/modmenu.json` declares everything the JSON cannot say: slider limits, dropdowns, colors, keybinds, defaults, tooltips, and buttons. Nothing is guessed.
- A Configure button that opens a mod's own screen if it exports `ModMenu_OpenConfigScreen`.
- Optional in-game access: turn on "Enable in game" in LL-ModMenu's settings and press the key set in Settings > Controls (default M).

Mod authors can start with the [mod integration guide](docs/ModMenuAPI.md) and the [source layout](docs/SourceLayout.md).

## Building

Build on Windows x64 with xmake and LeviLamina 26.51.7.

```powershell
xmake f -P . -y -p windows -a x64 -m release
xmake -P .
```

The packaged mod is generated under `bin/ModMenu`.

## Contributing

Contributions are welcome. Please [report bugs or open an issue](https://github.com/Kashifro/LL-ModMenu/issues). Include your LeviLamina and Minecraft versions, reproduction steps, and relevant logs with bug reports.
