#pragma once

#include <Geode/Geode.hpp>
#include <string>
#include <vector>
#include <array>
#include <atomic>

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
        int getCurrentTrackIndex() const { return m_currentIndex; }
        size_t getTrackCount() const { return m_tracks.size(); }
        MusicTrack const* getTrack(size_t index) const;

        void setVolume(float vol);
        float getVolume() const { return m_volume; }

        void setPitch(float pitch);
        float getPitch() const { return m_pitch; }

        void setLoopMode(LoopMode mode);
        LoopMode getLoopMode() const { return m_loopMode; }
        void cycleLoopMode();

        void update(float dt);

        // ─── Direct Audio Effect Interaction ─────────────────────────────────
        // 8D Audio
        bool is8DEnabled() const;
        void set8DEnabled(bool enabled);
        float get8DSpeed() const;
        void set8DSpeed(float speed);

        // Reverb DSP
        float getReverb() const;
        void setReverb(float ms);
        void setAudioPreset(std::string const& preset);

        // Muffle / Lowpass
        float getMuffle() const;
        void setMuffle(float val);
        void setAudioFilter(std::string const& filter);

        // Equalizer
        float getEqBand(int bandIndex) const;
        void setEqBand(int bandIndex, float gainDb);
        void applyEqPreset(int presetIndex); // 0=Flat, 1=Bass, 2=Vocal, 3=Treble, 4=EDM

        // Live visualizer spectrum data (computes 16-24 amplitude bars)
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

        float m_visualizerPhase = 0.0f;
        std::vector<float> m_visualizerSmooth;
        std::vector<float> m_visualizerPeaks;

        void playCurrent();
    };

} // namespace rickgdps::music
