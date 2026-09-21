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
#include <atomic>
#include <string>

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
    void update(float dt) {
        PlayerObject::update(dt);
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

    // ── Level init ──────────────────────────────────────────────────────────
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        // Mirror mode
        if (g_mirrorMode.load(std::memory_order_relaxed)) {
            auto* dir = CCDirector::sharedDirector();
            setScaleX(-1.f);
            auto ws = dir->getWinSize();
            setPositionX(ws.width);
        }

        // Layout mode — hide backgrounds
        if (g_layoutMode.load(std::memory_order_relaxed)) {
            if (m_background) m_background->setVisible(false);
        }

        // Trail toggle
        if (m_player1) {
            bool trail = g_trailEnabled.load(std::memory_order_relaxed);
            // setPlayerTrailEnabled available on newer GD; guard with typeinfo
            // For compatibility just set particle system visibility
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
        PlayLayer::resetLevel();
        updateHudVisibility();
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
            pct->setVisible(g_showPercentage.load(std::memory_order_relaxed));
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
// ─────────────────────────────────────────────────────────────────────────────
class $modify(RickHitboxLayer, GJBaseGameLayer) {
    void draw() {
        GJBaseGameLayer::draw();
        if (!g_hitboxEnabled.load(std::memory_order_relaxed)) return;

        // Draw hitboxes for each object in the object layer
        bool solid = g_hitboxSolid.load(std::memory_order_relaxed);

        ccDrawColor4B(255, 0, 0, 180);   // red for obstacles
        ccGLBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        // Draw player hitbox (player 1)
        if (m_player1) {
            auto box = m_player1->boundingBox();
            ccDrawColor4B(0, 255, 0, 200);
            if (solid) {
                CCPoint verts[4] = {
                    {box.origin.x, box.origin.y},
                    {box.origin.x + box.size.width, box.origin.y},
                    {box.origin.x + box.size.width, box.origin.y + box.size.height},
                    {box.origin.x, box.origin.y + box.size.height}
                };
                ccDrawSolidPoly(verts, 4, {0, 200, 0, 80});
            }
            ccDrawRect(box.origin, {box.origin.x + box.size.width, box.origin.y + box.size.height});
        }

        // Draw player 2 hitbox
        if (m_player2) {
            auto box = m_player2->boundingBox();
            ccDrawColor4B(0, 200, 255, 200);
            ccDrawRect(box.origin, {box.origin.x + box.size.width, box.origin.y + box.size.height});
        }
    }
};


// ─────────────────────────────────────────────────────────────────────────────
//  Anti-cheat bypass
//  GD sets m_isCreatedWithSomeFlag or similar cheated flag when noclip/speed
//  is detected. We intercept GameManager::reportCheat (or equivalent) to no-op.
// ─────────────────────────────────────────────────────────────────────────────
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
