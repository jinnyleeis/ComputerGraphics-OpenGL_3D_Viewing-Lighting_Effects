﻿#define _CRT_SECURE_NO_WARNINGS

#include "Scene_Definitions.h"
#include <glm/gtc/matrix_inverse.hpp>   // 파일 맨 위


void Static_Object::read_geometry(int bytes_per_primitive) {
	FILE* fp;

	// fprintf(stdout, "Reading geometry from the geometry file %s...\n", filename);
	fp = fopen(filename, "rb");
	if (fp == NULL) {
		fprintf(stderr, "Error: cannot open the object file %s ...\n", filename);
		exit(EXIT_FAILURE);
	}
	fread(&n_triangles, sizeof(int), 1, fp);
	vertices = (float*)malloc(n_triangles * bytes_per_primitive);
	if (vertices == NULL) {
		fprintf(stderr, "Error: cannot allocate memory for the geometry file %s ...\n", filename);
		exit(EXIT_FAILURE);
	}
	fread(vertices, bytes_per_primitive, n_triangles, fp); // assume the data file has no faults.
	// fprintf(stdout, "Read %d primitives successfully.\n\n", n_triangles);
	fclose(fp);
}

void Static_Object::prepare_geom_of_static_object() {
	int n_bytes_per_vertex, n_bytes_per_triangle;

	n_bytes_per_vertex = n_fields * sizeof(float); // 3 for vertex, 3 for normal, and 2 for texcoord
	n_bytes_per_triangle = 3 * n_bytes_per_vertex;

	/* ── (A) geometry 로드 여부 결정 ───────────────────────── */
	if (vertices == nullptr) {            // 아직 버텍스가 없는 경우
		if (this->filename[0] == '\0') {  // ←  멤버 변수를 정확히 참조
			fprintf(stderr,
				"[Static_Object] ERROR: no geometry source (filename & vertices both empty)\n");
			exit(EXIT_FAILURE);
		}
		/* 파일이름이 있으면 기존 방식대로 읽어 옴 */
		read_geometry(n_bytes_per_triangle);
	}

	// Initialize vertex buffer object.
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, n_triangles * n_bytes_per_triangle, vertices, GL_STATIC_DRAW);

	free(vertices);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	/* 위치 */
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
		n_bytes_per_vertex, (void*)0);
	glEnableVertexAttribArray(0);

	/* 법선 */
	if (n_fields >= 6) {
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
			n_bytes_per_vertex, (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);
	}

	/* 텍스처 좌표 */
	if (n_fields == 8) {
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
			n_bytes_per_vertex, (void*)(6 * sizeof(float)));
		glEnableVertexAttribArray(2);
	}

	glBindVertexArray(0);

}

void Building::define_object() {
	Material* cur_material;
	strcpy(filename, "Data/Building1_vnt.geom");
	n_fields = 8;
	front_face_mode = GL_CCW;
	prepare_geom_of_static_object();
	flag_valid = true;

	instances.emplace_back();
	instances.back().ModelMatrix = glm::mat4(1.0f);

	cur_material = &(instances.back().material);
	cur_material->emission = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	cur_material->ambient = glm::vec4(0.135f, 0.2225f, 0.1575f, 1.0f);
	cur_material->diffuse = glm::vec4(0.54f, 0.89f, 0.63f, 1.0f);
	cur_material->specular = glm::vec4(0.316228f, 0.316228f, 0.316228f, 1.0f);
	cur_material->exponent = 128.0f * 0.1f;
}

void Table::define_object() {
	glm::mat4* cur_MM;
	Material* cur_material;

	strcpy(filename, "Data/Table_vn.geom");
	n_fields = 6;
	front_face_mode = GL_CCW;
	prepare_geom_of_static_object();
	flag_valid = true;

	instances.emplace_back();
	cur_MM = &(instances.back().ModelMatrix);
	*cur_MM = glm::translate(glm::mat4(1.0f), glm::vec3(157.0f, 76.5f, 0.0f));
	*cur_MM = glm::scale(*cur_MM, glm::vec3(0.5f, 0.5f, 0.5f));
	cur_material = &(instances.back().material);
	cur_material->emission = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	cur_material->ambient = glm::vec4(0.1f, 0.3f, 0.1f, 1.0f);
	cur_material->diffuse = glm::vec4(0.4f, 0.6f, 0.3f, 1.0f);
	cur_material->specular = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
	cur_material->exponent = 15.0f;

	instances.emplace_back();
	cur_MM = &(instances.back().ModelMatrix);
	*cur_MM = glm::translate(glm::mat4(1.0f), glm::vec3(198.0f, 120.0f, 0.0f));
	*cur_MM = glm::scale(*cur_MM, glm::vec3(0.8f, 0.6f, 0.6f));
	cur_material = &(instances.back().material);
	cur_material->emission = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	cur_material->ambient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
	cur_material->diffuse = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
	cur_material->specular = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
	cur_material->exponent = 128.0f * 0.078125f;
}



