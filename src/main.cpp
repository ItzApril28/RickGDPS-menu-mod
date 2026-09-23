#include "render/BloomRenderer.hpp"
#include "render/GlStateGuard.hpp"
#include "render/PostProcessPipeline.hpp"
#include "render/PostProcessRenderer.hpp"
#include "render/SmaaRenderer.hpp"
#include "shaders/PostProcessShaders.hpp"
#include "shaders/aa/SmaaShader.hpp"

#include <Geode/Geode.hpp>
#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/fmod/fmod_dsp_effects.h>
#include <Geode/loader/Dirs.hpp>
#include <Geode/utils/file.hpp>
#include <Geode/loader/SettingV3.hpp>
#include <Geode/modify/CCDirector.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/platform/cplatform.h>
#include <Geode/ui/Popup.hpp>
#ifdef GEODE_IS_WINDOWS
    #include <Geode/modify/CCEGLView.hpp>
#endif
#ifdef GEODE_IS_MOBILE
    #include <Geode/modify/AppDelegate.hpp>
#endif
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

using namespace geode::prelude;

namespace {

    enum class AntiAliasingMethod {
        Off,
        Fxaa,
        SmaaHigh,
        SmaaUltra,
        Ssaa15x,
        Ssaa20x,
    };

    enum class UpscaleMethod {
        Nearest,
        Fsr,
    };

    enum class SplitScreenMode { Horizontal, Vertical, Quad };
    // Nature / Butter Glow / Nostalgic Bloom / Memories are appended rather than
    // inserted so the existing bloom parameter table indices keep their meaning.
    enum class BloomPreset {
        Custom, Soft, Dreamy, Neon, Sunset, Frost, Fire, Noir, Pastel, Intense,
        Nature, ButterGlow, Nostalgic, Memories
    };
    enum class EffectPreset { Custom, Cyberpunk, Retro, Cinema, Dreamscape, Comic, Noir, Arcade, Ethereal, Glitch, Rtx, Nostalgic, DeepWater };
    enum class PerformancePreset { Custom, MaxFPS, Balanced, VisualQuality, Cinematic };
    enum class DrsSensitivity { Smooth, Normal, Aggressive };

    struct PostProcessConfig {
        AntiAliasingMethod aa = AntiAliasingMethod::Off;
        bool cas = false;
        bool bloom = false;
        float bloomThreshold = 0.7f;
        float bloomIntensity = 0.3f;
        float bloomRadius = 8.f;
        bool dynamicBloom = false;
        float dynamicBloomSpeed = 1.0f;
        float dynamicBloomMin = 0.2f;
        float dynamicBloomMax = 0.8f;
        bool grayscale = false;
        bool pixelate = false;
        bool dithering = false;
        bool vhs = false;
        bool crt = false;
        bool aberration = false;
        bool neon = false;
        bool radialBlur = false;
        bool vignette = false;
        bool halftone = false;
        bool godRays = false;
        bool wobble = false;
        float wobbleSpeed = 1.0f;
        bool parallax = false;
        float parallaxDepth = 0.5f;
        bool splitScreen = false;
        SplitScreenMode splitScreenMode = SplitScreenMode::Horizontal;
        float splitScreenBorder = 1.0f;
        bool sepia = false;
        bool posterize = false;
        bool filmGrain = false;
        bool depthFocus = false;
        bool lensFlare = false;
        bool ascii = false;
        bool cinematicLut = false;
        bool ambient = false;
        bool motionBlur = false;
        bool colorEdit = false;
        bool animatedLut = false;
        float animatedLutSpeed = 0.5f;
        bool fakeHdr = false;
        bool sunset = false;
        bool customShader = false;
        // v1.4.6 lineup.
        bool butterCamera = false;
        bool raindrop = false;
        bool filmPolaroid = false;
        bool fog = false;
        bool dustFilm = false;
        bool deathWarp = false;
        float deathWarpProgress = 0.f;
        cocos2d::CCPoint deathWarpCenter = {-1.f, -1.f};
        UpscaleMethod upscaling = UpscaleMethod::Nearest;

        bool operator==(PostProcessConfig const&) const = default;

        int effectCount() const {
            return (aa != AntiAliasingMethod::Off && aa != AntiAliasingMethod::Ssaa15x && aa != AntiAliasingMethod::Ssaa20x) + cas + bloom + grayscale + pixelate +
                dithering + vhs + crt + aberration + neon + radialBlur + vignette + halftone + godRays +
                wobble + parallax + splitScreen + sepia + posterize + filmGrain + depthFocus + lensFlare + ascii + cinematicLut + ambient + motionBlur + colorEdit + fakeHdr + sunset + customShader + deathWarp +
                butterCamera + raindrop + filmPolaroid + fog + dustFilm;
        }
    };

    struct PostProcessKey {
        PostProcessConfig config;
        GLsizei width = 0;
        GLsizei height = 0;
        bool needsPresentation = false;

        bool operator==(PostProcessKey const&) const = default;
    };

    struct PostProcessFailureKey {
        PostProcessKey postProcess;
        GLuint framebuffer = 0;
        std::array<GLint, 4> viewport = {};

        bool operator==(PostProcessFailureKey const&) const = default;
    };

    std::atomic<AntiAliasingMethod> g_antiAliasingMethod = AntiAliasingMethod::SmaaHigh;
    std::atomic<UpscaleMethod> g_upscaleMethod = UpscaleMethod::Nearest;
    std::atomic<float> g_renderScale = 1.f;
    std::atomic<bool> g_drsEnabled = false;
    std::atomic<float> g_drsTargetFps = 144.f;
    std::atomic<float> g_drsCustomFps = 144.f;
    std::atomic<float> g_drsMinScale = 0.5f;
    std::atomic<float> g_drsMaxScale = 1.0f;
    std::atomic<DrsSensitivity> g_drsSensitivity = DrsSensitivity::Normal;
    float g_currentDrsScale = 1.0f;
    std::chrono::steady_clock::time_point g_lastDrsFrameTime = std::chrono::steady_clock::now();
    float g_smoothedFrametimeMs = 1000.f / 60.f;

    std::atomic<bool> g_casEnabled = false;
    std::atomic<float> g_casSharpness = 0.f;
    std::atomic<bool> g_bloomEnabled = false;
    std::atomic<float> g_bloomThreshold = 0.7f;
    std::atomic<float> g_bloomIntensity = 0.3f;
    std::atomic<float> g_bloomRadius = 8.f;
    std::atomic<bool> g_dynamicBloomEnabled = false;
    std::atomic<float> g_dynamicBloomSpeed = 1.0f;
    std::atomic<float> g_dynamicBloomMin = 0.2f;
    std::atomic<float> g_dynamicBloomMax = 0.8f;
    std::atomic<bool> g_butterCameraEnabled = false;
    std::atomic<float> g_butterCameraStrength = 1.f;
    std::atomic<bool> g_raindropEnabled = false;
    std::atomic<float> g_raindropStrength = 0.8f;
    std::atomic<bool> g_filmPolaroidEnabled = false;
    std::atomic<float> g_filmPolaroidStrength = 0.85f;
    std::atomic<bool> g_fogEnabled = false;
    std::atomic<float> g_fogStrength = 0.6f;
    std::atomic<bool> g_dustFilmEnabled = false;
    std::atomic<float> g_dustFilmStrength = 0.7f;
    std::atomic<BloomPreset> g_bloomPreset = BloomPreset::Custom;
    std::atomic<EffectPreset> g_effectPreset = EffectPreset::Custom;
    std::atomic<PerformancePreset> g_performancePreset = PerformancePreset::Custom;
    std::atomic<bool> g_grayscaleEnabled = false;
    std::atomic<bool> g_pixelateEnabled = false;
    std::atomic<bool> g_ditheringEnabled = false;
    std::atomic<bool> g_vhsEnabled = false;
    std::atomic<bool> g_crtEnabled = false;
    std::atomic<bool> g_aberrationEnabled = false;
    std::atomic<float> g_aberrationStrength = 0.005f;
    std::atomic<bool> g_neonEnabled = false;
    std::atomic<float> g_neonSpeed = 1.f;
    std::atomic<bool> g_radialBlurEnabled = false;
    std::atomic<float> g_radialBlurStrength = 0.02f;
    std::atomic<bool> g_vignetteEnabled = false;
    std::atomic<float> g_vignetteStrength = 0.4f;
    std::atomic<bool> g_halftoneEnabled = false;
    std::atomic<float> g_halftoneScale = 45.f;
    std::atomic<bool> g_godRaysEnabled = false;
    std::atomic<float> g_godRaysStrength = 0.35f;
    std::atomic<bool> g_wobbleEnabled = false;
    std::atomic<float> g_wobbleSpeed = 1.0f;
    std::atomic<bool> g_deathWarpEnabled = true;
    // Death warp animation state (atomic floats for thread-safe frame updates)
    std::atomic<float> g_deathWarpProgress{1.0f};  // 0=fresh death, 1=completed/invisible
    std::atomic<float> g_deathWarpCenterX{0.5f};
    std::atomic<float> g_deathWarpCenterY{0.5f};
    std::atomic<bool> g_parallaxEnabled = false;
    std::atomic<float> g_parallaxDepth = 0.5f;
    std::atomic<bool> g_splitScreenEnabled = false;
    std::atomic<SplitScreenMode> g_splitScreenMode = SplitScreenMode::Horizontal;
    std::atomic<float> g_splitScreenBorder = 1.0f;
    std::atomic<bool> g_sepiaEnabled = false;
    std::atomic<float> g_sepiaStrength = 1.f;
    std::atomic<bool> g_posterizeEnabled = false;
    std::atomic<float> g_posterizeLevels = 6.f;
    std::atomic<bool> g_filmGrainEnabled = false;
    std::atomic<float> g_filmGrainStrength = 0.45f;
    std::atomic<bool> g_depthFocusEnabled = false;
    std::atomic<float> g_depthFocusStrength = 0.5f;
    std::atomic<bool> g_lensFlareEnabled = false;
    std::atomic<float> g_lensFlareStrength = 0.5f;
    std::atomic<bool> g_asciiEnabled = false;
    std::atomic<float> g_asciiScale = 90.f;
    std::atomic<bool> g_cinematicLutEnabled = false;
    std::atomic<float> g_cinematicLutStrength = 1.f;
    std::atomic<bool> g_ambientEnabled = false;
    std::atomic<float> g_ambientStrength = 0.75f;
    std::atomic<bool> g_motionBlurEnabled = false;
    std::atomic<float> g_motionBlurStrength = 0.5f;
    std::atomic<bool> g_colorEditEnabled = false;
    std::atomic<float> g_colorLut = 0.f;
    std::atomic<bool> g_animatedLutEnabled = false;
    std::atomic<float> g_animatedLutSpeed = 0.5f;
    std::atomic<float> g_colorBrightness = 0.f;
    std::atomic<float> g_colorContrast = 0.f;
    std::atomic<float> g_colorSaturation = 0.f;
    std::atomic<float> g_colorTemperature = 0.f;
    std::atomic<bool> g_fakeHdrEnabled = false;
    std::atomic<float> g_fakeHdrStrength = 0.65f;
    std::atomic<bool> g_sunsetEnabled = false;
    std::atomic<float> g_sunsetStrength = 0.75f;
    std::atomic<bool> g_customShaderEnabled = false;
    std::atomic<bool> g_globalShaderEnabled = false;
    std::atomic<bool> g_shaderOptimization = false;
    std::string g_customShaderSource;
    bv::render::PostProcessShader g_customShader{
        "Custom GLSL", bv::shaders::kFullscreenVertexSource, {}, nullptr
    };
    std::atomic<float> g_audioReverb = 0.f;
    std::atomic<float> g_audioMuffle = 0.f;
    std::atomic<int> g_audioPreset = 0;
    std::atomic<int> g_audioFilter = 0;
    std::atomic<bool> g_audio8DEnabled = false;
    std::atomic<float> g_audio8DSpeed = 0.15f;
    constexpr std::array<float, 10> kAudioEqFrequencies{30.f, 60.f, 125.f, 250.f, 500.f, 1000.f, 2000.f, 4000.f, 8000.f, 16000.f};
    std::array<std::atomic<float>, kAudioEqFrequencies.size()> g_audioEqBands{};
    FMOD::DSP* g_audioReverbDsp = nullptr;
    FMOD::DSP* g_audioMuffleDsp = nullptr;
    std::array<FMOD::DSP*, kAudioEqFrequencies.size()> g_audioEqDsps{};
    FMOD::ChannelGroup* g_audioDspGroup = nullptr;
    std::atomic<bool> g_modEnabled = true;
    std::atomic<bool> g_funEnabled = true;
    std::atomic<bool> g_temporarilyDisabled = false;
    std::atomic<bool> g_disablePopupShown = false;
    std::atomic<bool> g_performancePopupShown = false;

