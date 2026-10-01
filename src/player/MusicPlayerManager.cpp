#include "MusicPlayerManager.hpp"

#include <Geode/binding/FMODAudioEngine.hpp>
#include <Geode/binding/LevelTools.hpp>
#include <Geode/binding/MusicDownloadManager.hpp>
#include <Geode/binding/SongInfoObject.hpp>
#include <algorithm>
#include <cmath>
#include <random>

using namespace geode::prelude;

namespace rickgdps::music {

    static constexpr char const* kEqSettingKeys[10] = {
        "audio-eq-30",
        "audio-eq-60",
        "audio-eq-125",
        "audio-eq-250",
        "audio-eq-500",
        "audio-eq-1000",
        "audio-eq-2000",
        "audio-eq-4000",
        "audio-eq-8000",
        "audio-eq-16000"
    };

    struct OfficialSongDef {
        int id;
        char const* title;
        char const* artist;
        char const* filename;
    };

    static constexpr OfficialSongDef kOfficialSongs[] = {
        {0, "Stereo Madness", "ForeverBound", "StereoMadness.mp3"},
        {1, "Back On Track", "DJVI", "BackOnTrack.mp3"},
        {2, "Polargeist", "Step", "Polargeist.mp3"},
        {3, "Dry Out", "DJVI", "DryOut.mp3"},
        {4, "Base After Base", "DJVI", "BaseAfterBase.mp3"},
        {5, "Can't Let Go", "DJVI", "CantLetGo.mp3"},
        {6, "Jumper", "Waterflame", "Jumper.mp3"},
        {7, "Time Machine", "Waterflame", "TimeMachine.mp3"},
        {8, "Cycles", "DJVI", "Cycles.mp3"},
        {9, "xStep", "DJVI", "xStep.mp3"},
        {10, "Clutterfunk", "Waterflame", "Clutterfunk.mp3"},
        {11, "Theory of Everything", "DJ-Nate", "TheoryOfEverything.mp3"},
        {12, "Electroman Adventures", "Waterflame", "Electroman.mp3"},
        {13, "Clubstep", "DJ-Nate", "Clubstep.mp3"},
        {14, "Electrodynamix", "DJ-Nate", "Electrodynamix.mp3"},
        {15, "Hexagon Force", "Waterflame", "HexagonForce.mp3"},
        {16, "Blast Processing", "Waterflame", "BlastProcessing.mp3"},
        {17, "Theory of Everything 2", "DJ-Nate", "TheoryOfEverything2.mp3"},
        {18, "Geometrical Dominator", "Waterflame", "GeometricalDominator.mp3"},
        {19, "Deadlocked", "F-777", "Deadlocked.mp3"},
        {20, "Fingerdash", "MDK", "Fingerdash.mp3"},
        {21, "Dash", "MDK", "Dash.mp3"},
        {22, "Stay Inside Me (Practice)", "OcularNebula", "StayInsideMe.mp3"},
        {23, "Menu Theme", "RobTop", "menuLoop.mp3"}
    };

    MusicPlayerManager& MusicPlayerManager::get() {
        static MusicPlayerManager instance;
        return instance;
    }

    MusicPlayerManager::MusicPlayerManager() {
        if (auto* mod = Mod::get()) {
            m_8dEnabled = mod->getSettingValue<bool>("audio-8d");
            m_8dSpeed = static_cast<float>(mod->getSettingValue<double>("audio-8d-speed"));
            m_reverb = static_cast<float>(mod->getSettingValue<double>("audio-reverb"));
            m_muffle = static_cast<float>(mod->getSettingValue<double>("audio-muffle"));
            for (int i = 0; i < 10; ++i) {
                m_eqBands[i] = static_cast<float>(mod->getSettingValue<double>(kEqSettingKeys[i]));
            }
        }
        initPlaylist();
        initFFT();
    }

