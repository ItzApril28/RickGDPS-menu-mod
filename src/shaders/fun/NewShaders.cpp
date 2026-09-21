#include "../../render/PostProcessRenderer.hpp"
#include "../PostProcessShaders.hpp"

namespace bv::shaders::aberration {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_strength;
varying vec2 v_texCoord;

void main() {
    vec2 dir = v_texCoord - 0.5;
    float r = length(dir);
    vec2 offset = dir * max(r * u_strength, max(u_invResolution.x, u_invResolution.y));

    float red   = texture2D(u_texture, v_texCoord - offset).r;
    float green = texture2D(u_texture, v_texCoord).g;
    float blue  = texture2D(u_texture, v_texCoord + offset).b;

    gl_FragColor = vec4(red, green, blue, texture2D(u_texture, v_texCoord).a);
}
)glsl";
} // namespace bv::shaders::aberration

namespace bv::shaders::neon {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform float u_time;
uniform vec2 u_invResolution;
varying vec2 v_texCoord;

vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main() {
    vec4 base = texture2D(u_texture, v_texCoord);
    float hue = fract(u_time + v_texCoord.x * (0.5 + u_invResolution.x) + v_texCoord.y * 0.3);
    vec3 rainbow = hsv2rgb(vec3(hue, 0.8, 1.0));

    vec3 neonColor = mix(base.rgb, base.rgb * rainbow * 1.5, 0.5);
    gl_FragColor = vec4(neonColor, base.a);
}
)glsl";
} // namespace bv::shaders::neon

namespace bv::shaders::radial_blur {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_strength;
varying vec2 v_texCoord;

void main() {
    vec2 center = vec2(0.5, 0.5);
    vec2 dir = v_texCoord - center;
    float dist = length(dir);

    float factor = smoothstep(0.2, 1.0, dist) * u_strength;
    vec2 step = dir * factor;

    vec4 color = vec4(0.0);
    for (int i = -7; i <= 7; i++) {
        vec2 sampleUv = v_texCoord + step * float(i) + u_invResolution * 0.001;
        color += texture2D(u_texture, sampleUv);
    }
    gl_FragColor = color / 15.0;
}
)glsl";
} // namespace bv::shaders::radial_blur

namespace bv::shaders::vignette {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_strength;
varying vec2 v_texCoord;

void main() {
    vec4 color = texture2D(u_texture, v_texCoord);
    vec2 uv = v_texCoord - 0.5;
    uv.x *= u_invResolution.y / u_invResolution.x;
    float len = length(uv);
    float vignette = smoothstep(0.8, 0.4, len * (1.0 + u_strength));
    color.rgb *= vignette;
    gl_FragColor = color;
}
)glsl";
} // namespace bv::shaders::vignette

namespace bv::shaders::halftone {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_scale;
varying vec2 v_texCoord;

void main() {
    vec4 color = texture2D(u_texture, v_texCoord);
    float luma = dot(color.rgb, vec3(0.299, 0.587, 0.114));

    vec2 resolution = 1.0 / u_invResolution;
    vec2 st = v_texCoord * (resolution / u_scale);
    vec2 nearest = floor(st) + 0.5;
    vec2 dist = st - nearest;
    float radius = length(dist) * 2.0;

    float threshold = sqrt(1.0 - luma);
    float dot = 1.0 - smoothstep(threshold - 0.1, threshold + 0.1, radius);

    vec3 finalColor = mix(vec3(0.0), color.rgb, dot);
    gl_FragColor = vec4(finalColor, color.a);
}
)glsl";
} // namespace bv::shaders::halftone

namespace bv::shaders::god_rays {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_strength;
varying vec2 v_texCoord;

float godRayNoise(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}

void main() {
    vec2 lightPosition = vec2(0.5, 0.5);
    vec2 delta = lightPosition - v_texCoord;
    float dist = length(delta);
    vec2 stepUv = delta * (u_strength / 32.0);
    
    // Dither starting step to eliminate color banding
    float jitter = godRayNoise(v_texCoord + u_invResolution) * 0.5;
    vec2 sampleUv = v_texCoord + stepUv * jitter;
    
    vec3 rays = vec3(0.0);
    float weight = 1.0;
    float totalWeight = 0.0;
    
    for (int i = 0; i < 32; i++) {
        vec3 s = texture2D(u_texture, clamp(sampleUv, 0.0, 1.0)).rgb;
        // Focus rays on high brightness areas
        float lum = dot(s, vec3(0.299, 0.587, 0.114));
        s *= smoothstep(0.3, 0.8, lum);
        rays += s * weight;
        totalWeight += weight;
        sampleUv += stepUv;
        weight *= 0.945;
    }
    
    rays /= max(totalWeight, 0.001);
    vec4 base = texture2D(u_texture, v_texCoord);
    vec3 radialGlow = vec3(1.0, 0.92, 0.78) * rays * u_strength * 0.95;
    gl_FragColor = vec4(base.rgb + radialGlow, base.a);
}
)glsl";
} // namespace bv::shaders::god_rays

