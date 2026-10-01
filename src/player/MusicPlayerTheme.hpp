#pragma once

#include "../ModernTheme.hpp"

#include <cocos2d.h>

// ═════════════════════════════════════════════════════════════════════════════
//  MusicPlayerTheme.hpp — Central Resource Configuration for Music Player Studio
// ═════════════════════════════════════════════════════════════════════════════
//
//  Colors and assets are copied directly from ModernTheme.hpp:
//    - Deep Teal Window Frame:       RGB(0, 45, 45)    Opacity: 200
//    - Card & Content Panels:        RGB(0, 68, 68)    Opacity: 230
//    - Active Tabs & Selections:     RGB(0, 64, 64)    Opacity: 255
//    - Unselected Tabs:              RGB(35, 75, 75)   Opacity: 220
//    - Buttons & Badges:             RGB(0, 44, 44)    Opacity: 240
//    - Cream / Butter Text Accents:  RGB(255, 255, 192)
//
//  HOW TO CUSTOMIZE:
//  - You can edit any value below, or swap sprite paths with "my_sprite.png"_spr
//    placed in your resources/ folder.
// ═════════════════════════════════════════════════════════════════════════════

namespace rickgdps::music::theme {

    // Every player element exposes its position, size and scale here. Positions
    // are relative to the element's immediate parent; edit these values to make
    // a custom player layout without touching the UI implementation.
    struct ElementLayout {
        float x;
        float y;
        float width;
        float height;
        float scale;
    };

    // ─── 1. SPRITES & TEXTURES ───────────────────────────────────────────────

    // Popup window outer frame with classic curved corners
    inline constexpr char const* kSprWindowFrame = "square02b_001-hd.png";

    // Card & deck container backgrounds with curved corners
    inline constexpr char const* kSprCardBg = "square02b_001-hd.png";

    // Sub-tab button background with curved pill shape
    inline constexpr char const* kSprTabBg = "square02b_001-hd.png";

    // Native Geometry Dash long button from GJ_GameSheet03.plist.
    // It is a sprite-frame (not a loose file), so makeTextButton loads it
    // through the frame cache before trying a regular resource path.
    inline constexpr char const* kSprButtonBg = "square02b_001-hd.png";

    // Equalizer slider thumb handle with curved corners
    inline constexpr char const* kSprSliderThumb = "square02b_001-hd.png";

    // Close button (X)
    inline constexpr char const* kSprCloseBtn = ModernTheme::kSprCloseBtn;

    // ─── 2. FONTS ────────────────────────────────────────────────────────────

    // Popup header title font (matches ModernTheme::kFontTitle)
    inline constexpr char const* kFontTitle = ModernTheme::kFontTitle;

    // Track name font (matches ModernTheme::kFontLabels)
    inline constexpr char const* kFontTrackTitle = ModernTheme::kFontLabels;

    // Artist name & secondary descriptions font (matches ModernTheme::kFontValues)
    inline constexpr char const* kFontArtist = ModernTheme::kFontValues;

    // Section headers & button labels font (matches ModernTheme::kFontTitle)
    inline constexpr char const* kFontSectionTitle = ModernTheme::kFontTitle;

    // Numeric readouts (dB, time mm:ss, frequencies) (matches ModernTheme::kFontValues)
    inline constexpr char const* kFontValues = ModernTheme::kFontValues;

    // ─── 3. COLOR PALETTE (COPIED DIRECTLY FROM ModernTheme.hpp) ─────────────

    // Window popup background tint & opacity (matches ModernTheme::kWindowBgColor)
    inline constexpr cocos2d::ccColor3B kWindowBgColor = ModernTheme::kWindowBgColor; // {0, 45, 45}
    inline constexpr GLubyte kWindowBgOpacity = ModernTheme::kWindowBgOpacity; // 200

    // Card & deck container background color & opacity (matches ModernTheme::kRowBgColor)
    inline constexpr cocos2d::ccColor3B kCardBgColor = ModernTheme::kRowBgColor; // {0, 68, 68}
    inline constexpr GLubyte kCardBgOpacity = ModernTheme::kRowBgOpacity; // 230

    // Accent highlight color (matches ModernTheme::kValueTextColor cream / butter)
    inline constexpr cocos2d::ccColor3B kAccentColor =
        ModernTheme::kValueTextColor; // {255, 255, 192}

    // Sub-tab bar: Selected tab (matches ModernTheme::kActiveTabColor)
    inline constexpr cocos2d::ccColor3B kTabActiveColor =
        ModernTheme::kActiveTabColor; // {0, 64, 64}
    inline constexpr GLubyte kTabActiveOpacity = ModernTheme::kActiveTabOpacity; // 255

    // Sub-tab bar: Unselected tab (matches ModernTheme::kInactiveTabColor)
    inline constexpr cocos2d::ccColor3B kTabInactiveColor =
        ModernTheme::kInactiveTabColor; // {35, 75, 75}
    inline constexpr GLubyte kTabInactiveOpacity = ModernTheme::kInactiveTabOpacity; // 220

    // Control buttons default background color & opacity (matches ModernTheme::kBadgeColor)
    inline constexpr cocos2d::ccColor3B kButtonBgColor = ModernTheme::kBadgeColor; // {0, 44, 44}
    inline constexpr GLubyte kButtonBgOpacity = ModernTheme::kBadgeOpacity; // 240

