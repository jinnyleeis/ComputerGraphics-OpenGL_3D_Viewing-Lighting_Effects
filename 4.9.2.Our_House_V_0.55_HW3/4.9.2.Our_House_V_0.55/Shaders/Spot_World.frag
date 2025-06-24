#version 400 core
const int MAX_SPOT = 8;
struct Spot {
    vec3  pos;
    vec3  dir;
    vec3  Ia, Id, Is;
    float cutoff;
    float expn;
};

uniform int  u_nSpot;
uniform Spot u_spot[MAX_SPOT];

uniform vec3  u_eyePos_ws;
uniform vec3  u_matKa, u_matKd, u_matKs;
uniform float u_matShin;

in vec3 v_pos_ws;
in vec3 v_nor_ws;
out vec4 FragColor;

void main() {
    vec3 N = normalize(v_nor_ws);
    vec3 V = normalize(u_eyePos_ws - v_pos_ws);
    vec3 col = vec3(0.0);

    for(int i=0;i<u_nSpot;++i){
        Spot L = u_spot[i];
        vec3  Ldir = normalize(L.pos - v_pos_ws);
        float d    = length(L.pos - v_pos_ws);

        float cosTheta = dot(normalize(-L.dir), Ldir);
        if(cosTheta < L.cutoff) continue;

        float spotF = pow(cosTheta, L.expn);
        float att   = 1.0 / (1.0 + 0.0005*d*d);

        float NdotL = max(dot(N, Ldir), 0.0);
        vec3  R = reflect(-Ldir,N);
        float RdotV = max(dot(R,V),0.0);

        vec3 amb  = L.Ia * u_matKa;
        vec3 diff = L.Id * u_matKd * NdotL;
        vec3 spec = L.Is * u_matKs * pow(RdotV, u_matShin);

        col += (amb+diff+spec)*att*spotF;
    }
    FragColor = vec4(col,1.0);
}
