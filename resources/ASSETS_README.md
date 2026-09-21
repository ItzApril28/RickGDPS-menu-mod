# 🎨 Custom Assets Guide — RickGdps Menu Mod

Place your custom `.png` sprite files directly in this `resources/` folder.
The mod automatically loads all `resources/*.png` files at startup.

## How to Add a Custom Asset

1. **Drop your `.png` file here** (in `resources/`)
2. **Open `src/ModernTheme.hpp`** and update the matching constant
3. **Rebuild the mod** — done!

## Asset Slot Reference

| What It Is                | Constant in `ModernTheme.hpp` | Default GD Texture         | How to Override                        |
|---------------------------|-------------------------------|----------------------------|----------------------------------------|
| **Popup window frame**    | `kSprWindowFrame`             | `GJ_square01.png`          | `"my_frame.png"_spr`                  |
| **Inner content panel**   | `kSprPanelBg`                 | `square02b_001.png`        | `"my_panel.png"_spr`                  |
| **Title badge**           | `kSprTitleBadge`              | `square02b_001.png`        | `"my_badge.png"_spr`                  |
| **Tab backgrounds**       | `kSprTabBg`                   | `square02b_001.png`        | `"my_tab.png"_spr`                    |
| **Row card backgrounds**  | `kSprRowCard`                 | `square02b_001.png`        | `"my_row.png"_spr`                    |
| **Cycle badge pill**      | `kSprBadgeBg`                 | `square02b_001.png`        | `"my_cycle_badge.png"_spr`            |
| **Checkbox OFF**          | `kSprCheckOff`                | `GJ_checkOff_001.png`      | `"my_check_off.png"_spr`              |
| **Checkbox ON**           | `kSprCheckOn`                 | `GJ_checkOn_001.png`       | `"my_check_on.png"_spr`               |
| **Close button (X)**      | `kSprCloseBtn`                | `GJ_closeBtn_001.png`      | `"my_close.png"_spr`                  |
| **Left arrow (cycles)**   | `kSprArrowLeft`               | `edit_leftBtn_001.png`     | `"my_arrow_left.png"_spr`             |
| **Right arrow (cycles)**  | `kSprArrowRight`              | `edit_rightBtn_001.png`    | `"my_arrow_right.png"_spr`            |
| **Settings button icon**  | *(in QuickSettingsButton.cpp)* | `Button.png`_spr          | Replace `resources/Button.png`         |

## Important Notes

- Use the `_spr` suffix to load mod-bundled sprites: `"MyFile.png"_spr`
- Without `_spr`, the engine looks in GD's built-in texture cache
- Scale9Sprite textures (frames, panels) should have proper 9-slice borders
- The `mod.json` already includes `"resources/*.png"` so any PNG you drop here is automatically included

## Current Files

```
resources/
├── Button.png                    — Settings button icon
├── arrow.png                     — Arrow sprite
├── background_hd.png             — Background (HD resolution)
├── background_uhd.png            — Background (UHD resolution)
├── blackscreen background.png    — Transition overlay
├── square_scale_hd.png           — Scalable square (HD)
└── square_scale_uhd.png          — Scalable square (UHD)
```
