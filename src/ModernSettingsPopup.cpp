#include "ModernSettingsPopup.hpp"
#include "ModernTheme.hpp"
#include <Geode/Geode.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/ui/SliderNode.hpp>
#include <algorithm>
#include <cstdio>
#include <cmath>

using namespace geode::prelude;

// ============================================================================
//  UI Layout & Sizing Configurations
// ============================================================================
namespace UIConfig {
    // Top Title Badge
    constexpr float kTitleBadgeW = 150.f;
    constexpr float kTitleBadgeH = 26.f;
    constexpr float kTitleBadgeYOffset = 18.f; // Distance from top edge
    constexpr float kTitleTextScale = 0.55f;

    // Close Button Position Offset
    constexpr cocos2d::CCPoint kCloseBtnPos = {18.f, 18.f}; // Relative to top-left ({X, Top-Y})

    // Tab Bar & Buttons
    constexpr float kTabGap = 6.f;
    constexpr float kTabTextScale = 0.40f;

    // Checkbox / Toggle Button
    constexpr float kCheckboxScale = 0.6f;
    constexpr float kCheckboxRightPadding = 24.f;

    // Cycle / Option Selector Button
    constexpr float kCycleBadgeMaxW = 150.f;
    constexpr float kCycleBadgeH = 22.f;
    constexpr float kCycleBadgeRightPadding = 8.f;
    constexpr float kCycleArrowScale = 0.5f;
    constexpr float kCycleArrowPadding = 10.f; // Offset from left/right edges of badge

    // Slider Row
    constexpr float kSliderScale = 0.65f;
    constexpr cocos2d::CCPoint kSliderPos = {12.f, 16.f};
    constexpr float kFloatRowHeightAdd = 18.f; // Extra height added to standard row height
}

ModernSettingsPopup* ModernSettingsPopup::create() {
    auto* ret = new ModernSettingsPopup();
    if (ret && ret->init(ModernTheme::kPopupWidth, ModernTheme::kPopupHeight)) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}

bool ModernSettingsPopup::init(float width, float height) {
    if (!Popup::init(width, height, ModernTheme::kSprWindowFrame)) {
        return false;
    }
    return setup();
}

bool ModernSettingsPopup::setup() {
    auto const bgSize = m_bgSprite->getContentSize();

    // ── Outer Window Frame ───────────────────────────────────────────────────
    m_bgSprite->setColor(ModernTheme::kWindowBgColor);
    m_bgSprite->setOpacity(ModernTheme::kWindowBgOpacity);

    // ── Header: Title Badge with "SETTINGS" ──────────────────────────────────
    auto* titleBadge = CCScale9Sprite::create(ModernTheme::kSprTitleBadge);
    if (!titleBadge) titleBadge = CCScale9Sprite::create("GJ_square01.png");
    titleBadge->setContentSize({UIConfig::kTitleBadgeW, UIConfig::kTitleBadgeH});
    titleBadge->setColor(ModernTheme::kTitleBadgeColor);
    titleBadge->setOpacity(ModernTheme::kTitleBadgeOpacity);
    titleBadge->setPosition({bgSize.width * 0.5f, bgSize.height - UIConfig::kTitleBadgeYOffset});
    m_mainLayer->addChild(titleBadge, 4);

    auto* header = CCLabelBMFont::create("SETTINGS", ModernTheme::kFontTitle);
    header->setScale(UIConfig::kTitleTextScale);
    header->setPosition({UIConfig::kTitleBadgeW * 0.5f, UIConfig::kTitleBadgeH * 0.5f + 1.f});
    titleBadge->addChild(header, 5);

    // ── Tab Bar (Shaders | Audio | Mods) ────────────────────────────────────
    m_tabMenuNode = CCNode::create();
    m_tabMenuNode->setPosition({14.f, bgSize.height - 66.f});
    m_tabMenuNode->setContentSize({bgSize.width - 28.f, ModernTheme::kTabHeight});
    m_mainLayer->addChild(m_tabMenuNode, 5);

    buildTabs();

    // ── Main Content Container (Dark Inset Panel) ────────────────────────────
    float contentX = 14.f;
    float contentY = 14.f;
    float contentW = bgSize.width - 28.f;
    float contentH = bgSize.height - 86.f;

    auto* panelBg = CCScale9Sprite::create(ModernTheme::kSprPanelBg);
    if (!panelBg) panelBg = CCScale9Sprite::create("GJ_square05.png");
    panelBg->setContentSize({contentW, contentH});
    panelBg->setAnchorPoint({0.f, 0.f});
    panelBg->setPosition({contentX, contentY});
    panelBg->setColor(ModernTheme::kContentPanelColor);
    panelBg->setOpacity(ModernTheme::kContentPanelOpacity);
    m_mainLayer->addChild(panelBg, 1);

    m_contentArea = CCNode::create();
    m_contentArea->setContentSize({contentW, contentH});
    m_contentArea->setPosition({contentX, contentY});
    m_mainLayer->addChild(m_contentArea, 2);

    selectTab(Tab::Shaders);

    // Reposition close button using configuration values
    if (m_closeBtn) {
        m_closeBtn->setPosition({UIConfig::kCloseBtnPos.x, bgSize.height - UIConfig::kCloseBtnPos.y});
    }

    return true;
}

