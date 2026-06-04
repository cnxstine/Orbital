#version 460 core

in float v_Density;
in float v_Phase;

layout(location = 0) out float outDensity;

uniform float u_IntensityScale = 0.1;

void main() {
    // Distance from the center of the point sprite [-0.5, 0.5]
    vec2 coord = gl_PointCoord - vec2(0.5);
    float d = length(coord) * 2.0; // [0, 1]
    
    if (d > 1.0) {
        discard;
    }
    
    // Gaussian-like falloff for soft edges
    float intensity = exp(-d * d * 4.0);
    
    // Output accumulated density scaled by the user's intensity parameter
    outDensity = intensity * v_Density * u_IntensityScale;
}
