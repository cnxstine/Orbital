#version 460 core

in vec2 v_TexCoord;
out vec4 FragColor;

layout(binding = 0) uniform sampler2D u_DensityTexture;

uniform float u_Exposure = 1.0;
uniform float u_Gamma = 2.2;

vec3 GetDensityColor(float t) {
    // Color ramp control points:
    vec3 c0 = vec3(0.0, 0.0, 0.0);             // Black
    vec3 c1 = vec3(0.2, 0.0, 0.6);             // Deep Violet
    vec3 c2 = vec3(0.8, 0.0, 0.6);             // Magenta
    vec3 c3 = vec3(1.0, 0.4, 0.0);             // Orange
    vec3 c4 = vec3(1.0, 1.0, 0.7);             // White-Yellow
    
    // Smooth interpolation along the ramp
    if (t < 0.25) {
        return mix(c0, c1, t / 0.25);
    } else if (t < 0.5) {
        return mix(c1, c2, (t - 0.25) / 0.25);
    } else if (t < 0.75) {
        return mix(c2, c3, (t - 0.5) / 0.25);
    } else {
        return mix(c3, c4, (t - 0.75) / 0.25);
    }
}

void main() {
    float rawDensity = texture(u_DensityTexture, v_TexCoord).r;
    
    // Tone mapping (exposure)
    float t = 1.0 - exp(-rawDensity * u_Exposure);
    t = clamp(t, 0.0, 1.0);
    
    vec3 color = GetDensityColor(t);
    
    // Gamma correction
    color = pow(color, vec3(1.0 / u_Gamma));
    
    FragColor = vec4(color, 1.0);
}