void ModernSettingsPopup::buildTabs() {
    m_tabMenuNode->removeAllChildren();
    m_tabButtons.clear();

    auto const totalW = m_tabMenuNode->getContentSize().width;
    auto* menu = CCMenu::create();
    menu->setPosition({0.f, 0.f});
    m_tabMenuNode->addChild(menu);

    struct TabDef {
        Tab tab;
        char const* name;
    };
    TabDef tabs[] = {
        {Tab::Shaders, "SHADERS"},
        {Tab::Audio,   "AUDIO"},
        {Tab::Mods,    "MODS"}
    };

    float tabW = (totalW - UIConfig::kTabGap * 2.f) / 3.f;

    for (int i = 0; i < 3; ++i) {
        auto const& t = tabs[i];
        bool isActive = (t.tab == m_activeTab);

        auto* tabBg = CCScale9Sprite::create(ModernTheme::kSprTabBg);
        if (!tabBg) tabBg = CCScale9Sprite::create("GJ_square01.png");
        tabBg->setContentSize({tabW, ModernTheme::kTabHeight});
        tabBg->setColor(isActive ? ModernTheme::kActiveTabColor : ModernTheme::kInactiveTabColor);
        tabBg->setOpacity(isActive ? ModernTheme::kActiveTabOpacity : ModernTheme::kInactiveTabOpacity);

        auto* tabLbl = CCLabelBMFont::create(t.name, ModernTheme::kFontTitle);
        tabLbl->setScale(UIConfig::kTabTextScale);
        tabLbl->setPosition({tabW * 0.5f, ModernTheme::kTabHeight * 0.5f});
        tabLbl->setOpacity(isActive ? 255 : 180);
        tabBg->addChild(tabLbl);

        auto tabVal = t.tab;
        auto* btn = CCMenuItemExt::createSpriteExtra(tabBg, [this, tabVal](CCObject*) {
            this->selectTab(tabVal);
        });
        btn->setPosition({i * (tabW + UIConfig::kTabGap) + tabW * 0.5f, ModernTheme::kTabHeight * 0.5f});
        menu->addChild(btn);
        m_tabButtons.push_back(btn);
    }
}

void ModernSettingsPopup::selectTab(Tab tab) {
    m_activeTab = tab;
    buildTabs();
    buildTabContent(tab);
}

