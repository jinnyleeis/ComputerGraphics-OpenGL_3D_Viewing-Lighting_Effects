#define _CRT_SECURE_NO_WARNINGS

#include "Scene_Definitions.h"
#include <glm/gtc/constants.hpp> 



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
		cur_MM = &(object_frames[i].instances.back().ModelMatrix);
		*cur_MM = glm::scale(glm::mat4(1.0f), glm::vec3(7.40f));


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

/* ─────────────────────  20-면체 동적 오브젝트  ───────────────────── */
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

	object_frames.emplace_back();          // 프레임 하나면 끝
	
	Static_Object& frm = object_frames.back();
	frm.filename[0] = '\0';
	frm.n_fields = 3;                // 위치만
	frm.front_face_mode = GL_CCW;
	frm.object_id = STATIC_OBJECT_BUILDING; // 아무거나; 의미 없음

	/* 20*3 정점 → 배열 생성 */
	std::vector<float> verts;
	verts.reserve(20 * 3 * 3);
	for (int f = 0; f < 20; ++f)
		for (int v = 0; v < 3; ++v)
			for (int c = 0; c < 3; ++c)
				verts.push_back(vdata[tindices[f][v]][c]);

	frm.n_triangles = 20;
	frm.vertices = (float*)malloc(verts.size() * sizeof(float));
	memcpy(frm.vertices, verts.data(), verts.size() * sizeof(float));

	frm.prepare_geom_of_static_object();

	/* ---- (2) 인스턴스&재질 ---- */
	frm.instances.emplace_back();
	frm.instances[0].ModelMatrix = glm::mat4(1.f);          // 매 프레임에 덮어씀
	auto& mat = frm.instances[0].material;
	mat.diffuse = glm::vec4(0.0, 0.7, 1.0, 1.0);
	mat.ambient = mat.specular = glm::vec4(0);
	flag_valid = true;
};






