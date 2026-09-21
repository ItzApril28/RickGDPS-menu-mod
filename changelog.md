# Changelog

## Unreleased

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