    void loadCustomShader(std::filesystem::path const& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return;
        std::string source{
            std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()
        };
        if (source.find("void mainImage") != std::string::npos) {
            g_customShaderSource =
                "uniform sampler2D u_texture;\n"
                "uniform vec2 u_invResolution;\n"
                "varying vec2 v_texCoord;\n"
                "#define iChannel0 u_texture\n"
                "#define iResolution (vec3(1.0 / u_invResolution, 1.0))\n"
                "#define iTime 0.0\n" + source +
                "\nvoid main() { vec4 shadertoyColor; mainImage(shadertoyColor, v_texCoord / u_invResolution); gl_FragColor = shadertoyColor; }\n";
        }
        else {
            g_customShaderSource = std::move(source);
        }
        if (!g_customShaderSource.empty()) {
            g_customShader.fragmentSource = g_customShaderSource;
            log::info("Loaded custom GLSL shader from {}", path.string());
        }
    }

    void loadCustomShader() {
        loadCustomShader(geode::dirs::getModPersistentDir() / "custom.glsl");
    }
    bv::render::PostProcessPipeline g_postProcessPipeline;
    bv::render::PostProcessRenderer g_fxaaRenderer;
    bv::render::PostProcessRenderer g_renderScaleRenderer;
    bv::render::PostProcessRenderer g_fsrRenderer;
    bv::render::PostProcessRenderer g_casRenderer;
    bv::render::BloomRenderer g_bloomRenderer;
    bv::render::PostProcessRenderer g_grayscaleRenderer;
    bv::render::PostProcessRenderer g_pixelateRenderer;
    bv::render::PostProcessRenderer g_ditheringRenderer;
    bv::render::PostProcessRenderer g_vhsRenderer;
    bv::render::PostProcessRenderer g_crtRenderer;
    bv::render::PostProcessRenderer g_aberrationRenderer;
    bv::render::PostProcessRenderer g_neonRenderer;
    bv::render::PostProcessRenderer g_radialBlurRenderer;
    bv::render::PostProcessRenderer g_vignetteRenderer;
    bv::render::PostProcessRenderer g_halftoneRenderer;
    bv::render::PostProcessRenderer g_godRaysRenderer;
    bv::render::PostProcessRenderer g_wobbleRenderer;
    bv::render::PostProcessRenderer g_parallaxRenderer;
    bv::render::PostProcessRenderer g_splitScreenRenderer;
    bv::render::PostProcessRenderer g_sepiaRenderer;
    bv::render::PostProcessRenderer g_posterizeRenderer;
    bv::render::PostProcessRenderer g_filmGrainRenderer;
    bv::render::PostProcessRenderer g_depthFocusRenderer;
    bv::render::PostProcessRenderer g_lensFlareRenderer;
    bv::render::PostProcessRenderer g_asciiRenderer;
    bv::render::PostProcessRenderer g_cinematicLutRenderer;
    bv::render::PostProcessRenderer g_ambientRenderer;
    bv::render::PostProcessRenderer g_motionBlurRenderer;
    bv::render::PostProcessRenderer g_colorEditRenderer;
    bv::render::PostProcessRenderer g_fakeHdrRenderer;
    bv::render::PostProcessRenderer g_sunsetRenderer;
    bv::render::PostProcessRenderer g_butterCameraRenderer;
    bv::render::PostProcessRenderer g_raindropRenderer;
    bv::render::PostProcessRenderer g_filmPolaroidRenderer;
    bv::render::PostProcessRenderer g_fogRenderer;
    bv::render::PostProcessRenderer g_dustFilmRenderer;
    bv::render::PostProcessRenderer g_customShaderRenderer;
    bv::render::PostProcessRenderer g_deathWarpRenderer;
    bv::render::SmaaRenderer g_smaaRenderer;
    std::optional<PostProcessKey> g_preparedPostProcessKey;
    std::optional<PostProcessFailureKey> g_failedPostProcessKey;
    bool g_isGameLayerVisitActive = false;
    bool g_isSceneCaptureActive = false;
    GLsizei g_captureWidth = 0;
    GLsizei g_captureHeight = 0;

    void applyCaptureViewport() {
        if (g_isSceneCaptureActive) {
            glViewport(0, 0, g_captureWidth, g_captureHeight);
        }
    }

    void resetRenderResources() {
        g_postProcessPipeline.reset();
        g_fxaaRenderer.reset();
        g_renderScaleRenderer.reset();
        g_fsrRenderer.reset();
        g_casRenderer.reset();
        g_bloomRenderer.reset();
        g_grayscaleRenderer.reset();
        g_pixelateRenderer.reset();
        g_ditheringRenderer.reset();
        g_vhsRenderer.reset();
        g_crtRenderer.reset();
        g_aberrationRenderer.reset();
        g_neonRenderer.reset();
        g_radialBlurRenderer.reset();
        g_vignetteRenderer.reset();
        g_halftoneRenderer.reset();
        g_godRaysRenderer.reset();
        g_wobbleRenderer.reset();
        g_parallaxRenderer.reset();
        g_splitScreenRenderer.reset();
        g_sepiaRenderer.reset();
        g_posterizeRenderer.reset();
        g_filmGrainRenderer.reset();
        g_depthFocusRenderer.reset();
        g_lensFlareRenderer.reset();
        g_asciiRenderer.reset();
        g_cinematicLutRenderer.reset();
        g_ambientRenderer.reset();
        g_motionBlurRenderer.reset();
        g_colorEditRenderer.reset();
        g_fakeHdrRenderer.reset();
        g_sunsetRenderer.reset();
        g_butterCameraRenderer.reset();
        g_raindropRenderer.reset();
        g_filmPolaroidRenderer.reset();
        g_fogRenderer.reset();
        g_dustFilmRenderer.reset();
        g_customShaderRenderer.reset();
        g_deathWarpRenderer.reset();
        g_smaaRenderer.reset();
        g_preparedPostProcessKey.reset();
        g_failedPostProcessKey.reset();
    }

    void updateAntiAliasingMethod(std::string_view value) {
        if (value == "SMAA High") {
            g_antiAliasingMethod.store(AntiAliasingMethod::SmaaHigh, std::memory_order_relaxed);
            return;
        }
        if (value == "SMAA Ultra") {
            g_antiAliasingMethod.store(AntiAliasingMethod::SmaaUltra, std::memory_order_relaxed);
            return;
        }
        if (value == "FXAA") {
            g_antiAliasingMethod.store(AntiAliasingMethod::Fxaa, std::memory_order_relaxed);
            return;
        }
        if (value == "SSAA 1.5x (Super-Sampling)") {
            g_antiAliasingMethod.store(AntiAliasingMethod::Ssaa15x, std::memory_order_relaxed);
            return;
        }
        if (value == "SSAA 2.0x (Super-Sampling)") {
            g_antiAliasingMethod.store(AntiAliasingMethod::Ssaa20x, std::memory_order_relaxed);
            return;
        }

        if (value != "Off") {
            log::warn("Unknown AA method '{}', disabling AA", value);
        }
        g_antiAliasingMethod.store(AntiAliasingMethod::Off, std::memory_order_relaxed);
    }

    void updateDrsSensitivity(std::string_view value) {
        if (value == "Smooth (Slow)") {
            g_drsSensitivity.store(DrsSensitivity::Smooth, std::memory_order_relaxed);
            return;
        }
        if (value == "Aggressive (Fast)") {
            g_drsSensitivity.store(DrsSensitivity::Aggressive, std::memory_order_relaxed);
            return;
        }
        g_drsSensitivity.store(DrsSensitivity::Normal, std::memory_order_relaxed);
    }

    void updateDrsTargetFps(std::string_view value) {
        if (value == "60 FPS") {
            g_drsTargetFps.store(60.f, std::memory_order_relaxed);
            return;
        }
        if (value == "120 FPS") {
            g_drsTargetFps.store(120.f, std::memory_order_relaxed);
            return;
        }
        if (value == "144 FPS") {
            g_drsTargetFps.store(144.f, std::memory_order_relaxed);
            return;
        }
        if (value == "165 FPS") {
            g_drsTargetFps.store(165.f, std::memory_order_relaxed);
            return;
        }
        if (value == "240 FPS") {
            g_drsTargetFps.store(240.f, std::memory_order_relaxed);
            return;
        }
        if (value == "360 FPS") {
            g_drsTargetFps.store(360.f, std::memory_order_relaxed);
            return;
        }
        if (value == "Custom") {
            g_drsTargetFps.store(-1.f, std::memory_order_relaxed); // Flag to use custom FPS setting
            return;
        }
        // Screen Refresh Rate
        g_drsTargetFps.store(0.f, std::memory_order_relaxed); // 0 indicates auto/refresh rate
    }

    void updateUpscaleMethod(std::string_view value) {
        if (value == "FSR 1") {
            g_upscaleMethod.store(UpscaleMethod::Fsr, std::memory_order_relaxed);
            return;
        }

        if (value != "Nearest") {
            log::warn("Unknown upscale method '{}', using nearest neighbour", value);
        }
        g_upscaleMethod.store(UpscaleMethod::Nearest, std::memory_order_relaxed);
    }

    void updateBloomPreset(std::string_view value) {
        constexpr std::array names{
            "Custom", "Soft", "Dreamy", "Neon", "Sunset", "Frost", "Fire", "Noir", "Pastel", "Intense",
            "Nature", "Butter Glow", "Nostalgic Bloom", "Memories"
        };
        for (std::size_t i = 0; i < names.size(); ++i) if (value == names[i]) { g_bloomPreset.store(static_cast<BloomPreset>(i), std::memory_order_relaxed); return; }
        g_bloomPreset.store(BloomPreset::Custom, std::memory_order_relaxed);
    }