void ModernSettingsPopup::buildTabContent(Tab tab) {
    m_contentArea->removeAllChildren();
    auto const cSize = m_contentArea->getContentSize();

    float const scrollW = cSize.width - 6.f;
    float const scrollH = cSize.height - 8.f;
    float const listW = scrollW - 6.f;

    auto* scroll = ScrollLayer::create({scrollW, scrollH});
    scroll->setPosition({3.f, 4.f});

    scroll->m_contentLayer->setLayout(
        ColumnLayout::create()
            ->setGap(4.f)
            ->setAxisReverse(true)
            ->setAxisAlignment(AxisAlignment::End)
            ->setCrossAxisAlignment(AxisAlignment::Center)
            ->setAutoGrowAxis(scrollH)
            ->setAutoScale(false)
    );

    auto addRow = [&](CCNode* r) {
        if (!r) return;
        scroll->m_contentLayer->addChild(r);
    };

    if (tab == Tab::Shaders) {
        addRow(makeSectionTitle(listW, "Master Shader Controls"));
        addRow(makeBoolRow(listW, "enable-fun", "Stylized & Fun Shaders", "Master toggle for shader post-processing."));
        addRow(makeBoolRow(listW, "global-shader-enabled", "Global Menu Shaders", "Apply shaders across all scenes and menus."));
        addRow(makeCycleRow(listW, "effect-preset", "Effect Theme Preset",
            {"Custom","Cyberpunk","Retro","Cinema","Dreamscape","Comic","Noir","Arcade","Ethereal","Glitch","RTX","Nostalgic","Deep Water"}));
        addRow(makeDivider(listW));

        addRow(makeSectionTitle(listW, "Display & Resolution Scaling"));
        addRow(makeFloatRow(listW, "render-scale", "Render Scale", 0.25f, 2.0f, 0.05f, "x"));
        addRow(makeCycleRow(listW, "upscale-method", "Upscaling / Reconstruction", {"FSR 1", "Nearest"}));
        addRow(makeBoolRow(listW, "drs-enabled", "Dynamic Render Scaling (DRS)", "Auto-adjust resolution to maintain target FPS."));
        addRow(makeCycleRow(listW, "drs-target-fps", "DRS Target FPS", {"Screen Refresh Rate", "60 FPS", "120 FPS", "144 FPS", "165 FPS", "240 FPS", "360 FPS", "Custom"}));
        addRow(makeFloatRow(listW, "drs-min-scale", "DRS Min Scale", 0.25f, 1.0f, 0.05f, "x"));
        addRow(makeFloatRow(listW, "drs-max-scale", "DRS Max Scale", 0.50f, 2.0f, 0.05f, "x"));
        addRow(makeDivider(listW));

        addRow(makeSectionTitle(listW, "Anti-Aliasing & Sharpening"));
        addRow(makeCycleRow(listW, "aa-method", "Anti-Aliasing",
            {"Off","FXAA","SMAA High","SMAA Ultra","SSAA 1.5x (Super-Sampling)","SSAA 2.0x (Super-Sampling)"}));
        addRow(makeBoolRow(listW, "cas-enabled", "AMD FidelityFX CAS", "Contrast-adaptive sharpening filter."));
        addRow(makeFloatRow(listW, "cas-sharpness", "CAS Sharpness", 0.f, 1.f, 0.05f));
        addRow(makeDivider(listW));

        addRow(makeSectionTitle(listW, "Bloom & Glow"));
        addRow(makeBoolRow(listW, "bloom-enabled", "Bloom Glow", "HDR light scattering and glow."));
        addRow(makeCycleRow(listW, "bloom-preset", "Bloom Profile",
            {"Custom","Soft","Dreamy","Neon","Sunset","Frost","Fire","Noir","Pastel","Intense","Nature","Butter Glow","Nostalgic Bloom","Memories"}));
        addRow(makeFloatRow(listW, "bloom-threshold", "Bloom Threshold", 0.f, 1.f, 0.05f));
        addRow(makeFloatRow(listW, "bloom-intensity", "Bloom Intensity", 0.f, 1.5f, 0.05f));
        addRow(makeFloatRow(listW, "bloom-radius", "Bloom Radius", 0.f, 16.f, 1.f));
        addRow(makeBoolRow(listW, "dynamic-bloom-enabled", "Dynamic Pulse Bloom", "Auto-pulsating bloom glow."));
        addRow(makeDivider(listW));

        addRow(makeSectionTitle(listW, "Atmosphere & Distortion"));
        addRow(makeBoolRow(listW, "vignette-enabled", "Vignette Darkening", "Soft perimeter edge darkening."));
        addRow(makeFloatRow(listW, "vignette-strength", "Vignette Amount", 0.f, 1.f, 0.05f));
        addRow(makeBoolRow(listW, "god-rays-enabled", "Volumetric God Rays", "Radial cinematic light shafts."));
        addRow(makeFloatRow(listW, "god-rays-strength", "God Rays Strength", 0.05f, 1.f, 0.05f));
        addRow(makeBoolRow(listW, "lens-flare-enabled", "Anamorphic Lens Flare", "Cinematic horizontal lens flare."));
        addRow(makeFloatRow(listW, "lens-flare-strength", "Lens Flare Amount", 0.f, 1.5f, 0.1f));
        addRow(makeBoolRow(listW, "ambient-enabled", "Ambient Aurora", "Layered aurora curtains drifting through the sky."));
        addRow(makeFloatRow(listW, "ambient-strength", "Aurora Intensity", 0.f, 1.5f, 0.05f));
        addRow(makeBoolRow(listW, "aberration-enabled", "Chromatic Aberration", "RGB color fringe separation."));
        addRow(makeFloatRow(listW, "aberration-strength", "Aberration Strength", 0.f, 0.03f, 0.001f));
        addRow(makeBoolRow(listW, "radial-blur-enabled", "Radial Speed Blur", "High speed radial blur effect."));
        addRow(makeFloatRow(listW, "radial-blur-strength", "Radial Blur Amount", 0.f, 0.1f, 0.005f));
        addRow(makeDivider(listW));

        addRow(makeSectionTitle(listW, "Color Grading & LUTs"));
        addRow(makeBoolRow(listW, "color-edit-enabled", "Color Grade Controls", "LUTs, brightness, contrast, saturation, and temperature."));
        addRow(makeCycleRow(listW, "color-lut", "Color Palette LUT",
            {"Vibrant", "Teal & Orange", "Warm Film", "Cool Night", "Pastel"}));
        addRow(makeFloatRow(listW, "color-brightness", "Brightness", -1.f, 1.f, 0.05f));
        addRow(makeFloatRow(listW, "color-contrast", "Contrast", -1.f, 1.f, 0.05f));
        addRow(makeFloatRow(listW, "color-saturation", "Saturation", -1.f, 1.f, 0.05f));
        addRow(makeFloatRow(listW, "color-temperature", "Color Temperature", -0.25f, 0.25f, 0.02f));
        addRow(makeBoolRow(listW, "cinematic-lut-enabled", "Cinematic LUT", "Hollywood movie color matrix tonemapping."));
        addRow(makeFloatRow(listW, "cinematic-lut-strength", "Cinematic LUT Amount", 0.f, 1.f, 0.05f));
        addRow(makeBoolRow(listW, "sepia-enabled", "Sepia Tone", "Classic vintage warm monochrome tone."));
        addRow(makeFloatRow(listW, "sepia-strength", "Sepia Strength", 0.f, 1.f, 0.05f));
        addRow(makeBoolRow(listW, "grayscale-enabled", "Grayscale", "Classic black and white desaturation."));
        addRow(makeDivider(listW));

        addRow(makeSectionTitle(listW, "Retro, CRT & Stylized"));
        addRow(makeBoolRow(listW, "crt-enabled", "CRT Monitor Scanlines", "Scanlines, subtle flicker & curvature."));
        addRow(makeBoolRow(listW, "vhs-enabled", "VHS Tape Distortion", "Analog tape jitter and noise artifacts."));
        addRow(makeBoolRow(listW, "film-grain-enabled", "Film Grain", "35mm analog grain overlay."));
        addRow(makeFloatRow(listW, "film-grain-strength", "Film Grain Amount", 0.f, 1.f, 0.05f));
        addRow(makeBoolRow(listW, "pixelate-enabled", "Pixel Art Downscale", "Retro mosaic pixelization."));
        addRow(makeBoolRow(listW, "dithering-enabled", "8-Bit Bayer Dithering", "Palette quantization dithering."));
        addRow(makeBoolRow(listW, "halftone-enabled", "Comic Halftone Dots", "Rasterized dot print matrix."));
        addRow(makeFloatRow(listW, "halftone-scale", "Halftone Density", 10.f, 120.f, 5.f));
        addRow(makeBoolRow(listW, "posterize-enabled", "Posterize Tiers", "Posterize color tier reduction."));
        addRow(makeFloatRow(listW, "posterize-levels", "Posterize Levels", 2.f, 16.f, 1.f));
        addRow(makeBoolRow(listW, "ascii-enabled", "ASCII Matrix Display", "Transforms pixels into ASCII text."));
        addRow(makeFloatRow(listW, "ascii-scale", "ASCII Grid Density", 20.f, 1200.f, 10.f));
        addRow(makeBoolRow(listW, "neon-enabled", "Neon Rainbow Cycle", "Animated hue cycle pulse."));
        addRow(makeFloatRow(listW, "neon-speed", "Rainbow Speed", 0.f, 5.f, 0.1f));
        addRow(makeBoolRow(listW, "wobble-enabled", "Camera Wobble", "Gentle organic camera wave displacement."));
        addRow(makeFloatRow(listW, "wobble-speed", "Wobble Speed", 0.1f, 5.f, 0.1f));
        addRow(makeDivider(listW));

        addRow(makeSectionTitle(listW, "Atmospheric Lighting & HDR"));
        addRow(makeBoolRow(listW, "fake-hdr-enabled", "Fake HDR / Dynamic Contrast", "ACES tone-curve, micro-detail contrast, and highlight recovery."));
        addRow(makeFloatRow(listW, "fake-hdr-strength", "HDR Intensity", 0.f, 1.5f, 0.05f));
        addRow(makeBoolRow(listW, "sunset-enabled", "Sunset Atmosphere", "Golden-hour sunset lighting with twilight dusk skies."));
        addRow(makeFloatRow(listW, "sunset-strength", "Sunset Strength", 0.f, 1.5f, 0.05f));
        addRow(makeDivider(listW));

        addRow(makeSectionTitle(listW, "Camera Glass & Film Stock"));
        addRow(makeBoolRow(listW, "butter-camera-enabled", "Butter Camera", "Warm, low-contrast colour grade with lifted blacks and soft creamy tones."));
        addRow(makeFloatRow(listW, "butter-camera-strength", "Butter Camera Strength", 0.f, 1.5f, 0.05f));
        addRow(makeBoolRow(listW, "raindrop-enabled", "Raindrop Glass", "Dynamic glass distortion with animated water refraction."));
        addRow(makeFloatRow(listW, "raindrop-strength", "Raindrop Strength", 0.f, 1.5f, 0.05f));
        addRow(makeBoolRow(listW, "film-polaroid-enabled", "Film Polaroid", "Vintage analog look with film grain, a warm corner light leak and edge vignetting."));
        addRow(makeFloatRow(listW, "film-polaroid-strength", "Film Polaroid Strength", 0.f, 1.5f, 0.05f));
        addRow(makeBoolRow(listW, "fog-enabled", "Fog", "Soft atmospheric haze that desaturates distant layers."));
        addRow(makeFloatRow(listW, "fog-strength", "Fog Density", 0.f, 1.5f, 0.05f));
        addRow(makeBoolRow(listW, "dust-film-enabled", "Dust Film", "Dirty camera lens with fingerprint grime and dust specks."));
        addRow(makeFloatRow(listW, "dust-film-strength", "Dust Film Strength", 0.f, 1.5f, 0.05f));
        addRow(makeDivider(listW));
        addRow(makeSectionTitle(listW, "Custom GLSL Shaders"));
        addRow(makeBoolRow(listW, "custom-shader-enabled", "Custom Shader Pass", "Load external user GLSL post-processing."));
    }
    else if (tab == Tab::Audio) {
        addRow(makeSectionTitle(listW, "8D Audio"));
        addRow(makeBoolRow(listW, "audio-8d", "8D Audio Effect", "Smooth left-to-right binaural rotation around your head with a hint of reverb, like 8D audio on YouTube."));
        addRow(makeFloatRow(listW, "audio-8d-speed", "8D Rotation Speed", 0.05f, 0.50f, 0.01f, "Hz"));
        addRow(makeDivider(listW));

        addRow(makeSectionTitle(listW, "Audio DSP & Reverb"));
        addRow(makeCycleRow(listW, "audio-preset", "Sound Profile Preset",
            {"Custom","GDH Reverb","Spatial","Cinema","Clear","Bass Boost","Late Night","Small Room","Cathedral","Plate"}));
        addRow(makeFloatRow(listW, "audio-reverb", "Reverb Strength", 0.f, 10000.f, 100.f, "ms"));
        addRow(makeFloatRow(listW, "audio-muffle", "Muffle / Lowpass", 0.f, 1.f, 0.05f));
        addRow(makeDivider(listW));

        addRow(makeSectionTitle(listW, "Audio Filters"));
        addRow(makeCycleRow(listW, "audio-filter", "Active Filter",
            {"None","Telephone","Underwater","Lo-Fi Tape","Vintage Radio",
             "Megaphone","Stadium","Bedroom Studio","Concert Hall",
             "Dark Room","Bright & Airy","Warm Tube","Space Echo"}));
        addRow(makeDivider(listW));

        addRow(makeSectionTitle(listW, "10-Band Equalizer (dB)"));
        addRow(makeFloatRow(listW, "audio-eq-30", "30 Hz (Sub Bass)", -6.f, 6.f, 0.5f, "dB"));
        addRow(makeFloatRow(listW, "audio-eq-60", "60 Hz (Bass Kick)", -6.f, 6.f, 0.5f, "dB"));
        addRow(makeFloatRow(listW, "audio-eq-125", "125 Hz (Low Mid)", -9.f, 9.f, 0.5f, "dB"));
        addRow(makeFloatRow(listW, "audio-eq-250", "250 Hz (Warmth)", -9.f, 9.f, 0.5f, "dB"));
        addRow(makeFloatRow(listW, "audio-eq-500", "500 Hz (Midrange)", -9.f, 9.f, 0.5f, "dB"));
        addRow(makeFloatRow(listW, "audio-eq-1000", "1 kHz (Presence)", -9.f, 9.f, 0.5f, "dB"));
        addRow(makeFloatRow(listW, "audio-eq-2000", "2 kHz (Clarity)", -9.f, 9.f, 0.5f, "dB"));
        addRow(makeFloatRow(listW, "audio-eq-4000", "4 kHz (High Mid)", -9.f, 9.f, 0.5f, "dB"));
        addRow(makeFloatRow(listW, "audio-eq-8000", "8 kHz (Treble)", -9.f, 9.f, 0.5f, "dB"));
        addRow(makeFloatRow(listW, "audio-eq-16000", "16 kHz (Air)", -9.f, 9.f, 0.5f, "dB"));
    }
    else if (tab == Tab::Mods) {
        addRow(makeSectionTitle(listW, "Gameplay Hacks"));
        addRow(makeBoolRow(listW, "noclip-enabled", "Noclip", "Pass through obstacles freely without dying."));
        addRow(makeFloatRow(listW, "noclip-opacity", "Noclip Player Opacity", 0.f, 1.f, 0.05f));
        addRow(makeBoolRow(listW, "speedhack-enabled", "Speedhack", "Adjust game speed multiplier."));
        addRow(makeFloatRow(listW, "speedhack-speed", "Speed Multiplier", 0.1f, 5.f, 0.1f, "x"));
        addRow(makeBoolRow(listW, "fps-bypass-enabled", "FPS Bypass", "Remove the monitor refresh rate FPS cap."));
        addRow(makeFloatRow(listW, "fps-bypass-value", "FPS Cap Override", 60.f, 1000.f, 10.f, "FPS"));
        addRow(makeDivider(listW));

        addRow(makeSectionTitle(listW, "Practice & Helpers"));
        addRow(makeBoolRow(listW, "practice-music-hack", "Practice Music Hack", "Keep normal level song playing in practice mode."));
        addRow(makeBoolRow(listW, "auto-retry-enabled", "Auto-Retry On Death", "Instantly restart level on death."));
        addRow(makeFloatRow(listW, "auto-retry-delay", "Auto-Retry Delay", 0.f, 2.f, 0.05f, "s"));
        addRow(makeFloatRow(listW, "respawn-delay", "Respawn Delay", 0.f, 3.f, 0.1f, "s"));
        addRow(makeDivider(listW));

        addRow(makeSectionTitle(listW, "HUD & Visual Cheats"));
        addRow(makeBoolRow(listW, "show-percentage", "Show Percentage Label", "Display decimal level percentage HUD."));
        addRow(makeBoolRow(listW, "hide-attempts", "Hide Attempt Counter", "Hide the attempt count during attempts."));
        addRow(makeBoolRow(listW, "hitbox-enabled", "Show Object Hitboxes", "Draw live bounding boxes for player and hazards."));
        addRow(makeBoolRow(listW, "hitbox-solid", "Solid Fill Hitboxes", "Fill hitboxes with semi-transparent color."));
    }

    scroll->m_contentLayer->updateLayout();
    scroll->scrollToTop();
    m_contentArea->addChild(scroll);
}

