#include "QuickToggleOverlay.hpp"

#include "ModernTheme.hpp"

#include <algorithm>

using namespace geode::prelude;

namespace rickgdps {

    namespace {
        constexpr float kHeight = 18.f;
        constexpr float kHandleWidth = 18.f;
        constexpr float kGap = 3.f;
        constexpr float kOverlayScale = 0.8f;

        constexpr cocos2d::ccColor3B kOnBg{0, 150, 115};
        constexpr cocos2d::ccColor3B kOffBg{0, 40, 40};
        constexpr cocos2d::ccColor3B kOnText{255, 255, 192};
        constexpr cocos2d::ccColor3B kOffText{165, 190, 190};

        cocos2d::extension::CCScale9Sprite* makePanelSprite() {
            auto* sprite = cocos2d::extension::CCScale9Sprite::create("square02b_001.png");
            if (!sprite) sprite = cocos2d::extension::CCScale9Sprite::create("GJ_square01.png");
            return sprite;
        }

        void setNodeOpacity(cocos2d::CCNode* node, GLubyte value) {
            if (!node) return;
            if (auto* rgba = typeinfo_cast<cocos2d::CCRGBAProtocol*>(node)) {
                rgba->setOpacity(value);
            }
        }
    }

    QuickToggleOverlay* QuickToggleOverlay::create() {
        auto* ret = new QuickToggleOverlay();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

    bool QuickToggleOverlay::init() {
        if (!CCLayer::init()) return false;

        setID(kNodeID);
        setTouchEnabled(true);
        setTouchMode(kCCTouchesOneByOne);
        setTouchPriority(-500);
        setScale(kOverlayScale);
        scheduleUpdate();

        // ── Drag handle ──────────────────────────────────────────────────────
        m_handleBg = makePanelSprite();
        if (m_handleBg) {
            m_handleBg->setContentSize({kHandleWidth, kHeight});
            m_handleBg->setAnchorPoint({0.f, 0.f});
            m_handleBg->setPosition({0.f, 0.f});
            m_handleBg->setColor({0, 64, 64});
            m_handleBg->setOpacity(235);
            addChild(m_handleBg, 0);
            m_opacityTargets.emplace_back(m_handleBg, static_cast<GLubyte>(235));
        }

        m_handleLabel = cocos2d::CCLabelBMFont::create("≡", "bigFont.fnt");
        if (m_handleLabel) {
            m_handleLabel->setScale(0.42f);
            m_handleLabel->setColor({255, 255, 192});
            m_handleLabel->setPosition({kHandleWidth * 0.5f, kHeight * 0.5f + 1.f});
            addChild(m_handleLabel, 1);
            m_opacityTargets.emplace_back(m_handleLabel, static_cast<GLubyte>(255));
        }

        m_menu = cocos2d::CCMenu::create();
        m_menu->setPosition({0.f, 0.f});
        addChild(m_menu, 1);

        struct ToggleDef {
            char const* text;
            char const* key;
            float width;
        };
        constexpr ToggleDef kToggles[] = {
            {"NOCLIP", "noclip-enabled",     48.f},
            {"SPEED",  "speedhack-enabled",  44.f},
            {"RETRY",  "auto-retry-enabled", 42.f},
            {"HITBOX", "hitbox-enabled",     48.f},
            {"LAYOUT", "layout-mode",        50.f},
            {"MIRROR", "mirror-mode",        48.f},
            {"UI",     "hide-ui-enabled",    26.f},
        };
        for (auto const& def : kToggles) {
            addToggle(def.text, def.key, def.width);
        }

        float totalWidth = kHandleWidth;
        if (!m_toggles.empty()) {
            totalWidth = m_toggles.back().x + m_toggles.back().width;
        }
        setContentSize({totalWidth, kHeight});

        // ── Restore saved position (or tuck under the top-left HUD) ──────────
        auto const winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
        float const savedX = static_cast<float>(
            Mod::get()->getSavedValue<double>("quick-overlay-pos-x", 12.0)
        );
        float const savedY = static_cast<float>(
            Mod::get()->getSavedValue<double>(
                "quick-overlay-pos-y", static_cast<double>(winSize.height) - 64.0
            )
        );
        setPosition(clampPosition({savedX, savedY}));

        refreshStates();
        applyOpacity();
        return true;
    }

    void QuickToggleOverlay::addToggle(char const* text, char const* settingKey, float width) {
        float const x = m_toggles.empty()
            ? (kHandleWidth + kGap)
            : (m_toggles.back().x + m_toggles.back().width + kGap);

        auto* bg = makePanelSprite();
        if (!bg) return;
        bg->setContentSize({width, kHeight});

        auto* label = cocos2d::CCLabelBMFont::create(text, "bigFont.fnt");
        if (label) {
            float const raw = label->getContentSize().width;
            label->setScale(raw > 0.f ? std::min(0.30f, (width - 6.f) / raw) : 0.30f);
            label->setPosition({width * 0.5f, kHeight * 0.5f + 1.f});
            bg->addChild(label);
        }

        auto* item = CCMenuItemExt::createSpriteExtra(
            bg,
            [this, settingKey](cocos2d::CCObject*) {
                queueInMainThread([this, settingKey]() {
                    auto* mod = Mod::get();
                    if (!mod) return;
                    bool const current = mod->getSettingValue<bool>(settingKey);
                    (void)mod->setSettingValue<bool>(settingKey, !current);
                    this->refreshStates();
                });
            }
        );
        item->setPosition({x + width * 0.5f, kHeight * 0.5f});
        m_menu->addChild(item);

        Toggle toggle;
        toggle.bg = bg;
        toggle.label = label;
        toggle.settingKey = settingKey;
        toggle.x = x;
        toggle.width = width;
        m_toggles.push_back(toggle);

        m_opacityTargets.emplace_back(bg, static_cast<GLubyte>(235));
        if (label) m_opacityTargets.emplace_back(label, static_cast<GLubyte>(255));
    }

    void QuickToggleOverlay::onEnter() {
        CCLayer::onEnter();
        refreshStates();
        applyOpacity();
    }

    void QuickToggleOverlay::update(float dt) {
        m_refreshTimer += dt;
        if (m_refreshTimer < 0.25f) return;
        m_refreshTimer = 0.f;
        refreshStates();
        applyOpacity();
    }

    void QuickToggleOverlay::refreshStates() {
        for (auto const& toggle : m_toggles) {
            updateToggleVisual(toggle);
        }
    }

    void QuickToggleOverlay::updateToggleVisual(Toggle const& toggle) {
        auto* mod = Mod::get();
        if (!mod) return;
        bool const on = mod->getSettingValue<bool>(toggle.settingKey);
        if (toggle.bg) toggle.bg->setColor(on ? kOnBg : kOffBg);
        if (toggle.label) toggle.label->setColor(on ? kOnText : kOffText);
    }

    void QuickToggleOverlay::applyOpacity() {
        auto* mod = Mod::get();
        if (!mod) return;
        float factor = static_cast<float>(mod->getSettingValue<double>("quick-overlay-opacity"));
        factor = std::clamp(factor, 0.05f, 1.f);
        for (auto& [node, base] : m_opacityTargets) {
            if (!node) continue;
            float const value = std::clamp(static_cast<float>(base) * factor, 0.f, 255.f);
            setNodeOpacity(node, static_cast<GLubyte>(value));
        }
    }

    bool QuickToggleOverlay::ccTouchBegan(cocos2d::CCTouch* touch, cocos2d::CCEvent*) {
        if (!m_handleBg) return false;
        auto const local = convertTouchToNodeSpace(touch);
        if (!m_handleBg->boundingBox().containsPoint(local)) return false;
        m_dragging = true;
        m_dragOffset = touch->getLocation() - getPosition();
        return true;
    }

    void QuickToggleOverlay::ccTouchMoved(cocos2d::CCTouch* touch, cocos2d::CCEvent*) {
        if (!m_dragging) return;
        setPosition(clampPosition(touch->getLocation() - m_dragOffset));
    }

    void QuickToggleOverlay::ccTouchEnded(cocos2d::CCTouch*, cocos2d::CCEvent*) {
        if (!m_dragging) return;
        m_dragging = false;
        persistPosition();
    }

    void QuickToggleOverlay::persistPosition() {
        auto* mod = Mod::get();
        if (!mod) return;
        (void)mod->setSavedValue("quick-overlay-pos-x", static_cast<double>(getPositionX()));
        (void)mod->setSavedValue("quick-overlay-pos-y", static_cast<double>(getPositionY()));
    }

    cocos2d::CCPoint QuickToggleOverlay::clampPosition(cocos2d::CCPoint const& pos) {
        auto const winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
        float const w = getContentSize().width * getScale();
        float const h = getContentSize().height * getScale();
        return {
            std::clamp(pos.x, 0.f, std::max(0.f, winSize.width - w)),
            std::clamp(pos.y, 0.f, std::max(0.f, winSize.height - h)),
        };
    }
} // namespace rickgdps