namespace bv::shaders::wobble {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_time;
varying vec2 v_texCoord;

void main() {
    vec2 uv = v_texCoord;
    float t = u_time;
    
    // Natural handheld camera motion: low-frequency breath & drift
    float driftX = sin(t * 1.1) * 0.55 + sin(t * 2.3 + 1.2) * 0.35 + cos(t * 0.6 + 2.1) * 0.25;
    float driftY = cos(t * 0.95 + 0.8) * 0.5 + sin(t * 1.9 + 3.4) * 0.3 + sin(t * 0.5) * 0.2;
    
    // Hand tremors & shake
    float tremorX = (sin(t * 18.3) * cos(t * 23.7 + 0.5)) * 0.25;
    float tremorY = (cos(t * 19.1 + 0.3) * sin(t * 24.4)) * 0.25;
    
    // Subtle rotational shake around center
    vec2 centered = uv - 0.5;
    float rotAngle = (sin(t * 1.3) * 0.008 + sin(t * 15.5) * 0.0025);
    vec2 rotated = vec2(
        centered.x * cos(rotAngle) - centered.y * sin(rotAngle),
        centered.x * sin(rotAngle) + centered.y * cos(rotAngle)
    ) + 0.5;
    
    // Scale the movement in pixels so it stays subtle at every resolution.
    float pixel = max(u_invResolution.x, u_invResolution.y);
    vec2 offset = vec2(driftX + tremorX, driftY + tremorY) * pixel * 3.0;
    vec2 finalUv = rotated + offset;
    
    // Clamp within valid texture bounds
    gl_FragColor = texture2D(u_texture, clamp(finalUv, 0.0, 1.0));
}
)glsl";
} // namespace bv::shaders::wobble

namespace bv::shaders::parallax {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_depth;
uniform float u_time;
varying vec2 v_texCoord;

void main() {
    vec2 uv = v_texCoord;
    vec2 center = vec2(0.5, 0.5);
    vec2 dir = uv - center;
    float dist = length(dir);
    
    // Parallax depth layering effect (foreground vs midground vs background)
    float wave = sin(u_time * 1.8 + uv.y * 6.0) * 0.02;
    // Keep the simulated depth displacement resolution-aware.
    float pixel = max(u_invResolution.x, u_invResolution.y);
    float depthFactor = u_depth * (dist * 0.25 + wave) * (1.0 + pixel * 240.0);
    
    vec2 parallaxUv = uv + dir * depthFactor;
    
    // Chromatic separation based on simulated depth
    float r = texture2D(u_texture, clamp(parallaxUv + dir * (u_depth * 0.015), 0.0, 1.0)).r;
    float g = texture2D(u_texture, clamp(parallaxUv, 0.0, 1.0)).g;
    float b = texture2D(u_texture, clamp(parallaxUv - dir * (u_depth * 0.015), 0.0, 1.0)).b;
    float a = texture2D(u_texture, clamp(parallaxUv, 0.0, 1.0)).a;
    
    gl_FragColor = vec4(r, g, b, a);
}
)glsl";
} // namespace bv::shaders::parallax

namespace bv::shaders::split_screen {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_mode; // 0: Horizontal split (top/bottom), 1: Vertical split (left/right), 2: Quad (2x2)
uniform float u_borderWidth;
varying vec2 v_texCoord;

void main() {
    vec2 uv = v_texCoord;
    vec2 mappedUv = uv;
    bool isBorder = false;
    // Border width is expressed in screen pixels and therefore needs the
    // inverse resolution to remain stable on different displays.
    float borderThick = max(max(u_invResolution.x, u_invResolution.y),
                            u_borderWidth * max(u_invResolution.x, u_invResolution.y));
    
    if (u_mode < 0.5) {
        // Horizontal split (Top / Bottom duplicate)
        mappedUv.y = fract(uv.y * 2.0);
        if (abs(uv.y - 0.5) < borderThick) {
            isBorder = true;
        }
    } else if (u_mode < 1.5) {
        // Vertical split (Left / Right duplicate)
        mappedUv.x = fract(uv.x * 2.0);
        if (abs(uv.x - 0.5) < borderThick) {
            isBorder = true;
        }
    } else {
        // Quad 2x2 split
        mappedUv = fract(uv * 2.0);
        if (abs(uv.x - 0.5) < borderThick || abs(uv.y - 0.5) < borderThick) {
            isBorder = true;
        }
    }
    
    if (isBorder) {
        gl_FragColor = vec4(0.08, 0.08, 0.12, 1.0);
    } else {
        gl_FragColor = texture2D(u_texture, clamp(mappedUv, 0.0, 1.0));
    }
}
)glsl";
} // namespace bv::shaders::split_screen

namespace bv::shaders::sepia {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_strength;
varying vec2 v_texCoord;

void main() {
    vec4 color = texture2D(u_texture, v_texCoord);
    vec3 sepia = vec3(
        dot(color.rgb, vec3(0.393, 0.769, 0.189)),
        dot(color.rgb, vec3(0.349, 0.686, 0.168)),
        dot(color.rgb, vec3(0.272, 0.534, 0.131))
    );
    float grain = fract(sin(dot(v_texCoord + u_invResolution, vec2(12.9898, 78.233))) * 43758.5453);
    color.rgb = mix(color.rgb, clamp(sepia + (grain - 0.5) * 0.025, 0.0, 1.0), u_strength);
    gl_FragColor = color;
}
)glsl";
} // namespace bv::shaders::sepia

namespace bv::shaders::posterize {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_levels;
varying vec2 v_texCoord;

void main() {
    vec4 color = texture2D(u_texture, v_texCoord);
    float levels = max(2.0, u_levels + u_invResolution.x * 0.001);
    color.rgb = floor(color.rgb * (levels - 1.0) + 0.5) / (levels - 1.0);
    gl_FragColor = color;
}
)glsl";
} // namespace bv::shaders::posterize

namespace bv::shaders::film_grain {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_time;
uniform float u_strength;
varying vec2 v_texCoord;

// High quality, stable hash function without large precision overflow
float hash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

void main() {
    vec4 color = texture2D(u_texture, v_texCoord);

    // Resolution-aware coordinate calculation
    vec2 res = vec2(1.0 / max(u_invResolution.x, 0.0001), 1.0 / max(u_invResolution.y, 0.0001));
    vec2 coord = v_texCoord * res;

    // Time seed in a smooth loop
    float seed = fract(u_time * 23.47);

    // Multi-layer grain noise
    float fine = hash(coord + vec2(seed * 43.1, seed * 19.7)) - 0.5;
    float coarse = hash(floor(coord * 0.5) + vec2(seed * 29.3, seed * 71.9)) - 0.5;
    float grain = fine * 0.75 + coarse * 0.25;

    // Luminance-weighting: film grain is most organic in midtones
    float luma = dot(color.rgb, vec3(0.299, 0.587, 0.114));
    float lumaWeight = clamp(1.0 - abs(luma - 0.5) * 1.5, 0.25, 1.0);

    float amount = u_strength * lumaWeight * 0.65;
    color.rgb = clamp(color.rgb + grain * amount, 0.0, 1.0);
    gl_FragColor = color;
}
)glsl";
} // namespace bv::shaders::film_grain

