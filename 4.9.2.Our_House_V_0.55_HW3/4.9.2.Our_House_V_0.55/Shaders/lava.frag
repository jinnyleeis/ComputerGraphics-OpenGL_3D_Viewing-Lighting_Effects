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
    // �� Radial-UV ����
    vec2 cen = vec2(0.5) - v_uv;
    float U = fract( length(cen) * u_uParams.x + u_time * u_uParams.y );

    vec2 tmp = v_uv * 2.0 - 1.0;
    float V = atan(tmp.x, tmp.y) / (2.0 * PI) + 0.5;
          V = fract( V * u_vParams.x + u_time * u_vParams.y );

    vec2 rUV = vec2(U, V);

    // �� �ؽ�ó ���� & �÷� ����
    vec3 col = texture(u_mainTex, rUV).rgb;
    col *= vec3(0.1, 1.0, 0.3);

    // �� �ø�Ŀ ���� ���: cos ������ 0..1 ������ ����
    float flick = clamp( cos(u_time * u_flickerFreq + u_time * u_flickerSpeed) * 0.5 + 0.5, 0.0, 1.0 );

    // �� ���� ���� + ����
    fragColor = vec4(col, flick);
}
