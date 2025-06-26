#version 430 core
#define PI 3.141592653589793

in vec2 v_uv;
in vec3 v_viewPos;
out vec4 fragColor;

uniform sampler2D u_mainTex;
uniform float     u_time;
uniform vec2      u_uParams;   /* x=tile, y=speed */
uniform vec2      u_vParams;   /* x=tile, y=speed */



void main(){

float     u_flickerFreq=20.0;
float     u_flickerSpeed=1.0;
    // ① Radial-UV 생성
    vec2 cen = vec2(0.5) - v_uv;
    float U = fract( length(cen) * u_uParams.x + u_time * u_uParams.y );

    vec2 tmp = v_uv * 2.0 - 1.0;
    float V = atan(tmp.x, tmp.y) / (2.0 * PI) + 0.5;
          V = fract( V * u_vParams.x + u_time * u_vParams.y );

    vec2 rUV = vec2(U, V);

    // ② 텍스처 샘플 & 컬러 연출
    vec3 col = texture(u_mainTex, rUV).rgb;
    col *= vec3(0.1, 1.0, 0.3);

    // ③ 플리커 알파 계산: cos 파형을 0..1 범위로 매핑
    float flick = clamp( cos(u_time * u_flickerFreq + u_time * u_flickerSpeed) * 0.5 + 0.5, 0.0, 1.0 );

    // ④ 최종 색상 + 알파
    fragColor = vec4(col, flick);
}