namespace bv::shaders::depth_focus {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_strength;
varying vec2 v_texCoord;
void main() {
    float focus = 0.52;
    float blur = smoothstep(0.08, 0.45, abs(v_texCoord.y - focus)) * u_strength;
    vec2 stepUv = vec2(u_invResolution.x * blur * 12.0, 0.0);
    vec4 color = texture2D(u_texture, v_texCoord) * 0.28;
    color += texture2D(u_texture, v_texCoord - stepUv) * 0.18;
    color += texture2D(u_texture, v_texCoord + stepUv) * 0.18;
    color += texture2D(u_texture, v_texCoord - stepUv * 2.0) * 0.18;
    color += texture2D(u_texture, v_texCoord + stepUv * 2.0) * 0.18;
    gl_FragColor = color;
}
)glsl";
} // namespace bv::shaders::depth_focus

namespace bv::shaders::lens_flare {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_strength;
varying vec2 v_texCoord;

vec3 getBright(vec2 uv) {
    vec3 c = texture2D(u_texture, clamp(uv, 0.0, 1.0)).rgb;
    float lum = dot(c, vec3(0.2126, 0.7152, 0.0722));
    float threshold = 0.68;
    return c * max(0.0, lum - threshold) / max(lum, 0.0001);
}

void main() {
    vec4 base = texture2D(u_texture, v_texCoord);
    vec2 center = vec2(0.5, 0.5);
    vec2 uv = v_texCoord;
    vec2 toCenter = center - uv;
    vec2 stepDir = normalize(toCenter + vec2(0.00001));

    // 1. Anamorphic horizontal streak with chromatic dispersion
    vec3 streak = vec3(0.0);
    for (int i = -8; i <= 8; i++) {
        float fi = float(i);
        float weight = exp(-0.06 * fi * fi);
        vec2 streakUv = uv + vec2(fi * u_invResolution.x * 4.5, 0.0);
        float r = getBright(streakUv + vec2(u_invResolution.x * 2.0, 0.0)).r;
        float g = getBright(streakUv).g;
        float b = getBright(streakUv - vec2(u_invResolution.x * 2.0, 0.0)).b;
        streak += vec3(r, g, b) * weight;
    }
    streak *= 0.11;
    vec3 streakColor = vec3(0.35, 0.65, 1.0) * streak;

    // 2. Multi-element optical ghost reflections along center vector
    vec3 ghosts = vec3(0.0);
    for (int n = 1; n <= 4; n++) {
        float fn = float(n);
        // Keep ghost samples inside the image. Wrapping them with fract caused
        // bright pixels at one edge to abruptly reappear on the opposite edge.
        float scale = 1.0 - fn * 0.18;
        vec2 gUv = center + (uv - center) * scale;
        float d = length(uv - center);
        float weight = 1.0 - smoothstep(0.18, 0.72, d);
        vec3 gBright = getBright(gUv);
        vec3 tint = mix(vec3(0.3, 0.8, 1.0), vec3(1.0, 0.4, 0.2), float(n) / 4.0);
        ghosts += gBright * tint * weight * 0.35;
    }

    // 3. Diffraction ring (halo)
    float haloDist = length(toCenter);
    float haloWeight = 1.0 - smoothstep(0.0, 0.10, abs(haloDist - 0.38));
    vec2 haloUv = uv + stepDir * 0.38;
    vec3 halo = getBright(haloUv) * vec3(0.8, 0.45, 1.0) * haloWeight * 0.28;

    // 4. Central starburst / corona glow
    vec3 brightHere = getBright(uv);
    vec3 starburst = brightHere * vec3(1.0, 0.9, 0.7) * 0.45;

    vec3 flareResult = (streakColor + ghosts + halo + starburst) * u_strength;
    gl_FragColor = vec4(base.rgb + flareResult, base.a);
}
)glsl";
} // namespace bv::shaders::lens_flare

namespace bv::shaders::fake_hdr {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_strength;
varying vec2 v_texCoord;

void main() {
    vec4 color = texture2D(u_texture, v_texCoord);
    vec3 c = color.rgb;

    // Local unsharp masking for micro-contrast enhancement
    vec2 off = u_invResolution * 1.5;
    vec3 blur = (
        texture2D(u_texture, v_texCoord + vec2( off.x, 0.0)).rgb +
        texture2D(u_texture, v_texCoord + vec2(-off.x, 0.0)).rgb +
        texture2D(u_texture, v_texCoord + vec2(0.0,  off.y)).rgb +
        texture2D(u_texture, v_texCoord + vec2(0.0, -off.y)).rgb
    ) * 0.25;

    // High frequency micro-contrast
    vec3 detail = c - blur;
    vec3 sharp = c + detail * (0.85 * u_strength);

    // ACES-inspired Reinhard hybrid tone-mapping curve for expanded dynamic range
    float lum = dot(sharp, vec3(0.2126, 0.7152, 0.0722));
    float whitePoint = 1.35;
    vec3 hdr = sharp * (1.0 + sharp / (whitePoint * whitePoint)) / (1.0 + sharp);

    // Shadow recovery & highlight compression
    vec3 shadowLift = max(vec3(0.0), 0.08 * (1.0 - c) * u_strength);
    hdr += shadowLift;

    // Vibrance enhancement on midtones
    float hdrLum = dot(hdr, vec3(0.2126, 0.7152, 0.0722));
    hdr = mix(vec3(hdrLum), hdr, 1.0 + 0.22 * u_strength);

    gl_FragColor = vec4(clamp(mix(c, hdr, u_strength), 0.0, 1.0), color.a);
}
)glsl";
} // namespace bv::shaders::fake_hdr

