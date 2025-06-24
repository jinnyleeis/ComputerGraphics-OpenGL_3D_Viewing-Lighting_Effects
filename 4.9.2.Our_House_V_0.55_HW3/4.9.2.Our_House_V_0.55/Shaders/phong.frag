#version 330
in vec3 Position;
in vec3 Normal;
layout(location=0) out vec4 FragColor;


uniform vec3 u_Kd;          // ‼ 확산 반사계수 (물체별)


void main() {
    vec3 N = normalize(Normal);
    vec3 L = normalize(vec3(0.3,0.7,1.0));
    vec3 V = normalize(-Position);
    vec3 H = normalize(L+V);

    vec3 Ka = vec3(0.05);
   // vec3 Kd = vec3(0.15,0.45,0.35);
   vec3 Kd = u_Kd;
    vec3 Ks = vec3(0.3);
    float shin = 32.0;

    float NdotL = max(dot(N,L),0.0);
    float NdotH = pow(max(dot(N,H),0.0), shin);

    vec3 color = Ka + Kd*NdotL + Ks*NdotH;
    FragColor  = vec4(color,1.0);
}
