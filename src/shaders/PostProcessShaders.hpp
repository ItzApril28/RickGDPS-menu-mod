#pragma once

namespace bv::render {
    struct PostProcessShader;
}

namespace bv::shaders {

    inline constexpr char kFullscreenVertexSource[] = R"glsl(
attribute vec2 a_position;
attribute vec2 a_texCoord;
varying vec2 v_texCoord;

void main() {
    v_texCoord = a_texCoord;
    gl_Position = vec4(a_position, 0.0, 1.0);
}
)glsl";

    extern render::PostProcessShader const kFxaaShader;
    extern render::PostProcessShader const kCasShader;
    extern render::PostProcessShader const kGrayscaleShader;
    extern render::PostProcessShader const kPixelateShader;
    extern render::PostProcessShader const kDitheringShader;
    extern render::PostProcessShader const kVhsShader;
    extern render::PostProcessShader const kCrtShader;
    extern render::PostProcessShader const kAberrationShader;
    extern render::PostProcessShader const kNeonShader;
    extern render::PostProcessShader const kRadialBlurShader;
    extern render::PostProcessShader const kVignetteShader;
    extern render::PostProcessShader const kHalftoneShader;
    extern render::PostProcessShader const kGodRaysShader;
    extern render::PostProcessShader const kWobbleShader;
    extern render::PostProcessShader const kParallaxShader;
    extern render::PostProcessShader const kSplitScreenShader;
    extern render::PostProcessShader const kSepiaShader;
    extern render::PostProcessShader const kPosterizeShader;
    extern render::PostProcessShader const kFilmGrainShader;
    extern render::PostProcessShader const kDepthFocusShader;
    extern render::PostProcessShader const kLensFlareShader;
    extern render::PostProcessShader const kAsciiShader;
    extern render::PostProcessShader const kCinematicLutShader;
    extern render::PostProcessShader const kAmbientShader;
    extern render::PostProcessShader const kMotionBlurShader;
    extern render::PostProcessShader const kColorGradeShader;
    extern render::PostProcessShader const kRenderScaleShader;
    extern render::PostProcessShader const kFsrShader;
    extern render::PostProcessShader const kDeathWarpShader;
    extern render::PostProcessShader const kFakeHdrShader;
    extern render::PostProcessShader const kSunsetShader;
    extern render::PostProcessShader const kButterCameraShader;
    extern render::PostProcessShader const kRaindropShader;
    extern render::PostProcessShader const kFilmPolaroidShader;
    extern render::PostProcessShader const kFogShader;
    extern render::PostProcessShader const kDustFilmShader;

} // namespace bv::shaders
