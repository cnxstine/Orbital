// =============================================================================
// assets/shaders/common/uniforms.glsl
//
// Shared UBO definitions included by all Orbital shaders via the
// ShaderLoader preprocessor (#include "common/uniforms.glsl").
//
// Layout follows GLSL std140 rules exactly, and must match the
// CameraUBOData struct in renderer/RenderContext.hpp.
//
// Binding points:
//   0 → CameraUBO     (updated every frame, all shaders)
//   1 → LightingUBO   (future: scene lights)
//   2 → MaterialUBO   (future: per-draw material data)
// =============================================================================

// ── Camera (binding = 0) ──────────────────────────────────────────────────────
layout(std140, binding = 0) uniform CameraUBO {
    mat4 u_View;           // World → View
    mat4 u_Proj;           // View  → Clip (perspective)
    mat4 u_ViewProj;       // World → Clip (cached product)
    mat4 u_InvViewProj;    // Clip  → World (for ray reconstruction)

    // vec3 padded to vec4 (std140 rule)
    vec3  u_Eye;     float u_Near;
    vec3  u_Forward; float u_Far;
    vec2  u_Resolution; float u_Time; float _u_Pad;
};
