#include <Geode/Geode.hpp>
#include <Geode/modify/EditorPauseLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include "ModernSettingsPopup.hpp"
#include "ModernTheme.hpp"

using namespace geode::prelude;

void makeSettingsButton(CCLayer* parent, char const* menuId) {
    if (!parent) return;
    auto menu = parent->getChildByID(menuId);
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

    // Use mod's custom button sprite or GD options button from ModernTheme
    auto spr = CCSprite::create(ModernTheme::kSprMenuBtn);
    if (!spr) {
        spr = CCSprite::createWithSpriteFrameName(ModernTheme::kSprMenuBtn);
    }
    if (!spr) {
        spr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
    }
    if (!spr) return;
    spr->setID("rickgdps-btn-spr"_spr);
    spr->setOpacity(ModernTheme::kMenuBtnOpacity);
    auto size = spr->getContentSize();
    auto scale = (size.width > 0.f) ? (32.f / size.width) : 1.f;
    spr->setScale(scale);

    auto btn = CCMenuItemExt::createSpriteExtra(spr, [](CCObject*) {
        if (auto* popup = ModernSettingsPopup::create()) {
            popup->show();
        }
    });
    btn->setID("rickgdps-settings-button"_spr);
    menu->addChild(btn);
    menu->updateLayout();
}

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

class $modify(BetterVisualAudioMainMenuButton, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;
        makeSettingsButton(this, "bottom-menu");
        return true;
    }
};
