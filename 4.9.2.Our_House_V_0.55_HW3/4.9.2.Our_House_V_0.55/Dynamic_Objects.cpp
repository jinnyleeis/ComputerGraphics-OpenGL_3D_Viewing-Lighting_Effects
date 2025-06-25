#define _CRT_SECURE_NO_WARNINGS

#include "Scene_Definitions.h"
#include <glm/gtc/constants.hpp> 
#include <glm/gtc/matrix_inverse.hpp>   // 파일 맨 위



/* ───────────────────────────────────────────────────────────────
   “건물 1층 평면도”를 따라가는 경로 정의
   ─ x-y   : 이미지 상 눈금 그대로 사용  (0‥220 , 0‥160)
   ─ z     : 모두 바닥면 9 (wolf) / 20 (spider 기본) 로 고정
   ─ dur   : 해당 세그먼트를 몇 ms 동안 이동할지(속도 제어)
   ──────────────────────────────────────────────────────────── */
struct PathSeg {
	glm::vec3 from, to;
	float     dur;          // milli-seconds
};

/* ───── 1. 공용 헬퍼 ───── */
static inline bool is_walkable_xy(float x, float y) {
	int gx = glm::clamp(int(x / 10.f), 0, Scene::W - 1);
	int gy = glm::clamp(int(y / 10.f), 0, Scene::H - 1);
	return Scene::floor_mask[gy][gx] == 0;
}
static glm::vec3 path_pos_dir(unsigned int t_ms,
	const PathSeg* path, int n_seg,
	glm::vec3* dir_out = nullptr) {
	// 세그먼트 총 길이
	float cycle = 0.f;
	for (int i = 0; i < n_seg; i++) cycle += path[i].dur;

	float t = fmodf(static_cast<float>(t_ms), cycle);

	const PathSeg* seg = path;
	while (t > seg->dur) { t -= seg->dur; ++seg; }

	float u = t / seg->dur;                      // 0‥1
	glm::vec3 pos = glm::mix(seg->from, seg->to, u);
	glm::vec3 dir = glm::normalize(seg->to - seg->from);

	if (dir_out) *dir_out = dir;
	return pos;
}

/* ────────────────────────────────
   늑대 1층 외곽 + 내부 복도 경로
   ──────────────────────────────── */
static const PathSeg TIGER_PATH[] = {


	/* ── 안쪽 ㄷ자 복도 ───────────── */
	{{ 30,  95,  0}, {155,  95,  0}, 1100},
	{{155,  95,  0}, {155, 115,  0},  700},
	{{155, 115,  0}, { 85, 115,  0},  800},
	{{ 85, 115,  0}, { 85,  55,  0},  900},
	{{ 85,  25,  0}, { 30,  25,  0}, 1000},
	{{ 30,  25,  0}, { 40,  25,  0},  600}    // 출발점 복귀
};
static const int N_TIGER_SEG =
sizeof(TIGER_PATH) / sizeof(PathSeg);



static const float ROOF_Z = 55.f;   // 지붕 바로 위

// 1. 기존 TIGER_PATH(늑대 경로)와 완전히 겹치지 않는, 
//    1층 복도 안쪽을 따라 순회하는 SPIDER_PATH 예시
static const PathSeg SPIDER_PATH[] = {
	// ───────────────────────────────────────────────────────────────
	// 아래 구간들은 모두 z=20 (스파이더 바닥 높이) 고정
	// 각 좌표는 복도 안쪽을 반환(벽에 충돌하지 않도록)하며, 
	// 사각형 형태로 한 바퀴 순회합니다.
	// ───────────────────────────────────────────────────────────────

	// ① 남서쪽(출발점) (40,  25, 20) → (85,  25, 20)
	//    → 서쪽 복도 바닥을 따라 동쪽으로 이동
	{{ 40,  70, 0}, { 85, 70, 0},   800},

	// ② (85,  25, 20) → (85,  55, 20)
	//    → 남북 복도를 따라 북쪽으로 이동
	{{ 85,  70, 0}, { 85,  45, 20},  1000},

	// ③ (85,  55, 20) → (155, 55, 20)
	//    → 복도 안쪽(중간) 가로 복도를 따라 동쪽으로 이동
	{{ 85,  50, 20}, {90, 50, 0},   900},

	// ④ (155, 55, 20) → (155, 95, 20)
	//    → 동쪽 복도를 따라 북쪽으로 이동
	{{90,  55, 0}, {90,  80, 20},   800},

	// ⑤ (155, 95, 20) → ( 40, 95, 20)
	//    → 북쪽 복도 전체를 따라 서쪽으로 이동 (가장 긴 구간)
	{{90,  80, 20}, { 40,   80, 0},  1200},

	// ⑥ (40,  95, 20) → ( 40, 25, 20)
	//    → 서쪽 복도를 따라 남쪽으로 내려와 출발점 복귀
	{{ 40,  80, 0}, { 40,  70, 0},  1000}
};
static const int N_SPIDER_SEG = sizeof(SPIDER_PATH) / sizeof(PathSeg);



