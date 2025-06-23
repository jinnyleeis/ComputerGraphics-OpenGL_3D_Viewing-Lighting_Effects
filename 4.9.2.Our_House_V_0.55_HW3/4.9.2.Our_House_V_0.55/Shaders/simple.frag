#version 330

in vec4 v_color;              // 읽기 전용

layout(location = 0) out vec4 final_color;

uniform int   u_flag_blending;
uniform float u_fragment_alpha;

void main(void)
{
    vec4 color = v_color;                 // 복사본
    if (u_flag_blending != 0)             // bool 비교가 더 안전
        color.a = u_fragment_alpha;       // 알파만 수정

    final_color = color;                  // 출력
}