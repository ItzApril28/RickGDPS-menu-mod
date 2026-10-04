// HacksLayer.cpp
// RickGdps Menu Mod v1.4.0 — Hack-client gameplay hooks
// Covers: noclip, speedhack, FPS bypass, auto-retry, practice music,
//         show%, hide-attempts, hitboxes, layout mode, mirror mode,

#include <Geode/Geode.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/CCScheduler.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/cocos/draw_nodes/CCDrawNode.h>
#include <atomic>
#include <cmath>
#include <string>
#include <unordered_set>

using namespace geode::prelude;

namespace rickgdps {

// ─────────────────────────────────────────────────────────────────────────────
//  Global hack state
// ─────────────────────────────────────────────────────────────────────────────
std::atomic<bool>  g_noclipEnabled{false};
std::atomic<float> g_noclipOpacity{0.5f};
std::atomic<bool>  g_speedhackEnabled{false};
std::atomic<float> g_speedhackSpeed{1.f};
std::atomic<bool>  g_fpsBypassEnabled{false};
std::atomic<float> g_fpsBypassValue{240.f};
std::atomic<bool>  g_autoRetryEnabled{false};
std::atomic<float> g_autoRetryDelay{0.5f};
std::atomic<bool>  g_practiceMusicHack{false};
std::atomic<float> g_respawnDelay{0.f};
std::atomic<bool>  g_showPercentage{false};
std::atomic<bool>  g_hideAttempts{false};
std::atomic<bool>  g_hideUi{false};
std::atomic<bool>  g_hitboxEnabled{false};
std::atomic<bool>  g_hitboxSolid{false};
std::atomic<bool>  g_trailEnabled{true};
std::atomic<bool>  g_layoutMode{false};
std::atomic<bool>  g_mirrorMode{false};
std::atomic<bool>  g_anticheatBypass{false};
std::atomic<bool>  g_startPosEnabled{false};
std::atomic<float> g_startPosPercent{0.f};

// ─────────────────────────────────────────────────────────────────────────────
//  Bind helpers — called from $on_mod(Loaded) at the bottom
// ─────────────────────────────────────────────────────────────────────────────
static void bindHackBool(char const* key, std::atomic<bool>& flag) {
    flag.store(Mod::get()->getSettingValue<bool>(key), std::memory_order_relaxed);
    listenForSettingChanges<bool>(key, [&flag](bool v) {
        flag.store(v, std::memory_order_relaxed);
    });
}
static void bindHackFloat(char const* key, std::atomic<float>& val) {
    val.store(static_cast<float>(Mod::get()->getSettingValue<double>(key)),
              std::memory_order_relaxed);
    listenForSettingChanges<double>(key, [&val](double v) {
        val.store(static_cast<float>(v), std::memory_order_relaxed);
    });
}

// ─────────────────────────────────────────────────────────────────────────────
//  Noclip — PlayerObject::update
// ─────────────────────────────────────────────────────────────────────────────
class $modify(RickNoclipPlayer, PlayerObject) {
    struct Fields {
        bool trailHidden = false;
    };

    void update(float dt) {
        PlayerObject::update(dt);
        applyTrailToggle();

        if (!g_noclipEnabled.load(std::memory_order_relaxed)) {
            if (getOpacity() < 255) setOpacity(255);
            return;
        }
        // Dim player while noclip active
        auto opacity = static_cast<GLubyte>(
            std::clamp(g_noclipOpacity.load(std::memory_order_relaxed), 0.f, 1.f) * 255.f
        );
        setOpacity(opacity);
    }