void Tiger_D::define_object() {
#define N_TIGER_FRAMES 12
	glm::mat4* cur_MM;
	Material* cur_material;
	flag_valid = true;

	for (int i = 0; i < N_TIGER_FRAMES; i++) {
		object_frames.emplace_back();
		sprintf(object_frames[i].filename,
			"Data/dynamic_objects/tiger/Tiger_%02d_triangles_vnt.geom", i);
		/* 2. ������Ʈ�� ���̾ƿ� */
		object_frames[i].n_fields = 8;      // v(3)+n(3)+t(2)
		object_frames[i].front_face_mode = GL_CCW; // CCW �� front

		/* 3. GPU ���� �غ� */
		object_frames[i].prepare_geom_of_static_object();

		/* 4. �ν��Ͻ� 1���� ��� �� ModelMatrix & Material ���� */
		object_frames[i].instances.emplace_back();
		cur_MM = &(object_frames[i].instances.back().ModelMatrix);

		/* ���� ������ / ���� ������ */
		*cur_MM = glm::scale(glm::mat4(1.0f), glm::vec3(0.21f));
		cur_material = &(object_frames[i].instances.back().material);

		cur_material->emission = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		cur_material->ambient = glm::vec4(0.19125f, 0.0735f, 0.0225f, 1.0f);
		cur_material->diffuse = glm::vec4(0.7038f, 0.27048f, 0.0828f, 1.0f);
		cur_material->specular = glm::vec4(0.256777f, 0.137622f, 0.086014f, 1.0f);
		cur_material->exponent = 128.0f * 0.1f;
	}
	
}

void Spider_D::define_object() {
#define N_SPIDER_FRAMES 16                 // spider_vnt_00 ~ 15
	glm::mat4* cur_MM;
	Material* cur_material;
	flag_valid = true;

	for (int i = 0; i < N_SPIDER_FRAMES; ++i) {
		object_frames.emplace_back();

		/* 1. ������ ���� ��� ���� */
		sprintf(object_frames[i].filename,
			"Data/dynamic_objects/spider/spider_vnt_%02d.geom", i);

		/* 2. ������Ʈ�� ����(VNT �� 8 floats) */
		object_frames[i].n_fields = 8;
		object_frames[i].front_face_mode = GL_CCW;
		object_frames[i].prepare_geom_of_static_object();

		/* 3. �ν��Ͻ�(1��) - ��ȯ & ���� */
		object_frames[i].instances.emplace_back();

		object_frames[i].tex_id = TEXTURE_ID_SPIDER;   // ← 추가

		cur_MM = &(object_frames[i].instances.back().ModelMatrix);
		* cur_MM = glm::scale(glm::mat4(1.f), glm::vec3(12.4f));


		cur_material = &(object_frames[i].instances.back().material);
		cur_material->emission = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
		cur_material->ambient = glm::vec4(0.12f, 0.08f, 0.02f, 1.0f); // ��ο� ���
		cur_material->diffuse = glm::vec4(0.15f, 0.25f, 0.05f, 1.0f);
		cur_material->specular = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
		cur_material->exponent = 128.0f * 0.3f;
	}
}


