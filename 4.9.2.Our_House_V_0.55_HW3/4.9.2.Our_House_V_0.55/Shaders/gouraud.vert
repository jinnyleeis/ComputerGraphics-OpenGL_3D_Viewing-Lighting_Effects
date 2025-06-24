#version 330
layout(location=0) in vec3 a_position;
layout(location=1) in vec3 a_normal;

uniform mat4 u_ModelViewProjectionMatrix;
uniform mat4 u_ModelViewMatrix;
uniform mat3 u_ModelViewMatrixInvTrans;

out vec4 v_color;

void main() {
    vec3  P  = vec3(u_ModelViewMatrix * vec4(a_position,1.0));
    vec3  N  = normalize(u_ModelViewMatrixInvTrans * a_normal);
    vec3  L  = normalize(vec3( 0.3, 0.7, 1.0));   // 간단한 고정 광원
    vec3  V  = normalize(-P);
    vec3  H  = normalize(L+V);

    vec3 Ka = vec3(0.05);
    vec3 Kd = vec3(0.55,0.45,0.35);
    vec3 Ks = vec3(0.3);
    float shin = 32.0;

    float NdotL = max(dot(N,L),0.0);
    float NdotH = pow(max(dot(N,H),0.0), shin);

    vec3 color = Ka + Kd*NdotL + Ks*NdotH;
    v_color    = vec4(color,1.0);

    gl_Position = u_ModelViewProjectionMatrix * vec4(a_position,1.0);
}
