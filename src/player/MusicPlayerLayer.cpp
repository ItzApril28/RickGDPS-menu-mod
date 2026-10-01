#include "MusicPlayerLayer.hpp"

#include "../ModernTheme.hpp"
#include "MusicPlayerTheme.hpp"

#include <Geode/binding/Slider.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <iomanip>
#include <sstream>

using namespace geode::prelude;

namespace {
    void fitLabel(cocos2d::CCLabelBMFont* lbl, float maxWidth, float maxScale, float minScale = 0.20f) {
        if (!lbl) return;
        float const raw = lbl->getContentSize().width;
        float const s = raw > 0.f ? std::min(maxScale, maxWidth / raw) : maxScale;
        lbl->setScale(std::max(minScale, s));
    }
} // namespace

namespace rickgdps::music {

    static std::string formatTime(float seconds) {
        if (seconds < 0.f) seconds = 0.f;
        int s = static_cast<int>(seconds);
        int m = s / 60;
        s %= 60;
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%d:%02d", m, s);
        return std::string(buf);
    }

    void openMusicPlayer() {
        auto* popup = MusicPlayerPopup::create();
        if (popup) {
            popup->show();
        }
    }

    MusicPlayerPopup* MusicPlayerPopup::create() {
        auto* ret = new MusicPlayerPopup();
        if (ret && ret->init(kWidth, kHeight)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    bool MusicPlayerPopup::init(float width, float height) {
        if (!Popup::init(width, height, theme::kSprWindowFrame)) {
            return false;
        }

        g_musicOverlayActive.store(true, std::memory_order_release);
        return setup();
    }

    void MusicPlayerPopup::onExit() {
        g_musicOverlayActive.store(false, std::memory_order_release);
        Popup::onExit();
    }

    bool MusicPlayerPopup::setup() {
        if (m_bgSprite) {
            m_bgSprite->setColor(theme::kWindowBgColor);
            m_bgSprite->setOpacity(theme::kWindowBgOpacity);
        }

        setTitle("MUSIC & AUDIO FX STUDIO", theme::kFontTitle, 0.45f, 16.f);

        auto const cSize = m_mainLayer->getContentSize();

        constexpr float contentW = theme::kContentPanelLayout.width;
        constexpr float contentH = theme::kContentPanelLayout.height;

        auto* panelBg = CCScale9Sprite::create(theme::kSprCardBg);
        if (!panelBg) panelBg = CCScale9Sprite::create("GJ_square01.png");
        panelBg->setContentSize({contentW, contentH});
        panelBg->setAnchorPoint({0.5f, 0.f});
        panelBg->setPosition(
            {cSize.width * 0.5f + theme::kContentPanelLayout.x, theme::kContentPanelLayout.y}
        );
        panelBg->setScale(theme::kContentPanelLayout.scale);
        panelBg->setColor(theme::kContentPanelColor);
        panelBg->setOpacity(theme::kContentPanelOpacity);
        m_mainLayer->addChild(panelBg, 1);

        m_tabMenuNode = CCNode::create();
        m_tabMenuNode->setPosition(
            {cSize.width * 0.5f + theme::kTabMenuLayout.x, cSize.height + theme::kTabMenuLayout.y}
        );
        m_tabMenuNode->setScale(theme::kTabMenuLayout.scale);
        m_mainLayer->addChild(m_tabMenuNode, 5);

        m_contentArea = CCNode::create();
        m_contentArea->setContentSize({contentW - 6.f, contentH - 8.f});
        m_contentArea->setAnchorPoint({0.5f, 0.f});
        m_contentArea->setPosition({cSize.width * 0.5f, 16.f});
        m_mainLayer->addChild(m_contentArea, 2);

        buildTabHeaders();
        buildTabContent();

        scheduleUpdate();
        return true;
    }

    void MusicPlayerPopup::selectTab(SubTab tab) {
        m_activeTab = tab;
        buildTabHeaders();
        buildTabContent();
    }

    void MusicPlayerPopup::buildTabHeaders() {
        if (!m_tabMenuNode) return;
        m_tabMenuNode->removeAllChildren();
        m_tabButtons.clear();

        struct TabDef {
            SubTab tab;
            char const* name;
        };

        static constexpr TabDef tabs[] = {
            {SubTab::Player, "♫ PLAYER"},
            {SubTab::Playlist, "≡ TRACKS"},
            {SubTab::Effects, "🎛 LIVE FX"},
            {SubTab::Equalizer, "🎚 10-BAND EQ"},
        };

        auto* menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        m_tabMenuNode->addChild(menu);

        constexpr float tabW = theme::kTabButtonLayout.width;
        constexpr float tabH = theme::kTabButtonLayout.height;
        constexpr float gap = 6.f;
        float const totalW = 4 * tabW + 3 * gap;
        float startX = -totalW * 0.5f + tabW * 0.5f;

        for (int i = 0; i < 4; ++i) {
            bool isActive = (m_activeTab == tabs[i].tab);

            auto* bg = CCScale9Sprite::create(theme::kSprTabBg);
            if (!bg) bg = CCScale9Sprite::create("GJ_square01.png");
            bg->setContentSize({tabW, tabH});
            bg->setColor(isActive ? theme::kTabActiveColor : theme::kTabInactiveColor);
            bg->setOpacity(isActive ? theme::kTabActiveOpacity : theme::kTabInactiveOpacity);

            auto* lbl = CCLabelBMFont::create(tabs[i].name, theme::kFontTitle);
            lbl->setScale(theme::kTabButtonLayout.scale);
            lbl->setPosition({tabW * 0.5f, tabH * 0.5f + 1.f});
            lbl->setColor(isActive ? theme::kAccentColor : theme::kTabTextColor);
            bg->addChild(lbl);

            auto tabVal = tabs[i].tab;
            auto* btn = CCMenuItemExt::createSpriteExtra(bg, [this, tabVal](CCObject*) {
                geode::queueInMainThread([this, tabVal]() {
                    this->selectTab(tabVal);
                });
            });
            btn->setPosition({startX + i * (tabW + gap), 0.f});
            menu->addChild(btn);
            m_tabButtons.push_back(btn);
        }
    }

    void MusicPlayerPopup::buildTabContent() {
        if (!m_contentArea) return;
        m_contentArea->removeAllChildren();
        m_visualizerBars.clear();
        m_titleLabel = nullptr;
        m_artistLabel = nullptr;
        m_timeElapsedLabel = nullptr;
        m_timeTotalLabel = nullptr;
        m_progressSlider = nullptr;
        m_playPauseBtn = nullptr;

        switch (m_activeTab) {
            case SubTab::Player: buildPlayerView(); break;
            case SubTab::Playlist: buildPlaylistView(); break;
            case SubTab::Effects: buildEffectsView(); break;
            case SubTab::Equalizer: buildEqualizerView(); break;
        }
    }

    CCNode* MusicPlayerPopup::makeCard(float w, float h) {
        auto* card = CCNode::create();
        card->setContentSize({w, h});

        auto* bg = CCScale9Sprite::create(theme::kSprCardBg);
        if (!bg) bg = CCScale9Sprite::create("GJ_square01.png");
        bg->setContentSize({w, h});
        bg->setColor(theme::kCardBgColor);
        bg->setOpacity(theme::kCardBgOpacity);
        bg->setPosition({w * 0.5f, h * 0.5f});
        card->addChild(bg, 0);

        return card;
    }

    CCMenuItemSpriteExtra* MusicPlayerPopup::makeTextButton(
        char const* text, float w, float h, std::function<void()> onClick, float textScale
    ) {
        auto* button = CCNode::create();
        button->setContentSize({w, h});

        auto* bg = CCScale9Sprite::create(theme::kSprButtonBg);
        if (!bg) bg = CCScale9Sprite::create("GJ_square01.png");
        bg->setContentSize({w, h});
        bg->setPosition({w * 0.5f, h * 0.5f});
        bg->setColor(theme::kButtonBgColor);
        bg->setOpacity(theme::kButtonBgOpacity);
        button->addChild(bg);

        auto* lbl = CCLabelBMFont::create(text, theme::kFontSectionTitle);
        fitLabel(lbl, w - 6.f, textScale);
        lbl->setPosition({w * 0.5f, h * 0.5f + 1.f});
        lbl->setColor(theme::kButtonTextColor);
        button->addChild(lbl);

        return CCMenuItemExt::createSpriteExtra(button, [onClick](CCObject*) {
            if (!onClick) return;
            geode::queueInMainThread([onClick]() {
                onClick();
            });
        });
    }

    // ─── Player View ─────────────────────────────────────────────────────────

    void MusicPlayerPopup::buildPlayerView() {
        auto const cSize = m_contentArea->getContentSize();
        auto& mgr = MusicPlayerManager::get();
        m_displayedTrackRevision = mgr.getTrackRevision();

        constexpr float cardW = theme::kNowPlayingLayout.width;
        constexpr float cardH = theme::kNowPlayingLayout.height;
        auto* card = makeCard(cardW, cardH);
        card->setPosition(
            {(cSize.width - cardW) * 0.5f + theme::kNowPlayingLayout.x,
             cSize.height - cardH + theme::kNowPlayingLayout.y}
        );
        card->setScale(theme::kNowPlayingLayout.scale);
        m_contentArea->addChild(card);

        auto* bar = CCLayerColor::create(
            {theme::kAccentColor.r, theme::kAccentColor.g, theme::kAccentColor.b, 255},
            theme::kAccentStripeLayout.width,
            theme::kAccentStripeLayout.height
        );
        bar->setPosition({theme::kAccentStripeLayout.x, theme::kAccentStripeLayout.y});
        bar->setScale(theme::kAccentStripeLayout.scale);
        card->addChild(bar, 1);

        auto const* curTrack = mgr.getCurrentTrack();

        m_titleLabel = CCLabelBMFont::create(
            curTrack ? curTrack->title.c_str() : "No Song Selected", theme::kFontTrackTitle
        );
        m_titleLabel->setScale(theme::kTrackTitleLayout.scale);
        m_titleLabel->setColor(theme::kTrackTitleColor);
        m_titleLabel->setAnchorPoint({0.f, 1.f});
        m_titleLabel->setPosition({theme::kTrackTitleLayout.x, cardH + theme::kTrackTitleLayout.y});
        fitLabel(m_titleLabel, theme::kTrackTitleLayout.width, theme::kTrackTitleLayout.scale);
        card->addChild(m_titleLabel, 1);

        std::string artistText = curTrack ?
            (curTrack->artist + " • Track " + std::to_string(mgr.getCurrentTrackIndex() + 1) +
             " of " + std::to_string(mgr.getTrackCount())) :
            "Select a track to start playback";
        m_artistLabel = CCLabelBMFont::create(artistText.c_str(), theme::kFontArtist);
        m_artistLabel->setScale(theme::kArtistLayout.scale);
        m_artistLabel->setColor(theme::kArtistTextColor);
        m_artistLabel->setAnchorPoint({0.f, 1.f});
        m_artistLabel->setPosition({theme::kArtistLayout.x, cardH + theme::kArtistLayout.y});
        fitLabel(m_artistLabel, theme::kArtistLayout.width, theme::kArtistLayout.scale);
        card->addChild(m_artistLabel, 1);

        constexpr size_t kBarCount = theme::kVisualizerBarCount;
        constexpr float visW = theme::kVisualizerLayout.width;
        constexpr float barW = (visW / kBarCount) - 3.f;
        float startVisX = theme::kVisualizerLayout.x;
        float startVisY = theme::kVisualizerLayout.y;

        m_visualizerBars.clear();
        for (size_t i = 0; i < kBarCount; ++i) {
            auto* vBar = CCLayerColor::create(
                {theme::kVisLowColor.r, theme::kVisLowColor.g, theme::kVisLowColor.b, 220}, barW, 4.f
            );
            vBar->setPosition({startVisX + i * (barW + 3.f), startVisY});
            card->addChild(vBar, 1);
            m_visualizerBars.push_back(vBar);
        }

        float const scrubberY = cSize.height - cardH + theme::kScrubberLayout.y;

        m_timeElapsedLabel = CCLabelBMFont::create("0:00", theme::kFontValues);
        m_timeElapsedLabel->setScale(0.35f);
        m_timeElapsedLabel->setColor(theme::kMetaTextColor);
        m_timeElapsedLabel->setAnchorPoint({0.f, 0.5f});
        m_timeElapsedLabel->setPosition({16.f, scrubberY});
        m_contentArea->addChild(m_timeElapsedLabel);

        m_timeTotalLabel = CCLabelBMFont::create("0:00", theme::kFontValues);
        m_timeTotalLabel->setScale(0.35f);
        m_timeTotalLabel->setColor(theme::kMetaTextColor);
        m_timeTotalLabel->setAnchorPoint({1.f, 0.5f});
        m_timeTotalLabel->setPosition({cSize.width - 16.f, scrubberY});
        m_contentArea->addChild(m_timeTotalLabel);

        constexpr float sliderW = theme::kScrubberLayout.width;
        m_progressSlider = Slider::create(
            this, menu_selector(MusicPlayerPopup::onSliderValueChanged), theme::kScrubberLayout.scale
        );
        m_progressSlider->setContentSize({sliderW, theme::kScrubberLayout.height});
        m_progressSlider->setPosition({cSize.width * 0.5f, scrubberY});
        m_progressSlider->setValue(0.f);
        m_contentArea->addChild(m_progressSlider);

        float const controlsY = cSize.height - cardH + theme::kControlsLayout.y;
        auto* controlsMenu = CCMenu::create();
        controlsMenu->setPosition({cSize.width * 0.5f, controlsY});
        m_contentArea->addChild(controlsMenu);

        m_shuffleBtn = makeTextButton(
            "SHUFFLE",
            72.f,
            20.f,
            [this] {
                auto& m = MusicPlayerManager::get();
                if (m.getLoopMode() == LoopMode::Shuffle) {
                    m.setLoopMode(LoopMode::All);
                }
                else {
                    m.setLoopMode(LoopMode::Shuffle);
                }
                buildTabContent();
            },
            0.32f
        );
        m_shuffleBtn->setPosition({-164.f, 0.f});
        controlsMenu->addChild(m_shuffleBtn);

        auto* prevBtn = makeTextButton(
            "<< PREV",
            72.f,
            20.f,
            [this] {
                MusicPlayerManager::get().prevTrack();
                refreshPlayerState();
            },
            0.32f
        );
        prevBtn->setPosition({-82.f, 0.f});
        controlsMenu->addChild(prevBtn);

        bool isPlaying = mgr.isPlaying();
        char const* playLabel = isPlaying ? "PAUSE ||" : "PLAY >";
        m_playPauseBtn = makeTextButton(
            playLabel,
            92.f,
            24.f,
            [this] {
                MusicPlayerManager::get().togglePlayPause();
                refreshPlayerState();
            },
            0.38f
        );
        m_playPauseBtn->setPosition({0.f, 0.f});
        controlsMenu->addChild(m_playPauseBtn);

        auto* nextBtn = makeTextButton(
            "NEXT >>",
            72.f,
            20.f,
            [this] {
                MusicPlayerManager::get().nextTrack();
                refreshPlayerState();
            },
            0.32f
        );
        nextBtn->setPosition({82.f, 0.f});
        controlsMenu->addChild(nextBtn);

        char const* loopText = (mgr.getLoopMode() == LoopMode::Track) ? "LOOP: 1" : "LOOP: ALL";
        m_loopBtn = makeTextButton(
            loopText,
            72.f,
            20.f,
            [this] {
                MusicPlayerManager::get().cycleLoopMode();
                buildTabContent();
            },
            0.32f
        );
        m_loopBtn->setPosition({164.f, 0.f});
        controlsMenu->addChild(m_loopBtn);

        constexpr float fxDeckW = theme::kFxDeckLayout.width;
        constexpr float fxDeckH = theme::kFxDeckLayout.height;
        auto* fxDeck = makeCard(fxDeckW, fxDeckH);
        fxDeck->setPosition(
            {(cSize.width - fxDeckW) * 0.5f + theme::kFxDeckLayout.x, theme::kFxDeckLayout.y}
        );
        fxDeck->setScale(theme::kFxDeckLayout.scale);
        m_contentArea->addChild(fxDeck);

        auto* deckTitle = CCLabelBMFont::create("REAL-TIME AUDIO DSP FX", theme::kFontSectionTitle);
        deckTitle->setScale(theme::kFxTitleLayout.scale);
        deckTitle->setColor(theme::kAccentColor);
        deckTitle->setPosition(
            {fxDeckW * 0.5f + theme::kFxTitleLayout.x, fxDeckH + theme::kFxTitleLayout.y}
        );
        fxDeck->addChild(deckTitle);

        auto* fxMenu = CCMenu::create();
        fxMenu->setPosition({0.f, 0.f});
        fxDeck->addChild(fxMenu);

        bool is8d = mgr.is8DEnabled();
        std::string btn8dText = is8d ? "8D AUDIO: [ON]" : "8D AUDIO: [OFF]";
        auto* btn8d = makeTextButton(
            btn8dText.c_str(),
            116.f,
            18.f,
            [this, &mgr, is8d] {
                mgr.set8DEnabled(!is8d);
                buildTabContent();
            },
            0.28f
        );
        btn8d->setPosition({74.f, 34.f});
        fxMenu->addChild(btn8d);

        bool hasReverb = (mgr.getReverb() > 10.f);
        std::string btnRevText = hasReverb ? "REVERB: [ON]" : "REVERB: [OFF]";
        auto* btnRev = makeTextButton(
            btnRevText.c_str(),
            116.f,
            18.f,
            [this, &mgr, hasReverb] {
                mgr.setReverb(hasReverb ? 0.f : 2000.f);
                buildTabContent();
            },
            0.28f
        );
        btnRev->setPosition({216.f, 34.f});
        fxMenu->addChild(btnRev);

        bool hasMuffle = (mgr.getMuffle() > 0.05f);
        std::string btnMufText = hasMuffle ? "MUFFLE: [ON]" : "MUFFLE: [OFF]";
        auto* btnMuf = makeTextButton(
            btnMufText.c_str(),
            116.f,
            18.f,
            [this, &mgr, hasMuffle] {
                mgr.setMuffle(hasMuffle ? 0.f : 0.65f);
                buildTabContent();
            },
            0.28f
        );
        btnMuf->setPosition({358.f, 34.f});
        fxMenu->addChild(btnMuf);

        auto* btnVolLow = makeTextButton(
            "VOL 50%",
            62.f,
            16.f,
            [&mgr] {
                mgr.setVolume(0.5f);
            },
            0.24f
        );
        btnVolLow->setPosition({38.f, 15.f});
        fxMenu->addChild(btnVolLow);

        auto* btnVolFull = makeTextButton(
            "VOL 100%",
            66.f,
            16.f,
            [&mgr] {
                mgr.setVolume(1.0f);
            },
            0.24f
        );
        btnVolFull->setPosition({104.f, 15.f});
        fxMenu->addChild(btnVolFull);

        auto* btnSpeed08 = makeTextButton(
            "0.8X",
            58.f,
            16.f,
            [&mgr] {
                mgr.setPitch(0.8f);
            },
            0.25f
        );
        btnSpeed08->setPosition({168.f, 15.f});
        fxMenu->addChild(btnSpeed08);

        auto* btnSpeed10 = makeTextButton(
            "1.0X",
            58.f,
            16.f,
            [&mgr] {
                mgr.setPitch(1.0f);
            },
            0.25f
        );
        btnSpeed10->setPosition({230.f, 15.f});
        fxMenu->addChild(btnSpeed10);

        auto* btnSpeed12 = makeTextButton(
            "1.25X",
            62.f,
            16.f,
            [&mgr] {
                mgr.setPitch(1.25f);
            },
            0.24f
        );
        btnSpeed12->setPosition({292.f, 15.f});
        fxMenu->addChild(btnSpeed12);

        auto* btnSpeed15 = makeTextButton(
            "1.5X",
            58.f,
            16.f,
            [&mgr] {
                mgr.setPitch(1.5f);
            },
            0.25f
        );
        btnSpeed15->setPosition({354.f, 15.f});
        fxMenu->addChild(btnSpeed15);
    }

    void MusicPlayerPopup::onSliderValueChanged(CCObject* sender) {
        if (!m_progressSlider) return;
        auto& mgr = MusicPlayerManager::get();
        float dur = mgr.getDuration();
        if (dur > 0.01f) {
            float targetSec = m_progressSlider->getValue() * dur;
            mgr.seek(targetSec);
        }
    }

    void MusicPlayerPopup::refreshPlayerState() {
        auto& mgr = MusicPlayerManager::get();
        auto const* curTrack = mgr.getCurrentTrack();

        if (m_titleLabel) {
            m_titleLabel->setString(curTrack ? curTrack->title.c_str() : "No Song Selected");
            fitLabel(m_titleLabel, theme::kTrackTitleLayout.width, theme::kTrackTitleLayout.scale);
        }
        if (m_artistLabel) {
            std::string artistText = curTrack ?
                (curTrack->artist + " • Track " + std::to_string(mgr.getCurrentTrackIndex() + 1) +
                 " of " + std::to_string(mgr.getTrackCount())) :
                "Select a track to start playback";
            m_artistLabel->setString(artistText.c_str());
            fitLabel(m_artistLabel, theme::kArtistLayout.width, theme::kArtistLayout.scale);
        }

        if (m_playPauseBtn) {
            bool isPlaying = mgr.isPlaying();
            for (auto* child : CCArrayExt<CCNode*>(m_playPauseBtn->getChildren())) {
                if (auto* bg = typeinfo_cast<CCScale9Sprite*>(child)) {
                    for (auto* sub : CCArrayExt<CCNode*>(bg->getChildren())) {
                        if (auto* lbl = typeinfo_cast<CCLabelBMFont*>(sub)) {
                            lbl->setString(isPlaying ? "PAUSE ||" : "PLAY >");
                        }
                    }
                }
            }
        }
    }

    void MusicPlayerPopup::update(float dt) {
        auto& mgr = MusicPlayerManager::get();

        if (m_activeTab == SubTab::Player) {
            if (m_displayedTrackRevision != mgr.getTrackRevision()) {
                m_displayedTrackRevision = mgr.getTrackRevision();
                refreshPlayerState();
            }
            float curTime = mgr.getCurrentTime();
            float dur = mgr.getDuration();

            if (m_timeElapsedLabel) {
                m_timeElapsedLabel->setString(formatTime(curTime).c_str());
            }
            if (m_timeTotalLabel) {
                m_timeTotalLabel->setString(formatTime(dur).c_str());
            }

            // SliderTouchLogic does not expose an is-touching field in the
            // current Geode bindings. Keep the playback indicator in sync
            // without relying on an unavailable private binding member.
            if (m_progressSlider && dur > 0.01f) {
                m_progressSlider->setValue(std::clamp(curTime / dur, 0.f, 1.f));
            }

            updateVisualizer();
        }
    }

    void MusicPlayerPopup::updateVisualizer() {
        if (m_visualizerBars.empty()) return;

        auto bars = MusicPlayerManager::get().getVisualizerBars(m_visualizerBars.size());
        constexpr float maxH = theme::kVisualizerLayout.height;

        for (size_t i = 0; i < m_visualizerBars.size() && i < bars.size(); ++i) {
            float val = std::clamp(bars[i], 0.05f, 1.0f);
            float h = std::max(3.f, val * maxH);

            m_visualizerBars[i]->setContentSize({m_visualizerBars[i]->getContentSize().width, h});

            GLubyte r = static_cast<GLubyte>(
                theme::kVisLowColor.r + val * (theme::kVisPeakColor.r - theme::kVisLowColor.r)
            );
            GLubyte g = static_cast<GLubyte>(
                theme::kVisLowColor.g + val * (theme::kVisPeakColor.g - theme::kVisLowColor.g)
            );
            GLubyte b = static_cast<GLubyte>(
                theme::kVisLowColor.b + val * (theme::kVisPeakColor.b - theme::kVisLowColor.b)
            );
            m_visualizerBars[i]->setColor({r, g, b});
        }
    }

    // ─── Playlist / Tracks View ──────────────────────────────────────────────

    void MusicPlayerPopup::buildPlaylistView() {
        auto const cSize = m_contentArea->getContentSize();
        auto& mgr = MusicPlayerManager::get();

        constexpr float listW = theme::kPlaylistLayout.width;
        constexpr float listH = theme::kPlaylistLayout.height;

        auto* scroll = ScrollLayer::create({listW, listH});
        scroll->setPosition(
            {(cSize.width - listW) * 0.5f + theme::kPlaylistLayout.x, theme::kPlaylistLayout.y}
        );
        scroll->setScale(theme::kPlaylistLayout.scale);

        scroll->m_contentLayer->setLayout(
            ColumnLayout::create()
                ->setGap(3.f)
                ->setAxisReverse(true)
                ->setAxisAlignment(AxisAlignment::End)
                ->setCrossAxisAlignment(AxisAlignment::Center)
                ->setAutoGrowAxis(listH)
                ->setAutoScale(false)
        );

        int currentIdx = mgr.getCurrentTrackIndex();
        size_t count = mgr.getTrackCount();

        for (size_t i = 0; i < count; ++i) {
            auto const* track = mgr.getTrack(i);
            if (!track) continue;

            bool isCurrent = (static_cast<int>(i) == currentIdx);

            constexpr float rowW = listW - 4.f;
            constexpr float rowH = 26.f;

            auto* row = CCNode::create();
            row->setContentSize({rowW, rowH});

            auto* bg = CCScale9Sprite::create(theme::kSprCardBg);
            if (!bg) bg = CCScale9Sprite::create("GJ_square01.png");
            bg->setContentSize({rowW, rowH});
            bg->setColor(isCurrent ? theme::kPlaylistActiveColor : theme::kCardBgColor);
            bg->setOpacity(isCurrent ? 255 : theme::kCardBgOpacity);
            bg->setPosition({rowW * 0.5f, rowH * 0.5f});
            row->addChild(bg, 0);

            std::string numStr = std::to_string(i + 1) + ".";
            auto* numLbl = CCLabelBMFont::create(numStr.c_str(), theme::kFontValues);
            numLbl->setScale(0.35f);
            numLbl->setColor(isCurrent ? theme::kAccentColor : ccColor3B{150, 180, 180});
            numLbl->setAnchorPoint({0.f, 0.5f});
            numLbl->setPosition({8.f, rowH * 0.5f});
            row->addChild(numLbl, 1);

            auto* titleLbl = CCLabelBMFont::create(track->title.c_str(), theme::kFontTrackTitle);
            titleLbl->setScale(0.35f);
            titleLbl->setColor({255, 255, 255});
            titleLbl->setAnchorPoint({0.f, 0.5f});
            titleLbl->setPosition({34.f, rowH * 0.5f});
            fitLabel(titleLbl, 180.f, 0.35f);
            row->addChild(titleLbl, 1);

            auto* artistLbl = CCLabelBMFont::create(track->artist.c_str(), theme::kFontArtist);
            artistLbl->setScale(0.30f);
            artistLbl->setColor({140, 180, 180});
            artistLbl->setAnchorPoint({0.f, 0.5f});
            artistLbl->setPosition({225.f, rowH * 0.5f});
            fitLabel(artistLbl, 120.f, 0.30f);
            row->addChild(artistLbl, 1);

            auto* rowMenu = CCMenu::create();
            rowMenu->setPosition({0.f, 0.f});
            row->addChild(rowMenu, 2);

            int trackIdx = static_cast<int>(i);
            char const* playBtnText = isCurrent ? "PLAYING" : "PLAY";
            auto* btn = makeTextButton(
                playBtnText,
                62.f,
                18.f,
                [this, trackIdx] {
                    MusicPlayerManager::get().playTrack(trackIdx);
                    buildTabContent();
                },
                0.28f
            );
            btn->setPosition({rowW - 36.f, rowH * 0.5f});
            rowMenu->addChild(btn);

            scroll->m_contentLayer->addChild(row);
        }

        scroll->m_contentLayer->updateLayout();
        scroll->scrollToTop();
        m_contentArea->addChild(scroll);
    }

    // ─── Live Audio FX View ──────────────────────────────────────────────────

    void MusicPlayerPopup::buildEffectsView() {
        auto const cSize = m_contentArea->getContentSize();
        auto& mgr = MusicPlayerManager::get();

        constexpr float listW = theme::kEffectsLayout.width;
        constexpr float listH = theme::kEffectsLayout.height;

        auto* scroll = ScrollLayer::create({listW, listH});
        scroll->setPosition(
            {(cSize.width - listW) * 0.5f + theme::kEffectsLayout.x, theme::kEffectsLayout.y}
        );
        scroll->setScale(theme::kEffectsLayout.scale);

        scroll->m_contentLayer->setLayout(
            ColumnLayout::create()
                ->setGap(6.f)
                ->setAxisReverse(true)
                ->setAxisAlignment(AxisAlignment::End)
                ->setCrossAxisAlignment(AxisAlignment::Center)
                ->setAutoGrowAxis(listH)
                ->setAutoScale(false)
        );

        auto addSection = [&](char const* title) {
            auto* node = CCNode::create();
            node->setContentSize({listW, 18.f});
            auto* lbl = CCLabelBMFont::create(title, theme::kFontSectionTitle);
            lbl->setScale(0.38f);
            lbl->setColor(theme::kAccentColor);
            lbl->setAnchorPoint({0.f, 0.5f});
            lbl->setPosition({8.f, 9.f});
            node->addChild(lbl);
            scroll->m_contentLayer->addChild(node);
        };

        addSection("8D BINAURAL ROTATION");
        {
            auto* row = makeCard(listW - 4.f, 44.f);
            auto* menu = CCMenu::create();
            menu->setPosition({0.f, 0.f});
            row->addChild(menu);

            bool is8d = mgr.is8DEnabled();
            auto* toggleBtn = makeTextButton(
                is8d ? "ENABLED [ON]" : "DISABLED [OFF]",
                105.f,
                22.f,
                [this, &mgr, is8d] {
                    mgr.set8DEnabled(!is8d);
                    buildTabContent();
                },
                0.32f
            );
            toggleBtn->setPosition({65.f, 22.f});
            menu->addChild(toggleBtn);

            auto* desc = CCLabelBMFont::create(
                "Smooth 3D head rotation with subtle room acoustics", theme::kFontValues
            );
            desc->setScale(0.30f);
            desc->setColor({150, 190, 190});
            desc->setPosition({270.f, 30.f});
            row->addChild(desc);

            auto* spdSlow = makeTextButton(
                "Slow (0.08Hz)",
                78.f,
                16.f,
                [&mgr] {
                    mgr.set8DSpeed(0.08f);
                },
                0.26f
            );
            spdSlow->setPosition({170.f, 12.f});
            menu->addChild(spdSlow);

            auto* spdMed = makeTextButton(
                "Normal (0.15Hz)",
                82.f,
                16.f,
                [&mgr] {
                    mgr.set8DSpeed(0.15f);
                },
                0.26f
            );
            spdMed->setPosition({256.f, 12.f});
            menu->addChild(spdMed);

            auto* spdFast = makeTextButton(
                "Fast (0.30Hz)",
                78.f,
                16.f,
                [&mgr] {
                    mgr.set8DSpeed(0.30f);
                },
                0.26f
            );
            spdFast->setPosition({342.f, 12.f});
            menu->addChild(spdFast);

            scroll->m_contentLayer->addChild(row);
        }

        addSection("STUDIO REVERB ENGINE");
        {
            auto* row = makeCard(listW - 4.f, 52.f);
            auto* menu = CCMenu::create();
            menu->setPosition({0.f, 0.f});
            row->addChild(menu);

            float rev = mgr.getReverb();
            std::string revText = "Reverb: " + std::to_string(static_cast<int>(rev)) + " ms";
            auto* revLbl = CCLabelBMFont::create(revText.c_str(), theme::kFontValues);
            revLbl->setScale(0.35f);
            revLbl->setColor({255, 255, 255});
            revLbl->setPosition({65.f, 38.f});
            row->addChild(revLbl);

            auto* pNone = makeTextButton(
                "Off",
                40.f,
                18.f,
                [this, &mgr] {
                    mgr.setReverb(0.f);
                    buildTabContent();
                },
                0.28f
            );
            pNone->setPosition({150.f, 38.f});
            menu->addChild(pNone);

            auto* pRoom = makeTextButton(
                "Room (800ms)",
                74.f,
                18.f,
                [this, &mgr] {
                    mgr.setReverb(800.f);
                    buildTabContent();
                },
                0.26f
            );
            pRoom->setPosition({212.f, 38.f});
            menu->addChild(pRoom);

            auto* pCinema = makeTextButton(
                "Cinema (2.5s)",
                74.f,
                18.f,
                [this, &mgr] {
                    mgr.setReverb(2500.f);
                    buildTabContent();
                },
                0.26f
            );
            pCinema->setPosition({292.f, 38.f});
            menu->addChild(pCinema);

            auto* pCath = makeTextButton(
                "Cathedral (5s)",
                74.f,
                18.f,
                [this, &mgr] {
                    mgr.setReverb(5000.f);
                    buildTabContent();
                },
                0.26f
            );
            pCath->setPosition({372.f, 38.f});
            menu->addChild(pCath);

            auto* pGdh = makeTextButton(
                "GDH Reverb (10s)",
                95.f,
                18.f,
                [this, &mgr] {
                    mgr.setReverb(10000.f);
                    buildTabContent();
                },
                0.26f
            );
            pGdh->setPosition({190.f, 14.f});
            menu->addChild(pGdh);

            auto* pPlate = makeTextButton(
                "Plate Reverb",
                80.f,
                18.f,
                [this, &mgr] {
                    mgr.setAudioPreset("Plate");
                    buildTabContent();
                },
                0.26f
            );
            pPlate->setPosition({285.f, 14.f});
            menu->addChild(pPlate);

            scroll->m_contentLayer->addChild(row);
        }

        addSection("MUFFLE & ACOUSTIC FILTERS");
        {
            auto* row = makeCard(listW - 4.f, 60.f);
            auto* menu = CCMenu::create();
            menu->setPosition({0.f, 0.f});
            row->addChild(menu);

            float muf = mgr.getMuffle();
            std::string mufText = "Muffle: " + std::to_string(static_cast<int>(muf * 100.f)) + "%";
            auto* mufLbl = CCLabelBMFont::create(mufText.c_str(), theme::kFontValues);
            mufLbl->setScale(0.35f);
            mufLbl->setColor({255, 255, 255});
            mufLbl->setPosition({65.f, 44.f});
            row->addChild(mufLbl);

            auto* m0 = makeTextButton(
                "Clean (0%)",
                62.f,
                18.f,
                [this, &mgr] {
                    mgr.setMuffle(0.f);
                    buildTabContent();
                },
                0.26f
            );
            m0->setPosition({150.f, 44.f});
            menu->addChild(m0);

            auto* m30 = makeTextButton(
                "Cozy (30%)",
                62.f,
                18.f,
                [this, &mgr] {
                    mgr.setMuffle(0.3f);
                    buildTabContent();
                },
                0.26f
            );
            m30->setPosition({220.f, 44.f});
            menu->addChild(m30);

            auto* m60 = makeTextButton(
                "Party Next Door (60%)",
                110.f,
                18.f,
                [this, &mgr] {
                    mgr.setMuffle(0.6f);
                    buildTabContent();
                },
                0.24f
            );
            m60->setPosition({315.f, 44.f});
            menu->addChild(m60);

            auto* fWater = makeTextButton(
                "Underwater",
                70.f,
                16.f,
                [this, &mgr] {
                    mgr.setAudioFilter("Underwater");
                    buildTabContent();
                },
                0.26f
            );
            fWater->setPosition({60.f, 16.f});
            menu->addChild(fWater);

            auto* fPhone = makeTextButton(
                "Telephone",
                65.f,
                16.f,
                [this, &mgr] {
                    mgr.setAudioFilter("Telephone");
                    buildTabContent();
                },
                0.26f
            );
            fPhone->setPosition({135.f, 16.f});
            menu->addChild(fPhone);

            auto* fLofi = makeTextButton(
                "Lo-Fi Tape",
                65.f,
                16.f,
                [this, &mgr] {
                    mgr.setAudioFilter("Lo-Fi Tape");
                    buildTabContent();
                },
                0.26f
            );
            fLofi->setPosition({208.f, 16.f});
            menu->addChild(fLofi);

            auto* fRadio = makeTextButton(
                "Vintage Radio",
                75.f,
                16.f,
                [this, &mgr] {
                    mgr.setAudioFilter("Vintage Radio");
                    buildTabContent();
                },
                0.25f
            );
            fRadio->setPosition({284.f, 16.f});
            menu->addChild(fRadio);

            auto* fSpace = makeTextButton(
                "Space Echo",
                68.f,
                16.f,
                [this, &mgr] {
                    mgr.setAudioFilter("Space Echo");
                    buildTabContent();
                },
                0.25f
            );
            fSpace->setPosition({362.f, 16.f});
            menu->addChild(fSpace);

            scroll->m_contentLayer->addChild(row);
        }

        scroll->m_contentLayer->updateLayout();
        scroll->scrollToTop();
        m_contentArea->addChild(scroll);
    }

    // ─── 10-Band Equalizer View ──────────────────────────────────────────────

    void MusicPlayerPopup::buildEqualizerView() {
        auto const cSize = m_contentArea->getContentSize();
        auto& mgr = MusicPlayerManager::get();

        constexpr float cardW = theme::kEqualizerLayout.width;
        constexpr float cardH = theme::kEqualizerLayout.height;

        auto* card = makeCard(cardW, cardH);
        card->setPosition(
            {(cSize.width - cardW) * 0.5f + theme::kEqualizerLayout.x, theme::kEqualizerLayout.y}
        );
        card->setScale(theme::kEqualizerLayout.scale);
        m_contentArea->addChild(card);

        auto* menu = CCMenu::create();
        menu->setPosition({0.f, 0.f});
        card->addChild(menu);

        auto* eqTitle = CCLabelBMFont::create(
            "10-BAND GRAPHIC EQUALIZER (-9 dB to +9 dB)", theme::kFontSectionTitle
        );
        eqTitle->setScale(0.35f);
        eqTitle->setColor(theme::kAccentColor);
        eqTitle->setPosition({cardW * 0.5f, cardH - 12.f});
        card->addChild(eqTitle);

        auto* pFlat = makeTextButton(
            "FLAT",
            48.f,
            16.f,
            [this, &mgr] {
                mgr.applyEqPreset(0);
                buildTabContent();
            },
            0.28f
        );
        pFlat->setPosition({60.f, cardH - 28.f});
        menu->addChild(pFlat);

        auto* pBass = makeTextButton(
            "BASS BOOST",
            72.f,
            16.f,
            [this, &mgr] {
                mgr.applyEqPreset(1);
                buildTabContent();
            },
            0.26f
        );
        pBass->setPosition({126.f, cardH - 28.f});
        menu->addChild(pBass);

        auto* pVocal = makeTextButton(
            "VOCAL BOOST",
            74.f,
            16.f,
            [this, &mgr] {
                mgr.applyEqPreset(2);
                buildTabContent();
            },
            0.26f
        );
        pVocal->setPosition({205.f, cardH - 28.f});
        menu->addChild(pVocal);

        auto* pTreb = makeTextButton(
            "TREBLE",
            52.f,
            16.f,
            [this, &mgr] {
                mgr.applyEqPreset(3);
                buildTabContent();
            },
            0.26f
        );
        pTreb->setPosition({274.f, cardH - 28.f});
        menu->addChild(pTreb);

        auto* pEdm = makeTextButton(
            "EDM / V-SHAPE",
            80.f,
            16.f,
            [this, &mgr] {
                mgr.applyEqPreset(4);
                buildTabContent();
            },
            0.26f
        );
        pEdm->setPosition({346.f, cardH - 28.f});
        menu->addChild(pEdm);

        constexpr char const* kFreqLabels[10] = {
            "30Hz", "60Hz", "125Hz", "250Hz", "500Hz", "1kHz", "2kHz", "4kHz", "8kHz", "16kHz"
        };

        constexpr size_t kBandCount = 10;
        constexpr float startX = 26.f;
        constexpr float spacing = 42.f;
        constexpr float sliderH = 110.f;
        constexpr float sliderY = 46.f;

        for (size_t i = 0; i < kBandCount; ++i) {
            float x = startX + i * spacing;
            float currentDb = mgr.getEqBand(static_cast<int>(i));

            auto* freqLbl = CCLabelBMFont::create(kFreqLabels[i], theme::kFontValues);
            freqLbl->setScale(0.28f);
            freqLbl->setColor({140, 190, 190});
            freqLbl->setPosition({x, 24.f});
            card->addChild(freqLbl);

            char dbBuf[16];
            std::snprintf(dbBuf, sizeof(dbBuf), "%+.1f", currentDb);
            auto* dbLbl = CCLabelBMFont::create(dbBuf, theme::kFontValues);
            dbLbl->setScale(0.26f);
            dbLbl->setColor(
                currentDb > 0.01f ?
                    theme::kEqBoostTextColor :
                    (currentDb < -0.01f ? theme::kEqCutTextColor : theme::kEqNeutralTextColor)
            );
            dbLbl->setPosition({x, sliderY + sliderH + 8.f});
            card->addChild(dbLbl);

            auto* trackBg = CCLayerColor::create(theme::kEqTrackColor, 6.f, sliderH);
            trackBg->setPosition({x - 3.f, sliderY});
            card->addChild(trackBg);

            auto* zeroLine = CCLayerColor::create(theme::kEqCenterLineColor, 14.f, 1.5f);
            zeroLine->setPosition({x - 7.f, sliderY + sliderH * 0.5f});
            card->addChild(zeroLine);

            float norm = std::clamp((currentDb + 9.f) / 18.f, 0.f, 1.f);
            float fillH = std::abs(norm - 0.5f) * sliderH;
            float fillY = (norm >= 0.5f) ? (sliderY + sliderH * 0.5f) : (sliderY + norm * sliderH);

            ccColor4B fillColor = (currentDb >= 0.f) ? theme::kEqBoostColor : theme::kEqCutColor;
            auto* fillBar = CCLayerColor::create(fillColor, 6.f, std::max(2.f, fillH));
            fillBar->setPosition({x - 3.f, fillY});
            card->addChild(fillBar);

            auto* thumb = CCScale9Sprite::create(theme::kSprSliderThumb);
            if (!thumb) thumb = CCScale9Sprite::create("GJ_square01.png");
            thumb->setContentSize({14.f, 8.f});
            thumb->setColor({255, 255, 255});
            thumb->setPosition({x, sliderY + norm * sliderH});
            card->addChild(thumb);

            int bandIdx = static_cast<int>(i);
            auto* plusBtn = makeTextButton(
                "+",
                16.f,
                14.f,
                [this, &mgr, bandIdx, currentDb] {
                    mgr.setEqBand(bandIdx, std::clamp(currentDb + 1.5f, -9.f, 9.f));
                    buildTabContent();
                },
                0.28f
            );
            plusBtn->setPosition({x, sliderY + sliderH - 8.f});
            menu->addChild(plusBtn);

            auto* minusBtn = makeTextButton(
                "-",
                16.f,
                14.f,
                [this, &mgr, bandIdx, currentDb] {
                    mgr.setEqBand(bandIdx, std::clamp(currentDb - 1.5f, -9.f, 9.f));
                    buildTabContent();
                },
                0.28f
            );
            minusBtn->setPosition({x, sliderY + 8.f});
            menu->addChild(minusBtn);
        }
    }

} // namespace rickgdps::music