// ============================================================================
//  Row construction helpers
// ============================================================================

namespace {
    double snapToStep(double v, double minV, double maxV, double step) {
        if (step > 0.0) v = std::round(v / step) * step;
        return std::clamp(v, minV, maxV);
    }

    unsigned valueDecimals(double step) {
        if (step >= 1.0)   return 0;
        if (step >= 0.1)   return 1;
        if (step >= 0.01)  return 2;
        if (step >= 0.001) return 3;
        return 4;
    }

    CCLayerColor* makeHairline(float width, cocos2d::ccColor4B const& color, float thickness = 1.5f) {
        auto* line = CCLayerColor::create(color, width, thickness);
        line->setAnchorPoint({0.f, 0.f});
        return line;
    }

    cocos2d::CCNode* makeRowCard(float width, float height) {
        auto* row = cocos2d::CCNode::create();
        row->setContentSize({width, height});

        auto* bg = CCScale9Sprite::create(ModernTheme::kSprRowCard);
        if (!bg) bg = CCScale9Sprite::create("GJ_square01.png");
        bg->setContentSize({width, height});
        bg->setAnchorPoint({0.f, 0.f});
        bg->setPosition({0.f, 0.f});
        bg->setColor(ModernTheme::kRowBgColor);
        bg->setOpacity(ModernTheme::kRowBgOpacity);
        row->addChild(bg, -1);
        return row;
    }

