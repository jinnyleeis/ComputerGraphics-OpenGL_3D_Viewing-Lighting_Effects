#version 330
layout(location=0) in vec3 VertexPosition;
layout(location=1) in vec3 VertexNormal;

uniform mat4 u_ModelViewProjectionMatrix;
uniform mat4 u_ModelViewMatrix;
uniform mat3 u_ModelViewMatrixInvTrans;

out vec3 Position;
out vec3 Normal;

void main() {
    Position = vec3(u_ModelViewMatrix * vec4(VertexPosition,1.0));
    Normal   = normalize(u_ModelViewMatrixInvTrans * VertexNormal);
    gl_Position = u_ModelViewProjectionMatrix * vec4(VertexPosition,1.0);
}
