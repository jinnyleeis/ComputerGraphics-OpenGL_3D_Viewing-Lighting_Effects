#version 330
in vec3 Position;
in vec3 Normal;
layout(location=0) out vec4 FragColor;

void main() {
    vec3 N = normalize(Normal);
    vec3 L = normalize(vec3(0.3,0.7,1.0));
    vec3 V = normalize(-Position);
    vec3 H = normalize(L+V);

    vec3 Ka = vec3(0.05);
    vec3 Kd = vec3(0.55,0.45,0.35);
    vec3 Ks = vec3(0.3);
    float shin = 32.0;

    float NdotL = max(dot(N,L),0.0);
    float NdotH = pow(max(dot(N,H),0.0), shin);

    vec3 color = Ka + Kd*NdotL + Ks*NdotH;
    FragColor  = vec4(color,1.0);
}