    cocos2d::CCLabelBMFont* addRowTitle(cocos2d::CCNode* row, char const* title, float y, float maxW = 0.f) {
        constexpr float kTitleScale = 0.38f;
        auto* lbl = cocos2d::CCLabelBMFont::create(title, ModernTheme::kFontLabels);
        lbl->setScale(kTitleScale);
        lbl->setAnchorPoint({0.f, 1.f});
        lbl->setPosition({10.f, y});
        if (maxW > 0.f && lbl->getContentSize().width * kTitleScale > maxW) {
            lbl->setScale(maxW / lbl->getContentSize().width);
        }
        row->addChild(lbl);
        return lbl;
    }

    void fitLabel(cocos2d::CCLabelBMFont* lbl, float maxWidth, float maxScale, float minScale = 0.25f) {
        float const raw = lbl->getContentSize().width;
        float const s = raw > 0.f ? std::min(maxScale, maxWidth / raw) : maxScale;
        lbl->setScale(std::max(minScale, s));
    }
}

cocos2d::CCNode* ModernSettingsPopup::makeSectionTitle(float width, char const* title) {
    auto* row = cocos2d::CCNode::create();
    row->setContentSize({width, 24.f});

    auto* lbl = cocos2d::CCLabelBMFont::create(title, ModernTheme::kFontTitle);
    lbl->setScale(0.45f);
    lbl->setColor(ModernTheme::kSectionTitleColor);
    lbl->setAnchorPoint({0.f, 1.f});
    lbl->setPosition({6.f, 22.f});
    row->addChild(lbl);

    auto* line = makeHairline(width, ModernTheme::kSectionLineColor, 1.5f);
    line->setPosition({0.f, 2.f});
    row->addChild(line);
    return row;
}