void Teapot::define_object() {
	glm::mat4* cur_MM;
	Material* cur_material;
	strcpy(filename, "Data/Teapotn_vn.geom");
	n_fields = 6;
	front_face_mode = GL_CCW;
	prepare_geom_of_static_object();
	flag_valid = true;

	instances.emplace_back();
	cur_MM = &(instances.back().ModelMatrix);
	*cur_MM = glm::translate(glm::mat4(1.0f), glm::vec3(193.0f, 120.0f, 11.0f));
	*cur_MM = glm::scale(*cur_MM, glm::vec3(2.0f, 2.0f, 2.0f));
	cur_material = &(instances.back().material);
	cur_material->emission = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	cur_material->ambient = glm::vec4(0.1745f, 0.01175f, 0.01175f, 1.0f);
	cur_material->diffuse = glm::vec4(0.61424f, 0.04136f, 0.04136f, 1.0f);
	cur_material->specular = glm::vec4(0.727811f, 0.626959f, 0.626959f, 1.0f);
	cur_material->exponent = 128.0f * 0.6;
}


void Frame::define_object() {
	glm::mat4* cur_MM;
	Material* cur_material;
	strcpy(filename, "Data/Frame_vn.geom");
	n_fields = 6;
	front_face_mode = GL_CCW;
	prepare_geom_of_static_object();
	flag_valid = true;

	instances.emplace_back();
	cur_MM = &(instances.back().ModelMatrix);
	*cur_MM = glm::translate(glm::mat4(1.0f), glm::vec3(188.0f, 116.0f, 30.0f));
	*cur_MM = glm::scale(*cur_MM, glm::vec3(0.6f, 0.6f, 0.6f));
	*cur_MM = glm::rotate(*cur_MM, 90.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
	cur_material = &(instances.back().material);
	cur_material->emission = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	cur_material->ambient = glm::vec4(0.19125f, 0.0735f, 0.0225f, 1.0f);
	cur_material->diffuse = glm::vec4(0.7038f, 0.27048f, 0.0828f, 1.0f);
	cur_material->specular = glm::vec4(0.256777f, 0.137622f, 0.086014f, 1.0f);
	cur_material->exponent = 128.0f * 0.1f;
}




/*-------------------------------------------------- 1. Bike ----------*/
void Bike::define_object() {
	
		glm::mat4* M; Material* mat;
		strcpy(filename, "Data/static_objects/bike_vnt.geom");  // 경로 확인!
		n_fields = 8;  front_face_mode = GL_CCW;
		prepare_geom_of_static_object();  flag_valid = true;

		instances.emplace_back();
		M = &instances.back().ModelMatrix;
		*M = glm::translate(glm::mat4(1.0f), glm::vec3(100.0f, 50.0f, 0.0f));
		*M = glm::scale(*M, glm::vec3(3.4f));
		*M = glm::rotate(*M, -90.f * TO_RADIAN, glm::vec3(0, 1, 0));
		*M = glm::rotate(*M, -90.f * TO_RADIAN, glm::vec3(1, 0, 0));
		mat = &instances.back().material;
		mat->ambient = glm::vec4(0.1f, 0.1f, 0.1f, 1);
		mat->diffuse = glm::vec4(0.6f, 0.2f, 0.2f, 1);
		mat->specular = glm::vec4(0.7f, 0.7f, 0.7f, 1);
		mat->exponent = 30.f;
	
};

/*-------------------------------------------------- 2. Cat -----------*/
void Cat::define_object() {
		glm::mat4* M; Material* mat;
		strcpy(filename, "Data/static_objects/cat_vnt.geom");
		n_fields = 8;  front_face_mode = GL_CCW;
		prepare_geom_of_static_object();  flag_valid = true;

		instances.emplace_back();
		M = &instances.back().ModelMatrix;
		*M = glm::translate(glm::mat4(1.0f), glm::vec3(200.0f, 116.0f, 2.5f));
		*M = glm::scale(*M, glm::vec3(7.0f,7.0f,7.f));
		*M = glm::rotate(*M, 90.f * TO_RADIAN, glm::vec3(0, 1, 0));
		*M = glm::rotate(*M, 90.f * TO_RADIAN, glm::vec3(0, 0, 1));
		mat = &instances.back().material;
		mat->ambient = glm::vec4(0.05f, 0.05f, 0.05f, 1);
		mat->diffuse = glm::vec4(0.5f, 0.4f, 0.3f, 1);
		mat->specular = glm::vec4(0.3f, 0.3f, 0.3f, 1);
		mat->exponent = 12.f;
	
};

void Ironman :: define_object() {
		glm::mat4* M; Material* mat;
		strcpy(filename, "Data/static_objects/ironman_vnt.geom");
		n_fields = 8;  front_face_mode = GL_CCW;
		prepare_geom_of_static_object();  flag_valid = true;

		instances.emplace_back();
		M = &instances.back().ModelMatrix;
		*M = glm::translate(glm::mat4(1.0f), glm::vec3(157.0f, 76.5f, 0.0f));
		*M = glm::scale(*M, glm::vec3(5.0f));   // 실물 크기 석상
		*M = glm::rotate(*M, -270.f * TO_RADIAN, glm::vec3(1, 0, 0));
		mat = &instances.back().material;
		mat->ambient = glm::vec4(0.247f, 0.199f, 0.074f, 1);
		mat->diffuse = glm::vec4(0.752f, 0.606f, 0.226f, 1);
		mat->specular = glm::vec4(0.628f, 0.556f, 0.366f, 1);
		mat->exponent = 50.f;

};

/*-------------------------------------------------- 4. Dragon --------*/
void Dragon ::define_object() {
		glm::mat4* M; Material* mat;
		strcpy(filename, "Data/static_objects/dragon_vnt.geom");
		n_fields = 8;  front_face_mode = GL_CCW;
		prepare_geom_of_static_object();  flag_valid = true;

		instances.emplace_back();
		M = &instances.back().ModelMatrix;
		*M = glm::translate(glm::mat4(1.0f), glm::vec3(157.0f, 76.5f, 40.0f));
		*M = glm::scale(*M, glm::vec3(0.3f));
		*M = glm::rotate(*M, -180.0f * TO_RADIAN, glm::vec3(0.0f, 1.0f, 0.0f));
		*M = glm::scale(*M, glm::vec3(1.0f,1.0f,-1.0f));
		mat = &instances.back().material;
		mat->ambient = glm::vec4(0.15f, 0.05f, 0.05f, 1);
		mat->diffuse = glm::vec4(0.6f, 0.2f, 0.2f, 1);
		mat->specular = glm::vec4(0.8f, 0.8f, 0.8f, 1);
		mat->exponent = 80.f;
	
};

void Wood_Tower :: define_object() {
		glm::mat4* M; Material* mat;
		strcpy(filename, "Data/static_objects/woodTower_vnt.geom");
		n_fields = 8;  front_face_mode = GL_CCW;
		prepare_geom_of_static_object();  flag_valid = true;

		instances.emplace_back();

		tex_id = TEXTURE_ID_WOOD_TOWER;

		M = &instances.back().ModelMatrix;
		*M = glm::translate(glm::mat4(1.0f), glm::vec3(200.0f, 109.0f, 13.0f));
		*M = glm::scale(*M, glm::vec3(1.3f));
		mat = &instances.back().material;
		mat->ambient = glm::vec4(0.21f, 0.13f, 0.05f, 1);
		mat->diffuse = glm::vec4(0.71f, 0.43f, 0.18f, 1);
		mat->specular = glm::vec4(0.3f, 0.3f, 0.3f, 1);
		mat->exponent = 15.f;

};


class Icosahedron : public Static_Object {
public:
	Icosahedron(STATIC_OBJECT_ID id) : Static_Object(id) {}
	void define_object() {
		/* 1) 정점/인덱스 데이터 */
		float X = 0.525731f, Z = 0.850651f; // 정규화된 값
		const GLfloat vdata[12][3] = {
			{-X,0,Z},{X,0,Z},{-X,0,-Z},{X,0,-Z},
			{0,Z,X},{0,Z,-X},{0,-Z,X},{0,-Z,-X},
			{Z,X,0},{-Z,X,0},{Z,-X,0},{-Z,-X,0}
		};
		const GLint tindices[20][3] = {
			{0,4,1},{0,9,4},{9,5,4},{4,5,8},{4,8,1},
			{8,10,1},{8,3,10},{5,3,8},{5,2,3},{2,7,3},
			{7,10,3},{7,6,10},{7,11,6},{11,0,6},{0,1,6},
			{6,1,10},{9,0,11},{9,11,2},{9,2,5},{7,2,11}
		};

		/* 2) 한 면(삼각형)마다 3-버텍스 push → vertices[] */
		std::vector<GLfloat> verts;
		for (int f = 0; f < 20; ++f)
			for (int k = 0; k < 3; ++k) {
				int idx = tindices[f][k];
				verts.insert(verts.end(), { vdata[idx][0],vdata[idx][1],vdata[idx][2] });
			}
		n_fields = 3;                  // pos only
		n_triangles = 20;              // 20 faces
		front_face_mode = GL_CCW;
		flag_valid = true;

		/* 3) GPU 전송 */
		glGenBuffers(1, &VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER,
			verts.size() * sizeof(GLfloat),
			verts.data(), GL_STATIC_DRAW);

		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (void*)0);
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);


		/* 2) 인스턴스 한 개 생성 */
		instances.emplace_back();
		/* 위치·스케일은 고정, 회전은 매 프레임 별도 갱신 */
		instances.back().ModelMatrix = glm::mat4(1.f);   // ★ 초기값만 1
	}
};