    void MusicPlayerManager::initPlaylist() {
        m_tracks.clear();

        for (auto const& def : kOfficialSongs) {
            MusicTrack track;
            track.title = def.title;
            track.artist = def.artist;
            track.filename = def.filename;
            track.isCustom = false;
            track.customSongID = def.id;
            track.duration = 0.f;
            m_tracks.push_back(track);
        }

        refreshPlaylist();
    }

    void MusicPlayerManager::refreshPlaylist() {
        constexpr size_t officialCount = sizeof(kOfficialSongs) / sizeof(OfficialSongDef);
        if (m_tracks.size() > officialCount) {
            m_tracks.erase(m_tracks.begin() + officialCount, m_tracks.end());
        }

        auto* mdm = MusicDownloadManager::sharedState();
        if (!mdm) return;

        auto* downloaded = mdm->getDownloadedSongs();
        if (!downloaded) return;

        for (auto* obj : CCArrayExt<SongInfoObject*>(downloaded)) {
            if (!obj) continue;
            auto localPath = mdm->pathForSong(obj->m_songID);
            if (localPath.empty()) continue;

            MusicTrack track;
            track.title = obj->m_songName.empty() ?
                ("Custom Song #" + std::to_string(obj->m_songID)) :
                std::string(obj->m_songName);
            track.artist =
                obj->m_artistName.empty() ? "Unknown Artist" : std::string(obj->m_artistName);
            track.filename = std::string(localPath);
            track.isCustom = true;
            track.customSongID = obj->m_songID;
            track.duration = static_cast<float>(obj->m_duration);
            m_tracks.push_back(track);
        }
    }

    void MusicPlayerManager::playTrack(int index) {
        if (m_tracks.empty()) return;
        if (index < 0 || index >= static_cast<int>(m_tracks.size())) {
            index = 0;
        }

        m_currentIndex = index;
        ++m_trackRevision;
        playCurrent();
    }

    void MusicPlayerManager::playCurrent() {
        if (m_currentIndex < 0 || m_currentIndex >= static_cast<int>(m_tracks.size())) {
            return;
        }

        auto* engine = FMODAudioEngine::sharedEngine();
        if (!engine) return;

        auto const& track = m_tracks[static_cast<size_t>(m_currentIndex)];
        bool shouldLoop = (m_loopMode == LoopMode::Track);

        engine->stopAllMusic(false);
        engine->playMusic(track.filename, shouldLoop, 0.15f, 0);

        engine->setChannelPitch(0, AudioTargetType::MusicChannel, m_pitch);
        engine->setBackgroundMusicVolume(m_volume);

        m_isPlaying = true;
        m_isPaused = false;
    }

    void MusicPlayerManager::togglePlayPause() {
        if (!m_isPlaying) {
            if (m_currentIndex < 0) {
                playTrack(0);
            }
            else {
                playCurrent();
            }
            return;
        }

        if (m_isPaused) {
            resume();
        }
        else {
            pause();
        }
    }

    void MusicPlayerManager::pause() {
        auto* engine = FMODAudioEngine::sharedEngine();
        if (!engine) return;
        engine->pauseMusic(0);
        m_isPaused = true;
    }

    void MusicPlayerManager::resume() {
        auto* engine = FMODAudioEngine::sharedEngine();
        if (!engine) return;
        engine->resumeMusic(0);
        m_isPaused = false;
        m_isPlaying = true;
    }

    void MusicPlayerManager::stop() {
        auto* engine = FMODAudioEngine::sharedEngine();
        if (!engine) return;
        engine->stopAllMusic(false);
        m_isPlaying = false;
        m_isPaused = false;
    }