cocos2d::CCNode* ModernSettingsPopup::makeDivider(float width) {
    auto* row = cocos2d::CCNode::create();
    row->setContentSize({width, 8.f});

    auto* line = makeHairline(width, ModernTheme::kDividerColor, 1.5f);
    line->setPosition({0.f, 3.f});
    row->addChild(line);
    return row;
}

cocos2d::CCNode* ModernSettingsPopup::makeBoolRow(float width, char const* key, char const* title, char const* desc) {
    bool const value = Mod::get()->getSettingValue<bool>(key);

    auto* row = makeRowCard(width, ModernTheme::kRowHeight);
    float const rightBound = width - (UIConfig::kCheckboxRightPadding + 36.f);
    addRowTitle(row, title, ModernTheme::kRowHeight - 7.f, rightBound);

    if (desc && desc[0] != '\0') {
        auto* descLbl = cocos2d::CCLabelBMFont::create(desc, ModernTheme::kFontValues);
        descLbl->setScale(0.35f);
        descLbl->setColor({165, 200, 200});
        descLbl->setAnchorPoint({0.f, 1.f});
        descLbl->setPosition({10.f, ModernTheme::kRowHeight - 20.f});
        if (descLbl->getContentSize().width * 0.35f > rightBound) {
            descLbl->setScale(rightBound / descLbl->getContentSize().width);
        }
        row->addChild(descLbl);
    }

    auto* menu = cocos2d::CCMenu::create();
    menu->setPosition({width - UIConfig::kCheckboxRightPadding, ModernTheme::kRowHeight * 0.5f});
    row->addChild(menu);

    auto* check = cocos2d::CCSprite::createWithSpriteFrameName(
        value ? ModernTheme::kSprCheckOn : ModernTheme::kSprCheckOff
    );
    if (!check) check = CCSprite::createWithSpriteFrameName("GJ_checkOn_001.png");
    if (check) check->setScale(UIConfig::kCheckboxScale);

    auto* btn = CCMenuItemExt::createSpriteExtra(check, [key, check](cocos2d::CCObject*) {
        bool const next = !Mod::get()->getSettingValue<bool>(key);
        Mod::get()->setSettingValue<bool>(key, next);
        if (check) {
            if (auto* frame = CCSpriteFrameCache::sharedSpriteFrameCache()->spriteFrameByName(
                    next ? ModernTheme::kSprCheckOn : ModernTheme::kSprCheckOff)) {
                check->setDisplayFrame(frame);
            }
        }
    });
    menu->addChild(btn);

    return row;
}

