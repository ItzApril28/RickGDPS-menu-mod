#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/CCScene.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include "ModernSettingsPopup.hpp"
#include "ModernTheme.hpp"

using namespace geode::prelude;

// ─────────────────────────────────────────────────────────────────────────────
//  Static button helper (PauseLayer / EditorPauseLayer)
// ─────────────────────────────────────────────────────────────────────────────
static void makeSettingsButton(CCLayer* parent, char const* menuId) {
    if (!parent) return;
    auto* menu = parent->getChildByID(menuId);
    if (!menu) {
        for (auto* child : CCArrayExt<CCNode*>(parent->getChildren())) {
            if (auto* m = typeinfo_cast<CCMenu*>(child)) {
                menu = m;
                break;
            }
        }
    }
    if (!menu) return;
    if (menu->getChildByID("rickgdps-settings-button"_spr)) return;

    auto* spr = CCSprite::create(ModernTheme::kSprMenuBtn);
    if (!spr) spr = CCSprite::createWithSpriteFrameName(ModernTheme::kSprMenuBtn);
    if (!spr) spr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
    if (!spr) return;
    spr->setID("rickgdps-btn-spr"_spr);
    spr->setOpacity(ModernTheme::kMenuBtnOpacity);
    float const scale = spr->getContentSize().width > 0.f
        ? (32.f / spr->getContentSize().width) : 1.f;
    spr->setScale(scale);

    auto* btn = CCMenuItemExt::createSpriteExtra(spr, [](CCObject*) {
        if (auto* popup = ModernSettingsPopup::create()) popup->show();
    });
    btn->setID("rickgdps-settings-button"_spr);
    menu->addChild(btn);
    menu->updateLayout();
}

// ─────────────────────────────────────────────────────────────────────────────
//  DraggableModButton — floating, draggable button across all scenes
//  Tap = open ModernSettingsPopup
//  Drag = reposition globally, persisted across sessions and scenes
// ─────────────────────────────────────────────────────────────────────────────
class DraggableModButton : public CCLayer {
public:
    static constexpr char const* kNodeID = "rickgdps-draggable-btn";

    static DraggableModButton* create() {
        auto* ret = new DraggableModButton();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }

protected:
    bool init() override {
        if (!CCLayer::init()) return false;
        setID(kNodeID);

        // Build sprite (no CCMenu — we handle taps in ccTouchEnded directly)
        auto* spr = CCSprite::create(ModernTheme::kSprMenuBtn);
        if (!spr) spr = CCSprite::createWithSpriteFrameName(ModernTheme::kSprMenuBtn);
        if (!spr) spr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
        if (!spr) return false;

        constexpr float kBtnSize = 40.f;
        float const scale = spr->getContentSize().width > 0.f
            ? (kBtnSize / spr->getContentSize().width) : 1.f;
        spr->setScale(scale);

        double opacity = 0.85;
        if (Mod::get()->hasSetting("draggable-btn-opacity")) {
            opacity = Mod::get()->getSettingValue<double>("draggable-btn-opacity");
        }
        spr->setOpacity(static_cast<GLubyte>(std::clamp(opacity, 0.1, 1.0) * 255.0));

        setContentSize({kBtnSize, kBtnSize});
        setAnchorPoint({0.5f, 0.5f});
        ignoreAnchorPointForPosition(false);

        spr->setPosition({kBtnSize * 0.5f, kBtnSize * 0.5f});
        spr->setID("rickgdps-btn-spr"_spr);
        addChild(spr);

        setTouchEnabled(true);
        setTouchMode(kCCTouchesOneByOne);
        setTouchPriority(-510);

        // Restore or default position
        auto const ws = CCDirector::sharedDirector()->getWinSize();
        float const sx = static_cast<float>(
            Mod::get()->getSavedValue<double>("draggable-btn-x", static_cast<double>(ws.width - 30.f))
        );
        float const sy = static_cast<float>(
            Mod::get()->getSavedValue<double>("draggable-btn-y", static_cast<double>(ws.height - 30.f))
        );
        setPosition(clamp({sx, sy}));
        return true;
    }

