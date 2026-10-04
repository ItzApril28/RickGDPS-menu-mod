# BetterVisual and Audio (RickGdps Menu Mod) — v1.4.7 - beta

BetterVisual and Audio is an advanced visual enhancement, audio DSP workstation, and menu mod for Geometry Dash (Geode 2.2081). It features custom shaders, dynamic render scaling, super-sampling, studio audio processing, and an interactive in-game music player.

Configure the mod through the custom in-game settings popup or Geode mod settings.

## What's New in v1.4.8 - beta 3

- **Draggable Quick Toggle Overlay**: floating in-game strip with mid-run toggles for Noclip, Speedhack, Auto-Retry, Hitboxes, Layout, Mirror and Hide UI. Drag the `≡` handle to move it; position and opacity are remembered.
- **Pluggable EQ Presets**: custom 10-band EQ presets are read from `eq_presets.json` in the mod's persistent folder. `SAVE CURRENT` stores the current curve with a name, `RELOAD` picks up hand-edited files.
- **AMOLED Idle Dim**: while the Music Studio is open, the screen dims to near-black after an idle timeout (default 15 s) and shows the current track. Touch or press any key to wake.
- **Hide In-Game HUD**: hides percentage, attempts, progress bar and pause button during gameplay.
- **Fixed previously dead settings**: Player Trail toggle, Anti-Cheat Bypass (blocks score submission), Custom Start Position, Respawn Delay, real object hitboxes (hazards highlighted red), and full Layout Mode (background + ground).

## What's New in v1.4.7 - beta

- **Music Player Studio (`♫ MUSIC STUDIO`)**:
  - Full-featured in-game audio workstation with direct FMOD channel and DSP effect integration.
  - Pre-loaded with official Geometry Dash soundtracks and automatic discovery of downloaded Custom Songs.
  - Interactive playback deck: Play, Pause, Resume, Stop, Next, Previous, Scrubber seek bar, Volume presets, and Pitch/Speed dials (0.5x to 2.0x).
  - Loop modes: `Loop Track`, `Loop All`, and `Shuffle`.
  - Real-time 20-band Spectrum Visualizer reacting dynamically to audio energy, equalizer gains, muffle cutoff, and 8D binaural pan.
  - 4 sub-tabs: **♫ PLAYER**, **≡ TRACKS**, **🎛 LIVE FX** (8D audio, studio reverb, and acoustic muffle filters), and **🎚 10-BAND EQ** (-9 dB to +9 dB graphic equalizer).
  - Modern curved GD-styled UI with deep teal and cream/butter aesthetic (`ModernTheme.hpp`), curved corners, dark inset panels, and dynamic label fitting.
  - Fully editable resource theme configuration in `src/player/MusicPlayerTheme.hpp`.
- **Cleanups**:
  - Removed obsolete web browser, file explorer, and terminal components.

## Features

### Audio DSP & Music Studio
- **Music Player Studio**: In-game player with visualizer, playlist, and audio effect manipulation.
- **8D Audio Effect**: Immersive 3D binaural rotation with subtle room acoustics.
- **Studio Reverb Engine**: Handcrafted acoustic profiles (Room, Cinema, Cathedral, GDH Reverb, Plate).
- **Acoustic Muffle Filters**: Cozy lowpass filters, Underwater, Telephone, Lo-Fi Tape, Vintage Radio, and Space Echo.
- **10-Band Graphic Equalizer**: Precise frequency sculpting (30 Hz to 16 kHz) with Flat, Bass Boost, Vocal, Treble, and EDM presets.

### Rendering & Shaders
- **Dynamic Render Scaling (DRS)**: Target-framerate based resolution scaling.
- **Super-Sampling Anti-Aliasing (SSAA)**: 1.5x and 2.0x super-sampling with AMD FidelityFX FSR 1 reconstruction.
- **Anti-Aliasing**: FXAA, SMAA High, SMAA Ultra.
- **Sharpening**: AMD FidelityFX CAS (Contrast-Adaptive Sharpening).
- **Bloom & Atmosphere**: Multi-pass HDR bloom with 14 profiles, volumetric God Rays, anamorphic lens flare, ambient aurora, and cinematic vignette.
- **Stylized & Retro Shaders**: CRT scanlines, VHS tape jitter, 35mm film grain, 8-bit Bayer dithering, comic halftone, ASCII typography, pixel art downscale, and neon pulse.
- **Camera Effects**: Handheld camera wobble, camera motion blur, depth of field focus, and death warp shockwave.
- **Custom GLSL Shader**: Hot-reloadable `custom.glsl` from mod persistent folder.

## Performance Profiles

- **Max FPS:** 0.5x render scale, Nearest upscaling, AA off, sharpening off.
- **Balanced:** 0.75x render scale, FSR 1, AA off, sharpening off.
- **Visual Quality:** 1.0x render scale, SMAA High, CAS sharpening.
- **Cinematic:** 1.5x SSAA, Bloom, Vignette, Film Grain.

## Credits & Fork Status

- Original project: [BetterVisuals](https://github.com/ItMe12s/BetterVisuals/) by ItMe12s
- Fork and enhancements: BetterVisual and Audio by ItzApril28 / RickGdps
- Framebuffer and VBO references: [Geode DevTools](https://github.com/geode-sdk/DevTools)

## License

Shader-specific credits, citations, and licenses are included in the source files where applicable.