cocos2d::CCNode* ModernSettingsPopup::makeFloatRow(float width, char const* key, char const* title, float minV, float maxV, float step, char const* unit) {
    auto* mod = Mod::get();
    double const value = snapToStep(mod->getSettingValue<double>(key), minV, maxV, step);
    unsigned const decimals = valueDecimals(step);

    float const rowH = ModernTheme::kRowHeight + UIConfig::kFloatRowHeightAdd;
    auto* row = makeRowCard(width, rowH);
    addRowTitle(row, title, rowH - 6.f, width - 100.f);

    float unitW = 0.f;
    if (unit && unit[0] != '\0') {
        auto* unitLbl = cocos2d::CCLabelBMFont::create(unit, ModernTheme::kFontValues);
        unitLbl->setScale(0.35f);
        unitLbl->setColor({150, 190, 190});
        unitLbl->setAnchorPoint({1.f, 0.5f});
        unitLbl->setPosition({width - 10.f, rowH - 12.f});
        unitW = unitLbl->getScaledContentWidth();
        row->addChild(unitLbl);
    }

    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.*f", static_cast<int>(decimals), value);
    auto* valueLbl = cocos2d::CCLabelBMFont::create(buf, ModernTheme::kFontValues);
    valueLbl->setScale(0.38f);
    valueLbl->setColor(ModernTheme::kValueTextColor);
    valueLbl->setAnchorPoint({1.f, 0.5f});
    valueLbl->setPosition({width - 14.f - unitW, rowH - 12.f});
    row->addChild(valueLbl);

    auto* slider = geode::SliderNode::create([key, minV, maxV](geode::SliderNode*, float v) {
        Mod::get()->setSettingValue<double>(key, std::clamp(v, minV, maxV));
    });
    slider->setScale(UIConfig::kSliderScale);
    slider->setAnchorPoint({0.f, 0.5f});
    slider->setPosition(UIConfig::kSliderPos);

    slider->setMin(minV);
    slider->setMax(maxV);
    slider->setSnapStep(step > 0.f ? step : 0.f);
    slider->setValue(static_cast<float>(value));
    slider->linkLabel(valueLbl, decimals);
    row->addChild(slider);

    return row;
}