    void onEnter() override {
        CCLayer::onEnter();
        setPosition(clamp(getPosition()));
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        if (!isVisible()) return false;

        // Don't intercept if popup is already open
        if (auto* running = CCDirector::sharedDirector()->getRunningScene()) {
            if (running->getChildByID("rickgdps-settings-popup")) return false;
        }

        CCPoint const local = convertTouchToNodeSpace(touch);
        CCRect const box{0.f, 0.f, getContentSize().width, getContentSize().height};
        if (!box.containsPoint(local)) return false;

        m_dragging = true;
        m_didDrag  = false;
        m_dragOffset = touch->getLocation() - getPosition();
        return true;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent*) override {
        if (!m_dragging) return;
        CCPoint const newPos = clamp(touch->getLocation() - m_dragOffset);
        if (!m_didDrag) {
            CCPoint const delta = newPos - getPosition();
            if (std::abs(delta.x) > 5.f || std::abs(delta.y) > 5.f) m_didDrag = true;
        }
        if (m_didDrag) setPosition(newPos);
    }

    void ccTouchEnded(CCTouch*, CCEvent*) override {
        if (!m_dragging) return;
        m_dragging = false;
        if (m_didDrag) {
            // Save dragged position globally
            (void)Mod::get()->setSavedValue("draggable-btn-x", static_cast<double>(getPositionX()));
            (void)Mod::get()->setSavedValue("draggable-btn-y", static_cast<double>(getPositionY()));
        } else {
            // Pure tap — open the settings popup
            if (auto* running = CCDirector::sharedDirector()->getRunningScene()) {
                if (running->getChildByID("rickgdps-settings-popup")) return;
            }
            if (auto* popup = ModernSettingsPopup::create()) {
                popup->setID("rickgdps-settings-popup");
                popup->show();
            }
        }
    }

    void ccTouchCancelled(CCTouch*, CCEvent*) override {
        m_dragging = false;
        m_didDrag  = false;
    }

private:
    CCPoint clamp(CCPoint const& p) const {
        auto const ws = CCDirector::sharedDirector()->getWinSize();
        float const hw = getContentSize().width * 0.5f;
        float const hh = getContentSize().height * 0.5f;
        return {
            std::clamp(p.x, hw, ws.width  - hw),
            std::clamp(p.y, hh, ws.height - hh),
        };
    }

    bool m_dragging = false;
    bool m_didDrag  = false;
    CCPoint m_dragOffset{};
};

// ─────────────────────────────────────────────────────────────────────────────
//  Global scene attachment
// ─────────────────────────────────────────────────────────────────────────────
static void attachGlobalButton(CCNode* target) {
    if (!target) return;
    if (Mod::get()->hasSetting("draggable-btn-enabled") &&
        !Mod::get()->getSettingValue<bool>("draggable-btn-enabled")) {
        return;
    }
    if (typeinfo_cast<CCTransitionScene*>(target)) return;
    if (target->getChildByID(DraggableModButton::kNodeID)) return;

    if (auto* btn = DraggableModButton::create()) {
        target->addChild(btn, 9999);
    }
}

class $modify(BetterVisualAudioScene, CCScene) {
    bool init() {
        if (!CCScene::init()) return false;
        attachGlobalButton(this);
        return true;
    }
};

class $modify(BetterVisualAudioDirector, CCDirector) {
    void willSwitchToScene(CCScene* scene) {
        CCDirector::willSwitchToScene(scene);
        attachGlobalButton(scene);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  PauseLayer & EditorPauseLayer static menu buttons
// ─────────────────────────────────────────────────────────────────────────────
class $modify(BetterVisualAudioPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();
        makeSettingsButton(this, "left-button-menu");
    }
};

class $modify(BetterVisualAudioEditorPauseLayer, EditorPauseLayer) {
    bool init(LevelEditorLayer* po) {
        if (!EditorPauseLayer::init(po)) return false;
        makeSettingsButton(this, "guidelines-menu");
        return true;
    }
};
