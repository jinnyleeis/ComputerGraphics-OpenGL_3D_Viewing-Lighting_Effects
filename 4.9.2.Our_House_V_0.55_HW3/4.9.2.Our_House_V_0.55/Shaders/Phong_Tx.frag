#version 330

in  vec3 v_normal_ec;
in  vec2 v_tex;
out vec4 final_color;

uniform sampler2D u_base_texture;

void main()
{
    vec3 N  = normalize(v_normal_ec);
    vec3 L  = normalize(vec3(0.3, 0.7, 1.0));   // 임시 광원
    float NdotL = max(dot(N, L), 0.0);

    vec4 tex = texture(u_base_texture, v_tex);
    final_color = vec4(tex.rgb * NdotL, tex.a);
}