namespace bv::shaders::sunset {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_strength;
varying vec2 v_texCoord;

void main() {
    vec4 color = texture2D(u_texture, v_texCoord);
    vec3 c = color.rgb;

    // Vertical atmospheric golden-hour gradient
    float gradY = v_texCoord.y;
    vec3 skyTop = vec3(0.58, 0.22, 0.55);     // Twilight dusk purple
    vec3 skyHorizon = vec3(1.0, 0.45, 0.12); // Burning orange sunset
    vec3 skyGround = vec3(0.95, 0.75, 0.35);  // Warm golden amber
    vec3 atmosphericTint = mix(skyGround, mix(skyHorizon, skyTop, smoothstep(0.40, 1.0, gradY)), smoothstep(0.0, 0.55, gradY));

    // Warmth color balance (push reds & ambers, drop cold blues)
    vec3 sunsetColor = c;
    sunsetColor.r = pow(sunsetColor.r, 0.88);
    sunsetColor.g = pow(sunsetColor.g, 0.94);
    sunsetColor.b = pow(sunsetColor.b, 1.25);

    // Soft light / overlay blend with atmospheric sunset tint
    vec3 blended = mix(sunsetColor, sunsetColor * atmosphericTint * 1.35, 0.45);

    // Warm golden highlight glow
    float lum = dot(c, vec3(0.299, 0.587, 0.114));
    float highlight = smoothstep(0.55, 1.0, lum);
    blended += vec3(1.0, 0.58, 0.18) * highlight * 0.32;

    // A wide, low-contrast colour gradient like this one bands easily, so dither
    // it by a fraction of one pixel. This also keeps the gradient tied to the
    // real pixel grid: u_invResolution used to be declared but never sampled, so
    // the driver stripped it and the renderer's uniform lookup failed, which
    // disabled Sunset Atmosphere (and the whole chain it sits in) at runtime.
    vec2 pixel = v_texCoord / max(u_invResolution, vec2(0.00001));
    float dither = fract(sin(dot(pixel, vec2(12.9898, 78.233))) * 43758.5453) - 0.5;

    vec3 graded = clamp(mix(c, blended, u_strength) + dither * u_strength * 0.012, 0.0, 1.0);
    gl_FragColor = vec4(graded, color.a);
}
)glsl";
} // namespace bv::shaders::sunset


namespace bv::shaders::ascii {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_scale;
varying vec2 v_texCoord;
float glyph(vec2 p, float level) {
    p = floor(p * 5.0);
    float lineA = step(3.0, level) * step(abs(p.x - p.y), 0.1);
    float lineB = step(5.0, level) * step(abs(p.x - (4.0 - p.y)), 0.1);
    float bar = step(1.0, level) * step(abs(p.y - 2.0), 0.1);
    float stem = step(4.0, level) * step(abs(p.x - 2.0), 0.1);
    return clamp(max(max(lineA, lineB), max(bar, stem)), 0.0, 1.0);
}
void main() {
    vec2 grid = vec2(u_scale, u_scale * u_invResolution.x / u_invResolution.y);
    vec2 cell = floor(v_texCoord * grid);
    vec2 local = fract(v_texCoord * grid);
    vec3 source = texture2D(u_texture, (cell + 0.5) / grid).rgb;
    float luma = dot(source, vec3(0.299, 0.587, 0.114));
    float mark = glyph(local, floor(luma * 7.0));
    gl_FragColor = vec4(source * mark, 1.0);
}
)glsl";
} // namespace bv::shaders::ascii

namespace bv::shaders::cinematic_lut {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_strength;
varying vec2 v_texCoord;
void main() {
    vec4 color = texture2D(u_texture, v_texCoord);
    vec3 graded = color.rgb;
    graded = (graded - 0.5) * 1.18 + 0.5;
    float shadows = 1.0 - max(max(graded.r, graded.g), graded.b);
    float highlights = min(max(max(graded.r, graded.g), graded.b), 1.0);
    graded += vec3(-0.05, 0.07, 0.10) * shadows;
    graded += vec3(0.10, 0.035, -0.04) * highlights;
    float vignette = smoothstep(0.9, 0.25, length((v_texCoord - 0.5) * vec2(u_invResolution.y / u_invResolution.x, 1.0)));
    graded *= mix(0.82, 1.0, vignette);
    gl_FragColor = vec4(mix(color.rgb, clamp(graded, 0.0, 1.0), u_strength), color.a);
}
)glsl";
} // namespace bv::shaders::cinematic_lut

/*
 * Ambient Aurora
 *
 * The earlier revision was a flat two-colour tint whose strength drifted with
 * the render resolution (1.0 + u_invResolution.x * 10.0) and had no user
 * control. This version builds real aurora curtains: a shared flow field pushes
 * three coloured ribbons sideways while thin strands run along their length,
 * and the glow is biased towards the darker parts of the frame so it reads as
 * light in the sky instead of a colour wash.
 *
 * The shapes come from resolution independent noise, so the curtains keep the
 * same physical size at every render scale.
 */