void Light::define_object() {
	glm::mat4* cur_MM;
	Material* cur_material;
	strcpy(filename, "Data/Light_vn.geom");
	n_fields = 6;
	front_face_mode = GL_CCW;
	prepare_geom_of_static_object();
	flag_valid = true;

	instances.emplace_back();
	cur_MM = &(instances.back().ModelMatrix);
	*cur_MM = glm::translate(glm::mat4(1.0f), glm::vec3(120.0f, 100.0f, 49.0f));
	*cur_MM = glm::rotate(*cur_MM, 90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	cur_material = &(instances.back().material);
	cur_material->emission = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	cur_material->ambient = glm::vec4(0.24725f, 0.1995f, 0.0745f, 1.0f);
	cur_material->diffuse = glm::vec4(0.75164f, 0.60648f, 0.22648f, 1.0f);
	cur_material->specular = glm::vec4(0.628281f, 0.555802f, 0.366065f, 1.0f);
	cur_material->exponent = 128.0f * 0.4f;

	instances.emplace_back();
	cur_MM = &(instances.back().ModelMatrix);
	*cur_MM = glm::translate(glm::mat4(1.0f), glm::vec3(80.0f, 47.5f, 49.0f));
	*cur_MM = glm::rotate(*cur_MM, 90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	cur_material = &(instances.back().material);
	cur_material->emission = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	cur_material->ambient = glm::vec4(0.24725f, 0.1995f, 0.0745f, 1.0f);
	cur_material->diffuse = glm::vec4(0.75164f, 0.60648f, 0.22648f, 1.0f);
	cur_material->specular = glm::vec4(0.628281f, 0.555802f, 0.366065f, 1.0f);
	cur_material->exponent = 128.0f * 0.4f;

	instances.emplace_back();
	cur_MM = &(instances.back().ModelMatrix);
	*cur_MM = glm::translate(glm::mat4(1.0f), glm::vec3(40.0f, 130.0f, 49.0f));
	*cur_MM = glm::rotate(*cur_MM, 90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	cur_material = &(instances.back().material);
	cur_material->emission = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	cur_material->ambient = glm::vec4(0.24725f, 0.1995f, 0.0745f, 1.0f);
	cur_material->diffuse = glm::vec4(0.75164f, 0.60648f, 0.22648f, 1.0f);
	cur_material->specular = glm::vec4(0.628281f, 0.555802f, 0.366065f, 1.0f);
	cur_material->exponent = 128.0f * 0.4f;

	instances.emplace_back();
	cur_MM = &(instances.back().ModelMatrix);
	*cur_MM = glm::translate(glm::mat4(1.0f), glm::vec3(190.0f, 60.0f, 49.0f));
	*cur_MM = glm::rotate(*cur_MM, 90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	cur_material = &(instances.back().material);
	cur_material->emission = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	cur_material->ambient = glm::vec4(0.24725f, 0.1995f, 0.0745f, 1.0f);
	cur_material->diffuse = glm::vec4(0.75164f, 0.60648f, 0.22648f, 1.0f);
	cur_material->specular = glm::vec4(0.628281f, 0.555802f, 0.366065f, 1.0f);
	cur_material->exponent = 128.0f * 0.4f;

	instances.emplace_back();
	cur_MM = &(instances.back().ModelMatrix);
	*cur_MM = glm::translate(glm::mat4(1.0f), glm::vec3(210.0f, 112.5f, 49.0));
	*cur_MM = glm::rotate(*cur_MM, 90.0f * TO_RADIAN, glm::vec3(1.0f, 0.0f, 0.0f));
	cur_material = &(instances.back().material);
	cur_material->emission = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	cur_material->ambient = glm::vec4(0.24725f, 0.1995f, 0.0745f, 1.0f);
	cur_material->diffuse = glm::vec4(0.75164f, 0.60648f, 0.22648f, 1.0f);
	cur_material->specular = glm::vec4(0.628281f, 0.555802f, 0.366065f, 1.0f);
	cur_material->exponent = 128.0f * 0.4f;

}




void print_mat4(const char* string, glm::mat4 M) {
	fprintf(stdout, "\n***** %s ******\n", string);
	for (int i = 0; i < 4; i++)
		fprintf(stdout, "*** COL[%d] (%f, %f, %f, %f)\n", i, M[i].x, M[i].y, M[i].z, M[i].w);
	fprintf(stdout, "**************\n\n");
}


/* -------------------------------------------------------------------- */
/*  Static_Object::draw_object                                          */
/*  - 정적 오브젝트 1개(또는 인스턴스들)를 주어진 셰이더로 그린다          */
/* -------------------------------------------------------------------- */
void Static_Object::draw_object(glm::mat4& ViewMatrix,
	glm::mat4& ProjectionMatrix,
	SHADER_ID  default_shader,
	std::vector<std::reference_wrapper<Shader>>& shader_list)
{
	glFrontFace(front_face_mode);

	/* ---------- (1) 이 오브젝트에 적용할 최종 셰이더 결정 ---------------- */
	const bool  has_tex = (tex_id >= 0);
	bool        is_cat = (object_id == STATIC_OBJECT_CAT);          // ★ 고양이만 특수 처리

	SHADER_ID eff;
	if (has_tex)
		eff = SHADER_PHONG_TEXUTRE;                                  // (기존) 텍스처-Phong
	else if (is_cat) {                                               // ★ shading mode 토글
		switch (g_shading_mode) {
		case SHADE_GOURAUD: eff = SHADER_GOURAUD;  break;
		case SHADE_PHONG:   eff = SHADER_PHONG;    break;
		default:            eff = SHADER_SIMPLE;   break;
		}
	}
	else
		eff = default_shader;                                        // 기타 오브젝트

	/* ---------- (2) 고양이는 항상 폴리곤-FILL로 강제 -------------------- */
	GLint prevPoly[2] = { GL_FILL, GL_FILL };
	if (is_cat) {
		glGetIntegerv(GL_POLYGON_MODE, prevPoly);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);                   // ★
	}

	/* ---------- (3) 모든 인스턴스 렌더링 -------------------------------- */
	for (const Instance& inst : instances) {
		glm::mat4 MV = ViewMatrix * inst.ModelMatrix;
		glm::mat4 MVP = ProjectionMatrix * MV;
		glm::mat3 MVN = glm::inverseTranspose(glm::mat3(MV));

		switch (eff) {
			/* ----- (a) 단순 색 (기존) ------------------------------------ */
		case SHADER_SIMPLE: {
			auto* sh = static_cast<Shader_Simple*>(
				&shader_list[shader_ID_mapper[SHADER_SIMPLE]].get());

			glUseProgram(sh->h_ShaderProgram);
			scene.upload_lights_to_current_prog();
			glUniformMatrix4fv(sh->loc_ModelViewProjectionMatrix, 1, GL_FALSE, &MVP[0][0]);
			glUniform3fv(sh->loc_primitive_color, 1, &inst.material.diffuse[0]);
			break;
		}

						  /* ----- (b) Gouraud ------------------------------------------- */
		case SHADER_GOURAUD: {
			auto* sh = static_cast<Shader_Gouraud*>(
				&shader_list[shader_ID_mapper[SHADER_GOURAUD]].get());

			glUseProgram(sh->h_ShaderProgram);
			scene.upload_lights_to_current_prog();

			glUniformMatrix4fv(sh->loc_ModelViewProjectionMatrix, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(sh->loc_ModelViewMatrix, 1, GL_FALSE, &MV[0][0]);
			glUniformMatrix3fv(sh->loc_ModelViewMatrixInvTrans, 1, GL_FALSE, &MVN[0][0]);
			break;
		}

						   /* ----- (c) Phong --------------------------------------------- */
		case SHADER_PHONG: {
			auto* sh = static_cast<Shader_Phong*>(
				&shader_list[shader_ID_mapper[SHADER_PHONG]].get());

			glUseProgram(sh->h_ShaderProgram);
			scene.upload_lights_to_current_prog();

			glUniformMatrix4fv(sh->loc_ModelViewProjectionMatrix, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(sh->loc_ModelViewMatrix, 1, GL_FALSE, &MV[0][0]);
			glUniformMatrix3fv(sh->loc_ModelViewMatrixInvTrans, 1, GL_FALSE, &MVN[0][0]);
			/* --- NEW: Kd 업로드 -------------------------------- */
			glUniform3fv(sh->loc_Kd, 1, &inst.material.diffuse[0]);
		
			break;
		}

						 /* ----- (d) 텍스처-Phong (기존) -------------------------------- */
		case SHADER_PHONG_TEXUTRE: {
			auto* sh = static_cast<Shader_Phong_Texture*>(
				&shader_list[shader_ID_mapper[SHADER_PHONG_TEXUTRE]].get());

			glUseProgram(sh->h_ShaderProgram);
			scene.upload_lights_to_current_prog();

			glUniformMatrix4fv(sh->loc_ModelViewProjectionMatrix, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(sh->loc_ModelViewMatrix, 1, GL_FALSE, &MV[0][0]);
			glUniformMatrix3fv(sh->loc_ModelViewMatrixInvTrans, 1, GL_FALSE, &MVN[0][0]);

			glActiveTexture(GL_TEXTURE0 + tex_id);
			glBindTexture(GL_TEXTURE_2D, texture_names[tex_id]);
			scene.apply_user_filter();
			glUniform1i(sh->loc_texture, tex_id);
			break;
		}
								 /* ── Static_Object::draw_object(...)  switch(eff) 부분 ───────── */
		case SHADER_SPOT_PHONG: {
			auto* sh = static_cast<Shader_Spot_Phong*>(
				&shader_list[shader_ID_mapper[SHADER_SPOT_PHONG]].get());

			glUseProgram(sh->h_ShaderProgram);
			scene.upload_lights_to_current_prog();

			glUniformMatrix4fv(sh->loc_ModelViewProjectionMatrix, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(sh->loc_ModelViewMatrix, 1, GL_FALSE, &MV[0][0]);
			glUniformMatrix3fv(sh->loc_ModelViewMatrixInvTrans, 1, GL_FALSE, &MVN[0][0]);

			/* --- NEW: 재질 필드별 업로드 ----------------------------- */
			glUniform4fv(sh->loc_mat_ambient, 1, &inst.material.ambient[0]);
			glUniform4fv(sh->loc_mat_diffuse, 1, &inst.material.diffuse[0]);
			glUniform4fv(sh->loc_mat_specular, 1, &inst.material.specular[0]);
			glUniform4fv(sh->loc_mat_emissive, 1, &inst.material.emission[0]);
			glUniform1f(sh->loc_mat_shininess, inst.material.exponent);
			break;
		}

		} /* switch */

		/* ----- (공통) 정점 배열 & 드로우 ----------------------------- */
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3 * n_triangles);
	}

	/* ---------- (4) 상태 복원 ---------------------------------------- */
	if (is_cat)
		glPolygonMode(GL_FRONT_AND_BACK, prevPoly[0]);               // ★ 되돌리기

	glBindVertexArray(0);
	glUseProgram(0);
}
