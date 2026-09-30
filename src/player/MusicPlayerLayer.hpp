#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <Geode/binding/Slider.hpp>
#include "MusicPlayerManager.hpp"
#include "MusicPlayerTheme.hpp"

namespace rickgdps::music {

    class MusicPlayerPopup : public geode::Popup {
    public:
        static constexpr float kWidth = theme::kPopupWidth;
        static constexpr float kHeight = theme::kPopupHeight;

        enum class SubTab {
            Player = 0,
            Playlist = 1,
            Effects = 2,
            Equalizer = 3,
        };

        static MusicPlayerPopup* create();

        void onExit() override;
        void update(float dt) override;

    private:
        SubTab m_activeTab = SubTab::Player;
        cocos2d::CCNode* m_tabMenuNode = nullptr;
        cocos2d::CCNode* m_contentArea = nullptr;
        std::vector<cocos2d::CCMenuItem*> m_tabButtons;

        // Player tab controls & nodes
        cocos2d::CCLabelBMFont* m_titleLabel = nullptr;
        cocos2d::CCLabelBMFont* m_artistLabel = nullptr;
        cocos2d::CCLabelBMFont* m_timeElapsedLabel = nullptr;
        cocos2d::CCLabelBMFont* m_timeTotalLabel = nullptr;
        Slider* m_progressSlider = nullptr;
        bool m_isDraggingProgress = false;
        CCMenuItemSpriteExtra* m_playPauseBtn = nullptr;
        CCMenuItemSpriteExtra* m_loopBtn = nullptr;
        CCMenuItemSpriteExtra* m_shuffleBtn = nullptr;

        // Visualizer bars
        std::vector<cocos2d::CCLayerColor*> m_visualizerBars;

        bool init(float width, float height);
        bool setup();

        void selectTab(SubTab tab);
        void buildTabHeaders();
        void buildTabContent();

        // Sub-tab view builders
        void buildPlayerView();
        void buildPlaylistView();
        void buildEffectsView();
        void buildEqualizerView();

        // Helpers
        void refreshPlayerState();
        void updateVisualizer();
        cocos2d::CCNode* makeCard(float w, float h);
        CCMenuItemSpriteExtra* makeTextButton(char const* text, float w, float h, std::function<void()> onClick, float textScale = 0.38f);
    };

    void openMusicPlayer();

} // namespace rickgdps::music