namespace bv::shaders::ambient {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_time;
uniform float u_strength;
varying vec2 v_texCoord;

float auroraHash(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float auroraNoise(vec2 p) {
    vec2 cell = floor(p);
    vec2 f = fract(p);
    vec2 weight = f * f * (3.0 - 2.0 * f);
    float a = auroraHash(cell);
    float b = auroraHash(cell + vec2(1.0, 0.0));
    float c = auroraHash(cell + vec2(0.0, 1.0));
    float d = auroraHash(cell + vec2(1.0, 1.0));
    return mix(mix(a, b, weight.x), mix(c, d, weight.x), weight.y);
}

// Three octaves are enough for the soft, low frequency shapes an aurora has,
// and they keep the pass affordable next to the cheap tint it replaces.
float auroraFbm(vec2 p) {
    float sum = 0.0;
    float amplitude = 0.55;
    for (int octave = 0; octave < 3; ++octave) {
        sum += auroraNoise(p) * amplitude;
        p = p * 2.07 + vec2(13.1, 7.7);
        amplitude *= 0.5;
    }
    return sum;
}

// A single vertical curtain: a soft ribbon whose centre is displaced sideways by
// the shared flow field, with thin strands running along its length.
float auroraRibbon(vec2 uv, float flow, float detail, float offset, float width) {
    float sway = (flow - 0.5) * 0.5 + (detail - 0.5) * 0.15;
    float x = (uv.x - 0.5 - offset + sway) / width;
    float ribbon = exp(-x * x * 3.2);
    float strands = 0.4 + 0.6 * smoothstep(0.1, 0.9, detail + 0.22 * sin(uv.y * 9.0 + flow * 6.0));
    return ribbon * strands;
}
void main() {
    vec2 uv = clamp(v_texCoord, 0.0, 1.0);

    float t = u_time * 0.11;
    float flow = auroraFbm(vec2(uv.y * 1.7 - t, t * 0.6));
    float detail = auroraFbm(vec2(uv.x * 2.6 + t * 0.4, uv.y * 5.5 - t * 1.1));

    // Auroras hang from the top of the sky and fade out before the ground.
    float vertical = smoothstep(0.04, 0.62, uv.y);

    float emerald = auroraRibbon(uv, flow, detail, -0.22, 0.15);
    float teal = auroraRibbon(uv, flow, detail, 0.05, 0.20);
    float violet = auroraRibbon(uv, flow, detail, 0.30, 0.13);

    vec3 auroraColor =
        vec3(0.24, 1.00, 0.52) * emerald +
        vec3(0.16, 0.82, 0.95) * teal +
        vec3(0.72, 0.30, 1.00) * violet;

    vec4 color = texture2D(u_texture, uv);

    // Bias the aura towards the shadows so it lifts the sky rather than washing
    // out geometry that is already bright.
    float luma = dot(color.rgb, vec3(0.2126, 0.7152, 0.0722));
    float shadowBias = mix(1.0, 0.18, smoothstep(0.35, 0.95, luma));

    // A single noise tap in screen space breaks up gradient banding and keeps
    // the grain locked to the real pixel grid at any render scale.
    vec2 pixel = uv / max(u_invResolution, vec2(0.00001));
    float dither = (auroraHash(pixel) - 0.5) * 0.02;

    vec3 aura = auroraColor * vertical * shadowBias * u_strength * 1.35;
    color.rgb = clamp(color.rgb + aura + dither * u_strength, 0.0, 1.0);
    gl_FragColor = color;
}
)glsl";
} // namespace bv::shaders::ambient

namespace bv::shaders::motion_blur {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_strength;
varying vec2 v_texCoord;
void main() {
    vec2 velocity = vec2(u_strength * 26.0 * u_invResolution.x, 0.0);
    vec4 color = vec4(0.0);
    float totalWeight = 0.0;
    
    // Gaussian-weighted tap distribution for smoother motion blur trails
    color += texture2D(u_texture, v_texCoord - velocity * 4.0) * 0.05;
    color += texture2D(u_texture, v_texCoord - velocity * 3.0) * 0.09;
    color += texture2D(u_texture, v_texCoord - velocity * 2.0) * 0.12;
    color += texture2D(u_texture, v_texCoord - velocity * 1.0) * 0.15;
    color += texture2D(u_texture, v_texCoord) * 0.18;
    color += texture2D(u_texture, v_texCoord + velocity * 1.0) * 0.15;
    color += texture2D(u_texture, v_texCoord + velocity * 2.0) * 0.12;
    color += texture2D(u_texture, v_texCoord + velocity * 3.0) * 0.09;
    color += texture2D(u_texture, v_texCoord + velocity * 4.0) * 0.05;
    
    gl_FragColor = color;
}
)glsl";
} // namespace bv::shaders::motion_blur

namespace bv::shaders::color_grade {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_lut;
uniform float u_brightness;
uniform float u_contrast;
uniform float u_saturation;
uniform float u_temperature;
varying vec2 v_texCoord;

vec3 applyLutTone(vec3 c, float lutIdx) {
    if (lutIdx < 0.5) { // Vibrant
        c = (c - 0.5) * 1.12 + 0.5;
        c = mix(vec3(dot(c, vec3(0.299, 0.587, 0.114))), c, 1.22);
    } else if (lutIdx < 1.5) { // Teal & Orange
        float l = dot(c, vec3(0.299, 0.587, 0.114));
        c += mix(vec3(-0.06, 0.08, 0.10), vec3(0.12, 0.035, -0.05), l);
    } else if (lutIdx < 2.5) { // Warm Film
        c = (c - 0.5) * 1.08 + 0.5 + vec3(0.07, 0.025, -0.03);
    } else if (lutIdx < 3.5) { // Cool Night
        c = (c - 0.5) * 1.16 + 0.5 + vec3(-0.045, 0.015, 0.09);
    } else { // Pastel
        c = mix(c, vec3(dot(c, vec3(0.299, 0.587, 0.114))), 0.24);
        c = c * 0.82 + vec3(0.12, 0.08, 0.14);
    }
    return c;
}

void main() {
    vec4 color = texture2D(u_texture, v_texCoord);
    vec3 c = color.rgb;
    
    // Smooth transition interpolation if animated LUT is active
    float baseLut = floor(u_lut);
    float nextLut = mod(baseLut + 1.0, 5.0);
    float progress = fract(u_lut);
    
    vec3 c1 = applyLutTone(c, baseLut);
    vec3 c2 = applyLutTone(c, nextLut);
    c = mix(c1, c2, progress);
    
    c = (c - 0.5) * (1.0 + u_contrast) + 0.5 + u_brightness;
    float luminance = dot(c, vec3(0.299, 0.587, 0.114));
    c = mix(vec3(luminance), c, 1.0 + u_saturation);
    c += vec3(u_temperature, 0.0, -u_temperature);
    c += u_invResolution.x * 0.0001;
    gl_FragColor = vec4(clamp(c, 0.0, 1.0), color.a);
}
)glsl";
} // namespace bv::shaders::color_grade