    void updateEffectPreset(std::string_view value) {
        constexpr std::array names{"Custom", "Cyberpunk", "Retro", "Cinema", "Dreamscape", "Comic", "Noir", "Arcade", "Ethereal", "Glitch", "RTX", "Nostalgic", "Deep Water"};
        for (std::size_t i = 0; i < names.size(); ++i) if (value == names[i]) { g_effectPreset.store(static_cast<EffectPreset>(i), std::memory_order_relaxed); return; }
        g_effectPreset.store(EffectPreset::Custom, std::memory_order_relaxed);
    }

    void updatePerformancePreset(std::string_view value) {
        constexpr std::array names{"Custom", "Max FPS", "Balanced", "Visual Quality", "Cinematic"};
        for (std::size_t i = 0; i < names.size(); ++i) if (value == names[i]) { g_performancePreset.store(static_cast<PerformancePreset>(i), std::memory_order_relaxed); return; }
        g_performancePreset.store(PerformancePreset::Custom, std::memory_order_relaxed);
    }

    void updateColorLut(std::string_view value) {
        constexpr std::array names{"Vibrant", "Teal & Orange", "Warm Film", "Cool Night", "Pastel"};
        for (std::size_t i = 0; i < names.size(); ++i) if (value == names[i]) { g_colorLut.store(static_cast<float>(i), std::memory_order_relaxed); return; }
        g_colorLut.store(0.f, std::memory_order_relaxed);
    }

    void updateAudioPreset(std::string_view value) {
        constexpr std::array names{"Custom", "GDH Reverb", "Spatial", "Cinema", "Clear", "Bass Boost", "Late Night", "Small Room", "Cathedral", "Plate"};
        for (std::size_t i = 0; i < names.size(); ++i) if (value == names[i]) { g_audioPreset.store(static_cast<int>(i), std::memory_order_relaxed); return; }
        g_audioPreset.store(0, std::memory_order_relaxed);
    }

    void updateAudioFilter(std::string_view value) {
        constexpr std::array names{
            "None", "Telephone", "Underwater", "Lo-Fi Tape", "Vintage Radio",
            "Megaphone", "Stadium", "Bedroom Studio", "Concert Hall",
            "Dark Room", "Bright & Airy", "Warm Tube", "Space Echo"
        };
        for (std::size_t i = 0; i < names.size(); ++i) {
            if (value == names[i]) {
                g_audioFilter.store(static_cast<int>(i), std::memory_order_relaxed);
                return;
            }
        }
        g_audioFilter.store(0, std::memory_order_relaxed);
    }

    void update8DAudio(float dt) {
        static float s_8dPhase = 0.0f;
        static bool s_was8dActive = false;

        bool enabled = g_audio8DEnabled.load(std::memory_order_relaxed);
        if (!enabled) {
            if (s_was8dActive) {
                s_was8dActive = false;
                s_8dPhase = 0.0f;
                auto* engine = FMODAudioEngine::get();
                if (engine && engine->m_system) {
                    FMOD::ChannelGroup* masterGroup = nullptr;
                    if (engine->m_system->getMasterChannelGroup(&masterGroup) == FMOD_OK && masterGroup) {
                        masterGroup->setPan(0.0f);
                    }
                }
            }
            return;
        }

        s_was8dActive = true;
        float speed = g_audio8DSpeed.load(std::memory_order_relaxed);
        if (speed <= 0.0f) speed = 0.15f;

        constexpr float kTwoPi = 6.28318530717958647692f;
        s_8dPhase += dt * speed * kTwoPi;
        if (s_8dPhase > kTwoPi) s_8dPhase = std::fmod(s_8dPhase, kTwoPi);

        // Sinusoidal pan shifts smoothly Left (-1.0) to Right (+1.0)
        float pan = std::sin(s_8dPhase);

        auto* engine = FMODAudioEngine::get();
        if (!engine || !engine->m_system) return;

        FMOD::ChannelGroup* masterGroup = nullptr;
        if (engine->m_system->getMasterChannelGroup(&masterGroup) != FMOD_OK || !masterGroup) return;

        masterGroup->setPan(pan);
    }

