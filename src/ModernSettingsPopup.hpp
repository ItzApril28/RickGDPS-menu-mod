#pragma once
#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <string>
#include <vector>

class ModernSettingsPopup : public geode::Popup {
public:
    static constexpr float kWidth      = 440.f;
    static constexpr float kHeight     = 310.f;
    static constexpr float kTabHeight  = 30.f;

    enum class Tab {
        Shaders = 0,
        Audio   = 1,
        Mods    = 2,
    };

    static ModernSettingsPopup* create();

private:
    Tab m_activeTab = Tab::Shaders;
    cocos2d::CCNode* m_tabMenuNode   = nullptr;
    cocos2d::CCNode* m_contentArea   = nullptr;
    geode::ScrollLayer* m_scrollArea = nullptr;
    std::vector<cocos2d::CCMenuItem*> m_tabButtons;

    bool init(float width, float height);
    bool setup();

    void selectTab(Tab tab);
    void buildTabs();
    void buildTabContent(Tab tab);

    // Row construction helpers
    cocos2d::CCNode* makeBoolRow(float width, char const* key, char const* title, char const* desc = nullptr);
    cocos2d::CCNode* makeFloatRow(float width, char const* key, char const* title, float minV, float maxV, float step, char const* unit = nullptr);
    cocos2d::CCNode* makeCycleRow(float width, char const* key, char const* title, std::vector<std::string> const& options);
    cocos2d::CCNode* makeSectionTitle(float width, char const* title);
    cocos2d::CCNode* makeDivider(float width);
};
