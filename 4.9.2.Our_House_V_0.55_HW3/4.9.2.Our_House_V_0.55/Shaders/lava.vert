#version 430 core
layout(location=0) in vec3 aPos;
layout(location=2) in vec2 aTex;   /* �� ���� t(uv) */

uniform mat4 u_MVP;
uniform mat4 u_MV;

out vec2 v_uv;

void main(){
    v_uv        = aTex;                 /* �״�� ���� */
    gl_Position = u_MVP * vec4(aPos,1.0);
}