    void applyAudioEffects() {
        auto* engine = FMODAudioEngine::get();
        if (!engine || !engine->m_system) return;

        FMOD::ChannelGroup* masterGroup = nullptr;
        if (engine->m_system->getMasterChannelGroup(&masterGroup) != FMOD_OK || !masterGroup) return;
        if (!g_audioReverbDsp) {
            if (engine->m_system->createDSPByType(FMOD_DSP_TYPE_SFXREVERB, &g_audioReverbDsp) != FMOD_OK ||
                engine->m_system->createDSPByType(FMOD_DSP_TYPE_LOWPASS, &g_audioMuffleDsp) != FMOD_OK) {
                return;
            }
            for (auto& dsp : g_audioEqDsps) {
                if (engine->m_system->createDSPByType(FMOD_DSP_TYPE_PARAMEQ, &dsp) != FMOD_OK) return;
            }
        }

        // The master channel group is the final common path for music, SFX, menu audio,
        // and every other FMOD group. Attaching here makes the controls genuinely global.
        if (g_audioDspGroup != masterGroup) {
            if (g_audioDspGroup) {
                g_audioDspGroup->removeDSP(g_audioReverbDsp);
                g_audioDspGroup->removeDSP(g_audioMuffleDsp);
                for (auto* dsp : g_audioEqDsps) g_audioDspGroup->removeDSP(dsp);
            }
            if (masterGroup->addDSP(0, g_audioReverbDsp) != FMOD_OK ||
                masterGroup->addDSP(1, g_audioMuffleDsp) != FMOD_OK) return;
            for (std::size_t i = 0; i < g_audioEqDsps.size(); ++i) {
                if (masterGroup->addDSP(static_cast<int>(i + 2), g_audioEqDsps[i]) != FMOD_OK) return;
            }
            g_audioDspGroup = masterGroup;
        }

        float reverbMs = g_audioReverb.load(std::memory_order_relaxed);
        float muffle = g_audioMuffle.load(std::memory_order_relaxed);
        std::array<float, kAudioEqFrequencies.size()> gains{};
        for (std::size_t i = 0; i < gains.size(); ++i) gains[i] = g_audioEqBands[i].load(std::memory_order_relaxed);

        // Sound profile preset
        switch (g_audioPreset.load(std::memory_order_relaxed)) {
            case 1: { // GDH Reverb
                if (reverbMs < 100.f) reverbMs = 10000.f;
                reverbMs = std::clamp(reverbMs, 100.f, 20000.f);
                muffle = 0.f;
                gains = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
                g_audioReverbDsp->setParameterFloat(FMOD_DSP_SFXREVERB_DECAYTIME, reverbMs / 1000.f);
                g_audioReverbDsp->setParameterFloat(FMOD_DSP_SFXREVERB_WETLEVEL, 0.f);
                g_audioReverbDsp->setParameterFloat(FMOD_DSP_SFXREVERB_DRYLEVEL, 0.f);
                g_audioReverbDsp->setParameterFloat(FMOD_DSP_SFXREVERB_DIFFUSION, 100.f);
                g_audioReverbDsp->setParameterFloat(FMOD_DSP_SFXREVERB_DENSITY, 100.f);
                g_audioReverbDsp->setParameterFloat(FMOD_DSP_SFXREVERB_EARLYDELAY, 20.f);
                g_audioReverbDsp->setParameterFloat(FMOD_DSP_SFXREVERB_LATEDELAY, 40.f);
                g_audioReverbDsp->setParameterFloat(FMOD_DSP_SFXREVERB_HFDECAYRATIO, 50.f);
                g_audioReverbDsp->setParameterFloat(FMOD_DSP_SFXREVERB_LOWSHELFFREQUENCY, 250.f);
                g_audioReverbDsp->setBypass(false);
                g_audioMuffleDsp->setBypass(false);
                g_audioMuffleDsp->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, 22000.f);
                for (std::size_t i = 0; i < g_audioEqDsps.size(); ++i) {
                    g_audioEqDsps[i]->setParameterFloat(FMOD_DSP_PARAMEQ_CENTER, kAudioEqFrequencies[i]);
                    g_audioEqDsps[i]->setParameterFloat(FMOD_DSP_PARAMEQ_BANDWIDTH, 0.6f);
                    g_audioEqDsps[i]->setParameterFloat(FMOD_DSP_PARAMEQ_GAIN, 0.f);
                    g_audioEqDsps[i]->setBypass(true);
                }
                return;
            }
            case 2: // Spatial
                reverbMs = 1750.f; gains = {1.f, 1.f, 0.f, 0.f, -1.f, 0.f, 1.f, 2.f, 2.f, 1.f}; break;
            case 3: // Cinema
                reverbMs = 2500.f; gains = {2.f, 2.f, 1.f, 0.f, -1.f, 0.f, 1.f, 2.f, 1.f, 0.f}; break;
            case 4: // Clear
                reverbMs = 600.f; gains = {-1.f, -1.f, 0.f, 0.f, 1.f, 2.f, 3.f, 3.f, 2.f, 1.f}; break;
            case 5: // Bass Boost
                reverbMs = 900.f; gains = {4.f, 4.f, 3.f, 1.f, 0.f, -1.f, -1.f, 0.f, 1.f, 1.f}; break;
            case 6: // Late Night
                reverbMs = 1250.f; muffle = 0.25f; gains = {1.f, 1.f, 0.f, 0.f, -1.f, 0.f, 1.f, 1.f, 0.f, -2.f}; break;
            case 7: // Small Room
                reverbMs = 450.f; gains = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}; break;
            case 8: // Cathedral
                reverbMs = 4200.f; gains = {-2.f, -1.f, 0.f, 1.f, 2.f, 2.f, 1.f, 0.f, -1.f, -2.f}; break;
            case 9: // Plate
                reverbMs = 2200.f; gains = {1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 2.f, 2.f, 1.f, 0.f}; break;
            default: break;
        }

        // Apply Audio Filter preset
        switch (g_audioFilter.load(std::memory_order_relaxed)) {
            case 1: // Telephone
                muffle = std::max(muffle, 0.55f);
                gains[0] -= 6.f; gains[1] -= 6.f; gains[2] -= 4.f;
                gains[5] += 4.f; gains[6] += 5.f;
                gains[8] -= 9.f; gains[9] -= 9.f;
                break;
            case 2: // Underwater
                muffle = std::max(muffle, 0.85f);
                gains[1] += 3.f; gains[2] += 2.f;
                if (reverbMs < 1200.f) reverbMs = 1200.f;
                break;
            case 3: // Lo-Fi Tape
                muffle = std::max(muffle, 0.22f);
                gains[2] += 2.5f; gains[3] += 1.5f;
                gains[8] -= 4.f; gains[9] -= 6.f;
                if (reverbMs < 350.f) reverbMs = 350.f;
                break;
            case 4: // Vintage Radio
                muffle = std::max(muffle, 0.35f);
                gains[0] -= 6.f; gains[1] -= 5.f;
                gains[4] += 2.f; gains[5] += 4.f; gains[6] += 3.f;
                gains[8] -= 4.f; gains[9] -= 8.f;
                break;
            case 5: // Megaphone
                muffle = std::max(muffle, 0.25f);
                gains[0] -= 6.f; gains[1] -= 6.f; gains[2] -= 3.f;
                gains[5] += 6.f; gains[6] += 6.f; gains[7] += 3.f;
                gains[8] -= 6.f; gains[9] -= 6.f;
                break;
            case 6: // Stadium
                if (reverbMs < 3800.f) reverbMs = 3800.f;
                gains[1] += 3.f; gains[8] += 2.f; gains[9] += 1.f;
                break;
            case 7: // Bedroom Studio
                if (reverbMs < 450.f) reverbMs = 450.f;
                gains[2] += 1.5f; gains[3] += 1.f;
                break;
            case 8: // Concert Hall
                if (reverbMs < 2600.f) reverbMs = 2600.f;
                gains[2] += 1.5f; gains[3] += 1.f; gains[8] += 1.5f;
                break;
            case 9: // Dark Room
                muffle = std::max(muffle, 0.40f);
                gains[1] += 4.f; gains[2] += 2.f;
                gains[8] -= 6.f; gains[9] -= 9.f;
                if (reverbMs < 1800.f) reverbMs = 1800.f;
                break;
            case 10: // Bright & Airy
                gains[7] += 2.f; gains[8] += 4.f; gains[9] += 5.f;
                break;
            case 11: // Warm Tube
                gains[1] += 2.f; gains[2] += 3.f; gains[3] += 2.f;
                gains[8] -= 1.f; gains[9] -= 2.f;
                break;
            case 12: // Space Echo
                if (reverbMs < 5500.f) reverbMs = 5500.f;
                gains[7] += 2.f; gains[8] += 3.f;
                muffle = std::max(muffle, 0.1f);
                break;
            default: break;
        }

        // 8D Audio hint of reverb check
        bool is8D = g_audio8DEnabled.load(std::memory_order_relaxed);
        if (is8D && reverbMs < 500.f) {
            reverbMs = 500.f; // subtle hint of room reverb for binaural 8D effect
        }

        // Configure Reverb DSP
        if (reverbMs <= 10.f) {
            g_audioReverbDsp->setBypass(true);
        } else {
            g_audioReverbDsp->setBypass(false);
            g_audioReverbDsp->setParameterFloat(FMOD_DSP_SFXREVERB_DECAYTIME, std::max(0.1f, reverbMs / 1000.f));
            float wetLevel = (is8D && g_audioReverb.load(std::memory_order_relaxed) < 100.f) ? -16.f : -12.f;
            g_audioReverbDsp->setParameterFloat(FMOD_DSP_SFXREVERB_WETLEVEL, wetLevel);
            g_audioReverbDsp->setParameterFloat(FMOD_DSP_SFXREVERB_DRYLEVEL, 0.f);
            g_audioReverbDsp->setParameterFloat(FMOD_DSP_SFXREVERB_DIFFUSION, 85.f);
            g_audioReverbDsp->setParameterFloat(FMOD_DSP_SFXREVERB_DENSITY, 85.f);
            g_audioReverbDsp->setParameterFloat(FMOD_DSP_SFXREVERB_EARLYDELAY, 15.f);
            g_audioReverbDsp->setParameterFloat(FMOD_DSP_SFXREVERB_LATEDELAY, 30.f);
            g_audioReverbDsp->setParameterFloat(FMOD_DSP_SFXREVERB_HFDECAYRATIO, 60.f);
            g_audioReverbDsp->setParameterFloat(FMOD_DSP_SFXREVERB_LOWSHELFFREQUENCY, 250.f);
        }

        // Configure Lowpass / Muffle DSP
        if (muffle <= 0.001f) {
            g_audioMuffleDsp->setBypass(true);
        } else {
            g_audioMuffleDsp->setBypass(false);
            g_audioMuffleDsp->setParameterFloat(FMOD_DSP_LOWPASS_CUTOFF, 22000.f - muffle * 20500.f);
        }

        // Configure Parametric EQ with widened bandwidth (0.6f) and safe clamping
        for (std::size_t i = 0; i < g_audioEqDsps.size(); ++i) {
            float gain = gains[i];
            if (i == 0 || i == 1) { // 30 Hz and 60 Hz bass bands
                gain = std::clamp(gain, -6.f, 6.f);
            } else {
                gain = std::clamp(gain, -9.f, 9.f);
            }
            g_audioEqDsps[i]->setParameterFloat(FMOD_DSP_PARAMEQ_CENTER, kAudioEqFrequencies[i]);
            g_audioEqDsps[i]->setParameterFloat(FMOD_DSP_PARAMEQ_BANDWIDTH, 0.6f);
            g_audioEqDsps[i]->setParameterFloat(FMOD_DSP_PARAMEQ_GAIN, gain);
            g_audioEqDsps[i]->setBypass(std::abs(gain) < 0.01f);
        }
    }

    void bindBoolSetting(char const* name, std::atomic<bool>& value) {
        value.store(Mod::get()->getSettingValue<bool>(name), std::memory_order_relaxed);
        listenForSettingChanges<bool>(name, [&value](bool updated) {
            value.store(updated, std::memory_order_relaxed);
        });
    }

    void bindAudioBoolSetting(char const* name, std::atomic<bool>& value) {
        value.store(Mod::get()->getSettingValue<bool>(name), std::memory_order_relaxed);
        listenForSettingChanges<bool>(name, [&value](bool updated) {
            value.store(updated, std::memory_order_relaxed);
            applyAudioEffects();
        });
    }

    void bindDoubleSetting(char const* name, std::atomic<float>& value) {
        auto update = [&value](double input) {
            value.store(static_cast<float>(input), std::memory_order_relaxed);
        };
        update(Mod::get()->getSettingValue<double>(name));
        listenForSettingChanges<double>(name, [update](double input) {
            update(input);
            applyAudioEffects();
        });
    }

    void bindStringSetting(char const* name, void (*update)(std::string_view)) {
        update(Mod::get()->getSettingValue<std::string>(name));
        listenForSettingChanges<std::string>(name, [update](std::string value) {
            update(value);
            applyAudioEffects();
        });
    }

    void bindAudioEqBand(char const* name, std::size_t band) {
        auto update = [band](double input) {
            g_audioEqBands[band].store(static_cast<float>(input), std::memory_order_relaxed);
        };
        update(Mod::get()->getSettingValue<double>(name));
        listenForSettingChanges<double>(name, [update](double input) {
            update(input);
            applyAudioEffects();
        });
    }

    GLsizei scaledDimension(GLsizei output, float scale) {
        return static_cast<GLsizei>(
            std::max<long>(1, std::lround(static_cast<double>(output) * scale))
        );
    }

    std::array<GLint, 4> scaledScissorBox(
        std::array<GLint, 4> const& box, std::array<GLint, 4> const& viewport, GLsizei width,
        GLsizei height
    ) {
        auto mapStart = [](GLint value, GLint origin, GLsizei internalSize, GLsizei outputSize) {
            return static_cast<GLint>(
                std::floor(static_cast<double>(value - origin) * internalSize / outputSize)
            );
        };
        auto mapEnd = [](GLint value, GLint origin, GLsizei internalSize, GLsizei outputSize) {
            return static_cast<GLint>(
                std::ceil(static_cast<double>(value - origin) * internalSize / outputSize)
            );
        };

        auto const left = mapStart(box[0], viewport[0], width, viewport[2]);
        auto const bottom = mapStart(box[1], viewport[1], height, viewport[3]);
        auto const right = mapEnd(box[0] + box[2], viewport[0], width, viewport[2]);
        auto const top = mapEnd(box[1] + box[3], viewport[1], height, viewport[3]);
        return {
            left,
            bottom,
            std::max<GLint>(0, right - left),
            std::max<GLint>(0, top - bottom),
        };
    }

    void updateSplitScreenMode(std::string_view value) {
        if (value == "Vertical (Left / Right)") {
            g_splitScreenMode.store(SplitScreenMode::Vertical, std::memory_order_relaxed);
            return;
        }
        if (value == "Quad (2x2)") {
            g_splitScreenMode.store(SplitScreenMode::Quad, std::memory_order_relaxed);
            return;
        }
        g_splitScreenMode.store(SplitScreenMode::Horizontal, std::memory_order_relaxed);
    }

    PostProcessConfig selectedPostProcessConfig() {
        bool const fun = g_funEnabled.load(std::memory_order_relaxed);
        auto config = PostProcessConfig{
            g_antiAliasingMethod.load(std::memory_order_relaxed),
            g_casEnabled.load(std::memory_order_relaxed),
            fun && g_bloomEnabled.load(std::memory_order_relaxed),
            g_bloomThreshold.load(std::memory_order_relaxed),
            g_bloomIntensity.load(std::memory_order_relaxed),
            g_bloomRadius.load(std::memory_order_relaxed),
            fun && g_dynamicBloomEnabled.load(std::memory_order_relaxed),
            g_dynamicBloomSpeed.load(std::memory_order_relaxed),
            g_dynamicBloomMin.load(std::memory_order_relaxed),
            g_dynamicBloomMax.load(std::memory_order_relaxed),
            fun && g_grayscaleEnabled.load(std::memory_order_relaxed),
            fun && g_pixelateEnabled.load(std::memory_order_relaxed),
            fun && g_ditheringEnabled.load(std::memory_order_relaxed),
            fun && g_vhsEnabled.load(std::memory_order_relaxed),
            fun && g_crtEnabled.load(std::memory_order_relaxed),
            fun && g_aberrationEnabled.load(std::memory_order_relaxed),
            fun && g_neonEnabled.load(std::memory_order_relaxed),
            fun && g_radialBlurEnabled.load(std::memory_order_relaxed),
            fun && g_vignetteEnabled.load(std::memory_order_relaxed),
            fun && g_halftoneEnabled.load(std::memory_order_relaxed),
            fun && g_godRaysEnabled.load(std::memory_order_relaxed),
            fun && g_wobbleEnabled.load(std::memory_order_relaxed),
            g_wobbleSpeed.load(std::memory_order_relaxed),
            fun && g_parallaxEnabled.load(std::memory_order_relaxed),
            g_parallaxDepth.load(std::memory_order_relaxed),
            fun && g_splitScreenEnabled.load(std::memory_order_relaxed),
            g_splitScreenMode.load(std::memory_order_relaxed),
            g_splitScreenBorder.load(std::memory_order_relaxed),
            fun && g_sepiaEnabled.load(std::memory_order_relaxed),
            fun && g_posterizeEnabled.load(std::memory_order_relaxed),
            fun && g_filmGrainEnabled.load(std::memory_order_relaxed),
            fun && g_depthFocusEnabled.load(std::memory_order_relaxed),
            fun && g_lensFlareEnabled.load(std::memory_order_relaxed),
            fun && g_asciiEnabled.load(std::memory_order_relaxed),
            fun && g_cinematicLutEnabled.load(std::memory_order_relaxed),
            fun && g_ambientEnabled.load(std::memory_order_relaxed),
            fun && g_motionBlurEnabled.load(std::memory_order_relaxed),
            fun && g_colorEditEnabled.load(std::memory_order_relaxed),
            fun && g_animatedLutEnabled.load(std::memory_order_relaxed),
            g_animatedLutSpeed.load(std::memory_order_relaxed),
            fun && g_fakeHdrEnabled.load(std::memory_order_relaxed),
            fun && g_sunsetEnabled.load(std::memory_order_relaxed),
            fun && g_customShaderEnabled.load(std::memory_order_relaxed) && !g_customShaderSource.empty(),
            // v1.4.6 lineup. These must stay in struct field order: the config is
            // built with positional aggregate initialisation, so omitting them
            // would silently shift every field that follows.
            fun && g_butterCameraEnabled.load(std::memory_order_relaxed),
            fun && g_raindropEnabled.load(std::memory_order_relaxed),
            fun && g_filmPolaroidEnabled.load(std::memory_order_relaxed),
            fun && g_fogEnabled.load(std::memory_order_relaxed),
            fun && g_dustFilmEnabled.load(std::memory_order_relaxed),
            // Death warp: active while progress < 1.0 and setting enabled
            g_deathWarpEnabled.load(std::memory_order_relaxed) && g_deathWarpProgress.load(std::memory_order_relaxed) < 0.999f,
            g_deathWarpProgress.load(std::memory_order_relaxed),
            {g_deathWarpCenterX.load(std::memory_order_relaxed), g_deathWarpCenterY.load(std::memory_order_relaxed)},
            g_upscaleMethod.load(std::memory_order_relaxed),
        };
        // {threshold, intensity, radius}. The last four are the v1.4.6 presets:
        // Nature (outdoor diffusion), Butter Glow (warm amber, pairs with the
        // Butter Camera shader), Nostalgic Bloom (light bleeding) and Memories
        // (muted and vintage).
        constexpr std::array bloomParams{
            std::array{0.7f, 0.3f, 8.f}, std::array{0.75f, 0.2f, 6.f}, std::array{0.55f, 0.38f, 11.f},
            std::array{0.48f, 0.65f, 9.f}, std::array{0.62f, 0.42f, 10.f}, std::array{0.72f, 0.3f, 7.f},
            std::array{0.5f, 0.7f, 12.f}, std::array{0.8f, 0.18f, 5.f}, std::array{0.6f, 0.32f, 9.f}, std::array{0.35f, 1.1f, 15.f},
            std::array{0.66f, 0.26f, 7.f}, std::array{0.58f, 0.36f, 9.f},
            std::array{0.70f, 0.32f, 13.f}, std::array{0.74f, 0.24f, 8.f},
        };
        auto const bloomIndex = static_cast<std::size_t>(g_bloomPreset.load(std::memory_order_relaxed));
        if (bloomIndex != 0) { config.bloom = fun; config.bloomThreshold = bloomParams[bloomIndex][0]; config.bloomIntensity = bloomParams[bloomIndex][1]; config.bloomRadius = bloomParams[bloomIndex][2]; }
        if (fun) switch (g_effectPreset.load(std::memory_order_relaxed)) {
            case EffectPreset::Cyberpunk: config.neon = config.aberration = config.cinematicLut = true; break;
            case EffectPreset::Retro: config.crt = config.vhs = config.filmGrain = true; break;
            case EffectPreset::Cinema: config.cinematicLut = config.lensFlare = config.depthFocus = true; break;
            case EffectPreset::Dreamscape: config.bloom = config.wobble = config.ambient = true; break;
            case EffectPreset::Comic: config.halftone = config.posterize = config.aberration = true; break;
            case EffectPreset::Noir: config.grayscale = config.filmGrain = config.vignette = true; break;
            case EffectPreset::Arcade: config.pixelate = config.neon = config.posterize = true; break;
            case EffectPreset::Ethereal: config.bloom = config.godRays = config.ambient = true; break;
            case EffectPreset::Glitch: config.aberration = config.wobble = config.vhs = true; break;
            case EffectPreset::Rtx: config.cinematicLut = config.godRays = config.lensFlare = config.depthFocus = true; break;
            case EffectPreset::Nostalgic: config.sepia = config.filmGrain = config.vhs = config.vignette = true; break;
            case EffectPreset::DeepWater: config.ambient = config.wobble = config.radialBlur = config.cinematicLut = true; break;
            case EffectPreset::Custom: break;
        }
        switch (g_performancePreset.load(std::memory_order_relaxed)) {
            case PerformancePreset::MaxFPS: config.aa = AntiAliasingMethod::Off; config.cas = false; config.upscaling = UpscaleMethod::Nearest; break;
            case PerformancePreset::Balanced: config.aa = AntiAliasingMethod::Fxaa; config.cas = false; config.upscaling = UpscaleMethod::Nearest; break;
            case PerformancePreset::VisualQuality: config.aa = AntiAliasingMethod::SmaaHigh; config.cas = true; config.upscaling = UpscaleMethod::Fsr; break;
            case PerformancePreset::Cinematic: config.aa = AntiAliasingMethod::SmaaUltra; config.cas = true; config.upscaling = UpscaleMethod::Fsr; break;
            case PerformancePreset::Custom: break;
        }
        if (g_shaderOptimization.load(std::memory_order_relaxed)) {
            // Keep the expensive multi-sample passes off while preserving the
            // lightweight color and grading effects.
            config.bloom = false;
            config.godRays = false;
            config.radialBlur = false;
            config.motionBlur = false;
            config.depthFocus = false;
            config.lensFlare = false;
        }
        return config;
    }

    bool preparePostProcess(PostProcessKey const& key) {
        if (g_preparedPostProcessKey == key) {
            return true;
        }

        bv::render::GlStateGuard prepareState;
        auto const& config = key.config;
        bool prepared = g_postProcessPipeline.prepare(key.width, key.height);
        switch (config.aa) {
            case AntiAliasingMethod::Fxaa:
                g_smaaRenderer.reset();
                prepared = prepared &&
                    g_fxaaRenderer.prepare(bv::shaders::kFxaaShader, key.width, key.height);
                break;
            case AntiAliasingMethod::SmaaHigh:
                g_fxaaRenderer.reset();
                prepared = prepared &&
                    g_smaaRenderer.prepare(
                        bv::shaders::smaa::kSmaaHighShaderSet, key.width, key.height
                    );
                break;
            case AntiAliasingMethod::SmaaUltra:
                g_fxaaRenderer.reset();
                prepared = prepared &&
                    g_smaaRenderer.prepare(
                        bv::shaders::smaa::kSmaaUltraShaderSet, key.width, key.height
                    );
                break;
            case AntiAliasingMethod::Off:
            case AntiAliasingMethod::Ssaa15x:
            case AntiAliasingMethod::Ssaa20x:
                g_fxaaRenderer.reset();
                g_smaaRenderer.reset();
                break;
        }
        if (prepared && config.cas) {
            prepared = g_casRenderer.prepare(bv::shaders::kCasShader, key.width, key.height);
        }
        else if (!config.cas) {
            g_casRenderer.reset();
        }
        if (prepared && config.bloom) {
            prepared = g_bloomRenderer.prepare(key.width, key.height);
            if (prepared) {
                g_bloomRenderer.setParams(
                    config.bloomThreshold, config.bloomIntensity, config.bloomRadius
                );
            }
        }
        else if (!config.bloom) {
            g_bloomRenderer.reset();
        }

        struct Effect {
            bool enabled;
            bv::render::PostProcessRenderer* renderer;
            bv::render::PostProcessShader const* shader;
        };

        // The size is deduced from the initialiser list rather than hardcoded;
        // a fixed count here silently breaks the build every time a pass is
        // added, which is how the v1.4.6 shaders first failed to compile.
        auto const effects = std::to_array<Effect>({
            {config.grayscale, &g_grayscaleRenderer, &bv::shaders::kGrayscaleShader},
            {config.pixelate, &g_pixelateRenderer, &bv::shaders::kPixelateShader},
            {config.dithering, &g_ditheringRenderer, &bv::shaders::kDitheringShader},
            {config.vhs, &g_vhsRenderer, &bv::shaders::kVhsShader},
            {config.crt, &g_crtRenderer, &bv::shaders::kCrtShader},
            {config.aberration, &g_aberrationRenderer, &bv::shaders::kAberrationShader},
            {config.neon, &g_neonRenderer, &bv::shaders::kNeonShader},
            {config.radialBlur, &g_radialBlurRenderer, &bv::shaders::kRadialBlurShader},
            {config.vignette, &g_vignetteRenderer, &bv::shaders::kVignetteShader},
            {config.halftone, &g_halftoneRenderer, &bv::shaders::kHalftoneShader},
            {config.godRays, &g_godRaysRenderer, &bv::shaders::kGodRaysShader},
            {config.wobble, &g_wobbleRenderer, &bv::shaders::kWobbleShader},
            {config.parallax, &g_parallaxRenderer, &bv::shaders::kParallaxShader},
            {config.splitScreen, &g_splitScreenRenderer, &bv::shaders::kSplitScreenShader},
            {config.sepia, &g_sepiaRenderer, &bv::shaders::kSepiaShader},
            {config.posterize, &g_posterizeRenderer, &bv::shaders::kPosterizeShader},
            {config.filmGrain, &g_filmGrainRenderer, &bv::shaders::kFilmGrainShader},
            {config.depthFocus, &g_depthFocusRenderer, &bv::shaders::kDepthFocusShader},
            {config.lensFlare, &g_lensFlareRenderer, &bv::shaders::kLensFlareShader},
            {config.ascii, &g_asciiRenderer, &bv::shaders::kAsciiShader},
            {config.cinematicLut, &g_cinematicLutRenderer, &bv::shaders::kCinematicLutShader},
            {config.ambient, &g_ambientRenderer, &bv::shaders::kAmbientShader},
            {config.motionBlur, &g_motionBlurRenderer, &bv::shaders::kMotionBlurShader},
            {config.colorEdit, &g_colorEditRenderer, &bv::shaders::kColorGradeShader},
            {config.fakeHdr, &g_fakeHdrRenderer, &bv::shaders::kFakeHdrShader},
            {config.sunset, &g_sunsetRenderer, &bv::shaders::kSunsetShader},
            {config.butterCamera, &g_butterCameraRenderer, &bv::shaders::kButterCameraShader},
            {config.raindrop, &g_raindropRenderer, &bv::shaders::kRaindropShader},
            {config.filmPolaroid, &g_filmPolaroidRenderer, &bv::shaders::kFilmPolaroidShader},
            {config.fog, &g_fogRenderer, &bv::shaders::kFogShader},
            {config.dustFilm, &g_dustFilmRenderer, &bv::shaders::kDustFilmShader},
            {config.customShader, &g_customShaderRenderer, &g_customShader},
            {config.deathWarp, &g_deathWarpRenderer, &bv::shaders::kDeathWarpShader},
        });
        for (auto const& effect : effects) {
            if (prepared && effect.enabled) {
                prepared = effect.renderer->prepare(*effect.shader, key.width, key.height);
            }
            else if (!effect.enabled) {
                effect.renderer->reset();
            }
        }

        if (prepared && key.needsPresentation) {
            if (config.upscaling == UpscaleMethod::Fsr) {
                g_renderScaleRenderer.reset();
                prepared = g_fsrRenderer.prepare(bv::shaders::kFsrShader, key.width, key.height);
            }
            else {
                g_fsrRenderer.reset();
                prepared = g_renderScaleRenderer.prepare(
                    bv::shaders::kRenderScaleShader, key.width, key.height
                );
            }
        }
        else if (!key.needsPresentation) {
            g_renderScaleRenderer.reset();
            g_fsrRenderer.reset();
        }

        if (prepared) {
            g_preparedPostProcessKey = key;
        }
        return prepared;
    }

    void renderEffects(
        PostProcessConfig const& config, GLfloat casSharpness,
        bv::render::RenderTarget const& frameTarget, bool needsPresentation
    ) {
        auto remaining = config.effectCount();
        assert(remaining > 0);

        auto runStage = [&](auto&& render) {
            auto const terminal = --remaining == 0;
            auto const writeToCaller = terminal && !needsPresentation;
            auto const target = writeToCaller ? frameTarget : g_postProcessPipeline.nextTarget();
            render(target);
            if (!writeToCaller) {
                g_postProcessPipeline.advanceStage();
            }
        };
        auto runPost = [&](bv::render::PostProcessRenderer& renderer, GLfloat scalar = 0.f) {
            runStage([&](bv::render::RenderTarget const& target) {
                glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
                glViewport(target.x, target.y, target.width, target.height);
                renderer.apply(g_postProcessPipeline.currentTexture(), scalar);
            });
        };

        switch (config.aa) {
            case AntiAliasingMethod::Fxaa: runPost(g_fxaaRenderer); break;
            case AntiAliasingMethod::SmaaHigh:
            case AntiAliasingMethod::SmaaUltra:
                runStage([&](bv::render::RenderTarget const& target) {
                    g_smaaRenderer.apply(g_postProcessPipeline.currentTexture(), target);
                });
                break;
            case AntiAliasingMethod::Off:
            case AntiAliasingMethod::Ssaa15x:
            case AntiAliasingMethod::Ssaa20x:
                break;
        }
        if (config.cas) {
            runPost(g_casRenderer, casSharpness);
        }
        if (config.bloom) {
            float bloomThreshold = config.bloomThreshold;
            float bloomIntensity = config.bloomIntensity;
            if (config.dynamicBloom) {
                bloomIntensity = std::clamp(bloomIntensity, config.dynamicBloomMin, config.dynamicBloomMax);
            }
            g_bloomRenderer.setParams(bloomThreshold, bloomIntensity, config.bloomRadius, config.dynamicBloom);
            runStage([&](bv::render::RenderTarget const& target) {
                g_bloomRenderer.apply(g_postProcessPipeline.currentTexture(), target);
            });
        }
        if (config.grayscale) {
            runPost(g_grayscaleRenderer);
        }
        if (config.pixelate) {
            runPost(g_pixelateRenderer);
        }
        if (config.dithering) {
            runPost(g_ditheringRenderer);
        }
        if (config.vhs) {
            static auto const clockStart = std::chrono::steady_clock::now();
            auto const elapsed =
                std::chrono::duration<GLfloat>(std::chrono::steady_clock::now() - clockStart).count();
            runPost(g_vhsRenderer, elapsed);
        }
        if (config.crt) {
            runPost(g_crtRenderer);
        }
        if (config.aberration) {
            runPost(g_aberrationRenderer, g_aberrationStrength.load(std::memory_order_relaxed));
        }
        if (config.neon) {
            static auto const clockStart = std::chrono::steady_clock::now();
            auto const elapsed =
                std::chrono::duration<GLfloat>(std::chrono::steady_clock::now() - clockStart).count();
            runPost(g_neonRenderer, elapsed * g_neonSpeed.load(std::memory_order_relaxed));
        }
        if (config.radialBlur) {
            runPost(g_radialBlurRenderer, g_radialBlurStrength.load(std::memory_order_relaxed));
        }
        if (config.vignette) {
            runPost(g_vignetteRenderer, g_vignetteStrength.load(std::memory_order_relaxed));
        }
        if (config.halftone) {
            runPost(g_halftoneRenderer, g_halftoneScale.load(std::memory_order_relaxed));
        }
        if (config.godRays) {
            runPost(g_godRaysRenderer, g_godRaysStrength.load(std::memory_order_relaxed));
        }
        if (config.wobble) {
            static auto const clockStart = std::chrono::steady_clock::now();
            auto const elapsed =
                std::chrono::duration<GLfloat>(std::chrono::steady_clock::now() - clockStart).count();
            runPost(g_wobbleRenderer, elapsed * config.wobbleSpeed);
        }
        if (config.parallax) {
            static auto const clockStart = std::chrono::steady_clock::now();
            auto const elapsed =
                std::chrono::duration<GLfloat>(std::chrono::steady_clock::now() - clockStart).count();
            runStage([&](bv::render::RenderTarget const& target) {
                glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
                glViewport(target.x, target.y, target.width, target.height);
                g_parallaxRenderer.apply(
                    g_postProcessPipeline.currentTexture(), config.parallaxDepth, elapsed
                );
            });
        }
        if (config.splitScreen) {
            float modeIdx = 0.f;
            if (config.splitScreenMode == SplitScreenMode::Vertical) modeIdx = 1.f;
            else if (config.splitScreenMode == SplitScreenMode::Quad) modeIdx = 2.f;
            runStage([&](bv::render::RenderTarget const& target) {
                glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
                glViewport(target.x, target.y, target.width, target.height);
                g_splitScreenRenderer.apply(
                    g_postProcessPipeline.currentTexture(), modeIdx, config.splitScreenBorder
                );
            });
        }
        if (config.sepia) {
            runPost(g_sepiaRenderer, g_sepiaStrength.load(std::memory_order_relaxed));
        }
        if (config.posterize) {
            runPost(g_posterizeRenderer, g_posterizeLevels.load(std::memory_order_relaxed));
        }
        if (config.filmGrain) {
            static auto const clockStart = std::chrono::steady_clock::now();
            auto const elapsed =
                std::chrono::duration<GLfloat>(std::chrono::steady_clock::now() - clockStart).count();
            auto const strength = g_filmGrainStrength.load(std::memory_order_relaxed);
            runStage([&](bv::render::RenderTarget const& target) {
                glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
                glViewport(target.x, target.y, target.width, target.height);
                g_filmGrainRenderer.apply(
                    g_postProcessPipeline.currentTexture(), elapsed, strength
                );
            });
        }
        if (config.depthFocus) runPost(g_depthFocusRenderer, g_depthFocusStrength.load(std::memory_order_relaxed));
        if (config.lensFlare) runPost(g_lensFlareRenderer, g_lensFlareStrength.load(std::memory_order_relaxed));
        if (config.ascii) runPost(g_asciiRenderer, g_asciiScale.load(std::memory_order_relaxed));
        if (config.cinematicLut) runPost(g_cinematicLutRenderer, g_cinematicLutStrength.load(std::memory_order_relaxed));
        if (config.ambient) {
            static auto const clockStart = std::chrono::steady_clock::now();
            auto const elapsed = std::chrono::duration<GLfloat>(std::chrono::steady_clock::now() - clockStart).count();
            auto const strength = g_ambientStrength.load(std::memory_order_relaxed);
            runStage([&](bv::render::RenderTarget const& target) {
                glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
                glViewport(target.x, target.y, target.width, target.height);
                g_ambientRenderer.apply(
                    g_postProcessPipeline.currentTexture(), elapsed, strength
                );
            });
        }
        if (config.motionBlur) runPost(g_motionBlurRenderer, g_motionBlurStrength.load(std::memory_order_relaxed));
        if (config.colorEdit) {
            float lutValue = g_colorLut.load(std::memory_order_relaxed);
            if (config.animatedLut) {
                static auto const clockStart = std::chrono::steady_clock::now();
                auto const elapsed =
                    std::chrono::duration<GLfloat>(std::chrono::steady_clock::now() - clockStart).count();
                lutValue = std::fmod(elapsed * config.animatedLutSpeed * 0.5f, 5.0f);
            }
            runStage([&](bv::render::RenderTarget const& target) {
                glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
                glViewport(target.x, target.y, target.width, target.height);
                g_colorEditRenderer.apply(
                    g_postProcessPipeline.currentTexture(),
                    lutValue,
                    g_colorBrightness.load(std::memory_order_relaxed),
                    g_colorContrast.load(std::memory_order_relaxed),
                    g_colorSaturation.load(std::memory_order_relaxed),
                    g_colorTemperature.load(std::memory_order_relaxed)
                );
            });
        }
        if (config.fakeHdr) runPost(g_fakeHdrRenderer, g_fakeHdrStrength.load(std::memory_order_relaxed));
        if (config.sunset) runPost(g_sunsetRenderer, g_sunsetStrength.load(std::memory_order_relaxed));
        // v1.4.6 camera-glass passes. The Butter Camera grade runs after the
        // colour work so it grades the finished image; the animated lens effects
        // share one clock so their motion stays in step with each other.
        if (config.butterCamera) runPost(g_butterCameraRenderer, g_butterCameraStrength.load(std::memory_order_relaxed));
        if (config.raindrop || config.filmPolaroid || config.fog || config.dustFilm) {
            static auto const glassClockStart = std::chrono::steady_clock::now();
            auto const glassElapsed =
                std::chrono::duration<GLfloat>(std::chrono::steady_clock::now() - glassClockStart).count();
            auto runTimed = [&](bv::render::PostProcessRenderer& renderer, GLfloat strength) {
                runStage([&](bv::render::RenderTarget const& target) {
                    glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
                    glViewport(target.x, target.y, target.width, target.height);
                    renderer.apply(g_postProcessPipeline.currentTexture(), glassElapsed, strength);
                });
            };
            if (config.raindrop) runTimed(g_raindropRenderer, g_raindropStrength.load(std::memory_order_relaxed));
            if (config.filmPolaroid) runTimed(g_filmPolaroidRenderer, g_filmPolaroidStrength.load(std::memory_order_relaxed));
            if (config.fog) runTimed(g_fogRenderer, g_fogStrength.load(std::memory_order_relaxed));
            if (config.dustFilm) runTimed(g_dustFilmRenderer, g_dustFilmStrength.load(std::memory_order_relaxed));
        }
        if (config.customShader) {
            runPost(g_customShaderRenderer);
        }
        // Death warp shockwave: rendered last so it overlays all other effects
        if (config.deathWarp) {
            runStage([&](bv::render::RenderTarget const& target) {
                glBindFramebuffer(GL_FRAMEBUFFER, target.framebuffer);
                glViewport(target.x, target.y, target.width, target.height);
                g_deathWarpRenderer.apply(
                    g_postProcessPipeline.currentTexture(),
                    config.deathWarpProgress,
                    config.deathWarpCenter.x,
                    config.deathWarpCenter.y
                );
            });
        }
    }

    void renderSceneWithPostProcessing(auto&& visitNext) {
        auto const config = selectedPostProcessConfig();
        GLint callerFramebuffer = 0;
        std::array<GLint, 4> callerViewport = {};
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &callerFramebuffer);
        glGetIntegerv(GL_VIEWPORT, callerViewport.data());
        auto const width = static_cast<GLsizei>(callerViewport[2]);
        auto const height = static_cast<GLsizei>(callerViewport[3]);
        if (width <= 0 || height <= 0) {
            visitNext();
            return;
        }

        auto const effectCount = config.effectCount();
        auto renderScale = g_renderScale.load(std::memory_order_relaxed);

        if (config.aa == AntiAliasingMethod::Ssaa15x) {
            renderScale = 1.5f;
        }
        else if (config.aa == AntiAliasingMethod::Ssaa20x) {
            renderScale = 2.0f;
        }
        else {
            switch (g_performancePreset.load(std::memory_order_relaxed)) {
                case PerformancePreset::MaxFPS: renderScale = 0.5f; break;
                case PerformancePreset::Balanced: renderScale = 0.75f; break;
                case PerformancePreset::VisualQuality: renderScale = 1.0f; break;
                case PerformancePreset::Cinematic: renderScale = 1.0f; break;
                case PerformancePreset::Custom: break;
            }
        }

        if (g_drsEnabled.load(std::memory_order_relaxed) &&
            config.aa != AntiAliasingMethod::Ssaa15x && config.aa != AntiAliasingMethod::Ssaa20x) {
            auto const now = std::chrono::steady_clock::now();
            float dtMs = std::chrono::duration<float, std::milli>(now - g_lastDrsFrameTime).count();
            g_lastDrsFrameTime = now;

            if (dtMs > 0.1f && dtMs < 200.f) {
                float targetFps = g_drsTargetFps.load(std::memory_order_relaxed);
                if (targetFps < 0.f) {
                    targetFps = g_drsCustomFps.load(std::memory_order_relaxed);
                }
                else if (targetFps == 0.f) {
                    float refreshRate = 60.f;
                    if (auto* director = CCDirector::sharedDirector()) {
                        float interval = director->getAnimationInterval();
                        if (interval > 0.0001f) {
                            refreshRate = 1.0f / interval;
                        }
                    }
                    targetFps = std::clamp(refreshRate, 30.f, 1000.f);
                }
                float targetDtMs = 1000.0f / std::max(targetFps, 1.0f);

                float alpha = 0.10f;
                switch (g_drsSensitivity.load(std::memory_order_relaxed)) {
                    case DrsSensitivity::Smooth: alpha = 0.04f; break;
                    case DrsSensitivity::Normal: alpha = 0.10f; break;
                    case DrsSensitivity::Aggressive: alpha = 0.22f; break;
                }

                g_smoothedFrametimeMs = g_smoothedFrametimeMs * (1.0f - alpha) + dtMs * alpha;

                float minScale = g_drsMinScale.load(std::memory_order_relaxed);
                float maxScale = g_drsMaxScale.load(std::memory_order_relaxed);
                if (minScale > maxScale) std::swap(minScale, maxScale);

                if (g_smoothedFrametimeMs > targetDtMs * 1.05f) {
                    float step = (g_smoothedFrametimeMs - targetDtMs) / targetDtMs * 0.05f;
                    g_currentDrsScale = std::max(minScale, g_currentDrsScale - std::max(0.01f, step));
                }
                else if (g_smoothedFrametimeMs < targetDtMs * 0.90f) {
                    g_currentDrsScale = std::min(maxScale, g_currentDrsScale + 0.01f);
                }

                renderScale = g_currentDrsScale;
            }
        }
        else {
            g_currentDrsScale = renderScale;
        }

        auto const internalWidth = scaledDimension(width, renderScale);
        auto const internalHeight = scaledDimension(height, renderScale);
        auto const needsPresentation = internalWidth != width || internalHeight != height;
        if (effectCount == 0 && !needsPresentation) {
            if (g_preparedPostProcessKey || g_failedPostProcessKey) {
                resetRenderResources();
            }
            visitNext();
            return;
        }

        auto const scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
        std::array<GLint, 4> callerScissor = {};
        glGetIntegerv(GL_SCISSOR_BOX, callerScissor.data());

        PostProcessKey const key{config, internalWidth, internalHeight, needsPresentation};
        PostProcessFailureKey const failureKey{
            key,
            static_cast<GLuint>(callerFramebuffer),
            callerViewport,
        };

        if (g_failedPostProcessKey && *g_failedPostProcessKey == failureKey) {
            visitNext();
            return;
        }
        if (!preparePostProcess(key)) {
            resetRenderResources();
            g_failedPostProcessKey = failureKey;
            log::warn("Post-processing disabled for this configuration");
            visitNext();
            return;
        }

        bv::render::RenderTarget const callerTarget{
            static_cast<GLuint>(callerFramebuffer),
            callerViewport[0],
            callerViewport[1],
            width,
            height,
        };

        g_postProcessPipeline.beginSceneCapture();
        if (scissorEnabled == GL_TRUE) {
            auto const scissor =
                scaledScissorBox(callerScissor, callerViewport, internalWidth, internalHeight);
            glScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
        }
        g_isSceneCaptureActive = true;
        g_captureWidth = internalWidth;
        g_captureHeight = internalHeight;
        visitNext();
        g_isSceneCaptureActive = false;

        {
            bv::render::GlStateGuard postSceneState;
            glDisable(GL_BLEND);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_STENCIL_TEST);
            glDisable(GL_SCISSOR_TEST);
            glDisable(GL_CULL_FACE);
            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            g_postProcessPipeline.bindQuad();

            if (effectCount > 0) {
                renderEffects(
                    config,
                    config.cas ? g_casSharpness.load(std::memory_order_relaxed) : 0.f,
                    callerTarget,
                    needsPresentation
                );
            }

            if (needsPresentation) {
                auto const sourceTexture = g_postProcessPipeline.currentTexture();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, sourceTexture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glBindFramebuffer(GL_FRAMEBUFFER, callerTarget.framebuffer);
                glViewport(callerTarget.x, callerTarget.y, callerTarget.width, callerTarget.height);
                if (config.upscaling == UpscaleMethod::Fsr) {
                    g_fsrRenderer.apply(sourceTexture);
                }
                else {
                    g_renderScaleRenderer.apply(sourceTexture);
                }
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }
        }
        glBindFramebuffer(GL_FRAMEBUFFER, callerTarget.framebuffer);
        glViewport(callerTarget.x, callerTarget.y, callerTarget.width, callerTarget.height);
        g_failedPostProcessKey.reset();
    }

} // namespace

