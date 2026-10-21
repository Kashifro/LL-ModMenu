# LL-ModMenu

A LeviLamina client mod for browsing installed mods, viewing their details, and opening their settings.

Mod authors can start with the [registry and settings API](docs/ModMenuAPI.md), the public [C API header](src/modmenu/api/ModMenuAPI.h), and the [source layout](docs/SourceLayout.md).

## Building

Build on Windows x64 with xmake and LeviLamina 26.51.7. xmake fetches [MossUI](https://github.com/Kashifro/MossUI).

```powershell
xmake f -P . -y -p windows -a x64 -m release
xmake -P .
```

The packaged mod is generated under `bin/ModMenu`.

## Contributing

Contributions are welcome. Please [report bugs or open an issue](https://github.com/Kashifro/LL-ModMenu/issues). Include your LeviLamina and Minecraft versions, reproduction steps, and relevant logs with bug reports.
