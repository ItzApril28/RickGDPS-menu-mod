# Changelog

## 1.4.7 - beta.2 — Global Draggable Mod Button & Global AMOLED Idle Dim

### New Features & Enhancements
- **Global Draggable Mod Button**:
  - Replaced the in-game quick toggle overlay with a floating mod button that persists across all scenes (main menu, level select, garage, gameplay, editor).
  - Can be smoothly dragged anywhere on screen and automatically saves its position globally between scenes and game launches.
  - Symmetrical edge-clamping with centered anchor points for consistent positioning across different screen resolutions.
  - Robust tap vs. drag detection: single tap opens the settings menu, while dragging repositions the button.
  - Configurable toggle (`draggable-btn-enabled`) and opacity slider (`draggable-btn-opacity`) in mod settings.
- **Global AMOLED Idle Dim**:
  - AMOLED idle dim now works globally across the entire game, not just within the Music Studio popup.
  - Ticked every frame via `CCDirector::drawScene()`.
  - Comprehensive activity detection: wakes immediately on any keyboard key, screen touch / mouse click, drag, or mouse wheel scroll.
  - Dynamic display context:
    - **Music Playing**: Shows current track title, playback position, and total duration.
    - **In-Game / PlayLayer**: Shows current level name and progress percentage.
    - **Menus / Other**: Shows "Geometry Dash" with an idle wake hint.
  - Waking tap is absorbed to prevent accidental button clicks beneath the dim overlay.
  - Automatic wake and timer reset on scene transitions.

### Fixes & Improvements
- **AMOLED Title Sync**: Title now refreshes every frame from `MusicPlayerManager::getCurrentTrack()` and auto-scales to fit long song names.
- **Spectrum Visualizer Accuracy**: Improved accuracy and reactivity of the real-time spectrum visualizer to track live audio energy.
- **UI Asset Consistency**: Updated button sprites to use `square02b_001.png` for a cohesive theme.

## 1.4.8 - beta — Quick Toggle Overlay, EQ Presets, AMOLED & HUD Fixes

### New Features
- **Draggable Quick Toggle Overlay**: floating in-game strip (drag the `≡` handle) with mid-run toggles for Noclip, Speedhack, Auto-Retry, Hitboxes, Layout, Mirror and Hide UI. Position and opacity are remembered between sessions.
- **Pluggable EQ Presets**: the Music Studio 10-band EQ now shows custom presets from `eq_presets.json` in the mod's persistent folder. `SAVE CURRENT` writes the current curve (with a name prompt), `RELOAD` re-reads the file - hand-edit or share preset files freely.
- **AMOLED Idle Dim**: while the Music Studio is open, the screen fades to near-black after an idle timeout (default 15 s) showing the current track; touch or press any key to wake. Timeout + dim opacity are configurable.
- **Hide In-Game HUD**: hides the percentage, attempt counter, progress bar and pause button during gameplay for clean screenshots. The quick toggle overlay stays visible so you can flip it back.

### Fixed — settings that previously did nothing
- `Player Trail` (`trail-enabled`) now actually hides / restores the trail streaks and trail particles.
- `Anti-Cheat Bypass` now hooks `GameManager::reportPercentageForLevel` and blocks score / percentage submission while active.
- `Custom Start Position` (`start-pos-enabled`, `start-pos-percent`) now creates a real start position at the chosen percentage when the level loads.
- `Respawn Delay` now defers the practice-mode checkpoint respawn by the configured time.
- `Show Object Hitboxes` now draws real bounding boxes for every on-screen object (hazards highlighted red) through a camera-aligned draw node instead of only player outlines.
- `Layout Mode` now also hides the ground layers, and both Layout and Mirror modes react instantly when toggled mid-run.

## 1.4.7 - beta — Music Player Studio & Live Audio DSP

### Music Player Studio
- **Full In-Game Music Player (`MusicPlayerPopup` / `MusicPlayerManager`)**:
  - Built-in audio workstation directly connected to the mod's real-time FMOD DSP effects.
  - Pre-loaded with official Geometry Dash soundtracks (Stereo Madness through Dash + StayInsideMe + menuLoop) plus automatic detection of downloaded Custom Songs.
  - Complete playback controls: Play, Pause, Resume, Stop, Next, Previous, Progress scrubber bar, Volume presets, and Pitch/Speed dials (0.5x to 2.0x).
  - Loop modes: `Loop Track`, `Loop All`, and `Shuffle`.
  - Real-time 20-band Spectrum Visualizer reacting dynamically to audio energy, equalizer gains, muffle cutoff, and 8D binaural pan.
