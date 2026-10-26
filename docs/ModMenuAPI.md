# LL-ModMenu

Every mod LeviLamina has loaded is listed in LL-ModMenu. You choose how much more to give it, from nothing to a screen of your own:

1. **Config pages** (`config/config.json` + optional `config/modmenu.json`).
2. **Manifest metadata** (`manifest.json` `extraInfo`): icon, links, and badges.
3. **Your own config screen**: one optional DLL export `ConfigScreenFactory`.

Everything optional is found by looking inside your mod's own files and DLL,

## 1. Config pages (`config/config.json` + `config/modmenu.json`)

LL-ModMenu builds a settings page for every loaded mod that has `<mod folder>/config/config.json`,

It reads the *shape* of each value from `config.json` and takes everything else (limits, allowed values, colors, defaults, labels, tooltips, visibility) only from what you declare in `config/modmenu.json`. If you want a slider, a dropdown, or a color picker, say so in `modmenu.json`.

### 1.1 What a value becomes with no declaration

| Value in `config.json` | Control |
| :--- | :--- |
| `true` / `false` | Toggle |
| A number (`10`, `1.5`) | A typed number field (no limits). Invalid text is ignored; integers stay integers. |
| A string | Text field |
| A `"#rrggbb"` string | Color picker (the shape is unambiguous, so no declaration needed) |
| An object `{ ... }` | Category holding its own rows |
| Arrays and `null` | Not shown |

Rows appear in the order of the file, and LL-ModMenu keeps that order when it rewrites the file. Only keys present in `config.json` get a row, so write your defaults into the file. The page is built when it is first opened, so keys added later show up the next time the game starts.

### 1.2 `config/modmenu.json`

An optional JSON object next to `config.json`. Each key is the **path** of a setting and each value is an object of declarations for it:

```json
{
  "<path>": { "<field>": <value>, ... }
}
```

**Paths** are the keys from the root of `config.json` down to the setting, joined with `/`, with no leading slash: `general/banking/strength`. A key that itself contains `/` is written `~1`, and a key containing `~` is written `~0`. The same escaped string is what `ModMenu_OnConfigChanged` receives. Declarations for a path that does not exist in `config.json` are ignored, except `"type": "action"` (see below).

**Fields** 

| Field | Type | Applies to | Effect |
| :--- | :--- | :--- | :--- |
| `label` | string | any row, category, action | Display name. Default: the key with `_` turned into spaces and the first letter capitalised. |
| `tip` | string | any row, category, action | Hover tooltip. There is no tooltip unless you write one. |
| `hidden` | bool | any row, category, action | `true` removes it from the page. Use it for keys like `format_version_do_not_edit`. |
| `shown_if` | string | any row, category, action | Path of a boolean setting. The row is shown only while that setting is `true`; prefix `!` to show it while `false`. |
| `default` | same type as the value | toggle, number, text, choice, color, keybind | What **Reset** restores. Without it, Reset restores the value the row had when the page was opened, which is only an undo. |
| `min`, `max` | number | number | **Both** are required to get a slider. With only one (or neither) the number stays a typed field. |
| `step`, `decimals` | number | slider | Precision. Defaults: `1` and `0` for integers, `0.01` and `2` for decimals. |
| `unit` | string | slider | Suffix shown on the slider, e.g. `"%"`. |
| `choices` | array of strings | string or integer | Dropdown. A string value is written back as the chosen string; an integer value as the 0-based index. `default` is a string from `choices` (or an index for an integer). |
| `type` | `"color"` | integer | Color picker. The value is `0xRRGGBB` written as a decimal integer; an existing alpha byte is kept. (`"#rrggbb"` strings need no `type`; their letter case is kept.) |
| `type` | `"keybind"` | integer | Key-binding row. The value is the key code. |
| `type` | `"action"` | a path with **no** value in `config.json` | A button at the end of the top-level page, in the order written. Clicking it calls `ModMenu_OnConfigChanged(path)`. `confirm: true` asks first. If `config.json` does have a value at that path, the value's row wins. |