    // The "Player Trail" setting used to be a no-op. Hide / restore the trail
    // streaks + particles, remembering our own state so we never fight the
    // game once the trail is restored.
    void applyTrailToggle() {
        bool const want = g_trailEnabled.load(std::memory_order_relaxed);
        if (want == !m_fields->trailHidden) return;
        m_fields->trailHidden = !want;
        if (m_regularTrail) m_regularTrail->setVisible(want);
        if (m_waveTrail) m_waveTrail->setVisible(want);
        if (m_trailingParticles) m_trailingParticles->setVisible(want);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  Speedhack — CCScheduler::update
// ─────────────────────────────────────────────────────────────────────────────
class $modify(RickSpeedhack, CCScheduler) {
    void update(float dt) {
        if (g_speedhackEnabled.load(std::memory_order_relaxed)) {
            float speed = g_speedhackSpeed.load(std::memory_order_relaxed);
            CCScheduler::update(dt * speed);
        } else {
            CCScheduler::update(dt);
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  FPS Bypass — CCDirector::setAnimationInterval
// ─────────────────────────────────────────────────────────────────────────────
class $modify(RickFpsBypass, CCDirector) {
    void setAnimationInterval(double interval) {
        if (g_fpsBypassEnabled.load(std::memory_order_relaxed)) {
            float target = g_fpsBypassValue.load(std::memory_order_relaxed);
            if (target > 0.f) {
                CCDirector::setAnimationInterval(1.0 / static_cast<double>(target));
                return;
            }
        }
        CCDirector::setAnimationInterval(interval);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  PlayLayer hooks — noclip death, auto-retry, percentage, attempts
// ─────────────────────────────────────────────────────────────────────────────
class $modify(RickPlayLayer, PlayLayer) {
    struct Fields {
        bool layoutApplied = false;
        bool mirrorApplied = false;
        bool hideUiApplied = false;
        bool prevPercentVisible = true;
        bool prevAttemptVisible = true;
        bool prevProgressVisible = true;
        bool prevPauseVisible = true;
        bool respawnPending = false;
    };

    // ── Level init ──────────────────────────────────────────────────────────
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        // Layout & mirror modes (kept in sync with the settings in update())
        applyVisualToggles(true);

        // Hide UI (percentage / attempts / progress bar / pause button)
        updateHideUi();

        // Custom start position — deferred one frame so the level is fully set up
        if (g_startPosEnabled.load(std::memory_order_relaxed)) {
            scheduleOnce(schedule_selector(RickPlayLayer::rickApplyStartPosition), 0.f);
        }

        // Show/Hide attempts label
        updateHudVisibility();

        return true;
    }

    // ── Called every frame ─────────────────────────────────────────────────
    void update(float dt) {

        PlayLayer::update(dt);

        // Noclip death cancel — check if player was killed this frame and revive
        if (g_noclipEnabled.load(std::memory_order_relaxed)) {
            if (m_player1 && m_player1->m_isDead) {
                m_player1->m_isDead = false;
                m_player1->m_isOnGround = true;
            }
            if (m_player2 && m_player2->m_isDead) {
                m_player2->m_isDead = false;
            }
        }

        // Keep layout / mirror / hide-ui in sync when settings change mid-run
        applyVisualToggles(false);
        updateHideUi();

        // Show percentage
        if (g_showPercentage.load(std::memory_order_relaxed)) {
            updatePercentageLabel();
        }
    }

    // ── Death hook ─────────────────────────────────────────────────────────
    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        if (g_noclipEnabled.load(std::memory_order_relaxed)) return; // skip death entirely

        PlayLayer::destroyPlayer(player, obj);

        // Auto-retry
        if (g_autoRetryEnabled.load(std::memory_order_relaxed)) {
            float delay = g_autoRetryDelay.load(std::memory_order_relaxed);
            if (delay <= 0.f) {
                resetLevel();
            } else {
                scheduleOnce(schedule_selector(RickPlayLayer::autoRetryTick), delay);
            }
        }
    }

    void autoRetryTick(float) {
        resetLevel();
    }

    // ── Reset (new attempt) ────────────────────────────────────────────────
    void resetLevel() {
        unschedule(schedule_selector(RickPlayLayer::rickFinishRespawn));
        m_fields->respawnPending = false;
        PlayLayer::resetLevel();
        updateHudVisibility();
    }

    // ── Respawn delay (practice mode) ──────────────────────────────────────
    // GD calls delayedResetLevel() after a death to respawn at the last
    // checkpoint; deferring that call implements the "Respawn Delay" setting.
    void delayedResetLevel() {
        float const delay = g_respawnDelay.load(std::memory_order_relaxed);
        bool const practice = m_isPracticeMode;
        bool const dead = (m_player1 && m_player1->m_isDead) || (m_player2 && m_player2->m_isDead);
        if (delay > 0.01f && practice && dead && !m_fields->respawnPending) {
            m_fields->respawnPending = true;
            scheduleOnce(schedule_selector(RickPlayLayer::rickFinishRespawn), delay);
            return;
        }
        m_fields->respawnPending = false;
        PlayLayer::delayedResetLevel();
    }

    void rickFinishRespawn(float) {
        m_fields->respawnPending = false;
        PlayLayer::delayedResetLevel();
    }

    // ── Custom start position ──────────────────────────────────────────────
    void rickApplyStartPosition(float) {
        if (!g_startPosEnabled.load(std::memory_order_relaxed)) return;

        float const percent = std::clamp(g_startPosPercent.load(std::memory_order_relaxed), 0.f, 100.f);
        if (percent <= 0.01f) return;

        auto* startPos = StartPosObject::create();
        if (!startPos) return;

        float levelLength = m_levelLength;
        if (levelLength <= 0.f && m_level) levelLength = static_cast<float>(m_level->m_levelLength);
        if (levelLength <= 0.f) levelLength = 10000.f;

        float const x = levelLength * (percent / 100.f);
        float const y = m_player1 ? m_player1->getPositionY() : 105.f;
        startPos->setPosition({x, y});
        if (m_levelSettings) startPos->setSettings(m_levelSettings);
        startPos->retain();

        setStartPosObject(startPos);
        resetLevel();
        log::info("[RickGdps] Custom start position applied at {}% (x = {})", percent, x);
    }

    // ── Layout / mirror helpers ────────────────────────────────────────────
    void applyVisualToggles(bool force) {
        bool const layout = g_layoutMode.load(std::memory_order_relaxed);
        if (force || layout != m_fields->layoutApplied) {
            m_fields->layoutApplied = layout;
            if (m_background) m_background->setVisible(!layout);
            if (m_groundLayer) m_groundLayer->setVisible(!layout);
            if (m_groundLayer2) m_groundLayer2->setVisible(!layout);
        }

        bool const mirror = g_mirrorMode.load(std::memory_order_relaxed);
        if (force || mirror != m_fields->mirrorApplied) {
            m_fields->mirrorApplied = mirror;
            if (auto* director = CCDirector::sharedDirector()) {
                auto const winSize = director->getWinSize();
                if (mirror) {
                    setScaleX(-1.f);
                    setPositionX(winSize.width);
                } else {
                    setScaleX(1.f);
                    setPositionX(0.f);
                }
            }
        }
    }

    // ── Hide UI ────────────────────────────────────────────────────────────
    void updateHideUi() {
        bool const hide = g_hideUi.load(std::memory_order_relaxed);
        if (hide == m_fields->hideUiApplied) return;
        m_fields->hideUiApplied = hide;

        if (hide) {
            if (m_percentageLabel) {
                m_fields->prevPercentVisible = m_percentageLabel->isVisible();
                m_percentageLabel->setVisible(false);
            }
            if (m_attemptLabel) {
                m_fields->prevAttemptVisible = m_attemptLabel->isVisible();
                m_attemptLabel->setVisible(false);
            }
            if (m_progressBar) {
                m_fields->prevProgressVisible = m_progressBar->isVisible();
                m_progressBar->setVisible(false);
            }
            if (m_uiLayer && m_uiLayer->m_pauseBtn) {
                m_fields->prevPauseVisible = m_uiLayer->m_pauseBtn->isVisible();
                m_uiLayer->m_pauseBtn->setVisible(false);
            }
        } else {
            if (m_percentageLabel) m_percentageLabel->setVisible(m_fields->prevPercentVisible);
            if (m_attemptLabel) m_attemptLabel->setVisible(m_fields->prevAttemptVisible);
            if (m_progressBar) m_progressBar->setVisible(m_fields->prevProgressVisible);
            if (m_uiLayer && m_uiLayer->m_pauseBtn) m_uiLayer->m_pauseBtn->setVisible(m_fields->prevPauseVisible);
        }
    }


    // ── Helpers ────────────────────────────────────────────────────────────
    void updatePercentageLabel() {
        // The percentage label ID is "percentage-label"; ensure it exists
        if (!getChildByID("rickgdps-percent-label")) {
            auto* pct = CCLabelBMFont::create("0%", "bigFont.fnt");
            pct->setScale(0.5f);
            pct->setAnchorPoint({1.f, 1.f});
            auto ws = CCDirector::sharedDirector()->getWinSize();
            pct->setPosition({ws.width - 6.f, ws.height - 4.f});
            pct->setID("rickgdps-percent-label");
            pct->setZOrder(100);
            addChild(pct);
        }
        if (auto* pct = typeinfo_cast<CCLabelBMFont*>(getChildByID("rickgdps-percent-label"))) {
            char buf[16];
            int perc = static_cast<int>(getCurrentPercent());
            std::snprintf(buf, sizeof(buf), "%d%%", perc);
            pct->setString(buf);
            pct->setVisible(
                g_showPercentage.load(std::memory_order_relaxed) &&
                !g_hideUi.load(std::memory_order_relaxed)
            );
        }
    }

    void updateHudVisibility() {
        // Hide/show the attempt counter
        bool hide = g_hideAttempts.load(std::memory_order_relaxed);
        if (auto* attLabel = getChildByID("attempt-label")) {
            attLabel->setVisible(!hide);
        }
        // Fallback: look for percent/attempt nodes by type
        if (!hide) return;
        for (auto* child : CCArrayExt<CCNode*>(getChildren())) {
            if (auto* label = typeinfo_cast<CCLabelBMFont*>(child)) {
                // GD's attempt label is typically at tag 18 or has "attempt" in string
                std::string str = label->getString();
                if (str.find("Attempt") != std::string::npos ||
                    str.find("attempt") != std::string::npos) {
                    label->setVisible(false);
                }
            }
        }
    }

    // Practice music hack
    void togglePracticeMode(bool practice) {
        PlayLayer::togglePracticeMode(practice);
        if (g_practiceMusicHack.load(std::memory_order_relaxed) && practice) {
            // Resume music that GD would have stopped
            auto* engine = FMODAudioEngine::get();
            if (engine) engine->resumeMusic(0);
        }
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  Hitbox drawer — GJBaseGameLayer::draw
//  Draws real bounding boxes for the player(s) and for every object on screen
//  through a CCDrawNode parented to the object layer, so the rectangles line up
//  with the game camera. Hazard objects are highlighted in red.
// ─────────────────────────────────────────────────────────────────────────────
class $modify(RickHitboxLayer, GJBaseGameLayer) {
    struct Fields {
        cocos2d::CCDrawNode* hitboxNode = nullptr;
        CCLayer* objectLayer = nullptr;
    };

    void draw() {
        GJBaseGameLayer::draw();

        if (!g_hitboxEnabled.load(std::memory_order_relaxed)) return;
        if (!m_fields->objectLayer) return;

        // Recreate the node if the object layer was rebuilt (reset, new level…)
        if (m_fields->hitboxNode && m_fields->hitboxNode->getParent() != m_fields->objectLayer) {
            m_fields->hitboxNode = nullptr;
        }
        if (!m_fields->hitboxNode) {
            m_fields->hitboxNode = cocos2d::CCDrawNode::create();
            if (!m_fields->hitboxNode) return;
            m_fields->hitboxNode->setID("rickgdps-hitbox-node");
            m_fields->objectLayer->addChild(m_fields->hitboxNode, 1000);
        }

        auto* node = m_fields->hitboxNode;
        node->clear();

        bool const solid = g_hitboxSolid.load(std::memory_order_relaxed);

        std::unordered_set<GameObject*> hazards;
        hazards.reserve(m_hazardCollisionObjects.size() * 2 + 8);
        for (auto* hazard : m_hazardCollisionObjects) {
            if (hazard) hazards.insert(hazard);
        }

        // ── Players ─────────────────────────────────────────────────────────
        drawPlayerBox(m_player1, ccColor4F{0.3f, 1.f, 0.45f, 1.f}, solid);
        drawPlayerBox(m_player2, ccColor4F{0.3f, 0.75f, 1.f, 1.f}, solid);

        // ── Objects ─────────────────────────────────────────────────────────
        auto const winSize = CCDirector::sharedDirector()->getWinSize();
        float const viewLeft = -m_fields->objectLayer->getPositionX() - 80.f;
        float const viewRight = viewLeft + winSize.width + 160.f;

        int budget = 700;
        for (auto* child : CCArrayExt<CCNode*>(m_fields->objectLayer->getChildren())) {
            if (budget <= 0) break;
            auto* object = typeinfo_cast<GameObject*>(child);
            if (!object || !object->isVisible()) continue;

            auto const& rect = object->getObjectRect();
            if (rect.size.width <= 0.f || rect.size.height <= 0.f) continue;
            if (rect.origin.x + rect.size.width < viewLeft) continue;
            if (rect.origin.x > viewRight) continue;

            bool const hazard = hazards.count(object) > 0;
            ccColor4F const border = hazard
                ? ccColor4F{1.f, 0.2f, 0.2f, 1.f}
                : ccColor4F{0.35f, 0.75f, 1.f, 0.85f};
            ccColor4F const fill = solid
                ? (hazard ? ccColor4F{1.f, 0.1f, 0.1f, 0.25f} : ccColor4F{0.2f, 0.55f, 1.f, 0.18f})
                : ccColor4F{0.f, 0.f, 0.f, 0.f};

            node->drawRect(rect, fill, 1.5f, border);
            --budget;
        }
    }

    void drawPlayerBox(PlayerObject* player, ccColor4F const& color, bool solid) {
        if (!player || !m_fields->hitboxNode || !m_fields->objectLayer) return;

        auto rect = player->boundingBox();
        if (auto* parent = player->getParent(); parent && parent != m_fields->objectLayer) {
            CCPoint const a = m_fields->objectLayer->convertToNodeSpace(
                parent->convertToWorldSpace(rect.origin)
            );
            CCPoint const b = m_fields->objectLayer->convertToNodeSpace(
                parent->convertToWorldSpace({
                    rect.origin.x + rect.size.width,
                    rect.origin.y + rect.size.height
                })
            );
            rect = cocos2d::CCRect(
                std::min(a.x, b.x), std::min(a.y, b.y),
                std::abs(b.x - a.x), std::abs(b.y - a.y)
            );
        }

        ccColor4F const fill = solid
            ? ccColor4F{color.r, color.g, color.b, 0.3f}
            : ccColor4F{0.f, 0.f, 0.f, 0.f};
        m_fields->hitboxNode->drawRect(rect, fill, 2.f, color);
    }
};


// ─────────────────────────────────────────────────────────────────────────────
//  Anti-cheat bypass — block score / percentage submission while active.
//  GD funnels completion reports through GameManager::reportPercentageForLevel;
//  while the bypass is on we swallow that report so cheated runs never reach
//  GD's scoring pipeline. Turn the setting off to submit normally again.
// ─────────────────────────────────────────────────────────────────────────────
class $modify(RickAntiCheatManager, GameManager) {
    void reportPercentageForLevel(int levelID, int percentage, bool isPlatformer) {
        if (g_anticheatBypass.load(std::memory_order_relaxed)) {
            log::info(
                "[RickGdps] Anti-cheat bypass active — blocked score report ({}% on level {})",
                percentage, levelID
            );
            return;
        }
        GameManager::reportPercentageForLevel(levelID, percentage, isPlatformer);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  $on_mod(Loaded) — bind all hack settings
// ─────────────────────────────────────────────────────────────────────────────
$on_mod(Loaded) {
    bindHackBool("noclip-enabled",       g_noclipEnabled);
    bindHackFloat("noclip-opacity",      g_noclipOpacity);
    bindHackBool("speedhack-enabled",    g_speedhackEnabled);
    bindHackFloat("speedhack-speed",     g_speedhackSpeed);
    bindHackBool("fps-bypass-enabled",   g_fpsBypassEnabled);
    bindHackFloat("fps-bypass-value",    g_fpsBypassValue);
    bindHackBool("auto-retry-enabled",   g_autoRetryEnabled);
    bindHackFloat("auto-retry-delay",    g_autoRetryDelay);
    bindHackBool("practice-music-hack",  g_practiceMusicHack);
    bindHackFloat("respawn-delay",       g_respawnDelay);
    bindHackBool("show-percentage",      g_showPercentage);
    bindHackBool("hide-attempts",        g_hideAttempts);
    bindHackBool("hide-ui-enabled",      g_hideUi);
    bindHackBool("hitbox-enabled",       g_hitboxEnabled);
    bindHackBool("hitbox-solid",         g_hitboxSolid);
    bindHackBool("trail-enabled",        g_trailEnabled);
    bindHackBool("layout-mode",          g_layoutMode);
    bindHackBool("mirror-mode",          g_mirrorMode);
    bindHackBool("anticheat-bypass",     g_anticheatBypass);
    bindHackBool("start-pos-enabled",    g_startPosEnabled);
    bindHackFloat("start-pos-percent",   g_startPosPercent);

    log::info("[RickGdps] Hack features loaded");
}

} // namespace rickgdps
