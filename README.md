# BetterVisual and Audio

BetterVisual and Audio is an unofficial fork of [BetterVisuals by ItMe12s](https://github.com/ItMe12s/BetterVisuals/). It keeps the original rendering and performance tools while expanding the collection of fullscreen visual effects and presets.

The mod works in levels and the editor. Configure it through Geode's mod settings or the pause-menu settings shortcut.

## Credits and fork status

- Original project: [BetterVisuals](https://github.com/ItMe12s/BetterVisuals/) by ItMe12s
- This fork: BetterVisual and Audio by ItzApril28
- Framebuffer and VBO handling references: [Geode DevTools](https://github.com/geode-sdk/DevTools)

This repository is a forked and customized version, not an official BetterVisuals release.

## Performance

Fullscreen post-processing can reduce frame rate, especially on mobile and lower-end devices. Use fewer simultaneous effects, lower render scale, or reduce effect strength if gameplay becomes slow.

Recommended baseline configurations:

- **Max FPS:** 0.5x render scale, Nearest upscaling, AA off, sharpening off.
- **Better FPS:** 0.7x render scale, FSR 1, AA off, sharpening off.
- **Better Visuals:** 1x render scale, SMAA High, sharpening off.

## Rendering tools

- Render scale with Nearest-neighbour or AMD FidelityFX FSR 1 upscaling
- FXAA, SMAA High, and SMAA Ultra anti-aliasing
- Bloom (with Dynamic Bloom automation) and ten bloom presets
- Parallax shader and Split Screen view modes (Horizontal, Vertical, Quad 2x2)
- Animated LUT transitions and color tonemapping
- Handheld camera wobble & shake, God Rays, film grain, lens flare glow, motion blur, and ambient aurora
- Chromatic aberration, neon pulse, radial blur, vignette, halftone, CRT, VHS, dithering, grayscale, and ASCII
- Combined theme presets and custom GLSL shader loading

Some effects may distort text and UI elements because they are applied to the full scene.

## Custom GLSL shaders

The **Custom GLSL Shader** setting loads `custom.glsl` from the mod's Geode persistent folder. The fragment shader must declare `u_texture`, `u_invResolution`, and `v_texCoord`, and must write `gl_FragColor`. The built-in fullscreen vertex shader is supplied automatically.

## Licenses and sources

Shader-specific credits, citations, and licenses are included in the source files where applicable. See the original [BetterVisuals repository](https://github.com/ItMe12s/BetterVisuals/) for its upstream project information.
