#version 460 core

layout(location = 0) in vec3 a_Position;
layout(location = 1) in float a_Density;
layout(location = 2) in float a_Phase;

layout(std140, binding = 0) uniform CameraUBO {
    mat4 u_View;
    mat4 u_Proj;
    mat4 u_ViewProj;
    mat4 u_InvViewProj;
    vec3 u_Eye;    float u_Near;
    vec3 u_Forward; float u_Far;
    vec2 u_Resolution; float u_Time; float _pad;
};

uniform float u_ParticleSize = 10.0;

out float v_Density;
out float v_Phase;

void main() {
    vec4 clipPos = u_ViewProj * vec4(a_Position, 1.0);
    gl_Position = clipPos;
    
    // Distance attenuation for point size
    // Prevent division by zero and excessive sizes
    float dist = clipPos.w;
    gl_PointSize = max(1.0, u_ParticleSize / (dist + 0.001));
    
    v_Density = a_Density;
    v_Phase = a_Phase;
}