- **Four Dedicated Studio Sub-Tabs**:
  - **♫ PLAYER**: Now Playing card with dynamic title scaling, live animated visualizer, scrubber, playback controls, and quick DSP toggles.
  - **≡ TRACKS**: Smooth scrollable playlist with track numbers, titles, artists, and one-tap play buttons.
  - **🎛 LIVE FX**: Dedicated controls for 8D Binaural Audio rotation (with speed presets), Studio Reverb (Room, Cinema, Cathedral, GDH Reverb, Plate), and Muffle filters (Clean, Cozy, Party Next Door, Underwater, Telephone, Lo-Fi Tape, Vintage Radio, Space Echo).
  - **🎚 10-BAND EQ**: Full graphic equalizer with 10 frequency sliders (-9 dB to +9 dB) and quick presets (`Flat`, `Bass Boost`, `Vocal Boost`, `Treble`, `EDM / V-Shape`).
- **Modern Theme & Curved Corner Overhaul**:
  - Unified theme with `ModernTheme.hpp` (Deep Teal `#002D2D`, `#004444`, and Cream / Butter `#FFFFC0`).
  - Switched from blocky rects to smooth curved GD corners (`GJ_square01.png`, `GJ_square05.png`, `GJ_square02.png`).
  - Dark inset panel behind the content area eliminating any white window backgrounds.
  - Added dynamic title fitting (`fitLabel`) eliminating all text overlap and overflow.
  - Centralized resource configuration in `src/player/MusicPlayerTheme.hpp` for easy customization.
- **Cleanups**:
  - Removed obsolete web browser, file explorer, and terminal shell.


## Small Patch 1.4.6-beta 1

### Audio Quality Overhaul & New Features
- **8D Audio Effect**: Immersive 3D audio mode that smoothly rotates audio from left to right around the listener's head with a subtle hint of room reverb and distance modeling, recreating the popular 8D music effect from YouTube.
- **Reverb Overhaul**: Replaced the complex multi-parameter reverb setup with a single, clean **Reverb Strength** slider (in ms, 0 to 10000 ms). Internally balances wet/dry, diffusion, density, and reflections for pristine acoustics without clutter.
- **Equalizer Bass Distortion Fix**: Refined the parametric EQ filter curve (widened bandwidth Q from 1.0 to 0.6) and clamped sub-bass bands (30 Hz and 60 Hz to ±6 dB, others to ±9 dB) to eliminate clipping, resonance overshoots, and bass distortion.
- **Audio Filter Presets**: Added 12 handcrafted creative sound filters:
  - **Telephone**, **Underwater**, **Lo-Fi Tape**, **Vintage Radio**, **Megaphone**, **Stadium**, **Bedroom Studio**, **Concert Hall**, **Dark Room**, **Bright & Airy**, **Warm Tube**, and **Space Echo**.

### v1.4.6 — Camera Glass & Film Stock

- Added 5 new shaders with full settings, in-game popup rows, and render-pipeline wiring:
  - **Butter Camera** — warm, low-contrast colour grade with lifted blacks and soft creamy tones
  - **Raindrop** — dynamic glass distortion with animated water refraction
  - **Film Polaroid** — vintage analog look with film grain, a warm corner light leak and edge vignetting
  - **Fog** — soft atmospheric haze that desaturates and washes out distant layers
  - **Dust Film** — dirty camera lens with fingerprint grime, dust specks and lint
- Added 4 new **Bloom Profiles**: **Nature**, **Butter Glow**, **Nostalgic Bloom** and **Memories**
- New *Camera Glass & Film Stock* settings section in mod settings and the in-game Shaders tab
- All five shaders keep the pipeline invariants: no uniform is declared without being sampled, and none of them depend on the render resolution (the lens dirt, grain and speck fields are deliberately keyed to the real pixel grid so they look like physical dirt rather than screen-space noise)

