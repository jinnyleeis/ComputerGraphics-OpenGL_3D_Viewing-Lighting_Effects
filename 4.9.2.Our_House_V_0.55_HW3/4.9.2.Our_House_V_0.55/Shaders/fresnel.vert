#version 430 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNrm;

uniform mat4 u_MVP;
uniform mat4 u_MV;
uniform mat3 u_MVN;

out vec3 v_viewN;
out vec3 v_viewPos;

void main() {
    vec4 viewPos = u_MV * vec4(aPos,1.0);
    v_viewPos = viewPos.xyz;
    v_viewN   = normalize(u_MVN * aNrm);
    gl_Position = u_MVP * vec4(aPos,1.0);
}