    void MusicPlayerManager::nextTrack() {
        if (m_tracks.empty()) return;

        if (m_loopMode == LoopMode::Shuffle && m_tracks.size() > 1) {
            static std::mt19937 rng{std::random_device{}()};
            std::uniform_int_distribution<int> dist(0, static_cast<int>(m_tracks.size()) - 1);
            int nextIdx = dist(rng);
            if (nextIdx == m_currentIndex) {
                nextIdx = (nextIdx + 1) % static_cast<int>(m_tracks.size());
            }
            playTrack(nextIdx);
        }
        else {
            int nextIdx = (m_currentIndex + 1) % static_cast<int>(m_tracks.size());
            playTrack(nextIdx);
        }
    }

    void MusicPlayerManager::prevTrack() {
        if (m_tracks.empty()) return;

        if (getCurrentTime() > 3.0f) {
            seek(0.0f);
            return;
        }

        int prevIdx = (m_currentIndex - 1 + static_cast<int>(m_tracks.size())) %
            static_cast<int>(m_tracks.size());
        playTrack(prevIdx);
    }

    void MusicPlayerManager::seek(float seconds) {
        auto* engine = FMODAudioEngine::sharedEngine();
        if (!engine) return;
        unsigned int ms = static_cast<unsigned int>(std::max(0.0f, seconds) * 1000.f);
        engine->setMusicTimeMS(ms, true, 0);
    }

    float MusicPlayerManager::getCurrentTime() {
        auto* engine = FMODAudioEngine::sharedEngine();
        if (!engine) return 0.f;
        return static_cast<float>(engine->getMusicTimeMS(0)) / 1000.f;
    }

    float MusicPlayerManager::getDuration() {
        auto* engine = FMODAudioEngine::sharedEngine();
        if (!engine) return 0.f;
        float dur = static_cast<float>(engine->getMusicLengthMS(0)) / 1000.f;
        if (dur <= 0.01f && m_currentIndex >= 0 &&
            m_currentIndex < static_cast<int>(m_tracks.size())) {
            dur = m_tracks[static_cast<size_t>(m_currentIndex)].duration;
        }
        return dur;
    }

    bool MusicPlayerManager::isPlaying() {
        auto* engine = FMODAudioEngine::sharedEngine();
        if (!engine) return false;
        return engine->isMusicPlaying(0) && !m_isPaused;
    }

    bool MusicPlayerManager::isPaused() {
        return m_isPaused;
    }

    MusicTrack const* MusicPlayerManager::getCurrentTrack() const {
        if (m_currentIndex < 0 || m_currentIndex >= static_cast<int>(m_tracks.size())) {
            return nullptr;
        }
        return &m_tracks[static_cast<size_t>(m_currentIndex)];
    }

    MusicTrack const* MusicPlayerManager::getTrack(size_t index) const {
        if (index >= m_tracks.size()) return nullptr;
        return &m_tracks[index];
    }

    void MusicPlayerManager::setVolume(float vol) {
        m_volume = std::clamp(vol, 0.f, 1.f);
        auto* engine = FMODAudioEngine::sharedEngine();
        if (engine) {
            engine->setBackgroundMusicVolume(m_volume);
        }
    }

    void MusicPlayerManager::setPitch(float pitch) {
        m_pitch = std::clamp(pitch, 0.5f, 2.0f);
        auto* engine = FMODAudioEngine::sharedEngine();
        if (engine) {
            engine->setChannelPitch(0, AudioTargetType::MusicChannel, m_pitch);
        }
    }

    void MusicPlayerManager::setLoopMode(LoopMode mode) {
        m_loopMode = mode;
    }

    void MusicPlayerManager::cycleLoopMode() {
        int next = (static_cast<int>(m_loopMode) + 1) % 3;
        m_loopMode = static_cast<LoopMode>(next);
    }

    void MusicPlayerManager::update(float dt) {
        m_visualizerPhase += dt * 3.5f;

        if (m_isPlaying && !m_isPaused) {
            auto* engine = FMODAudioEngine::sharedEngine();
            if (engine) {
                // Let FMOD handle single-track looping natively without manual truncation
                if (m_loopMode == LoopMode::Track) {
                    return;
                }

                float curTime = getCurrentTime();
                float dur = getDuration();
                if (dur > 2.0f && curTime >= dur - 0.05f) {
                    nextTrack();
                }
            }
        }
    }

