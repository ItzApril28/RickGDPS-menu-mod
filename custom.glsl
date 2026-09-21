// BetterVisual and Audio custom shader sample
// Copy this file to Geode's persistent folder for this mod as custom.glsl,
// then enable "Custom GLSL Shader" in the mod settings.
// Available inputs:
//   u_texture       - rendered scene texture
//   u_invResolution - vec2(1.0 / width, 1.0 / height)
//   v_texCoord      - fullscreen texture coordinates

uniform sampler2D u_texture;
uniform vec2 u_invResolution;
varying vec2 v_texCoord;

void main() {
    vec4 color = texture2D(u_texture, v_texCoord);

    // Animated scanlines with a restrained blue/purple tint.
    float scanline = 0.97 + 0.03 * sin(v_texCoord.y / u_invResolution.y * 0.35);
    vec3 tint = vec3(0.99, 1.01, 1.04);

    color.rgb *= scanline * tint;
    gl_FragColor = color;
}