$on_mod(Loaded) {
    bindBoolSetting("enabled", g_modEnabled);
    bindBoolSetting("enable-fun", g_funEnabled);
    bindBoolSetting("drs-enabled", g_drsEnabled);
    bindStringSetting("drs-target-fps", updateDrsTargetFps);
    bindDoubleSetting("drs-custom-fps", g_drsCustomFps);
    bindDoubleSetting("drs-min-scale", g_drsMinScale);
    bindDoubleSetting("drs-max-scale", g_drsMaxScale);
    bindStringSetting("drs-sensitivity", updateDrsSensitivity);
    bindDoubleSetting("render-scale", g_renderScale);
    bindStringSetting("upscale-method", updateUpscaleMethod);
    bindStringSetting("aa-method", updateAntiAliasingMethod);
    bindBoolSetting("cas-enabled", g_casEnabled);
    bindDoubleSetting("cas-sharpness", g_casSharpness);
    bindBoolSetting("bloom-enabled", g_bloomEnabled);
    bindStringSetting("bloom-preset", updateBloomPreset);
    bindDoubleSetting("bloom-threshold", g_bloomThreshold);
    bindDoubleSetting("bloom-intensity", g_bloomIntensity);
    bindDoubleSetting("bloom-radius", g_bloomRadius);
    bindBoolSetting("dynamic-bloom-enabled", g_dynamicBloomEnabled);
    bindDoubleSetting("dynamic-bloom-speed", g_dynamicBloomSpeed);
    bindDoubleSetting("dynamic-bloom-min", g_dynamicBloomMin);
    bindDoubleSetting("dynamic-bloom-max", g_dynamicBloomMax);
    bindBoolSetting("grayscale-enabled", g_grayscaleEnabled);
    bindBoolSetting("pixelate-enabled", g_pixelateEnabled);
    bindBoolSetting("dithering-enabled", g_ditheringEnabled);
    bindBoolSetting("vhs-enabled", g_vhsEnabled);
    bindBoolSetting("crt-enabled", g_crtEnabled);
    bindBoolSetting("aberration-enabled", g_aberrationEnabled);
    bindDoubleSetting("aberration-strength", g_aberrationStrength);
    bindBoolSetting("neon-enabled", g_neonEnabled);
    bindDoubleSetting("neon-speed", g_neonSpeed);
    bindBoolSetting("radial-blur-enabled", g_radialBlurEnabled);
    bindDoubleSetting("radial-blur-strength", g_radialBlurStrength);
    bindBoolSetting("vignette-enabled", g_vignetteEnabled);
    bindDoubleSetting("vignette-strength", g_vignetteStrength);
    bindBoolSetting("halftone-enabled", g_halftoneEnabled);
    bindDoubleSetting("halftone-scale", g_halftoneScale);
    bindBoolSetting("godRays-enabled", g_godRaysEnabled);
    bindBoolSetting("god-rays-enabled", g_godRaysEnabled);
    bindDoubleSetting("god-rays-strength", g_godRaysStrength);
    bindBoolSetting("wobble-enabled", g_wobbleEnabled);
    bindDoubleSetting("wobble-speed", g_wobbleSpeed);
    bindBoolSetting("death-warp-enabled", g_deathWarpEnabled);
    bindBoolSetting("parallax-enabled", g_parallaxEnabled);
    bindDoubleSetting("parallax-depth", g_parallaxDepth);
    bindBoolSetting("split-screen-enabled", g_splitScreenEnabled);
    bindStringSetting("split-screen-mode", updateSplitScreenMode);
    bindDoubleSetting("split-screen-border", g_splitScreenBorder);
    bindBoolSetting("sepia-enabled", g_sepiaEnabled);
    bindDoubleSetting("sepia-strength", g_sepiaStrength);
    bindBoolSetting("posterize-enabled", g_posterizeEnabled);
    bindDoubleSetting("posterize-levels", g_posterizeLevels);
    bindBoolSetting("film-grain-enabled", g_filmGrainEnabled);
    bindDoubleSetting("film-grain-strength", g_filmGrainStrength);
    bindBoolSetting("depth-focus-enabled", g_depthFocusEnabled);
    bindDoubleSetting("depth-focus-strength", g_depthFocusStrength);
    bindBoolSetting("lens-flare-enabled", g_lensFlareEnabled);
    bindDoubleSetting("lens-flare-strength", g_lensFlareStrength);
    bindBoolSetting("ascii-enabled", g_asciiEnabled);
    bindDoubleSetting("ascii-scale", g_asciiScale);
    bindBoolSetting("cinematic-lut-enabled", g_cinematicLutEnabled);
    bindDoubleSetting("cinematic-lut-strength", g_cinematicLutStrength);
    bindBoolSetting("ambient-enabled", g_ambientEnabled);
    bindDoubleSetting("ambient-strength", g_ambientStrength);
    bindBoolSetting("motion-blur-enabled", g_motionBlurEnabled);
    bindDoubleSetting("motion-blur-strength", g_motionBlurStrength);
    bindBoolSetting("color-edit-enabled", g_colorEditEnabled);
    bindStringSetting("color-lut", updateColorLut);
    bindBoolSetting("animated-lut-enabled", g_animatedLutEnabled);
    bindDoubleSetting("animated-lut-speed", g_animatedLutSpeed);
    bindDoubleSetting("color-brightness", g_colorBrightness);
    bindDoubleSetting("color-contrast", g_colorContrast);
    bindDoubleSetting("color-saturation", g_colorSaturation);
    bindDoubleSetting("color-temperature", g_colorTemperature);
    bindBoolSetting("fake-hdr-enabled", g_fakeHdrEnabled);
    bindDoubleSetting("fake-hdr-strength", g_fakeHdrStrength);
    bindBoolSetting("sunset-enabled", g_sunsetEnabled);
    bindDoubleSetting("sunset-strength", g_sunsetStrength);
    bindBoolSetting("butter-camera-enabled", g_butterCameraEnabled);
    bindDoubleSetting("butter-camera-strength", g_butterCameraStrength);
    bindBoolSetting("raindrop-enabled", g_raindropEnabled);
    bindDoubleSetting("raindrop-strength", g_raindropStrength);
    bindBoolSetting("film-polaroid-enabled", g_filmPolaroidEnabled);
    bindDoubleSetting("film-polaroid-strength", g_filmPolaroidStrength);
    bindBoolSetting("fog-enabled", g_fogEnabled);
    bindDoubleSetting("fog-strength", g_fogStrength);
    bindBoolSetting("dust-film-enabled", g_dustFilmEnabled);
    bindDoubleSetting("dust-film-strength", g_dustFilmStrength);
    bindBoolSetting("custom-shader-enabled", g_customShaderEnabled);
    bindBoolSetting("global-shader-enabled", g_globalShaderEnabled);
    bindBoolSetting("shader-optimization-enabled", g_shaderOptimization);
    loadCustomShader();
    ButtonSettingPressedEventV3(Mod::get(), "custom-shader-actions").listen([](std::string_view button) {
        if (button != "load-glsl") return;
        async::spawn(geode::utils::file::pick(
            geode::utils::file::PickMode::OpenFile,
            geode::utils::file::FilePickOptions{{}, {{"GLSL shaders", {"*.glsl", "*.frag", "*.txt"}}}}
        ), [](geode::Result<std::optional<std::filesystem::path>> result) {
            if (!result.isOk()) {
                FLAlertLayer::create("Unable to Select Shader", result.unwrapErr(), "OK")->show();
                return;
            }
            if (!result.unwrap().has_value()) return;
            auto path = result.unwrap().value();
            auto persistent = geode::dirs::getModPersistentDir() / "custom.glsl";
            std::error_code error;
            std::filesystem::create_directories(persistent.parent_path(), error);
            std::filesystem::copy_file(path, persistent, std::filesystem::copy_options::overwrite_existing, error);
            loadCustomShader(persistent);
            g_customShaderEnabled.store(true, std::memory_order_relaxed);
            resetRenderResources();
            Notification::create("Custom GLSL shader loaded", NotificationIcon::Success, 1.5f)->show();
        });
    }).leak();
    bindAudioBoolSetting("audio-8d", g_audio8DEnabled);
    bindDoubleSetting("audio-8d-speed", g_audio8DSpeed);
    bindStringSetting("audio-filter", updateAudioFilter);
    bindDoubleSetting("audio-reverb", g_audioReverb);
    bindDoubleSetting("audio-muffle", g_audioMuffle);
    bindAudioEqBand("audio-eq-30", 0);
    bindAudioEqBand("audio-eq-60", 1);
    bindAudioEqBand("audio-eq-125", 2);
    bindAudioEqBand("audio-eq-250", 3);
    bindAudioEqBand("audio-eq-500", 4);
    bindAudioEqBand("audio-eq-1000", 5);
    bindAudioEqBand("audio-eq-2000", 6);
    bindAudioEqBand("audio-eq-4000", 7);
    bindAudioEqBand("audio-eq-8000", 8);
    bindAudioEqBand("audio-eq-16000", 9);
    bindStringSetting("audio-preset", updateAudioPreset);
    applyAudioEffects();
    bindStringSetting("effect-preset", updateEffectPreset);
    bindStringSetting("performance-preset", updatePerformancePreset);
}