namespace bv::shaders::death_warp {
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_progress;
uniform float u_centerX;
uniform float u_centerY;
varying vec2 v_texCoord;

void main() {
    vec2 uv = v_texCoord;
    vec2 center = vec2(u_centerX, u_centerY);
    if (center.x <= 0.0 || center.y <= 0.0 || center.x >= 1.0 || center.y >= 1.0) {
        center = vec2(0.5, 0.5);
    }
    
    // Correct aspect ratio for circular shockwave
    float aspect = (u_invResolution.y > 0.00001) ? (u_invResolution.x / u_invResolution.y) : 1.0;
    vec2 diff = uv - center;
    diff.x /= max(aspect, 0.0001);
    float dist = length(diff);
    
    // Expand ring from player outwards (progress: 0.0 -> 1.0)
    float waveRadius = u_progress * 1.35;
    float waveWidth = 0.16;
    float waveDist = abs(dist - waveRadius);
    
    // Smooth bell-curve envelope for the wave
    float envelope = smoothstep(waveWidth, 0.0, waveDist) * (1.0 - u_progress);
    
    // S-curve displacement for a genuine organic ripple/warp
    vec2 dir = normalize(uv - center + vec2(0.00001));
    float disp = sin(clamp(waveDist / waveWidth, 0.0, 1.0) * 3.14159265) * envelope * 0.055;
    
    // Chromatic split along warp direction (Red outward, Blue inward, Green base)
    vec2 uvR = clamp(uv + dir * disp * 1.35, 0.0, 1.0);
    vec2 uvG = clamp(uv + dir * disp * 1.00, 0.0, 1.0);
    vec2 uvB = clamp(uv + dir * disp * 0.65, 0.0, 1.0);
    
    float r = texture2D(u_texture, uvR).r;
    float g = texture2D(u_texture, uvG).g;
    float b = texture2D(u_texture, uvB).b;
    
    // Clean color output: pure geometric distortion & chromatic aberration, no blinding flash
    gl_FragColor = vec4(r, g, b, 1.0);
}
)glsl";
} // namespace bv::shaders::death_warp

namespace bv::shaders::butter_camera {
    // Warm, low-contrast colour grade with lifted blacks and creamy tones.
    // Strength 0 is an exact identity pass.
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_strength;
varying vec2 v_texCoord;

void main() {
    vec4 src = texture2D(u_texture, v_texCoord);
    vec3 color = src.rgb;

    // Compress contrast toward the mean, then lift the black point so shadows
    // turn milky instead of crushed - the creamy analogue film response.
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    vec3 softened = mix(vec3(luma), color, mix(1.0, 0.82, u_strength));
    vec3 lifted = mix(softened, softened * 0.90 + 0.075, u_strength);

    // Warm amber-white balance.
    vec3 warm = vec3(1.055, 1.012, 0.945);
    vec3 graded = mix(lifted, lifted * warm, u_strength);

    // Dither keyed to the real pixel grid so the lifted blacks do not band on
    // 8-bit output. Gated by strength so the pass is a true no-op at zero.
    float dither = fract(sin(dot(v_texCoord * u_invResolution, vec2(12.9898, 78.233))) * 43758.5453);
    graded += (dither - 0.5) * u_strength / 255.0;

    gl_FragColor = vec4(clamp(graded, 0.0, 1.0), src.a);
}
)glsl";
} // namespace bv::shaders::butter_camera

namespace bv::shaders::raindrop {
    // Animated water droplets refracting the scene like glass on the lens.
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_time;
uniform float u_strength;
varying vec2 v_texCoord;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

// Three drifting layers of droplet cells. Only some cells spawn a droplet and
// each one offsets the lookup toward its rim, which is what makes the glass
// edge bend light harder than the flat centre.
vec2 dropletRefraction(vec2 uv, float t) {
    vec2 offset = vec2(0.0);
    for (int layer = 0; layer < 3; ++layer) {
        float scale = 7.0 + float(layer) * 6.0;
        vec2 drift = vec2(t * (0.05 + float(layer) * 0.03), -t * (0.11 + float(layer) * 0.05));
        vec2 p = uv * scale + drift;
        vec2 cell = floor(p);
        vec2 local = fract(p) - 0.5;
        float seed = hash(cell + float(layer) * 19.7);
        if (seed > 0.62) {
            vec2 center = (vec2(hash(cell + 1.3), hash(cell + 7.7)) - 0.5) * 0.55;
            vec2 delta = local - center;
            float radius = 0.16 + seed * 0.16;
            float dist = length(delta);
            if (dist < radius) {
                float falloff = 1.0 - dist / radius;
                vec2 dir = dist > 0.0001 ? delta / dist : vec2(0.0);
                offset += dir * falloff * falloff * (0.05 + 0.05 * falloff);
            }
        }
    }
    return offset;
}

void main() {
    vec2 uv = v_texCoord;
    vec2 offset = dropletRefraction(uv, u_time) * u_strength * 12.0;

    // Glass dispersion: red and blue bend slightly differently, and the split
    // is measured in real pixels so it stays consistent at every resolution.
    float dispersion = 3.0 / max(u_invResolution.y, 1.0);
    vec3 color = vec3(0.0);
    color.r = texture2D(u_texture, clamp(uv + offset * (1.0 + dispersion), 0.0, 1.0)).r;
    color.g = texture2D(u_texture, clamp(uv + offset, 0.0, 1.0)).g;
    color.b = texture2D(u_texture, clamp(uv + offset * (1.0 - dispersion), 0.0, 1.0)).b;

    // Wet specular sheen along the droplet rims.
    float sheen = length(offset);
    color += vec3(0.85, 0.92, 1.0) * min(sheen * 3.0, 1.0) * 0.10 * u_strength;

    gl_FragColor = vec4(clamp(color, 0.0, 1.0), texture2D(u_texture, clamp(uv + offset, 0.0, 1.0)).a);
}
)glsl";
} // namespace bv::shaders::raindrop