    // ─── Direct Audio Effect Interaction ─────────────────────────────────────

    bool MusicPlayerManager::is8DEnabled() const {
        return m_8dEnabled;
    }

    void MusicPlayerManager::set8DEnabled(bool enabled) {
        m_8dEnabled = enabled;
        if (auto* mod = Mod::get()) {
            mod->setSettingValue<bool>("audio-8d", enabled);
        }
    }

    float MusicPlayerManager::get8DSpeed() const {
        return m_8dSpeed;
    }

    void MusicPlayerManager::set8DSpeed(float speed) {
        m_8dSpeed = speed;
        if (auto* mod = Mod::get()) {
            mod->setSettingValue<double>("audio-8d-speed", static_cast<double>(speed));
        }
    }

    float MusicPlayerManager::getReverb() const {
        return m_reverb;
    }

    void MusicPlayerManager::setReverb(float ms) {
        m_reverb = ms;
        if (auto* mod = Mod::get()) {
            mod->setSettingValue<double>("audio-reverb", static_cast<double>(ms));
        }
    }

    void MusicPlayerManager::setAudioPreset(std::string const& preset) {
        if (auto* mod = Mod::get()) {
            mod->setSettingValue<std::string>("audio-preset", preset);
        }
    }

    float MusicPlayerManager::getMuffle() const {
        return m_muffle;
    }

    void MusicPlayerManager::setMuffle(float val) {
        m_muffle = val;
        if (auto* mod = Mod::get()) {
            mod->setSettingValue<double>("audio-muffle", static_cast<double>(val));
        }
    }

    void MusicPlayerManager::setAudioFilter(std::string const& filter) {
        if (auto* mod = Mod::get()) {
            mod->setSettingValue<std::string>("audio-filter", filter);
        }
    }

    float MusicPlayerManager::getEqBand(int bandIndex) const {
        if (bandIndex < 0 || bandIndex >= 10) return 0.f;
        return m_eqBands[bandIndex];
    }

    void MusicPlayerManager::setEqBand(int bandIndex, float gainDb) {
        if (bandIndex < 0 || bandIndex >= 10) return;
        m_eqBands[bandIndex] = gainDb;
        if (auto* mod = Mod::get()) {
            mod->setSettingValue<double>(kEqSettingKeys[bandIndex], static_cast<double>(gainDb));
        }
    }

    void MusicPlayerManager::applyEqPreset(int presetIndex) {
        constexpr float presets[5][10] = {
            {0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}, // Flat
            {5.5f, 6.0f, 4.0f, 2.0f, 0.0f, 0.0f, 0.5f, 1.0f, 1.5f, 2.0f}, // Bass Boost
            {-2.0f, -1.0f, 0.0f, 2.0f, 4.5f, 5.0f, 4.0f, 2.0f, 1.0f, 0.0f}, // Vocal Boost
            {0.0f, 0.0f, 0.5f, 1.0f, 1.5f, 3.0f, 4.5f, 5.5f, 6.0f, 6.0f}, // Treble Boost
            {5.0f, 4.5f, 2.0f, -1.0f, -2.5f, -2.0f, 1.5f, 3.5f, 5.0f, 5.5f} // EDM / V-Shape
        };

        if (presetIndex < 0 || presetIndex >= 5) presetIndex = 0;
        for (int i = 0; i < 10; ++i) {
            setEqBand(i, presets[presetIndex][i]);
        }
    }

    // ─── FMOD FFT DSP Initialisation ─────────────────────────────────────────