class $modify(BetterVisualsAudioEngineHook, FMODAudioEngine) {
    void update(float dt) {
        FMODAudioEngine::update(dt);
        update8DAudio(dt);
    }
};

class $modify(BetterVisualsDirectorHook, CCDirector) {
    void drawScene() {
        if (!g_globalShaderEnabled.load(std::memory_order_relaxed) ||
            g_temporarilyDisabled.load(std::memory_order_relaxed) ||
            !g_modEnabled.load(std::memory_order_relaxed) || g_isGameLayerVisitActive) {
            CCDirector::drawScene();
            return;
        }

        g_isGameLayerVisitActive = true;
        renderSceneWithPostProcessing([this] {
            CCDirector::drawScene();
        });
        g_isGameLayerVisitActive = false;
    }

    void setProjection(ccDirectorProjection kProjection) {
        CCDirector::setProjection(kProjection);
        applyCaptureViewport();
    }

    void setViewport() {
        CCDirector::setViewport();
        applyCaptureViewport();
    }
};

#ifdef GEODE_IS_WINDOWS
class $modify(BetterVisualsEGLViewHook, CCEGLView) {
    void setViewPortInPoints(float x, float y, float w, float h) {
        CCEGLView::setViewPortInPoints(x, y, w, h);
        applyCaptureViewport();
    }
};
#endif

