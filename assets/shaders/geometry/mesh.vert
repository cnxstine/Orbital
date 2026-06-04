#version 460 core

// =============================================================================
// assets/shaders/geometry/mesh.vert
// Geometry pass vertex shader — transforms mesh vertices into clip space.
//
// Vertex layout (matches GLVertexArray setup in MeshLoader):
//   location 0 → vec3 Position  (world-space, pre-transform)
//   location 1 → vec3 Normal    (object-space)
//   location 2 → vec2 TexCoord
//
// Per-draw uniforms (set by RenderSystem before Submit):
//   u_Model       → object-to-world matrix
//   u_NormalMatrix→ transpose(inverse(u_Model)) for correct normal transform
// =============================================================================

layout(location = 0) in vec3 a_Position;
layout(location = 1) in vec3 a_Normal;
layout(location = 2) in vec2 a_TexCoord;

// ── Camera UBO (binding 0, updated every frame) ───────────────────────────────
layout(std140, binding = 0) uniform CameraUBO {
    mat4  u_View;
    mat4  u_Proj;
    mat4  u_ViewProj;
    mat4  u_InvViewProj;
    vec3  u_Eye;     float u_Near;
    vec3  u_Forward; float u_Far;
    vec2  u_Resolution; float u_Time; float _Pad;
};

// ── Per-draw uniforms ─────────────────────────────────────────────────────────
uniform mat4 u_Model;
uniform mat4 u_NormalMatrix; ///< transpose(inverse(u_Model))

// ── Output to fragment shader ─────────────────────────────────────────────────
out VS_OUT {
    vec3 FragPos;   ///< World-space position (for lighting)
    vec3 Normal;    ///< World-space normal   (normalised)
    vec2 TexCoord;
} vs_out;

void main()
{
    vec4 worldPos      = u_Model * vec4(a_Position, 1.0);
    vs_out.FragPos     = worldPos.xyz;
    vs_out.Normal      = normalize(mat3(u_NormalMatrix) * a_Normal);
    vs_out.TexCoord    = a_TexCoord;

    gl_Position = u_ViewProj * worldPos;
}