- Reworked **Ambient Aurora** into layered emerald/teal/violet aurora curtains driven by a shared flow field, replacing the old flat colour tint
- Added an **Aurora Intensity** slider (`ambient-strength`) and exposed it in mod settings and the in-game settings popup
- Aurora brightness no longer scales with the render resolution, so the effect now looks identical at every render scale
- Added the missing **Ambient Aurora** toggle to the in-game Shaders tab
- Fixed **Sunset Atmosphere** declaring `u_invResolution` without ever sampling it, which made the driver strip the uniform and caused the whole post-process chain to fail to prepare while the effect was enabled

## v1.4.0

- Renamed mod to **RickGdps Menu Mod** by RickGdps
- Settings now use a custom **GD-styled popup UI** — brown panel with gold border, GD fonts, native GD toggles, sliders, and left/right arrow dropdowns
- Left-side tab navigation (Visual, Color, Camera, Retro, Audio) with right-side content panel
- Better Visual & FPS: improved render pipeline stability, frametime smoothing, and DRS response accuracy
- Updated quick-settings button in pause menu to use GD assets
- Added `"Better Visuals and FPS"` → `"RickGdps Menu Mod"` branding throughout in-game popups

## 2.0.0


- Added **Dynamic Bloom Automation**: Fast real-time intensity modulation and reactive pulsating bloom glow
- Added **Parallax Shader**: Pseudo-3D multi-layered depth warping and perspective layering
- Added **Split Screen View**: Horizontal (Top/Bottom), Vertical (Left/Right), and Quad (2x2) screen tiling with customizable borders
- Added **Animated LUT Transitions**: Smooth automatic crossfading between color grading palettes
- Updated **Camera Wobble**: Authentic handheld motion with low-frequency natural drift, breathing motion, and subtle hand micro-tremors
- Upgraded **Smoothed God Rays**: Jittered bilinear attenuation to eliminate color banding and improve visual softness
- Improved **Film Grain**: Stable multi-layer noise formula eliminating moiré patterns across resolutions
- Improved **Lens Flare Glow**: Added ghost reflection detection and refined thresholding
- Improved **Motion Blur**: Smooth Gaussian-weighted tap distribution
- Enhanced **Windows Fix & Stability**: Strict OpenGL framebuffer/viewport state guards, full-screen toggle handling, and shader pipeline safety
- Added **Global Shader Optimizations** and refined settings hierarchy

## 1.1.0

- Added **Dynamic Render Scaling (DRS)** with automatic target framerate detection and configurable response sensitivity
- Added **Super-Sampling Anti-Aliasing (SSAA 1.5x & 2.0x)** with FSR reconstruction
- Added upgraded studio reverb parameters: Early Delay, Late Delay, Modal Density, and Low-Frequency crossover
- Completely restructured `mod.json` into organized thematic categories with streamlined controls and enable-if constraints

## 1.0.1

- Reorganized Fun, Light & Atmosphere settings and moved Bloom/Combined presets to the top of the effects section
- Added extended reverb controls, including decay up to 5000 ms, wet/dry mix, diffusion, and high-frequency decay
- Improved film grain visibility and lens-flare bright-source detection
- Added Custom GLSL Shader side-loading from the mod persistent folder

## BetterVisual and Audio v1.0.0

- Created this project as an unofficial fork of [BetterVisuals by ItMe12s](https://github.com/ItMe12s/BetterVisuals/)
- Added a larger collection of fullscreen effects, including God Rays, depth focus, lens flare, cinematic LUT, ASCII, ambient aurora, and motion blur
- Added configurable effect controls, ten bloom presets, and ten combined-effect presets
- Reorganized visual-effect settings into clearer sections
- Added an in-game performance notice for demanding fullscreen effects
- Improved radial motion blur and updated God Rays and lens flare behavior

## 2.0.0

- Added master settings and render scale
- Added a button inside pause menus to open mod settings
- Added AMD FidelityFX FSR 1 (EASU) as an upscale method
- Fixed crash when changing fullscreen/windowed mode
- The mod now only works while in a level and the editor
- Optimized bloom

## 1.4.0

- Made bloom customizable and prettier

## 1.3.1

- New metadata for release

## 1.3.0

- Switched post-processing to scene visits

## 1.2.0

- Faster stacked effects with a single-copy renderer
- Fixed scaling, fullscreen, and cross-platform rendering

## 1.1.0

- Made stacked shaders run faster with ping-pong rendering
- Cleaned up the renderer to reduce unnecessary GPU work

## 1.0.0

- Initial release