class $modify(BetterVisualsGameLayer, GJBaseGameLayer) {
    static void onModify(auto& self) {
        if (!self.setHookPriorityPre("GJBaseGameLayer::visit", Priority::VeryLate)) {
            log::warn("Unable to set GJBaseGameLayer::visit hook priority");
        }
    }

    void visit() {
        if (g_temporarilyDisabled.load(std::memory_order_relaxed) ||
            !g_modEnabled.load(std::memory_order_relaxed) || g_isGameLayerVisitActive ||
            static_cast<GJBaseGameLayer*>(this) != GJBaseGameLayer::get()) {
            GJBaseGameLayer::visit();
            return;
        }

        g_isGameLayerVisitActive = true;
        renderSceneWithPostProcessing([this] {
            GJBaseGameLayer::visit();
        });
        g_isGameLayerVisitActive = false;
    }
};

#ifdef GEODE_IS_WINDOWS
class $modify(BetterVisualsEGLView, CCEGLView) {
    void toggleFullScreen(bool value, bool borderless, bool fix) {
        if (!g_temporarilyDisabled.exchange(true)) {
            resetRenderResources();
        }
        CCEGLView::toggleFullScreen(value, borderless, fix);
    }
};
#endif

class $modify(BetterVisualsMenuLayer, MenuLayer) {
    void showPerformanceNotice() {
        if (g_performancePopupShown.exchange(true)) {
            return;
        }
        createQuickPopup(
            "Performance Notice",
            "Fullscreen visual effects can reduce performance and cause lag, especially on mobile devices.\n"
            "Disable demanding effects or lower their strength if gameplay becomes slow.",
            "OK",
            nullptr,
            [](FLAlertLayer*, bool) {
                // Permanently dismiss â€” never ask again after user clicks OK
                Mod::get()->setSavedValue<bool>("performance-notice-dismissed", true);
            }
        );
    }