void Wolf_D::define_object() {
#define N_WOLF_FRAMES 17                
	glm::mat4* cur_MM;
	Material* cur_material;
	flag_valid = true;

	for (int i = 0; i < N_WOLF_FRAMES; ++i) {
		object_frames.emplace_back();

		/* 1. ������ ���ϸ� */
		sprintf(object_frames[i].filename,
			"Data/dynamic_objects/wolf/wolf_%02d_vnt.geom", i);

		/* 2. ������Ʈ�� ���̾ƿ� */
		object_frames[i].n_fields = 8;      // v(3)+n(3)+t(2)
		object_frames[i].front_face_mode = GL_CCW; // CCW �� front

		/* 3. GPU ���� �غ� */
		object_frames[i].prepare_geom_of_static_object();

		/* 4. �ν��Ͻ� 1���� ��� �� ModelMatrix & Material ���� */
		object_frames[i].instances.emplace_back();
		object_frames[i].tex_id = TEXTURE_ID_WOLF;
		cur_MM = &(object_frames[i].instances.back().ModelMatrix);

		/* ���� ������ / ���� ������ */
		glm::mat4 M = glm::mat4(1.f);
		M = glm::translate(M, glm::vec3(0.f, 0.f, -10.0f));
		M = glm::rotate(M, glm::half_pi<float>(), glm::vec3(1, 0, 0)); // X축 -90° : 발이 아래로
		M = glm::scale(M, glm::vec3(20.5f));                            // 크기 조정

		*cur_MM = M;

		cur_material = &(object_frames[i].instances.back().material);

		/* �ټ� ��Ӱ� �߼����� ���� (�ͽ��� ������) */
		cur_material->emission = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		cur_material->ambient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
		cur_material->diffuse = glm::vec4(0.45f, 0.45f, 0.45f, 1.0f);
		cur_material->specular = glm::vec4(0.25f, 0.25f, 0.25f, 1.0f);
		cur_material->exponent = 128.0f * 0.4f;
	}
}

/* ───────────────────── 20-면체 동적 오브젝트 ───────────────────── */
void Icosahedron_D::define_object() {
	/* ---- (1) 기하 생성 ---- */
	static const float X = .5257311121f, Z = .8506508084f;
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

	object_frames.emplace_back();
	Static_Object& frm = object_frames.back();

	/* (메타데이터) -------------------------------------------------- */
	frm.filename[0] = '\0';
	frm.n_fields = 6;          // 3 pos + 3 normal ❗
	frm.front_face_mode = GL_CCW;
	frm.object_id = STATIC_OBJECT_BUILDING;

	/* (버텍스 배열) -------------------------------------------------- */
	std::vector<float> verts;   verts.reserve(20 * 3 * 6);
	for (int f = 0; f < 20; ++f)
		for (int v = 0; v < 3; ++v) {
			int idx = tindices[f][v];

			/* 위치 */
			glm::vec3 P(vdata[idx][0],
				vdata[idx][1],
				vdata[idx][2]);

			/* 법선 = 정점 위치 정규화 */
			glm::vec3 N = glm::normalize(P);

			verts.insert(verts.end(),
				{ P.x, P.y, P.z,   N.x, N.y, N.z });
		}

	frm.n_triangles = 20;
	frm.vertices = (float*)malloc(verts.size() * sizeof(float));
	memcpy(frm.vertices, verts.data(), verts.size() * sizeof(float));

	/* (GPU 전송) ---------------------------------------------------- */
	frm.prepare_geom_of_static_object();

	/* (인스턴스 & 재질) --------------------------------------------- */
	frm.instances.emplace_back();
	frm.instances[0].ModelMatrix = glm::mat4(1.f);

	auto& mat = frm.instances[0].material;
	mat.diffuse = glm::vec4(0.0, 0.7, 1.0, 1.0);
	mat.ambient = glm::vec4(0);
	mat.specular = glm::vec4(0);

	flag_valid = true;
}



