#version 400 core
/* ────────────────────────────────────────────────────────────── *
 * Spot-only Phong — Fragment Stage
 *   − u_light[i].spot_cutoff_angle < 180° 이면 spot 활성
 *   − point / directional → 모두 spot 식과 동일하게 처리
 * ────────────────────────────────────────────────────────────── */

struct LIGHT {
    vec4 position;                  // EC 공간 (w=1: 점, w=0: 방향)
    vec4 ambient_color;
    vec4 diffuse_color;
    vec4 specular_color;
    vec4 attenuation;               // (k_c, k_l, k_q, flag)  flag≠0 ⇒ 무감쇠
    vec3 spot_direction;            // EC
    float spot_exponent;            // [0,128]
    float spot_cutoff;              // [0,90] 또는 180
    bool  light_on;
};

struct MATERIAL {
    vec4 ambient_color;
    vec4 diffuse_color;
    vec4 specular_color;
    vec4 emissive_color;
    float specular_exp;
};

#define MAX_L 4
uniform LIGHT    u_light[MAX_L];
uniform MATERIAL u_material;
uniform vec4     u_global_ambient_color;

in  vec3 v_pos_EC;
in  vec3 v_nrm_EC;
layout(location=0) out vec4 fragColor;

/* ------------------------------------------------------------ */
vec3 apply_attenuation(vec3 L, vec4 att) {
    if (att.w != 0.0) return vec3(1.0);
    float d = length(L);
    return vec3(1.0) /
           (att.x + att.y * d + att.z * d * d);
}

float spot_factor(vec3 L, vec3 Sdir, float cutoff, float expo) {
    if (cutoff >= 180.0) return 1.0;                 // spot OFF
    float cos_t = dot(-L, Sdir);
    float lim   = cos(radians(clamp(cutoff,0.0,90.0)));
    return (cos_t >= lim) ? pow(cos_t, expo) : 0.0;
}
/* ------------------------------------------------------------ */
void main() {
    vec3 N = normalize(v_nrm_EC);
    vec3 V = normalize(-v_pos_EC);   // 시선 벡터
    vec4 C = u_material.emissive_color +
             u_global_ambient_color * u_material.ambient_color;

    for (int i=0;i<MAX_L;++i) {
        if (!u_light[i].light_on) continue;

        /* 라이트-to-픽셀 벡터 */
        vec3  L = (u_light[i].position.w == 0.0)
                  ? normalize(u_light[i].position.xyz)
                  : normalize(u_light[i].position.xyz - v_pos_EC);

        /* spot + 감쇠 계수 */
        float sf  = spot_factor(L, normalize(u_light[i].spot_direction),
                                u_light[i].spot_cutoff,
                                u_light[i].spot_exponent);
        if (sf == 0.0) continue;

        vec3  att = apply_attenuation(u_light[i].position.xyz - v_pos_EC,
                                      u_light[i].attenuation);

        float LN = max(dot(N,L), 0.0);
        if (LN > 0.0) {
            /* diffuse */
            C += vec4(att,1) * sf *
                 u_light[i].diffuse_color *
                 u_material.diffuse_color * LN;

            /* specular (Blinn) */
            vec3  H  = normalize(L + V);
            float NH = max(dot(N,H), 0.0);
            if (NH > 0.0)
                C += vec4(att,1) * sf *
                     u_light[i].specular_color *
                     u_material.specular_color *
                     pow(NH, u_material.specular_exp);
        }

        /* ambient */
        C += vec4(att,1) * sf *
             u_light[i].ambient_color *
             u_material.ambient_color;
    }

    fragColor = C;
}
