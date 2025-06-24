#version 330
/* ────────────────────────────────────────────────────────────── *
 * Spot-only Phong — Vertex Stage
 *   − 눈(Eye) 좌표계용 변환만 수행
 *   − 조명 계산은 Frag 에서.
 * ────────────────────────────────────────────────────────────── */

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_ModelViewProjectionMatrix;
uniform mat4 u_ModelViewMatrix;
uniform mat3 u_ModelViewMatrixInvTrans;

/* 프래그먼트로 넘길 값 */
out vec3 v_pos_EC;
out vec3 v_nrm_EC;

void main() {
    v_pos_EC = vec3(u_ModelViewMatrix * vec4(a_position, 1.0));
    v_nrm_EC = normalize(u_ModelViewMatrixInvTrans * a_normal);

    gl_Position = u_ModelViewProjectionMatrix * vec4(a_position, 1.0);
}