void Dynamic_Object::draw_object(glm::mat4& ViewMatrix, glm::mat4& ProjectionMatrix, SHADER_ID shader_kind,
	std::vector<std::reference_wrapper<Shader>>& shader_list, int time_stamp) {
	int cur_object_index = time_stamp % object_frames.size();
	Static_Object& cur_object = object_frames[cur_object_index];
	glFrontFace(cur_object.front_face_mode);

	float t = 0; float u = 0; float s = 0;

	/* ② 실시간 시간(ms) 을 받아 경로 위치 계산 */
	const unsigned int t_ms = glutGet(GLUT_ELAPSED_TIME);


	float rotation_angle = 0.0f;
	glm::mat4 ModelMatrix = glm::mat4(1.0f);
	switch (object_id) {
	case DYNAMIC_OBJECT_TIGER:
	{
		glm::vec3 dir;
		glm::vec3 pos = path_pos_dir(t_ms, TIGER_PATH, N_TIGER_SEG, &dir);

		const float heading = atan2f(dir.y, dir.x);
		ModelMatrix = glm::translate(glm::mat4(1.f), pos);
		ModelMatrix = glm::rotate(ModelMatrix,
			heading + glm::half_pi<float>(),
			glm::vec3(0, 0, 1));
		break;

	}

	case DYNAMIC_OBJECT_SPIDER: {
		// 1) 경로 상 위치와 진행 방향(dir) 계산
		glm::vec3 dir;
		glm::vec3 pos = path_pos_dir(t_ms, SPIDER_PATH, N_SPIDER_SEG, &dir);
		// dir = normalize(seg.to - seg.from)

		// 2) heading: XY 평면 상의 진행 각도 (Z 축 기준 회전)
		float heading = atan2f(dir.y, dir.x); // -pi .. +pi 사이 값

		// 3) “바퀴 굴러가는” 회전 각도 계산
		//    여기서는 t_ms 에 비례하는 상수 속도를 주되, 부드럽게 보이도록 적당한 계수를 곱합니다.
		//    예: 𝜃 = (t_ms * 0.005) 라디안 (속도 계수는 필요에 따라 조절)
		float speed_factor = 0.005f; // ← 이 값을 바꾸면 굴러가는 속도 조절 가능
		float wheelRotation = t_ms * speed_factor; // time-based roll angle

		// 4) Translate -> Rotate_Z(heading) -> Rotate_X(wheelRotation) 순으로 ModelMatrix 구성
		//    - 로컬 X축(1,0,0)을 기준으로 “굴러가는” 회전을 추가한다.
		glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
		glm::mat4 R_heading = glm::rotate(glm::mat4(1.0f), heading + glm::half_pi<float>(), glm::vec3(0, 0, 1));
		glm::mat4 R_roll = glm::rotate(glm::mat4(1.0f), wheelRotation, glm::vec3(1, 0, 0));

		// 최종 ModelMatrix: (위치 이동) * (방향 향하게) * (굴러가는 회전)
		ModelMatrix = T * R_heading * R_roll;
		break;
	}

	case DYNAMIC_OBJECT_WOLF: {
		glm::mat4 T = glm::translate(glm::mat4(1.f), scene.g_wolf.pos);
		glm::mat4 R = glm::rotate(glm::mat4(1.f), scene.g_wolf.heading, glm::vec3(0, 0, 1));
		ModelMatrix = T * R;           // ← 이동·방향 모두 g_wolf 로부터
		break;
	}

	case DYNAMIC_OBJECT_ICOSAHEDRON: {            // ★ NEW ★
		/* ① 회전 각 누적 */
		static float g_ico_angle = 0.0f;
		g_ico_angle += 0.5f * TO_RADIAN;          // 속도 조절 가능

		/* ② 변환 행렬 */
		const glm::vec3 POS = { 125.f, 80.f, 60.f };
		const float     SCALE = 25.f;
		glm::mat4  S = glm::scale(glm::mat4(1.f), glm::vec3(SCALE));
		glm::mat4  R1 = glm::rotate(glm::mat4(1.f), g_ico_angle, glm::vec3(0, 1, 0));
		glm::mat4  R2 = glm::rotate(glm::mat4(1.f), g_ico_angle * 0.7f, glm::vec3(1, 0, 1));
		glm::mat4  T = glm::translate(glm::mat4(1.f), POS);
		ModelMatrix = T * R1 * R2 * S;


		/* ---- (B) 20-면체일 때만 투명/불투명 두 가지 경로 ---- */
		const bool isICO = (object_id == DYNAMIC_OBJECT_ICOSAHEDRON);
		const bool doBlend = (isICO && scene.g_flag_ico_blend);

		if (doBlend) {
			/* ① GL 상태 설정 ---------------------------------- */
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDepthMask(GL_FALSE);               // 깊이 버퍼 쓰기 off
			glEnable(GL_CULL_FACE);

			/* ② 두 번 그리기 : BACK ➜ FRONT ------------------- */
			GLenum passes[2] = { GL_BACK, GL_FRONT };
			for (int p = 0; p < 2; ++p) {
				glCullFace(passes[p]);

				for (int i = 0; i < cur_object.instances.size(); ++i) {
					glm::mat4 MVP = ProjectionMatrix * ViewMatrix *
						ModelMatrix *
						cur_object.instances[i].ModelMatrix;

					Shader_Simple* sh =
						static_cast<Shader_Simple*>(&shader_list[shader_ID_mapper[shader_kind]].get());

					glUseProgram(sh->h_ShaderProgram);
					glUniformMatrix4fv(sh->loc_ModelViewProjectionMatrix, 1, GL_FALSE, &MVP[0][0]);
					glUniform3fv(sh->loc_primitive_color, 1,
						&cur_object.instances[i].material.diffuse[0]);
					glUniform1i(sh->loc_u_flag_blending, 0);
					glUniform1f(sh->loc_u_fragment_alpha, 1.0f);

					glBindVertexArray(cur_object.VAO);
					glDrawArrays(GL_TRIANGLES, 0, 3 * cur_object.n_triangles);
				}
			}

			Shader_Simple* sh =
				static_cast<Shader_Simple*>(&shader_list[shader_ID_mapper[shader_kind]].get());

			/* ③ 상태 원복 ------------------------------------- */
			glBindVertexArray(0);
			glUseProgram(sh->h_ShaderProgram);      // 다시 바인딩
			glUniform1i(sh->loc_u_flag_blending, 0);
			glUseProgram(0);
			glDisable(GL_CULL_FACE);
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);

		}


	}

								   for (int i = 0; i < cur_object.instances.size(); i++) {
									   glm::mat4 ModelViewProjectionMatrix = ProjectionMatrix * ViewMatrix * ModelMatrix * cur_object.instances[i].ModelMatrix;
									   switch (shader_kind) {
									   case SHADER_SIMPLE:
										   Shader_Simple* shader_simple_ptr = static_cast<Shader_Simple*>(&shader_list[shader_ID_mapper[shader_kind]].get());
										   glUseProgram(shader_simple_ptr->h_ShaderProgram);
										   glUniformMatrix4fv(shader_simple_ptr->loc_ModelViewProjectionMatrix, 1, GL_FALSE,
											   &ModelViewProjectionMatrix[0][0]);
										   glUniform3f(shader_simple_ptr->loc_primitive_color, cur_object.instances[i].material.diffuse.r,
											   cur_object.instances[i].material.diffuse.g, cur_object.instances[i].material.diffuse.b);
										   break;
									   }
									   glBindVertexArray(cur_object.VAO);
									   glDrawArrays(GL_TRIANGLES, 0, 3 * cur_object.n_triangles);
									   glBindVertexArray(0);
									   glUseProgram(0);
								   }

								   break;

	}
}