    bool init() {
        if (!MenuLayer::init()) return false;
        applyAudioEffects();

        // Skip if user has already permanently dismissed the notice
        bool alreadyDismissed = Mod::get()->getSavedValue<bool>("performance-notice-dismissed", false);
        if (!alreadyDismissed && !g_performancePopupShown.load(std::memory_order_relaxed)) {
            runAction(CCSequence::create(
                CCDelayTime::create(1.25f),
                CCCallFunc::create(this, callfunc_selector(BetterVisualsMenuLayer::showPerformanceNotice)),
                nullptr
            ));
        } else if (alreadyDismissed) {
            g_performancePopupShown.store(true, std::memory_order_relaxed);
        }

        if (g_temporarilyDisabled.load(std::memory_order_relaxed) &&
            !g_disablePopupShown.exchange(true)) {
            queueInMainThread([] {
                createQuickPopup(
                    "RickGdps Menu Mod",
                    "RickGdps Menu Mod was temporarily disabled.\n"
                    "Please restart the game to restore fullscreen/windowed mode.\n"
                    "Sorry for the inconvenience!",
                    "OK",
                    nullptr,
                    [](FLAlertLayer*, bool) {}
                );
            });
        }
        return true;
    }
};

#ifdef GEODE_IS_MOBILE
class $modify(BetterVisualsAppDelegate, AppDelegate) {
    void applicationDidEnterBackground() {
        resetRenderResources();
        AppDelegate::applicationDidEnterBackground();
    }
};
#endif

// â”€â”€ Death Warp Hook â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
class $modify(BetterVisualsPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        // Ensure warp state is completely reset on level entry
        g_deathWarpProgress.store(1.0f, std::memory_order_relaxed);
        auto const ok = PlayLayer::init(level, useReplay, dontCreateObjects);
        return ok;
    }

    void destroyPlayer(PlayerObject* player, GameObject* obj) {
        PlayLayer::destroyPlayer(player, obj);

        if (!g_deathWarpEnabled.load(std::memory_order_relaxed)) return;
        if (!player || player != m_player1) return;

        // Calculate normalized screen position of player at death
        auto const winSize = CCDirector::sharedDirector()->getWinSize();
        auto const screenPos = player->convertToWorldSpace(CCPoint{0.f, 0.f});
        
        float cx = 0.5f;
        float cy = 0.5f;
        if (winSize.width > 0.f && winSize.height > 0.f) {
            cx = std::clamp(screenPos.x / winSize.width, 0.05f, 0.95f);
            cy = std::clamp(screenPos.y / winSize.height, 0.05f, 0.95f);
        }

        g_deathWarpCenterX.store(cx, std::memory_order_relaxed);
        g_deathWarpCenterY.store(cy, std::memory_order_relaxed);
        g_deathWarpProgress.store(0.0f, std::memory_order_relaxed);

        // Schedule shockwave animation progression
        this->unschedule(schedule_selector(BetterVisualsPlayLayer::tickDeathWarp));
        this->schedule(schedule_selector(BetterVisualsPlayLayer::tickDeathWarp), 1.f / 60.f);
    }

    void levelComplete() {
        PlayLayer::levelComplete();
    }

    void tickDeathWarp(float dt) {
        constexpr float kWarpDuration = 0.65f;
        float prog = g_deathWarpProgress.load(std::memory_order_relaxed);
        if (prog < 1.0f) {
            prog += dt / kWarpDuration;
            if (prog >= 1.0f) {
                prog = 1.0f;
                this->unschedule(schedule_selector(BetterVisualsPlayLayer::tickDeathWarp));
            }
            g_deathWarpProgress.store(prog, std::memory_order_relaxed);
        } else {
            this->unschedule(schedule_selector(BetterVisualsPlayLayer::tickDeathWarp));
        }
    }

    void resetLevel() {
        // Immediately reset and kill any active death shockwave before level reload
        g_deathWarpProgress.store(1.0f, std::memory_order_relaxed);
        this->unschedule(schedule_selector(BetterVisualsPlayLayer::tickDeathWarp));
        PlayLayer::resetLevel();
        g_deathWarpProgress.store(1.0f, std::memory_order_relaxed);
    }
};

