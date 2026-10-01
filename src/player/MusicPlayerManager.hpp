#pragma once

#include <Geode/Geode.hpp>
#include <Geode/fmod/fmod.hpp>
#include <Geode/fmod/fmod_dsp.h>
#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace rickgdps::music {

    inline std::atomic<bool> g_musicOverlayActive{false};

    struct MusicTrack {
        std::string title;
        std::string artist;
        std::string filename; // Path or filename passed to FMOD
        bool isCustom = false;
        int customSongID = 0;
        float duration = 0.f; // in seconds (0 if unknown)
    };

    enum class LoopMode {
        All = 0,
        Track = 1,
        Shuffle = 2,
    };

    class MusicPlayerManager {
    public:
        static MusicPlayerManager& get();

        void initPlaylist();
        void refreshPlaylist();

        void playTrack(int index);
        void togglePlayPause();
        void pause();
        void resume();
        void stop();
        void nextTrack();
        void prevTrack();
        void seek(float seconds);

        float getCurrentTime();
        float getDuration();
        bool isPlaying();
        bool isPaused();

        MusicTrack const* getCurrentTrack() const;

        int getCurrentTrackIndex() const {
            return m_currentIndex;
        }

        std::uint64_t getTrackRevision() const {
            return m_trackRevision;
        }

        size_t getTrackCount() const {
            return m_tracks.size();
        }

        MusicTrack const* getTrack(size_t index) const;

        void setVolume(float vol);

        float getVolume() const {
            return m_volume;
        }

        void setPitch(float pitch);

        float getPitch() const {
            return m_pitch;
        }

        void setLoopMode(LoopMode mode);

        LoopMode getLoopMode() const {
            return m_loopMode;
        }

        void cycleLoopMode();

        void update(float dt);

        // ─── Direct Audio Effect Interaction ─────────────────────────────────
        bool is8DEnabled() const;
        void set8DEnabled(bool enabled);
        float get8DSpeed() const;
        void set8DSpeed(float speed);

        float getReverb() const;
        void setReverb(float ms);
        void setAudioPreset(std::string const& preset);

        float getMuffle() const;
        void setMuffle(float val);
        void setAudioFilter(std::string const& filter);

        float getEqBand(int bandIndex) const;
        void setEqBand(int bandIndex, float gainDb);
        void applyEqPreset(int presetIndex);

        std::vector<float> getVisualizerBars(size_t barCount = 20);

    private:
        MusicPlayerManager();
        ~MusicPlayerManager() = default;

        std::vector<MusicTrack> m_tracks;
        int m_currentIndex = -1;
        bool m_isPlaying = false;
        bool m_isPaused = false;
        float m_volume = 1.0f;
        float m_pitch = 1.0f;
        LoopMode m_loopMode = LoopMode::All;
        std::uint64_t m_trackRevision = 0;

        float m_visualizerPhase = 0.0f;
        std::vector<float> m_visualizerSmooth;
        std::vector<float> m_visualizerPeaks;

        // Cached DSP settings to avoid per-frame mod lookup overhead
        bool m_8dEnabled = false;
        float m_8dSpeed = 0.15f;
        float m_reverb = 0.0f;
        float m_muffle = 0.0f;
        std::array<float, 10> m_eqBands{0.0f};

        // FMOD FFT DSP for real spectrum analysis
        FMOD::DSP* m_fftDsp = nullptr;
        int m_sampleRate = 44100;

        void playCurrent();
        void initFFT();
    };

} // namespace rickgdps::music