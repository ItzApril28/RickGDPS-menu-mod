#include "AmoledOverlay.hpp"

#include "MusicPlayerManager.hpp"

#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/CCKeyboardDispatcher.hpp>
#include <Geode/modify/CCMouseDispatcher.hpp>
#include <Geode/modify/CCTouchDispatcher.hpp>

#include <algorithm>
#include <cstdio>
#include <string>

using namespace geode::prelude;

namespace rickgdps::music::amoled {

    namespace {
        float s_idleSeconds = 0.f;
        bool s_dimmed = false;

        std::string formatTime(float seconds) {
            if (seconds < 0.f) seconds = 0.f;
            int total = static_cast<int>(seconds);
            char buffer[16];
            std::snprintf(buffer, sizeof(buffer), "%d:%02d", total / 60, total % 60);
            return std::string(buffer);
        }

        class DimLayer : public cocos2d::CCLayerColor {
        public:
            static constexpr char const* kNodeID = "rickgdps-amoled-dim";

            static DimLayer* create() {
                auto* ret = new DimLayer();
                if (ret && ret->init()) {
                    ret->autorelease();
                    return ret;
                }
                CC_SAFE_DELETE(ret);
                return nullptr;
            }

            bool init() {
                auto const winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
                if (!cocos2d::CCLayerColor::initWithColor({0, 0, 0, 255}, winSize.width, winSize.height)) {
                    return false;
                }

                setID(kNodeID);
                setTouchEnabled(true);
                setTouchMode(cocos2d::kCCTouchesOneByOne);
                setTouchPriority(-600);
                scheduleUpdate();

                m_titleLabel = cocos2d::CCLabelBMFont::create("Geometry Dash", "bigFont.fnt");
                if (m_titleLabel) {
                    m_titleLabel->setScale(0.6f);
                    m_titleLabel->setOpacity(130);
                    m_titleLabel->setPosition({winSize.width * 0.5f, winSize.height * 0.5f + 16.f});
                    addChild(m_titleLabel);
                }
                updateTitle();

                m_timeLabel = cocos2d::CCLabelBMFont::create("", "chatFont.fnt");
                if (m_timeLabel) {
                    m_timeLabel->setScale(0.7f);
                    m_timeLabel->setOpacity(95);
                    m_timeLabel->setPosition({winSize.width * 0.5f, winSize.height * 0.5f - 14.f});
                    addChild(m_timeLabel);
                }

                auto* hint = cocos2d::CCLabelBMFont::create("Touch or press any key to wake", "chatFont.fnt");
                if (hint) {
                    hint->setScale(0.6f);
                    hint->setOpacity(70);
                    hint->setPosition({winSize.width * 0.5f, winSize.height * 0.5f - 44.f});
                    addChild(hint);
                }

                return true;
            }

            void updateTitle() {
                if (!m_titleLabel) return;
                auto const* track = MusicPlayerManager::get().getCurrentTrack();
                if (track && !track->title.empty()) {
                    m_titleLabel->setString(track->title.c_str());
                } else if (auto* pl = PlayLayer::get()) {
                    if (pl->m_level && !pl->m_level->m_levelName.empty()) {
                        m_titleLabel->setString(pl->m_level->m_levelName.c_str());
                    } else {
                        m_titleLabel->setString("Geometry Dash");
                    }
                } else {
                    m_titleLabel->setString("Geometry Dash");
                }

                // Re-scale so long titles still fit within 80% screen width
                auto const winSize = cocos2d::CCDirector::sharedDirector()->getWinSize();
                float const raw = m_titleLabel->getContentSize().width;
                float const maxW = winSize.width * 0.8f;
                m_titleLabel->setScale(raw > 0.f ? std::min(0.6f, maxW / raw) : 0.6f);
            }

            void update(float) override {
                auto& mgr = MusicPlayerManager::get();
                if (m_timeLabel) {
                    if (mgr.isPlaying()) {
                        std::string text = formatTime(mgr.getCurrentTime()) + " / " + formatTime(mgr.getDuration());
                        m_timeLabel->setString(text.c_str());
                        m_timeLabel->setVisible(true);
                    } else if (auto* pl = PlayLayer::get()) {
                        char buf[32];
                        std::snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(pl->getCurrentPercent()));
                        m_timeLabel->setString(buf);
                        m_timeLabel->setVisible(true);
                    } else {
                        m_timeLabel->setVisible(false);
                    }
                }
                updateTitle();
            }