namespace bv::shaders::film_polaroid {
    // Vintage instant-film look: milky warm stock, soft corner vignette, a warm
    // light leak bleeding in from one corner and animated film grain.
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_time;
uniform float u_strength;
varying vec2 v_texCoord;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

void main() {
    vec2 uv = v_texCoord;
    vec4 src = texture2D(u_texture, uv);
    vec3 color = src.rgb;

    // Instant-film stock: gently reduced contrast, warm and slightly milky.
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    vec3 stock = mix(vec3(luma), color, 0.86) * 0.94 + 0.045;
    color = mix(color, stock * vec3(1.035, 1.005, 0.955), u_strength);

    // Soft edge vignette - far stronger than the cinematic vignette pass, which
    // is what gives the print its rounded instant-film framing.
    vec2 centered = (uv - 0.5) * 2.0;
    float radius = length(centered * vec2(1.0, 0.94));
    float vignette = smoothstep(1.05, 0.35, radius);
    color *= mix(1.0, 0.62 + 0.38 * vignette, u_strength);

    // Warm light leak bleeding in from the top-right corner.
    float corner = length(uv - vec2(1.02, 1.06));
    float leak = smoothstep(0.78, 0.0, corner);
    color += vec3(1.0, 0.55, 0.22) * leak * 0.16 * u_strength;

    // Animated film grain, keyed to the real pixel grid so the grain size is
    // independent of the render resolution.
    float grain = hash(uv * u_invResolution + fract(u_time) * 71.3) - 0.5;
    color += grain * 0.055 * u_strength;

    gl_FragColor = vec4(clamp(color, 0.0, 1.0), src.a);
}
)glsl";
} // namespace bv::shaders::film_polaroid

namespace bv::shaders::fog {
    // Soft atmospheric haze: drifting banks that desaturate and wash out the
    // scene with distance.
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_time;
uniform float u_strength;
varying vec2 v_texCoord;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 w = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), w.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), w.x), w.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int octave = 0; octave < 3; ++octave) {
        value += amplitude * noise(p);
        p *= 2.03;
        amplitude *= 0.5;
    }
    return value;
}

void main() {
    vec2 uv = v_texCoord;
    vec4 src = texture2D(u_texture, uv);
    vec3 color = src.rgb;

    // Drifting haze banks: the domain is warped by a second fbm so the banks
    // curl instead of sliding as a flat sheet.
    vec2 p = uv * vec2(2.6, 1.7);
    p.x += u_time * 0.02;
    float density = fbm(p + fbm(p * 0.6) * 0.5);

    // Haze settles toward the bottom of the frame.
    density *= mix(0.55, 1.0, smoothstep(0.0, 1.0, 1.0 - uv.y));
    float amount = clamp(density * 0.55 * u_strength, 0.0, 0.85);

    // Distance desaturation happens before the fog blends in.
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    vec3 desaturated = mix(color, vec3(luma), amount * 0.5);
    color = mix(desaturated, vec3(0.74, 0.78, 0.82), amount);

    // A touch of grain keeps the haze from banding on 8-bit output.
    float grain = hash(uv * u_invResolution) - 0.5;
    color += grain * 0.012 * u_strength;

    gl_FragColor = vec4(clamp(color, 0.0, 1.0), src.a);
}
)glsl";
} // namespace bv::shaders::fog

namespace bv::shaders::dust_film {
    // Dirty camera lens: fingerprint grime mottling plus dust specks and lint
    // that read as physical lens dirt because they are keyed to the pixel grid.
    constexpr char kFragmentSource[] = R"glsl(
uniform sampler2D u_texture;
uniform vec2 u_invResolution;
uniform float u_time;
uniform float u_strength;
varying vec2 v_texCoord;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 w = f * f * (3.0 - 2.0 * f);
    return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), w.x),
               mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), w.x), w.y);
}

float fbm(vec2 p) {
    float value = 0.0;
    float amplitude = 0.5;
    for (int octave = 0; octave < 3; ++octave) {
        value += amplitude * noise(p);
        p *= 2.03;
        amplitude *= 0.5;
    }
    return value;
}

// Sparse speck field: only cells above the threshold carry a speck, so the dirt
// stays irregular instead of tiling into a visible grid.
float speckField(vec2 p, float threshold) {
    vec2 cell = floor(p);
    vec2 local = fract(p) - 0.5;
    float seed = hash(cell);
    if (seed < threshold) return 0.0;
    vec2 center = (vec2(hash(cell + 3.1), hash(cell + 9.7)) - 0.5) * 0.7;
    float radius = 0.03 + hash(cell + 17.3) * 0.05;
    return smoothstep(radius, 0.0, length(local - center));
}