void Dynamic_Object::draw_object(glm::mat4& ViewMatrix,
	glm::mat4& ProjectionMatrix,
	SHADER_ID  shader_kind,
	std::vector<std::reference_wrapper<Shader>>& shader_list,
	int time_stamp)
{
	/* ---------- (A) 현재 프레임 데이터 ---------- */
	const int cur_idx = time_stamp % object_frames.size();
	Static_Object& frm = object_frames[cur_idx];

	const bool use_tex = (frm.tex_id >= 0);

	/* ---------- (B) 최종 사용할 셰이더 결정 ---------- */
	SHADER_ID eff;
	if (use_tex)
		eff = SHADER_PHONG_TEXUTRE;       // 텍스처 → Phong+Texture 고정
	else
		eff = shader_kind;               // 나머지는 Scene이 지정 (PHONG ↔ SPOT_PHONG)

	/* 항상 FILL 모드로 – 기존 설정 보존 */
	GLint prevPoly[2];
	glGetIntegerv(GL_POLYGON_MODE, prevPoly);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	/* ---------- (C) 셰이더 핸들 캐싱 ---------- */
	Shader_Simple* sh_simple = nullptr;
	Shader_Phong_Texture* sh_tx = nullptr;
	Shader_Phong* sh_phong = nullptr;
	Shader_Spot_Phong* sh_spot = nullptr;

	switch (eff) {
	case SHADER_SIMPLE:
		sh_simple = static_cast<Shader_Simple*>(
			&shader_list[shader_ID_mapper[SHADER_SIMPLE]].get()); break;
	case SHADER_PHONG:
		sh_phong = static_cast<Shader_Phong*>(
			&shader_list[shader_ID_mapper[SHADER_PHONG]].get());  break;
	case SHADER_SPOT_PHONG:
		sh_spot = static_cast<Shader_Spot_Phong*>(
			&shader_list[shader_ID_mapper[SHADER_SPOT_PHONG]].get()); break;
	case SHADER_PHONG_TEXUTRE:
		sh_tx = static_cast<Shader_Phong_Texture*>(
			&shader_list[shader_ID_mapper[SHADER_PHONG_TEXUTRE]].get()); break;
	}

	/* ---------- (D) 오브젝트별 ModelMatrix 계산 ---------- */
	glm::mat4 ModelMatrix(1.0f);
	const unsigned int t_ms = glutGet(GLUT_ELAPSED_TIME);

	switch (object_id) {
	case DYNAMIC_OBJECT_TIGER: {
		glm::vec3 dir, pos = path_pos_dir(t_ms, TIGER_PATH, N_TIGER_SEG, &dir);
		float heading = atan2f(dir.y, dir.x);
		ModelMatrix = glm::translate(glm::mat4(1.f), pos) *
			glm::rotate(glm::mat4(1.f), heading + glm::half_pi<float>(), glm::vec3(0, 0, 1));
		
		/* ②  모델좌표계 기준 라이트 원점·방향 정의  */
		const glm::vec4 Lpos_MC(0.f, 0.f, 40.f, 1.f);   // 호랑이 머리 위
		const glm::vec3 Ldir_MC(0.0f, 0.0f, -1.0f);       // 살짝 아래를 비춤

		/* ③ EC 변환 */
		glm::vec4 pos_EC =ModelMatrix * Lpos_MC;
		glm::vec3 dir_EC = glm::mat3(ModelMatrix) * Ldir_MC;

		scene.light[2].position = pos_EC;
		scene.light[2].spot_dir = dir_EC;

		/* ④ spot-Phong 셰이더에 uniform 업로드 */
		auto* sh = static_cast<Shader_Spot_Phong*>(
			&shader_list[shader_ID_mapper[SHADER_SPOT_PHONG]].get());

		glUseProgram(sh->h_ShaderProgram);
		glUniform4fv(scene.loc_light[2].position, 1, &pos_EC.x);
		glUniform3fv(scene.loc_light[2].spot_dir, 1, &dir_EC.x);
		glUseProgram(0);



		break;
	}
	case DYNAMIC_OBJECT_SPIDER: {
		glm::vec3 dir, pos = path_pos_dir(t_ms, SPIDER_PATH, N_SPIDER_SEG, &dir);
		float heading = atan2f(dir.y, dir.x);
		float roll = t_ms * 0.001f;
		ModelMatrix = glm::translate(glm::mat4(1.f), pos) *
			glm::rotate(glm::mat4(1.f), heading * 0.1f + glm::half_pi<float>(), glm::vec3(0, 0, 1)) *
			glm::rotate(glm::mat4(1.f), roll, glm::vec3(1, 0, 0));
		break;
	}
	case DYNAMIC_OBJECT_WOLF: {
		ModelMatrix = glm::translate(glm::mat4(1.f), scene.g_wolf.pos) *
			glm::rotate(glm::mat4(1.f), scene.g_wolf.heading, glm::vec3(0, 0, 1));
		break;
	}
	case DYNAMIC_OBJECT_ICOSAHEDRON: {
		static float theta = 0.0f; theta += 0.5f * TO_RADIAN;
		const glm::vec3 POS = { 125.f, 80.f, 60.f }; const float SCALE = 25.f;
		ModelMatrix = glm::translate(glm::mat4(1.f), POS) *
			glm::rotate(glm::mat4(1.f), theta, glm::vec3(0, 1, 0)) *
			glm::rotate(glm::mat4(1.f), theta * 0.7f, glm::vec3(1, 0, 1)) *
			glm::scale(glm::mat4(1.f), glm::vec3(SCALE));
		break;
	}
	}

	/* ---------- (E) 드로우 람다 ---------- */
	auto drawOne = [&](GLenum cullFace, int flagBlend, float alpha) {
		glCullFace(cullFace);

		for (const Instance& inst : frm.instances) {
			glm::mat4 MV = ViewMatrix * ModelMatrix * inst.ModelMatrix;
			glm::mat4 MVP = ProjectionMatrix * MV;
			glm::mat3 MVN = glm::inverseTranspose(glm::mat3(MV));

			switch (eff) {
			case SHADER_SIMPLE: {
				glUseProgram(sh_simple->h_ShaderProgram);
				glUniformMatrix4fv(sh_simple->loc_ModelViewProjectionMatrix, 1, GL_FALSE, &MVP[0][0]);
				glUniform3fv(sh_simple->loc_primitive_color, 1, &inst.material.diffuse[0]);
				glUniform1i(sh_simple->loc_u_flag_blending, flagBlend);
				glUniform1f(sh_simple->loc_u_fragment_alpha, alpha);
				break;
			}
			case SHADER_PHONG: {
				glUseProgram(sh_phong->h_ShaderProgram);
				scene.upload_lights_to_current_prog();
				glUniformMatrix4fv(sh_phong->loc_ModelViewProjectionMatrix, 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(sh_phong->loc_ModelViewMatrix, 1, GL_FALSE, &MV[0][0]);
				glUniformMatrix3fv(sh_phong->loc_ModelViewMatrixInvTrans, 1, GL_FALSE, &MVN[0][0]);
				/* --- NEW: Kd 업로드 -------------------------------- */
				glUniform3fv(sh_phong->loc_Kd, 1, &inst.material.diffuse[0]);
				glUniform1i(sh_phong->loc_u_flag_blending, flagBlend);
				glUniform1f(sh_phong->loc_u_fragment_alpha, alpha);
				break;
			}
			case SHADER_SPOT_PHONG: {
				glUseProgram(sh_spot->h_ShaderProgram);
				scene.upload_lights_to_current_prog();
				glUniformMatrix4fv(sh_spot->loc_ModelViewProjectionMatrix, 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(sh_spot->loc_ModelViewMatrix, 1, GL_FALSE, &MV[0][0]);
				glUniformMatrix3fv(sh_spot->loc_ModelViewMatrixInvTrans, 1, GL_FALSE, &MVN[0][0]);

				glUniform4fv(sh_spot->loc_mat_ambient, 1, &inst.material.ambient[0]);
				glUniform4fv(sh_spot->loc_mat_diffuse, 1, &inst.material.diffuse[0]);
				glUniform4fv(sh_spot->loc_mat_specular, 1, &inst.material.specular[0]);
				glUniform4fv(sh_spot->loc_mat_emissive, 1, &inst.material.emission[0]);
				glUniform1f(sh_spot->loc_mat_shininess, inst.material.exponent);

				glUniform1i(sh_spot->loc_u_flag_blending, flagBlend);
				glUniform1f(sh_spot->loc_u_fragment_alpha, alpha);

				break;
			}
			case SHADER_PHONG_TEXUTRE: {
				glUseProgram(sh_tx->h_ShaderProgram);
				scene.upload_lights_to_current_prog();
				glUniformMatrix4fv(sh_tx->loc_ModelViewProjectionMatrix, 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(sh_tx->loc_ModelViewMatrix, 1, GL_FALSE, &MV[0][0]);
				glUniformMatrix3fv(sh_tx->loc_ModelViewMatrixInvTrans, 1, GL_FALSE, &MVN[0][0]);

				glActiveTexture(GL_TEXTURE0 + frm.tex_id);
				glBindTexture(GL_TEXTURE_2D, texture_names[frm.tex_id]);
				scene.apply_user_filter();
				glUniform1i(sh_tx->loc_texture, frm.tex_id);
				break;
			}
			}

			glBindVertexArray(frm.VAO);
			glDrawArrays(GL_TRIANGLES, 0, 3 * frm.n_triangles);
		}
		};

	/* ---------- (F) 블렌딩(투명 20‑면체) 처리 ---------- */
	const bool doBlend =
		(object_id == DYNAMIC_OBJECT_ICOSAHEDRON) && scene.g_flag_ico_blend;

	if (doBlend) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDepthMask(GL_FALSE);
		glEnable(GL_CULL_FACE);

		drawOne(GL_BACK, 1, scene.g_ico_alpha);
		drawOne(GL_FRONT, 1, scene.g_ico_alpha);

		glDisable(GL_CULL_FACE);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
	}
	else {
		glDisable(GL_CULL_FACE);
		drawOne(GL_BACK, 0, 1.0f);
	}

	/* ---------- (G) 상태 복원 ---------- */
	glPolygonMode(GL_FRONT_AND_BACK, prevPoly[0]);
	glBindVertexArray(0);
	glUseProgram(0);
}