    // Control button label text color (matches ModernTheme::kValueTextColor)
    inline constexpr cocos2d::ccColor3B kButtonTextColor =
        ModernTheme::kValueTextColor; // {255, 255, 192}

    // Currently playing track highlight in playlist (matches ModernTheme::kActiveTabColor)
    inline constexpr cocos2d::ccColor3B kPlaylistActiveColor =
        ModernTheme::kActiveTabColor; // {0, 64, 64}

    // ─── 3b. PLAYER UI STYLE TOKENS ─────────────────────────────────────────
    // Keep these separate from ModernTheme so Music Studio can be recoloured
    // without changing the rest of the mod's settings UI.
    inline constexpr cocos2d::ccColor3B kContentPanelColor = {0, 36, 36};
    inline constexpr GLubyte kContentPanelOpacity = 238;
    inline constexpr cocos2d::ccColor3B kTrackTitleColor = {255, 255, 255};
    inline constexpr cocos2d::ccColor3B kArtistTextColor = {140, 195, 195};
    inline constexpr cocos2d::ccColor3B kMetaTextColor = {160, 210, 210};
    inline constexpr cocos2d::ccColor3B kTabTextColor = {160, 200, 200};
    inline constexpr cocos2d::ccColor3B kEqNeutralTextColor = {180, 210, 210};
    inline constexpr cocos2d::ccColor3B kEqBoostTextColor = {100, 240, 170};
    inline constexpr cocos2d::ccColor3B kEqCutTextColor = {240, 120, 100};

    // ─── 4. VISUALIZER BAR COLORS ────────────────────────────────────────────

    // Base color for low-energy visualizer bars (Teal)
    inline constexpr cocos2d::ccColor3B kVisLowColor = {0, 160, 160};

    // Peak color for high-energy visualizer bars (Cream / Butter from ModernTheme)
    inline constexpr cocos2d::ccColor3B kVisPeakColor =
        ModernTheme::kValueTextColor; // {255, 255, 192}

    // Number of visualizer spectrum bars (16 to 32)
    inline constexpr std::size_t kVisualizerBarCount = 20;

    // ─── 5. EQUALIZER COLORS ─────────────────────────────────────────────────

    // Boosted gain (+dB) column fill color (Teal Boost)
    inline constexpr cocos2d::ccColor4B kEqBoostColor = {0, 200, 180, 230};

    // Cut gain (-dB) column fill color (Warm Coral Cut)
    inline constexpr cocos2d::ccColor4B kEqCutColor = {210, 100, 85, 230};

    // Slider track background column color (matches ModernTheme::kDividerColor)
    inline constexpr cocos2d::ccColor4B kEqTrackColor =
        ModernTheme::kDividerColor; // {0, 36, 36, 150}

    // 0 dB center reference line color (matches ModernTheme::kSectionLineColor)
    inline constexpr cocos2d::ccColor4B kEqCenterLineColor =
        ModernTheme::kSectionLineColor; // {255, 255, 192, 120}

    // ─── 6. DIMENSIONS & SIZING ──────────────────────────────────────────────

    inline constexpr float kPopupWidth = 460.f;
    inline constexpr float kPopupHeight = 310.f;

    inline constexpr float kNowPlayingCardW = 432.f;
    inline constexpr float kNowPlayingCardH = 92.f;

    inline constexpr float kFxDeckW = 432.f;
    inline constexpr float kFxDeckH = 74.f;

    // ─── 7. ELEMENT LAYOUT ─────────────────────────────────────────────────
    inline constexpr ElementLayout kContentPanelLayout{0.f, 12.f, 436.f, 244.f, 1.f};
    inline constexpr ElementLayout kTabMenuLayout{0.f, -40.f, 0.f, 0.f, 1.f};
    inline constexpr ElementLayout kTabButtonLayout{0.f, 0.f, 100.f, 22.f, 0.32f};
    inline constexpr ElementLayout kNowPlayingLayout{0.f, -2.f, 432.f, 86.f, 1.f};
    inline constexpr ElementLayout kAccentStripeLayout{8.f, 8.f, 4.f, 70.f, 1.f};
    inline constexpr ElementLayout kTrackTitleLayout{20.f, -8.f, 396.f, 0.f, 0.42f};
    inline constexpr ElementLayout kArtistLayout{20.f, -30.f, 396.f, 0.f, 0.30f};
    inline constexpr ElementLayout kVisualizerLayout{18.f, 8.f, 396.f, 28.f, 1.f};
    inline constexpr ElementLayout kScrubberLayout{0.f, -20.f, 320.f, 16.f, 0.75f};
    inline constexpr ElementLayout kControlsLayout{0.f, -46.f, 0.f, 0.f, 1.f};
    inline constexpr ElementLayout kFxDeckLayout{0.f, 4.f, 432.f, 64.f, 1.f};
    inline constexpr ElementLayout kFxTitleLayout{0.f, -9.f, 0.f, 0.f, 0.28f};
    inline constexpr ElementLayout kPlaylistLayout{0.f, 2.f, 432.f, 224.f, 1.f};
    inline constexpr ElementLayout kEffectsLayout{0.f, 2.f, 432.f, 224.f, 1.f};
    inline constexpr ElementLayout kEqualizerLayout{0.f, 2.f, 432.f, 224.f, 1.f};

} // namespace rickgdps::music::theme
