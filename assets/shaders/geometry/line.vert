#version 460 core

layout(location = 0) in vec3 a_Position;

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

uniform mat4 u_Model;

void main()
{
    gl_Position = u_ViewProj * u_Model * vec4(a_Position, 1.0);
}
