#pragma once
#include <cocos2d.h>

// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//  ModernTheme.hpp â€” Central configuration for the Settings UI
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
//
//  HOW TO USE CUSTOM ASSETS:
//
//  1. Place your custom PNG files in:
//       resources/
//
//  2. Reference them below using the "_spr" suffix for Geode sprites:
//       "MyCustomButton.png"_spr   â†’  loads resources/MyCustomButton.png
//
//  3. Or use standard GD sprite frame names (no _spr needed):
//       "GJ_square01.png"          â†’  built-in GD texture
//
//  4. After changing any asset path below, rebuild the mod.
//
// â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•

namespace ModernTheme {

    // â”€â”€ â‘  CUSTOM ASSET PATHS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    //
    //  Change these to point to YOUR OWN PNGs in the resources/ folder.
    //  Use  "filename.png"_spr  for mod-bundled sprites in resources/.
    //  Use  "filename.png"      for GD built-in sprite frames.
    //
    //  Current resources/ contents:
    //    resources/Button.png              â€” Main settings button icon
    //    resources/arrow.png               â€” Cycle row arrow
    //    resources/background_hd.png       â€” Background texture (HD)
    //    resources/background_uhd.png      â€” Background texture (UHD)
    //    resources/blackscreen background.png â€” Transition screen
    //    resources/square_scale_hd.png     â€” Scalable square (HD)
    //    resources/square_scale_uhd.png    â€” Scalable square (UHD)
    //
    //  To add new custom assets:
    //    1. Drop your .png file into the resources/ folder
    //    2. Update the path constant below
    //    3. The mod.json already loads resources/*.png automatically
    //

    // â€” Window frame (the main popup outer border)
    //   Default: GD built-in "GJ_square01.png"
    //   Custom:  Put your frame in resources/ and use "my_frame.png"_spr
    inline constexpr char const* kSprWindowFrame = "square02b_001.png";

    // â€” Dark inner content panel background
    //   Default: GD built-in "square02b_001.png"
    inline constexpr char const* kSprPanelBg = "square02b_001.png";

    // â€” Title badge (the "SETTINGS" header pill)
    //   Default: GD built-in "square02b_001.png"
    inline constexpr char const* kSprTitleBadge = "square02b_001.png";

    // â€” Tab button backgrounds (SHADERS / AUDIO / MODS)
    //   Default: GD built-in "square02b_001.png"
    inline constexpr char const* kSprTabBg = "square02b_001.png";

    // â€” Settings row card backgrounds
    //   Default: GD built-in "square02b_001.png"
    inline constexpr char const* kSprRowCard = "square02b_001.png";

    // â€” Cycle option badge pill background
    //   Default: GD built-in "square02b_001.png"
    inline constexpr char const* kSprBadgeBg = "square02b_001.png";

    // â€” Toggle checkbox sprites (on/off)
    //   Default: GD built-in checkboxes
    //   Custom:  "my_check_off.png"_spr / "my_check_on.png"_spr
    inline constexpr char const* kSprCheckOff = "GJ_checkOff_001.png";
    inline constexpr char const* kSprCheckOn = "GJ_checkOn_001.png";

    // â€” Close button (X) sprite
    //   Default: GD built-in close button
    inline constexpr char const* kSprCloseBtn = "GJ_closeBtn_001.png";

    // â€” Cycle row left/right arrow buttons
    //   Default: GD built-in edit arrows
    //   Custom:  "my_arrow_left.png"_spr / "my_arrow_right.png"_spr
    inline constexpr char const* kSprArrowLeft = "edit_leftBtn_001.png";
    inline constexpr char const* kSprArrowRight = "edit_rightBtn_001.png";

    // â€” Main settings button icon (shown in pause/menu)
    //   Default: "Button.png"_spr (bundled in resources/)
    //   Custom:  Change to your own sprite name or GD sprite frame
    inline constexpr char const* kSprMenuBtn = "Button.png"_spr;
    inline constexpr GLubyte kMenuBtnOpacity = 255;

    // â”€â”€ â‘¡ FONT ASSETS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    //  To use custom fonts, place .fnt + .png in resources/ and use _spr.
    //  Default fonts are GD built-ins.
    inline constexpr char const* kFontTitle = "goldFont.fnt";
    inline constexpr char const* kFontLabels = "bigFont.fnt";
    inline constexpr char const* kFontValues = "chatFont.fnt";

    // â”€â”€ â‘¢ DIMENSIONS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    inline constexpr float kPopupWidth = 390.f;
    inline constexpr float kPopupHeight = 270.f;
    inline constexpr float kTabHeight = 30.f;
    inline constexpr float kRowHeight = 34.f;

    // â”€â”€ â‘£ THEME COLOR PALETTE â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
    //
    //  Current palette: Teal / Slate / Cream
    //
    //    Window Frame (Slate Teal):   #447272  RGB(68, 114, 114)
    //    Deep Teal accents:           #004040  RGB(0, 64, 64)
    //    Cream / Butter text:         #FFFFC0  RGB(255, 255, 192)
    //
    //  To change colors, edit the RGB values below.
    //  Tip: Use an online color picker and convert hex â†’ RGB.
    //

    // Window outer frame tint & opacity (0-255)
    inline constexpr cocos2d::ccColor3B kWindowBgColor = {0, 45, 45};
    inline constexpr GLubyte kWindowBgOpacity = 200;

    // Title badge header background & opacity
    inline constexpr cocos2d::ccColor3B kTitleBadgeColor = {0, 64, 64};
    inline constexpr GLubyte kTitleBadgeOpacity = 240;

    // Tab bar - selected tab
    inline constexpr cocos2d::ccColor3B kActiveTabColor = {0, 64, 64};
    inline constexpr GLubyte kActiveTabOpacity = 255;

    // Tab bar - unselected tabs
    inline constexpr cocos2d::ccColor3B kInactiveTabColor = {35, 75, 75};
    inline constexpr GLubyte kInactiveTabOpacity = 220;

    // Dark inner content panel & opacity
    inline constexpr cocos2d::ccColor3B kContentPanelColor = {0, 52, 52};
    inline constexpr GLubyte kContentPanelOpacity = 255;

    // Individual row card slots & opacity
    inline constexpr cocos2d::ccColor3B kRowBgColor = {0, 68, 68};
    inline constexpr GLubyte kRowBgOpacity = 230;

    // Cycle option badge pill & opacity
    inline constexpr cocos2d::ccColor3B kBadgeColor = {0, 44, 44};
    inline constexpr GLubyte kBadgeOpacity = 240;

    // Value readout text color (sliders, cycle labels)
    inline constexpr cocos2d::ccColor3B kValueTextColor = {255, 255, 192};

    // Section header text color
    inline constexpr cocos2d::ccColor3B kSectionTitleColor = {255, 255, 192};

    // Section title underline (RGBA)
    inline constexpr cocos2d::ccColor4B kSectionLineColor = {255, 255, 192, 120};

    // Divider line between groups (RGBA)
    inline constexpr cocos2d::ccColor4B kDividerColor = {0, 36, 36, 150};

} // namespace ModernTheme
