#version 460 core

// =============================================================================
// assets/shaders/geometry/mesh.frag
// Geometry pass fragment shader — Blinn-Phong shading with material uniforms.
//
// STATUS: FUTURE INFRASTRUCTURE (not loaded at runtime in v1.0)
//   Paired with mesh.vert. Will be activated when RenderSystem + MeshComponent
//   are implemented. See: src/scene/systems/RenderSystem.hpp
//
// Shading model: Blinn-Phong (placeholder for future PBR pass).
// The ambient term ensures no surface is fully black.
//
// Material uniforms are set per-draw by the material system.
// =============================================================================

in VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoord;
} fs_in;

out vec4 FragColor;

// ── Camera UBO (binding 0) ────────────────────────────────────────────────────
layout(std140, binding = 0) uniform CameraUBO {
    mat4  u_View;
    mat4  u_Proj;
    mat4  u_ViewProj;
    mat4  u_InvViewProj;
    vec3  u_Eye;     float u_Near;
    vec3  u_Forward; float u_Far;
    vec2  u_Resolution; float u_Time; float _Pad;
};

// ── Material uniforms ─────────────────────────────────────────────────────────
uniform vec4  u_Albedo    = vec4(0.8, 0.8, 0.85, 1.0); ///< Base colour + alpha
uniform float u_Metallic  = 0.0;
uniform float u_Roughness = 0.5;
uniform float u_Emissive  = 0.0; ///< Emissive intensity (future: IBL)

// ── Simple directional light (hardcoded for foundation milestone) ─────────────
const vec3  kLightDir   = normalize(vec3(1.0, 2.5, 1.5));
const vec3  kLightColor = vec3(1.0, 0.98, 0.95); ///< Warm white
const float kAmbient    = 0.12;

void main()
{
    vec3 N = normalize(fs_in.Normal);
    vec3 V = normalize(u_Eye - fs_in.FragPos);
    vec3 H = normalize(kLightDir + V);

    // ── Diffuse ───────────────────────────────────────────────────────────────
    float NdotL = max(dot(N, kLightDir), 0.0);
    vec3  diffuse = NdotL * kLightColor * u_Albedo.rgb;

    // ── Specular (Blinn-Phong) ────────────────────────────────────────────────
    float shininess  = mix(4.0, 256.0, 1.0 - u_Roughness);
    float NdotH      = max(dot(N, H), 0.0);
    float specFactor = pow(NdotH, shininess);
    vec3  specular   = specFactor * kLightColor * mix(vec3(0.04), u_Albedo.rgb, u_Metallic);

    // ── Ambient ───────────────────────────────────────────────────────────────
    vec3 ambient = kAmbient * u_Albedo.rgb;

    // ── Compose ───────────────────────────────────────────────────────────────
    vec3 colour = ambient + diffuse + specular;
    colour      = mix(colour, u_Albedo.rgb, u_Emissive); // Emissive override

    // Simple reinhard tone-mapping to prevent overexposure
    colour = colour / (colour + vec3(1.0));

    FragColor = vec4(colour, u_Albedo.a);
}
