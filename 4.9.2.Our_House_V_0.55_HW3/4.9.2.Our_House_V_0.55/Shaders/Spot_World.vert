#version 400 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNor;

uniform mat4 u_ModelMatrix;
uniform mat3 u_ModelMatrixInvTrans;
uniform mat4 u_ModelViewProj;

out vec3 v_pos_ws;
out vec3 v_nor_ws;

void main() {
    v_pos_ws = vec3(u_ModelMatrix * vec4(aPos,1.0));
    v_nor_ws = normalize(u_ModelMatrixInvTrans * aNor);
    gl_Position = u_ModelViewProj * vec4(aPos,1.0);
}