    void MusicPlayerManager::initFFT() {
        auto* engine = FMODAudioEngine::get();
        if (!engine || !engine->m_system) return;

        // Query real sample rate for accurate frequency mapping
        int sr = 44100;
        engine->m_system->getSoftwareFormat(&sr, nullptr, nullptr);
        m_sampleRate = sr;

        // Create FFT DSP
        FMOD::DSP* fftDsp = nullptr;
        if (engine->m_system->createDSPByType(FMOD_DSP_TYPE_FFT, &fftDsp) != FMOD_OK || !fftDsp) {
            return;
        }

        // 2048-sample window gives ~21 Hz resolution at 44100 Hz — good enough
        // for the 20-bar visualizer without being too CPU-heavy.
        fftDsp->setParameterInt(FMOD_DSP_FFT_WINDOWSIZE, 2048);
        fftDsp->setParameterInt(FMOD_DSP_FFT_WINDOWTYPE, FMOD_DSP_FFT_WINDOW_HANNING);

        // Attach to master channel group (tail end so we read post-effects)
        FMOD::ChannelGroup* masterGroup = nullptr;
        if (engine->m_system->getMasterChannelGroup(&masterGroup) != FMOD_OK || !masterGroup) {
            fftDsp->release();
            return;
        }

        if (masterGroup->addDSP(FMOD_CHANNELCONTROL_DSP_TAIL, fftDsp) != FMOD_OK) {
            fftDsp->release();
            return;
        }

        m_fftDsp = fftDsp;
    }

    // ─── Visualizer ───────────────────────────────────────────────────────────