            bool ccTouchBegan(cocos2d::CCTouch*, cocos2d::CCEvent*) override {
                notifyActivity();
                return true; // Swallow touch that woke the display
            }

        private:
            cocos2d::CCLabelBMFont* m_titleLabel = nullptr;
            cocos2d::CCLabelBMFont* m_timeLabel = nullptr;
        };

        void showDim() {
            auto* scene = cocos2d::CCDirector::sharedDirector()->getRunningScene();
            if (!scene) return;
            if (typeinfo_cast<cocos2d::CCTransitionScene*>(scene)) return;

            auto* layer = typeinfo_cast<DimLayer*>(scene->getChildByID(DimLayer::kNodeID));
            if (!layer) {
                layer = DimLayer::create();
                if (!layer) return;
                scene->addChild(layer, 200000);
            }

            float opacity = 0.96f;
            if (auto* mod = Mod::get()) {
                opacity = static_cast<float>(mod->getSettingValue<double>("audio-amoled-opacity"));
            }
            opacity = std::clamp(opacity, 0.5f, 1.f);
            layer->setOpacity(static_cast<GLubyte>(opacity * 255.f));
            layer->setVisible(true);
            layer->setTouchEnabled(true);
            s_dimmed = true;
        }

        void hideDim() {
            s_dimmed = false;
            auto* scene = cocos2d::CCDirector::sharedDirector()->getRunningScene();
            if (!scene) return;
            if (auto* layer = typeinfo_cast<DimLayer*>(scene->getChildByID(DimLayer::kNodeID))) {
                layer->removeFromParent();
            }
        }
    }

    bool isDimmed() {
        return s_dimmed;
    }

    void notifyActivity() {
        s_idleSeconds = 0.f;
        if (s_dimmed) hideDim();
    }

    void reset() {
        s_idleSeconds = 0.f;
        hideDim();
    }

    void tick(float dt) {
        auto* mod = Mod::get();
        if (!mod || !mod->getSettingValue<bool>("audio-amoled-enabled")) {
            if (s_dimmed) hideDim();
            return;
        }
        if (s_dimmed) return;

        s_idleSeconds += dt;
        float timeout = static_cast<float>(mod->getSettingValue<double>("audio-amoled-timeout"));
        if (timeout < 5.f) timeout = 5.f;
        if (s_idleSeconds >= timeout) {
            s_idleSeconds = 0.f;
            showDim();
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Global Activity Hooks & Frame Ticker
// ─────────────────────────────────────────────────────────────────────────────

// Wake on any key press
class $modify(RickGdpsAmoledKeyboard, cocos2d::CCKeyboardDispatcher) {
    void dispatchKeyboardMSG(cocos2d::enumKeyCodes key, bool isKeyDown, bool isRepeat, double timestamp) {
        if (isKeyDown) rickgdps::music::amoled::notifyActivity();
        cocos2d::CCKeyboardDispatcher::dispatchKeyboardMSG(key, isKeyDown, isRepeat, timestamp);
    }
};

// Wake on any screen touch or mouse click/drag
class $modify(RickGdpsAmoledTouch, cocos2d::CCTouchDispatcher) {
    void touchesBegan(cocos2d::CCSet* touches, cocos2d::CCEvent* event) {
        rickgdps::music::amoled::notifyActivity();
        cocos2d::CCTouchDispatcher::touchesBegan(touches, event);
    }
    void touchesMoved(cocos2d::CCSet* touches, cocos2d::CCEvent* event) {
        rickgdps::music::amoled::notifyActivity();
        cocos2d::CCTouchDispatcher::touchesMoved(touches, event);
    }
};

// Wake on mouse scroll wheel
class $modify(RickGdpsAmoledMouse, cocos2d::CCMouseDispatcher) {
    bool dispatchScrollMSG(float x, float y) {
        rickgdps::music::amoled::notifyActivity();
        return cocos2d::CCMouseDispatcher::dispatchScrollMSG(x, y);
    }
};

// Global frame ticker & scene transition handler
class $modify(RickGdpsAmoledDirector, cocos2d::CCDirector) {
    void drawScene() {
        cocos2d::CCDirector::drawScene();
        rickgdps::music::amoled::tick(getDeltaTime());
    }

    void willSwitchToScene(cocos2d::CCScene* scene) {
        cocos2d::CCDirector::willSwitchToScene(scene);
        rickgdps::music::amoled::notifyActivity();
    }
};