void main() {
    vec2 uv = v_texCoord;
    vec4 src = texture2D(u_texture, uv);
    vec3 color = src.rgb;

    // Fingerprint grime: slow cloudy mottling, drifting just perceptibly.
    float grime = fbm(uv * vec2(3.4, 2.6) + u_time * 0.004);
    grime = smoothstep(0.45, 0.85, grime);

    // Dust and lint, sized from the real pixel grid so it looks like dirt on the
    // glass rather than screen-space noise.
    vec2 speckUv = uv * u_invResolution * 0.28;
    float specks = speckField(speckUv, 0.955) + 0.6 * speckField(speckUv * 2.1 + 31.7, 0.97);

    // Where the grime is thickest it hazes the image and slightly warms it.
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    float grimeAmount = grime * 0.30 * u_strength;
    color = mix(color, mix(color, vec3(luma) * 1.06, 0.55), grimeAmount);

    // Specks scatter a little warm light and block a little more.
    color = mix(color, color * vec3(1.06, 1.02, 0.94), specks * 0.35 * u_strength);
    color -= specks * 0.10 * u_strength;

    gl_FragColor = vec4(clamp(color, 0.0, 1.0), src.a);
}
)glsl";
} // namespace bv::shaders::dust_film

namespace bv::shaders {

    render::PostProcessShader const kAberrationShader{
        "Chromatic Aberration",
        kFullscreenVertexSource,
        aberration::kFragmentSource,
        "u_strength",
    };

    render::PostProcessShader const kNeonShader{
        "Neon Rainbow Pulse",
        kFullscreenVertexSource,
        neon::kFragmentSource,
        "u_time",
    };

    render::PostProcessShader const kRadialBlurShader{
        "Radial Motion Blur",
        kFullscreenVertexSource,
        radial_blur::kFragmentSource,
        "u_strength",
    };

    render::PostProcessShader const kVignetteShader{
        "Cinematic Vignette",
        kFullscreenVertexSource,
        vignette::kFragmentSource,
        "u_strength",
    };

    render::PostProcessShader const kHalftoneShader{
        "Halftone Matrix",
        kFullscreenVertexSource,
        halftone::kFragmentSource,
        "u_scale",
    };

    render::PostProcessShader const kGodRaysShader{
        "God Rays",
        kFullscreenVertexSource,
        god_rays::kFragmentSource,
        "u_strength",
    };

    render::PostProcessShader const kWobbleShader{
        "Camera Wobble",
        kFullscreenVertexSource,
        wobble::kFragmentSource,
        "u_time",
    };

    render::PostProcessShader const kParallaxShader{
        "Parallax Shader",
        kFullscreenVertexSource,
        parallax::kFragmentSource,
        "u_depth",
        "u_time",
    };

    render::PostProcessShader const kSplitScreenShader{
        "Split Screen",
        kFullscreenVertexSource,
        split_screen::kFragmentSource,
        "u_mode",
        "u_borderWidth",
    };

    render::PostProcessShader const kSepiaShader{
        "Sepia",
        kFullscreenVertexSource,
        sepia::kFragmentSource,
        "u_strength",
    };

    render::PostProcessShader const kPosterizeShader{
        "Posterize",
        kFullscreenVertexSource,
        posterize::kFragmentSource,
        "u_levels",
    };

    render::PostProcessShader const kFilmGrainShader{
        "Film Grain",
        kFullscreenVertexSource,
        film_grain::kFragmentSource,
        "u_time",
        "u_strength",
    };

    render::PostProcessShader const kDepthFocusShader{"Depth Focus", kFullscreenVertexSource, depth_focus::kFragmentSource, "u_strength"};
    render::PostProcessShader const kLensFlareShader{"Lens Flare Glow", kFullscreenVertexSource, lens_flare::kFragmentSource, "u_strength"};
    render::PostProcessShader const kAsciiShader{"ASCII", kFullscreenVertexSource, ascii::kFragmentSource, "u_scale"};
    render::PostProcessShader const kCinematicLutShader{"Cinematic LUT", kFullscreenVertexSource, cinematic_lut::kFragmentSource, "u_strength"};
    render::PostProcessShader const kAmbientShader{
        "Ambient Aurora", kFullscreenVertexSource, ambient::kFragmentSource,
        "u_time", "u_strength"
    };
    render::PostProcessShader const kMotionBlurShader{"Motion Blur", kFullscreenVertexSource, motion_blur::kFragmentSource, "u_strength"};
    render::PostProcessShader const kColorGradeShader{
        "Color Edit", kFullscreenVertexSource, color_grade::kFragmentSource,
        "u_lut", "u_brightness", "u_contrast", "u_saturation", "u_temperature"
    };

    render::PostProcessShader const kFakeHdrShader{"Fake HDR", kFullscreenVertexSource, fake_hdr::kFragmentSource, "u_strength"};
    render::PostProcessShader const kSunsetShader{"Sunset Atmosphere", kFullscreenVertexSource, sunset::kFragmentSource, "u_strength"};

    // v1.4.6 lineup - film-emulation grades and lens-dirt passes.
    render::PostProcessShader const kButterCameraShader{"Butter Camera", kFullscreenVertexSource, butter_camera::kFragmentSource, "u_strength"};
    render::PostProcessShader const kRaindropShader{
        "Raindrop", kFullscreenVertexSource, raindrop::kFragmentSource,
        "u_time", "u_strength"
    };
    render::PostProcessShader const kFilmPolaroidShader{
        "Film Polaroid", kFullscreenVertexSource, film_polaroid::kFragmentSource,
        "u_time", "u_strength"
    };
    render::PostProcessShader const kFogShader{
        "Fog", kFullscreenVertexSource, fog::kFragmentSource,
        "u_time", "u_strength"
    };
    render::PostProcessShader const kDustFilmShader{
        "Dust Film", kFullscreenVertexSource, dust_film::kFragmentSource,
        "u_time", "u_strength"
    };

    render::PostProcessShader const kDeathWarpShader{
        "Death Warp",
        kFullscreenVertexSource,
        death_warp::kFragmentSource,
        "u_progress",
        "u_centerX",
        "u_centerY",
    };

} // namespace bv::shaders
