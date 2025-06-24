#include "Scene_Definitions.h"

void Shader_Simple::prepare_shader() {
	shader_info[0] = { GL_VERTEX_SHADER, "Shaders/simple.vert" };
	shader_info[1] = { GL_FRAGMENT_SHADER, "Shaders/simple.frag" };
	shader_info[2] = { GL_NONE, NULL };

	h_ShaderProgram = LoadShaders(shader_info);
	glUseProgram(h_ShaderProgram);

	loc_ModelViewProjectionMatrix = glGetUniformLocation(h_ShaderProgram, "u_ModelViewProjectionMatrix");
	loc_primitive_color = glGetUniformLocation(h_ShaderProgram, "u_primitive_color");
	loc_u_flag_blending = glGetUniformLocation(h_ShaderProgram, "u_flag_blending");
	loc_u_fragment_alpha = glGetUniformLocation(h_ShaderProgram, "u_fragment_alpha");
	glUseProgram(0);
}

void Shader_Phong_Texture::prepare_shader()
{
	shader_info[0] = { GL_VERTEX_SHADER,   "Shaders/Phong_Tx.vert" };
	shader_info[1] = { GL_FRAGMENT_SHADER, "Shaders/Phong_Tx.frag" };
	shader_info[2] = { GL_NONE, nullptr };

	h_ShaderProgram = LoadShaders(shader_info);
	glUseProgram(h_ShaderProgram);

	loc_ModelViewProjectionMatrix = glGetUniformLocation(h_ShaderProgram, "u_ModelViewProjectionMatrix");
	loc_ModelViewMatrix = glGetUniformLocation(h_ShaderProgram, "u_ModelViewMatrix");
	loc_ModelViewMatrixInvTrans = glGetUniformLocation(h_ShaderProgram, "u_ModelViewMatrixInvTrans");
	loc_texture = glGetUniformLocation(h_ShaderProgram, "u_base_texture");

	glUseProgram(0);
}

/* 3-A) Gouraud ----------------------------------------------------- */
void Shader_Gouraud::prepare_shader() {
    shader_info[0] = { GL_VERTEX_SHADER,   "Shaders/gouraud.vert" };
    shader_info[1] = { GL_FRAGMENT_SHADER, "Shaders/gouraud.frag" };
    shader_info[2] = { GL_NONE, nullptr };
    h_ShaderProgram = LoadShaders(shader_info);
    glUseProgram(h_ShaderProgram);
    loc_ModelViewProjectionMatrix = glGetUniformLocation(h_ShaderProgram,
        "u_ModelViewProjectionMatrix");
    loc_ModelViewMatrix = glGetUniformLocation(h_ShaderProgram,
        "u_ModelViewMatrix");
    loc_ModelViewMatrixInvTrans = glGetUniformLocation(h_ShaderProgram,
        "u_ModelViewMatrixInvTrans");
    glUseProgram(0);
}

/* 3-B) Phong ------------------------------------------------------- */
void Shader_Phong::prepare_shader() {
    shader_info[0] = { GL_VERTEX_SHADER,   "Shaders/phong.vert" };
    shader_info[1] = { GL_FRAGMENT_SHADER, "Shaders/phong.frag" };
    shader_info[2] = { GL_NONE, nullptr };
    h_ShaderProgram = LoadShaders(shader_info);
    glUseProgram(h_ShaderProgram);
    loc_ModelViewProjectionMatrix = glGetUniformLocation(h_ShaderProgram,
        "u_ModelViewProjectionMatrix");
    loc_ModelViewMatrix = glGetUniformLocation(h_ShaderProgram,
        "u_ModelViewMatrix");
    loc_ModelViewMatrixInvTrans = glGetUniformLocation(h_ShaderProgram,
        "u_ModelViewMatrixInvTrans");
    glUseProgram(0);
}
