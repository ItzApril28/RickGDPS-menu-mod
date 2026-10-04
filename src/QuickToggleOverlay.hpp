#pragma once

#include <Geode/cocos/extensions/cocos-ext.h>

#include <Geode/Geode.hpp>
#include <vector>

namespace rickgdps {

// ─────────────────────────────────────────────────────────────────────────────
//  QuickToggleOverlay — draggable in-game strip of hack toggles
//
//  Attached to PlayLayer by HacksLayer.cpp when the "quick-overlay-enabled"
//  setting is on. Every button flips its backing mod setting (so the existing
//  setting listeners update the hack state automatically) and the strip
//  remembers its position between sessions via saved values.
// ─────────────────────────────────────────────────────────────────────────────
class QuickToggleOverlay : public cocos2d::CCLayer {
public:
    static constexpr char const* kNodeID = "rickgdps-quick-overlay";

    static QuickToggleOverlay* create();

    void refreshStates();
    void update(float dt) override;
    void onEnter() override;

protected:
    bool init() override;
    bool ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;
    void ccTouchEnded(cocos2d::CCTouch* touch, cocos2d::CCEvent* event) override;

private:
    struct Toggle {
        cocos2d::extension::CCScale9Sprite* bg = nullptr;
        cocos2d::CCLabelBMFont* label = nullptr;
        char const* settingKey = nullptr;
        float x = 0.f;
        float width = 0.f;
    };

    void addToggle(char const* text, char const* settingKey, float width);
    void updateToggleVisual(Toggle const& toggle);
    void applyOpacity();
    void persistPosition();
    cocos2d::CCPoint clampPosition(cocos2d::CCPoint const& pos);

    std::vector<Toggle> m_toggles;
    std::vector<std::pair<cocos2d::CCNode*, GLubyte>> m_opacityTargets;
    cocos2d::CCMenu* m_menu = nullptr;
    cocos2d::extension::CCScale9Sprite* m_handleBg = nullptr;
    cocos2d::CCLabelBMFont* m_handleLabel = nullptr;
    bool m_dragging = false;
    cocos2d::CCPoint m_dragOffset{};
    float m_refreshTimer = 0.f;
};

} // namespace rickgdps
