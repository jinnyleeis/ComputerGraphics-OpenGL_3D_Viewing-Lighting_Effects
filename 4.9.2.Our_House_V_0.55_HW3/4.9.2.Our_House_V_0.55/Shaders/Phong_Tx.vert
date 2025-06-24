#version 330

uniform mat4 u_ModelViewProjectionMatrix;
uniform mat4 u_ModelViewMatrix;
uniform mat3 u_ModelViewMatrixInvTrans;

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texcoord;

out vec3 v_normal_ec;
out vec2 v_tex;

void main()
{
    v_normal_ec = normalize(u_ModelViewMatrixInvTrans * a_normal);
    v_tex       = a_texcoord;
    gl_Position = u_ModelViewProjectionMatrix * vec4(a_position, 1.0);
}
