#version 330

in vec4 v_color;              // �б� ����

layout(location = 0) out vec4 final_color;

uniform int   u_flag_blending;
uniform float u_fragment_alpha;

void main(void)
{
    vec4 color = v_color;                 // ���纻
    if (u_flag_blending != 0)             // bool �񱳰� �� ����
        color.a = u_fragment_alpha;       // ���ĸ� ����

    final_color = color;                  // ���
}