    std::vector<float> MusicPlayerManager::getVisualizerBars(size_t barCount) {
        if (m_visualizerSmooth.size() != barCount) {
            m_visualizerSmooth.assign(barCount, 0.05f);
            m_visualizerPeaks.assign(barCount, 0.05f);
        }

        bool active = isPlaying();
        float muffle = m_muffle;
        bool is8d = m_8dEnabled;

        // ── Try real FMOD FFT first ───────────────────────────────────────────
        bool usedRealFFT = false;
        if (active && m_fftDsp) {
            FMOD_DSP_PARAMETER_FFT* fftData = nullptr;
            unsigned int fftLen = 0;
            if (m_fftDsp->getParameterData(
                    FMOD_DSP_FFT_SPECTRUMDATA,
                    reinterpret_cast<void**>(&fftData),
                    &fftLen, nullptr, 0) == FMOD_OK && fftData && fftData->length > 0)
            {
                int numCh  = std::min(fftData->numchannels, 2);
                int fftSize = fftData->length; // half-spectrum (0..nyquist)

                // Logarithmic frequency mapping so bass gets more bars
                constexpr float kMinHz = 20.f;
                float maxHz  = static_cast<float>(m_sampleRate) * 0.5f;
                float logMin = std::log2(kMinHz);
                float logMax = std::log2(maxHz);
                float hzPerBin = maxHz / static_cast<float>(fftSize);

                for (size_t i = 0; i < barCount; ++i) {
                    float loHz = std::pow(2.f, logMin + (logMax - logMin) * static_cast<float>(i)     / static_cast<float>(barCount));
                    float hiHz = std::pow(2.f, logMin + (logMax - logMin) * static_cast<float>(i + 1) / static_cast<float>(barCount));

                    int binLo = std::clamp(static_cast<int>(loHz / hzPerBin), 0, fftSize - 1);
                    int binHi = std::clamp(static_cast<int>(hiHz / hzPerBin), binLo, fftSize - 1);

                    float sum = 0.f;
                    int   cnt = 0;
                    for (int b = binLo; b <= binHi; ++b) {
                        float mag = 0.f;
                        for (int ch = 0; ch < numCh; ++ch) mag += fftData->spectrum[ch][b];
                        sum += mag / static_cast<float>(numCh);
                        ++cnt;
                    }
                    float raw = (cnt > 0) ? (sum / static_cast<float>(cnt)) : 0.f;

                    // sqrt-compress linear magnitude into perceptual loudness scale
                    float target = std::sqrt(raw) * 2.5f;

                    // EQ band scaling
                    int eqIdx = std::clamp(static_cast<int>(static_cast<float>(i) / static_cast<float>(barCount) * 10.f), 0, 9);
                    target *= std::pow(10.f, m_eqBands[eqIdx] / 20.f) * m_volume;

                    // Muffle attenuates high-freq bars
                    float freqRatio = static_cast<float>(i) / static_cast<float>(barCount);
                    if (muffle > 0.01f && freqRatio > (1.f - muffle * 0.85f)) {
                        float cut = (freqRatio - (1.f - muffle * 0.85f)) / 0.85f;
                        target *= std::max(0.02f, 1.f - cut * 1.5f);
                    }

                    // 8D spatial panning modulation
                    if (is8d) {
                        float barPan = (freqRatio - 0.5f) * 2.f;
                        float pf     = std::sin(m_visualizerPhase * m_8dSpeed * 4.f);
                        target *= std::clamp(1.f + pf * barPan * 0.6f, 0.2f, 1.8f);
                    }

                    target = std::clamp(target, 0.01f, 1.0f);

                    // Asymmetric smoothing: fast attack, slow decay for punchy feel
                    float alpha = (target > m_visualizerSmooth[i]) ? 0.55f : 0.18f;
                    m_visualizerSmooth[i] += (target - m_visualizerSmooth[i]) * alpha;

                    if (m_visualizerSmooth[i] > m_visualizerPeaks[i]) {
                        m_visualizerPeaks[i] = m_visualizerSmooth[i];
                    } else {
                        m_visualizerPeaks[i] = std::max(0.02f, m_visualizerPeaks[i] - 0.012f);
                    }
                }
                usedRealFFT = true;
            }
        }

        // ── Fallback: animated sine-wave (paused / FFT not yet ready) ─────────
        if (!usedRealFFT) {
            float panFactor = is8d ? std::sin(m_visualizerPhase * m_8dSpeed * 4.0f) : 0.0f;

            for (size_t i = 0; i < barCount; ++i) {
                float target = 0.05f;

                if (active) {
                    float fi = static_cast<float>(i);
                    float freqRatio = fi / static_cast<float>(barCount);

                    int eqIdx = std::clamp(static_cast<int>(freqRatio * 10.0f), 0, 9);
                    float eqScale = std::pow(10.0f, m_eqBands[eqIdx] / 20.0f);

                    float raw = std::abs(
                        std::sin(m_visualizerPhase * 5.2f + fi * 0.7f) * 0.45f +
                        std::cos(m_visualizerPhase * 8.7f - fi * 1.3f) * 0.35f +
                        std::sin(m_visualizerPhase * 2.1f + fi * 0.3f) * 0.20f
                    );
                    target = raw * (1.0f - freqRatio * 0.45f) * eqScale * m_volume;

                    if (freqRatio > (1.0f - muffle * 0.85f)) {
                        float cut = (freqRatio - (1.0f - muffle * 0.85f)) / 0.85f;
                        target *= std::max(0.02f, 1.0f - cut * 1.5f);
                    }
                    if (is8d) {
                        float barPan = (fi / static_cast<float>(barCount) - 0.5f) * 2.0f;
                        target *= std::clamp(1.0f + panFactor * barPan * 0.6f, 0.2f, 1.8f);
                    }
                    target = std::clamp(target, 0.06f, 1.0f);
                } else {
                    target = 0.04f + 0.02f * std::sin(m_visualizerPhase * 1.5f + static_cast<float>(i) * 0.4f);
                }

                m_visualizerSmooth[i] += (target - m_visualizerSmooth[i]) * 0.32f;

                if (m_visualizerSmooth[i] > m_visualizerPeaks[i]) {
                    m_visualizerPeaks[i] = m_visualizerSmooth[i];
                } else {
                    m_visualizerPeaks[i] = std::max(0.04f, m_visualizerPeaks[i] - 0.015f);
                }
            }
        }

        return m_visualizerSmooth;
    }

} // namespace rickgdps::music