A `type` that does not fit the value (for example `"keybind"` on a string) is ignored and the value falls back to the control from 1.1.

### 1.3 Declare everything

For each setting, this is all you need to write:

| Your setting is... | Declare in `modmenu.json` |
| :--- | :--- |
| A number with real bounds | `min`, `max` (+ `step`, `decimals`, `unit`) |
| A number with no bounds | Nothing; it is a typed field |
| One of a fixed set of strings or indexes | `choices` |
| A color stored as an integer | `"type": "color"` |
| A key code | `"type": "keybind"` |
| Something only valid when another toggle is on | `shown_if` |
| Not for players to edit | `hidden: true` |
| Anything that should reset to a known value | `default` |
| A button with no stored value | `"type": "action"` |
| Unclear from its key name | `label`, `tip` |

### 1.4 Full example

`config/config.json`:

```json
{
  "enabled": true,
  "format_version_do_not_edit": 2,
  "flight": {
    "mode": "HYBRID",
    "strength": 20,
    "max_speed": 1.5,
    "trail_color": 16711680,
    "accent": "#ff8800",
    "boost_key": 70
  }
}
```

`config/modmenu.json`:

```json
{
  "format_version_do_not_edit": { "hidden": true },
  "enabled": { "label": "Enable feature", "default": false },
  "speed": { "label": "Speed Multiplier", "shown_if": "enabled" },
  "speed/mode": { "choices": ["SLOW", "NORMAL", "TURBO"], "default": "NORMAL", "tip": "Speed acceleration profile" },
  "speed/multiplier": { "min": 1, "max": 10, "step": 1, "decimals": 0, "unit": "x", "default": 2 },
  "speed/max_limit": { "default": 5.0, "tip": "Maximum velocity cap; configurable" },
  "speed/particle_color": { "type": "color", "default": 65280 },
  "speed/trigger_key": { "type": "keybind", "default": 82 },
  "reset_speed": { "type": "action", "label": "Reset speed settings", "confirm": true, "shown_if": "enabled" }
}
```

### 1.5 Applying changes

LL-ModMenu writes each edit straight into `config.json`. Your mod only sees an edit when it re reads the file. To react immediately, export one function from your mod's DLL; LL-ModMenu finds it on the mod's loaded DLL and calls it after every write and when an `action` button is clicked, passing the escaped path from 1.2:

```cpp
extern "C" __declspec(dllexport) void ModMenu_OnConfigChanged(const char* path) {
    // re-read config.json, or switch on `path` for an action button
}
```

This works for loaded native mods. Other mods still get their page; they just are not notified.

## 2. Manifest Metadata

LL-ModMenu reads your mod's `manifest.json` and its `extraInfo` block to populate the mod details panel, icon, badges, and external links:

```json
{
  "name": "ExampleMod",
  "entry": "ExampleMod.dll",
  "version": "1.0.0",
  "type": "native",
  "platform": "client",
  "author": "YourName",
  "description": "Short description shown in LL-ModMenu.",
  "extraInfo": {
    "icon": "icon.png",
    "website": "https://example.com",
    "source": "https://github.com/yourname/ExampleMod",
    "issues": "https://github.com/yourname/ExampleMod/issues",
    "license": "MIT",
    "wiki": "https://github.com/yourname/ExampleMod/wiki",
    "discord": "https://discord.gg/example",
    "support": "https://github.com/sponsors/yourname",
    "donate": "https://ko-fi.com/yourname"
  }
}
```

- **`icon`**: Either a `.png` file in the root of your mod folder (e.g. `"icon.png"`) or a resource pack texture path starting with `"textures/"`.
- **Badges**: Mods with `"type": "preload-native"` automatically receive the `Library` badge; client mods receive the `Client` badge.

## 3. Your own config screen

With `ConfigScreenFactory`, a mod can bring its own screen. Export this from the mod's DLL and the **Configure** button calls it instead of showing the generated page:

```cpp
extern "C" __declspec(dllexport) void ModMenu_OpenConfigScreen() {
    // open your own screen with whatever UI you use
}
```

LL-ModMenu does not host or lay out that screen, so there is no builder API.
