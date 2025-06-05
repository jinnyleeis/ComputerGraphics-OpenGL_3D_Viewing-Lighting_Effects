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
	{{ 30,  95,  9}, {155,  95,  9}, 1100},
	{{155,  95,  9}, {155, 115,  9},  700},
	{{155, 115,  9}, { 85, 115,  9},  800},
	{{ 85, 115,  9}, { 85,  55,  9},  900},
	{{ 85,  25,  9}, { 30,  25,  9}, 1000},
	{{ 30,  25,  9}, { 40,  25,  9},  600}    // 출발점 복귀
};
static const int N_TIGER_SEG =
sizeof(TIGER_PATH) / sizeof(PathSeg);



static const float ROOF_Z = 55.f;   // 지붕 바로 위

static const PathSeg SPIDER_PATH[] = {
	// 중앙 홀 기둥 주변(실내)
	  {{115,  80, 20}, {115,  80, ROOF_Z}, 1200},  // 수직 상승
	  {{115,  80, ROOF_Z}, {200,  80, ROOF_Z}, 1200}, // 지붕 위 동쪽 이동
	  {{200,  80, ROOF_Z}, {200, 140, ROOF_Z},  800}, // 북쪽
	  {{200, 140, ROOF_Z}, { 40, 140, ROOF_Z}, 1600}, // 서쪽 끝까지
	  {{ 40, 140, ROOF_Z}, { 40,  60, ROOF_Z}, 1600}, // 남쪽
	  {{ 40,  60, ROOF_Z}, {115,  80, 20   }, 2000}  // 대각선 하강하여 출발점 복귀
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

void Cow_D::define_object() {
#define N_FRAMES_COW_1 1
#define N_FRAMES_COW_2 1
	glm::mat4* cur_MM;
	Material* cur_material;
	flag_valid = true;
	switch (object_id) {

		int n_frames;
	case DYNAMIC_OBJECT_COW_1:
		n_frames = N_FRAMES_COW_1;
		for (int i = 0; i < n_frames; i++) {
			object_frames.emplace_back();
			strcpy(object_frames[i].filename, "Data/cow_vn.geom");
			object_frames[i].n_fields = 6;
			object_frames[i].front_face_mode = GL_CCW;
			object_frames[i].prepare_geom_of_static_object();
			object_frames[i].instances.emplace_back();
			cur_MM = &(object_frames[i].instances.back().ModelMatrix);
			*cur_MM = glm::scale(glm::mat4(1.0f), glm::vec3(30.0f, 30.0f, 30.0f));
			cur_material = &(object_frames[i].instances.back().material);
			cur_material->emission = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			cur_material->ambient = glm::vec4(0.329412f, 0.223529f, 0.027451f, 1.0f);
			cur_material->diffuse = glm::vec4(0.780392f, 0.568627f, 0.113725f, 1.0f);
			cur_material->specular = glm::vec4(0.992157f, 0.941176f, 0.807843f, 1.0f);
			cur_material->exponent = 128.0f * 0.21794872f;
		}
		break;
	case DYNAMIC_OBJECT_COW_2:
		n_frames = N_FRAMES_COW_2;
		for (int i = 0; i < n_frames; i++) {
			object_frames.emplace_back();
			strcpy(object_frames[i].filename, "Data/cow_vn.geom");
			object_frames[i].n_fields = 6;
			object_frames[i].front_face_mode = GL_CCW;
			object_frames[i].prepare_geom_of_static_object();

			object_frames[i].instances.emplace_back();
			cur_MM = &(object_frames[i].instances.back().ModelMatrix);
			*cur_MM = glm::scale(glm::mat4(1.0f), glm::vec3(30.0f, 30.0f, 30.0f));
			cur_material = &(object_frames[i].instances.back().material);
			glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			cur_material->ambient = glm::vec4(0.25f, 0.25f, 0.25f, 1.0f);
			cur_material->diffuse = glm::vec4(0.4f, 0.4f, 0.4f, 1.0f);
			cur_material->specular = glm::vec4(0.774597f, 0.774597f, 0.774597f, 1.0f);
			cur_material->exponent = 128.0f * 0.6f;
		}
		break;
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
		*cur_MM = glm::scale(glm::mat4(1.0f), glm::vec3(24.40f));


		cur_material = &(object_frames[i].instances.back().material);
		cur_material->emission = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		cur_material->ambient = glm::vec4(0.02f, 0.08f, 0.02f, 1.0f); // ��ο� ���
		cur_material->diffuse = glm::vec4(0.05f, 0.25f, 0.05f, 1.0f);
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
	case DYNAMIC_OBJECT_COW_1:
		rotation_angle = (2 * time_stamp % 360) * TO_RADIAN;
		ModelMatrix = glm::rotate(ModelMatrix, -rotation_angle, glm::vec3(0.0f, 0.0f, 1.0f));
		break;
	case DYNAMIC_OBJECT_COW_2:
		rotation_angle = (5 * time_stamp % 360) * TO_RADIAN;
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(100.0f, 50.0f, 0.0f));
		ModelMatrix = glm::rotate(ModelMatrix, rotation_angle, glm::vec3(1.0f, 0.0f, 0.0f));
		break;
	case DYNAMIC_OBJECT_SPIDER: {
		glm::vec3 dir;
		glm::vec3 pos = path_pos_dir(t_ms, SPIDER_PATH, N_SPIDER_SEG, &dir);

		const float heading = atan2f(dir.y, dir.x);          // XY-평면 진행 방향
		ModelMatrix = glm::translate(glm::mat4(1.f), pos);
		ModelMatrix = glm::rotate(ModelMatrix,
			heading + glm::half_pi<float>(),
			glm::vec3(0, 0, 1));
		break;
	}

	case DYNAMIC_OBJECT_WOLF: {
		glm::mat4 T = glm::translate(glm::mat4(1.f), scene.g_wolf.pos);
		glm::mat4 R = glm::rotate(glm::mat4(1.f), scene.g_wolf.heading, glm::vec3(0, 0, 1));
		ModelMatrix = T * R;           // ← 이동·방향 모두 g_wolf 로부터
		break;
	}
							/* ───── 3. Dynamic_Object::draw_object() – Nathan 분기 추가 ───── */


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
}