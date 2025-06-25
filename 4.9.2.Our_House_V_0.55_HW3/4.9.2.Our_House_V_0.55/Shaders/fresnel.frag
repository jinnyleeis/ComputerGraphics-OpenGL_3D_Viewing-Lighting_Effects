#version 430 core
in vec3 v_viewN;
in vec3 v_viewPos;
out vec4 fragColor;

uniform float u_time;
uniform vec3  u_emiCol;   // 네온색

void main(){
    /* Fresnel */
    float rim = 1.0 - clamp(dot(normalize(v_viewN), normalize(-v_viewPos)),0.0,1.0);
    rim = pow(rim, 3.0);

    /* “찢겨나가는” 노이즈 마스크 */
    float stripe = sin(50.0*v_viewPos.y + u_time*30.0)*0.5+0.5;
    float mask   = step(stripe, 0.4+0.3*sin(u_time));

    float glow   = rim * mask;
    vec3 baseCol = vec3(0.2,0.2,0.2);       // 금속 어두운 바탕
    vec3 color   = mix(baseCol, u_emiCol, glow);

    float alpha = max(0.2, glow);           // 살짝 투명
    fragColor = vec4(color, alpha);
}
