#include "BloomShader.hpp"

#include "../PostProcessShaders.hpp"

/*
 * Based on the bloom kernel from:
 * https://github.com/kiwipxl/GLSL-shaders/blob/master/bloom.glsl
 */

namespace bv::shaders::bloom {

    constexpr char kPrefilterSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_threshold;

vec3 brightPass(vec3 color) {
    float brightness = max(color.r, max(color.g, color.b));
    float knee = max(0.5 * (1.0 - u_threshold), 0.001);
    float soft = brightness - u_threshold + knee;
    soft = clamp(soft, 0.0, 2.0 * knee);
    soft = (soft * soft) / (4.0 * knee + 0.00001);
    float contribution = max(soft, brightness - u_threshold);
    contribution /= max(brightness, 0.00001);
    return color * clamp(contribution, 0.0, 1.0);
}

void main() {
    vec2 baseUv = gl_FragCoord.xy * 2.0 * u_invResolution;
    gl_FragColor = vec4(brightPass(texture2D(u_texture, baseUv).rgb), 1.0);
}
)glsl";

    constexpr char kBlurSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_texelStep;
varying vec2 v_texCoord;

void main() {
    vec3 bloom = texture2D(u_texture, v_texCoord).rgb * 0.121569;
    bloom += texture2D(u_texture, v_texCoord - u_texelStep).rgb * 0.116706;
    bloom += texture2D(u_texture, v_texCoord + u_texelStep).rgb * 0.116706;
    bloom += texture2D(u_texture, v_texCoord - u_texelStep * 2.0).rgb * 0.103256;
    bloom += texture2D(u_texture, v_texCoord + u_texelStep * 2.0).rgb * 0.103256;
    bloom += texture2D(u_texture, v_texCoord - u_texelStep * 3.0).rgb * 0.084195;
    bloom += texture2D(u_texture, v_texCoord + u_texelStep * 3.0).rgb * 0.084195;
    bloom += texture2D(u_texture, v_texCoord - u_texelStep * 4.0).rgb * 0.063270;
    bloom += texture2D(u_texture, v_texCoord + u_texelStep * 4.0).rgb * 0.063270;
    bloom += texture2D(u_texture, v_texCoord - u_texelStep * 5.0).rgb * 0.043819;
    bloom += texture2D(u_texture, v_texCoord + u_texelStep * 5.0).rgb * 0.043819;
    bloom += texture2D(u_texture, v_texCoord - u_texelStep * 6.0).rgb * 0.027969;
    bloom += texture2D(u_texture, v_texCoord + u_texelStep * 6.0).rgb * 0.027969;
    gl_FragColor = vec4(bloom, 1.0);
}
)glsl";

    constexpr char kCompositeSource[] = R"glsl(
uniform sampler2D u_source;
uniform sampler2D u_bloom;
uniform float u_intensity;
uniform float u_adaptive;
varying vec2 v_texCoord;

void main() {
    vec4 source = texture2D(u_source, v_texCoord);
    vec3 bloom = texture2D(u_bloom, v_texCoord).rgb;
    // Measure the extracted bloom itself, not the dark pixel currently being
    // composited. This keeps light halos visible around bright sources.
    float bloomLuminance = dot(bloom, vec3(0.299, 0.587, 0.114));
    float lightResponse = mix(1.0, mix(0.35, 1.0, smoothstep(0.01, 0.35, bloomLuminance)), u_adaptive);
    gl_FragColor = vec4(clamp(source.rgb + bloom * u_intensity * lightResponse, 0.0, 1.0), source.a);
}
)glsl";

} // namespace bv::shaders::bloom

namespace bv::shaders {

    BloomShaderSet const kBloomShaderSet{
        kFullscreenVertexSource,
        bloom::kPrefilterSource,
        bloom::kBlurSource,
        bloom::kCompositeSource,
    };

} // namespace bv::shaders
