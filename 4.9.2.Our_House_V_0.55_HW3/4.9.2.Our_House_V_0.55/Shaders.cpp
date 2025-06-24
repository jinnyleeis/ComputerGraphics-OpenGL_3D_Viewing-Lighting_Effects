#define _CRT_SECURE_NO_WARNINGS

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

void Shader_SpotWorld::prepare_shader() {
    shader_info[0] = { GL_VERTEX_SHADER,   "Shaders/Spot_World.vert" };
    shader_info[1] = { GL_FRAGMENT_SHADER, "Shaders/Spot_World.frag" };
    shader_info[2] = { GL_NONE, nullptr };

    h_ShaderProgram = LoadShaders(shader_info);
    glUseProgram(h_ShaderProgram);

    // object-º°
    loc_ModelMatrix = glGetUniformLocation(h_ShaderProgram, "u_ModelMatrix");
    loc_ModelMatrixInvTrans = glGetUniformLocation(h_ShaderProgram, "u_ModelMatrixInvTrans");
    loc_ModelViewProj = glGetUniformLocation(h_ShaderProgram, "u_ModelViewProj");

    // scene-º°
    loc_eyePos = glGetUniformLocation(h_ShaderProgram, "u_eyePos_ws");
    loc_nSpot = glGetUniformLocation(h_ShaderProgram, "u_nSpot");

    char nm[64];
    for (int i = 0; i < MAX_SPOT_LIGHTS; ++i) {
        sprintf(nm, "u_spot[%d].pos", i);      loc_spot[i].pos = glGetUniformLocation(h_ShaderProgram, nm);
        sprintf(nm, "u_spot[%d].dir", i);      loc_spot[i].dir = glGetUniformLocation(h_ShaderProgram, nm);
        sprintf(nm, "u_spot[%d].Ia", i);       loc_spot[i].Ia = glGetUniformLocation(h_ShaderProgram, nm);
        sprintf(nm, "u_spot[%d].Id", i);       loc_spot[i].Id = glGetUniformLocation(h_ShaderProgram, nm);
        sprintf(nm, "u_spot[%d].Is", i);       loc_spot[i].Is = glGetUniformLocation(h_ShaderProgram, nm);
        sprintf(nm, "u_spot[%d].cutoff", i);   loc_spot[i].cutoff = glGetUniformLocation(h_ShaderProgram, nm);
        sprintf(nm, "u_spot[%d].expn", i);     loc_spot[i].exp = glGetUniformLocation(h_ShaderProgram, nm);
    }
    glUseProgram(0);
}