cocos2d::CCNode* ModernSettingsPopup::makeCycleRow(float width, char const* key, char const* title, std::vector<std::string> const& options) {
    std::string const current = Mod::get()->getSettingValue<std::string>(key);
    size_t idx = 0;
    for (size_t i = 0; i < options.size(); ++i) {
        if (options[i] == current) { idx = i; break; }
    }

    auto* row = makeRowCard(width, ModernTheme::kRowHeight);
    
    float const badgeW = std::min(width * 0.45f, UIConfig::kCycleBadgeMaxW);
    float const maxTitleW = width - badgeW - 20.f;
    addRowTitle(row, title, ModernTheme::kRowHeight - 7.f, maxTitleW);

    float const badgeX = width - UIConfig::kCycleBadgeRightPadding - badgeW;
    
    auto* badge = CCScale9Sprite::create("square02b_001.png");
    if (!badge) badge = CCScale9Sprite::create(ModernTheme::kSprBadgeBg);
    badge->setContentSize({badgeW, UIConfig::kCycleBadgeH});
    badge->setAnchorPoint({0.f, 0.f});
    badge->setPosition({badgeX, (ModernTheme::kRowHeight - UIConfig::kCycleBadgeH) * 0.5f});
    badge->setColor(ModernTheme::kBadgeColor);
    badge->setOpacity(ModernTheme::kBadgeOpacity);
    row->addChild(badge);

    auto* optionLbl = cocos2d::CCLabelBMFont::create(options[idx].c_str(), ModernTheme::kFontValues);
    optionLbl->setColor(ModernTheme::kValueTextColor);
    optionLbl->setAnchorPoint({0.5f, 0.5f});
    optionLbl->setPosition({badgeW * 0.5f, UIConfig::kCycleBadgeH * 0.5f});
    fitLabel(optionLbl, badgeW - 36.f, 0.38f);
    badge->addChild(optionLbl, 1);

    auto* menu = cocos2d::CCMenu::create();
    menu->setPosition({0.f, 0.f});
    row->addChild(menu);

    auto apply = [key, options, optionLbl, badgeW](int dir) {
        std::string const cur = Mod::get()->getSettingValue<std::string>(key);
        size_t i = 0;
        for (size_t k = 0; k < options.size(); ++k) {
            if (options[k] == cur) { i = k; break; }
        }
        i = (i + dir + options.size()) % options.size();
        Mod::get()->setSettingValue<std::string>(key, options[i]);
        optionLbl->setString(options[i].c_str());
        fitLabel(optionLbl, badgeW - 36.f, 0.38f);
    };

    auto makeArrow = [&](bool left) {
        auto* arrow = cocos2d::CCSprite::createWithSpriteFrameName(
            left ? ModernTheme::kSprArrowLeft : ModernTheme::kSprArrowRight
        );
        if (!arrow) arrow = CCSprite::createWithSpriteFrameName(left ? "edit_leftBtn_001.png" : "edit_rightBtn_001.png");
        if (arrow) arrow->setScale(UIConfig::kCycleArrowScale);

        auto* btn = CCMenuItemExt::createSpriteExtra(arrow, [apply, left](cocos2d::CCObject*) {
            apply(left ? -1 : +1);
        });
        btn->setPosition({badgeX + (left ? UIConfig::kCycleArrowPadding : badgeW - UIConfig::kCycleArrowPadding), ModernTheme::kRowHeight * 0.5f});
        menu->addChild(btn);
    };
    makeArrow(true);
    makeArrow(false);

    return row;
}