#define _CRT_SECURE_NO_WARNINGS

#include "Scene_Definitions.h"
#include "Camera.h"  
#include "Texture_Helper.h"     // 파일 상단 include 목록에 추가

/* ── 디버깅 매크로 ─────────────────────────────────────────── */
#ifdef DEBUG_LIGHTS
#define DBG(fmt, ...) fprintf(stderr,"[DBG] " fmt "\n", ##__VA_ARGS__)
#define GL_CHECK()                                                         \
    { GLenum err = glGetError();                                             \
      if (err!=GL_NO_ERROR)                                                  \
         fprintf(stderr,"[GL_ERROR] 0x%X @ %s:%d\n",err,__FILE__,__LINE__); }
#else
#define DBG(...)
#define GL_CHECK()
#endif


Scene scene;

std::vector<Scene::WallRect> Scene::g_wall_rects;
unsigned int static_object_ID_mapper[N_MAX_STATIC_OBJECTS];
unsigned int dynamic_object_ID_mapper[N_MAX_DYNAMIC_OBJECTS];
unsigned int camera_ID_mapper[N_MAX_CAMERAS];
unsigned int shader_ID_mapper[N_MAX_SHADERS];
SHADING_MODE g_shading_mode = SHADE_SIMPLE;

GLuint texture_names[N_MAX_TEXTURES] = { 0 };

//static GLenum g_cur_filter = GL_LINEAR;   // 디폴트는 Linear
static GLenum g_cur_filter = GL_NEAREST;   // 디폴트는 Linear
/* Scene_Definitions.cpp – 전역 변수 정의 */
bool g_flag_dissolve = false;
bool g_flag_fresnel = false;

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



/* ---------------------------------------------------- */
/*  모든 라이트 uniform을 현재 바인드된 프로그램으로 전송  */
/* ---------------------------------------------------- */
void Scene::upload_lights_to_current_prog()
{
	GLint curProg = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &curProg);          // 현재 바인드된 쉐이더
	if (!curProg) return;                                 // 안전 검사용

	glUniform4fv(glGetUniformLocation(curProg, "u_global_ambient_color"),
		1, &scene.light[0].ambient[0]);

	for (int i = 0; i < NUMBER_OF_LIGHTS_SUPPORTED; ++i)
	{
		char buf[64];
		/*  ⬇︎ 콤마 연산자를 ( ... ) 로 감싸서 ‘하나의 인자’ 로 만든다 */
#define UL(name) (sprintf(buf, "u_light[%d]." name, i), \
                           glGetUniformLocation(curProg, buf))

		glUniform1i(UL("light_on"), scene.light[i].light_on);
		glUniform4fv(UL("position"), 1, &scene.light[i].position[0]);
		glUniform4fv(UL("ambient_color"), 1, &scene.light[i].ambient[0]);
		glUniform4fv(UL("diffuse_color"), 1, &scene.light[i].diffuse[0]);
		glUniform4fv(UL("specular_color"), 1, &scene.light[i].specular[0]);
		glUniform3fv(UL("spot_direction"), 1, &scene.light[i].spot_dir[0]);
		glUniform1f(UL("spot_exponent"), scene.light[i].spot_exp);
		glUniform1f(UL("spot_cutoff"), scene.light[i].spot_cut);
		glUniform4fv(UL("attenuation"), 1, &scene.light[i].atten[0]);

#undef UL
	}
}



void Scene::set_user_filter(unsigned int id)
{
	/* id == 0 : NEAREST  |  id == 1 : LINEAR */
	g_cur_mag_filter = (id == 0) ? GL_NEAREST : GL_LINEAR;
	g_cur_min_filter = g_cur_mag_filter;   // 밉맵 사용 안 함

	GLuint userTextures[] = {
		texture_names[TEXTURE_ID_SPIDER],
		texture_names[TEXTURE_ID_WOOD_TOWER]
	};

	for (GLuint tex : userTextures) {
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, g_cur_min_filter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, g_cur_mag_filter);

	}
	printf("[FILTER-SET ★] DONE  → g_cur_min_filter=0x%X  g_cur_mag_filter=0x%X\n\n",
		g_cur_min_filter, g_cur_mag_filter);

	glBindTexture(GL_TEXTURE_2D, 0);
}


#define GL_CHECK                                                                             \
    {                                                                                        \
        GLenum err = glGetError();                                                           \
        if (err != GL_NO_ERROR)                                                              \
            fprintf(stderr, "[GL_ERROR] 0x%X at %s:%d\n", err, __FILE__, __LINE__);          \
    }







/* 거미: 수직·수평을 섞은 3-D 경로 (그림의 중앙 기둥 → 천장 → 관람 통로) */
static const PathSeg SPIDER_PATH[] = {
	{{ 90,  80, 20}, { 90,  80,180}, 1800},   // 상승
	{{ 90,  80,180}, {150,  80,180}, 1000},   // 천장 동쪽
	{{150,  80,180}, {150, 130, 30}, 2000},   // 대각선 하강
	{{150, 130, 30}, { 60, 130, 30}, 1200},   // 서쪽
	{{ 60, 130, 30}, { 60,  30, 30}, 1800},   // 남쪽
	{{ 60,  30, 30}, { 90,  80, 20}, 1600}    // 출발점으로
};
static const int N_SPIDER_SEG = sizeof(SPIDER_PATH) / sizeof(PathSeg);

#define CAM_AXIS_LENGTH 20.0f
/*--------------------------------------------------------------*/
/*  모든 카메라 공통: ViewMatrix → (u,v,n,pos) & ModelMatrix_axis*/
/*--------------------------------------------------------------*/
inline void rebuild_cam_axes_and_mm(Camera& cam, float scale_mul = 20.f)
{
	/* (1) u,v,n,pos 계산 --------------*/
	glm::mat3 R = glm::mat3(cam.ViewMatrix);
	glm::mat3 RT = glm::transpose(R);
	cam.cam_view.uaxis = glm::vec3(R[0][0], R[1][0], R[2][0]);
	cam.cam_view.vaxis = glm::vec3(R[0][1], R[1][1], R[2][1]);
	cam.cam_view.naxis = glm::vec3(R[0][2], R[1][2], R[2][2]);
	glm::mat4 T = glm::mat4(RT) * cam.ViewMatrix;
	cam.cam_view.pos = -glm::vec3(T[3]);

	/* (2) 계층적 ModelMatrix ----------*/
	glm::mat4 M_world = glm::inverse(cam.ViewMatrix);
	cam.ModelMatrix_axis = M_world *
		glm::scale(glm::mat4(1.f),
			glm::vec3(CAM_AXIS_LENGTH * scale_mul));
}


void Scene::update_main_camera_follow_wolf() {

	Perspective_Camera& cam = camera_data.cam_main;

	/* 1) 늑대(소) 몸 전체 모델행렬 */
	glm::mat4 M_wolf =
		glm::translate(glm::mat4(1.f), g_wolf.pos) *
		glm::rotate(glm::mat4(1.f), g_wolf.heading, glm::vec3(0, 0, 1));

	/* 2) 머리-위 카메라 부착점 (정중앙) */
	const glm::vec3  EYE_OFFSET(0.f, 0.f, 5.f);
	glm::mat4        M_eye = glm::translate(glm::mat4(1.f), EYE_OFFSET);

	/* 3) 기본 시선 전방 + 사용자 yaw/pitch */
	glm::mat4 M_camRot =
		// 기본 tilt (X축 –90°)
		glm::rotate(glm::mat4(1.f), -glm::half_pi<float>(), glm::vec3(1, 0, 0)) *
		// W/S → 시야를 위/아래로
		glm::rotate(glm::mat4(1.f), -g_orbit.upDown, glm::vec3(1, 0, 0)) *
		// A/D → 좌/우로 둘러보기
		glm::rotate(glm::mat4(1.f), -g_orbit.leftRight, glm::vec3(0, 0, 1)) *
		// Q/E → 화면(머리) 기울이기
		glm::rotate(glm::mat4(1.f), -g_orbit.headTilt, glm::vec3(0, 1, 0)) *
		// Y축 뒤집기
		glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, -1.0f, 1.0f));




	/* 4) 최종 뷰행렬 */
	glm::mat4 M_cam = M_wolf * M_eye * M_camRot;
	cam.ViewMatrix = glm::inverse(M_cam);

	rebuild_cam_axes_and_mm(cam, 100.f);
	//printf("[AXIS_MM] pos = (%.1f, %.1f, %.1f)\n",
	//	cam.cam_view.pos.x, cam.cam_view.pos.y, cam.cam_view.pos.z);


}



void Axis_Object::define_axis() {
	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_axes), &vertices_axes[0][0], GL_STATIC_DRAW);

	// Initialize vertex array object.
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0));

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void Axis_Object::draw_axis(Shader_Simple* shader_simple, glm::mat4& ViewMatrix, glm::mat4& ProjectionMatrix) {
#define WC_AXIS_LENGTH		60.0f
	glm::mat4 ModelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(WC_AXIS_LENGTH, WC_AXIS_LENGTH, WC_AXIS_LENGTH));
	glm::mat4 ModelViewProjectionMatrix = ProjectionMatrix * ViewMatrix * ModelMatrix;

	glUseProgram(shader_simple->h_ShaderProgram);
	glUniformMatrix4fv(shader_simple->loc_ModelViewProjectionMatrix, 1, GL_FALSE, &ModelViewProjectionMatrix[0][0]);


	glBindVertexArray(VAO);
	glUniform3fv(shader_simple->loc_primitive_color, 1, axes_color[0]);
	glDrawArrays(GL_LINES, 0, 2);
	glUniform3fv(shader_simple->loc_primitive_color, 1, axes_color[1]);
	glDrawArrays(GL_LINES, 2, 2);
	glUniform3fv(shader_simple->loc_primitive_color, 1, axes_color[2]);
	glDrawArrays(GL_LINES, 4, 2);
	glBindVertexArray(0);
	glUseProgram(0);
}

#define WC_AXIS_LENGTH_2 0.4f

void Axis_Object::draw_axis_with_model(Shader_Simple* shader_simple,
	glm::mat4& ViewMatrix,
	glm::mat4& ProjectionMatrix,
	const glm::mat4& ModelMatrix)
{

	glm::mat4 ScaleMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.05f));

	glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix*ScaleMatrix;

	/* DEBUG: MVP 프린트 */
	// fprintf(stdout, "MVP[3] = (%.2f, %.2f, %.2f, %.2f)\n",
	//         MVP[3][0], MVP[3][1], MVP[3][2], MVP[3][3]);
	glUseProgram(shader_simple->h_ShaderProgram);
	glUniformMatrix4fv(shader_simple->loc_ModelViewProjectionMatrix, 1, GL_FALSE, &MVP[0][0]);


	glBindVertexArray(VAO);
	glUniform3fv(shader_simple->loc_primitive_color, 1, axes_color[0]);
	glDrawArrays(GL_LINES, 0, 2);
	glUniform3fv(shader_simple->loc_primitive_color, 1, axes_color[1]);
	glDrawArrays(GL_LINES, 2, 2);
	glUniform3fv(shader_simple->loc_primitive_color, 1, axes_color[2]);
	glDrawArrays(GL_LINES, 4, 2);
	glBindVertexArray(0);
	glUseProgram(0);
}
static void debug_axis_mm(const Camera& cam, const char* tag)
{
	/*
	fprintf(stderr,
		"[%s] POS=(%.1f, %.1f, %.1f)  | scale ≈ %.1f\n",
		tag,
		cam.ModelMatrix_axis[3][0],
		cam.ModelMatrix_axis[3][1],
		cam.ModelMatrix_axis[3][2],
		glm::length(glm::vec3(cam.ModelMatrix_axis[0])));
		*/
}


inline void update_axis_mm(Camera& cam)
{
	constexpr float AXIS_LEN = CAM_AXIS_LENGTH;    
	glm::mat4 M_camWorld = glm::inverse(cam.ViewMatrix);
	cam.ModelMatrix_axis = M_camWorld *
		glm::scale(glm::mat4(1.0f), glm::vec3(AXIS_LEN));
}


/* ──────────────────────────────────────────────────────────── *
 *  Scene::clock 수정 – 매틱마다 카메라 추적 갱신
 * ──────────────────────────────────────────────────────────── */
void Scene::clock(int clock_id)
{
	time_stamp = (time_stamp + 1U) % UINT_MAX; // 다른 동적 오브젝트 프레임용
	update_main_camera_follow_wolf();      // 늑대 추적
	rebuild_cam_axes_and_mm(camera_data.cam_cctv_d, 100.f);  // ★ 추가

	{
		auto& cam = scene.camera_data.cam_cctv_d;      // 편의 참조
		scene.light[1].position = glm::vec4(cam.cam_view.pos, 1.0f);   // WC eye
		scene.light[1].spot_dir = -cam.cam_view.naxis;                 // 시선 방향
	}

	g_ico_angle += 0.5f * TO_RADIAN;

}

void Scene::build_static_world() {
	static_geometry_data.building.define_object();
	static_object_ID_mapper[STATIC_OBJECT_BUILDING] = static_objects.size();
	static_objects.push_back(static_geometry_data.building);

	static_geometry_data.table.define_object();
	static_object_ID_mapper[STATIC_OBJECT_BUILDING] = static_objects.size();
	static_objects.push_back(static_geometry_data.table);

	static_geometry_data.teapot.define_object();
	static_object_ID_mapper[STATIC_OBJECT_BUILDING] = static_objects.size();
	static_objects.push_back(static_geometry_data.teapot);


	static_geometry_data.frame.define_object();
	static_object_ID_mapper[STATIC_OBJECT_BUILDING] = static_objects.size();
	static_objects.push_back(static_geometry_data.frame);


	

	static_geometry_data.bike.define_object();          // ID = BIKE
	static_object_ID_mapper[STATIC_OBJECT_BIKE] = static_objects.size();
	static_objects.push_back(static_geometry_data.bike);

	static_geometry_data.cat.define_object();           // ID = CAT
	static_object_ID_mapper[STATIC_OBJECT_CAT] = static_objects.size();
	static_objects.push_back(static_geometry_data.cat);

	static_geometry_data.ironman.define_object();       // ID = IRONMAN
	static_object_ID_mapper[STATIC_OBJECT_IRONMAN] = static_objects.size();
	static_objects.push_back(static_geometry_data.ironman);

	static_geometry_data.dragon.define_object();        // ID = DRAGON
	static_object_ID_mapper[STATIC_OBJECT_DRAGON] = static_objects.size();
	static_objects.push_back(static_geometry_data.dragon);

	static_geometry_data.wood_tower.define_object();    // ID = WOOD_TOWER
	static_object_ID_mapper[STATIC_OBJECT_WOOD_TOWER] = static_objects.size();
	static_objects.push_back(static_geometry_data.wood_tower);

	static_geometry_data.light.define_object();
	static_object_ID_mapper[STATIC_OBJECT_BUILDING] = static_objects.size();
	static_objects.push_back(static_geometry_data.light);
}

void Scene::build_dynamic_world() {
	dynamic_geometry_data.tiger_d.define_object();
	dynamic_object_ID_mapper[DYNAMIC_OBJECT_TIGER] = dynamic_objects.size();
	dynamic_objects.push_back(dynamic_geometry_data.tiger_d);

	// two different moving cows
	dynamic_geometry_data.spider_d.define_object();
	dynamic_object_ID_mapper[DYNAMIC_OBJECT_SPIDER] = dynamic_objects.size();
	dynamic_objects.push_back(dynamic_geometry_data.spider_d);

	dynamic_geometry_data.wolf_d.define_object();
	dynamic_object_ID_mapper[DYNAMIC_OBJECT_WOLF] = dynamic_objects.size();
	dynamic_objects.push_back(dynamic_geometry_data.wolf_d);

	dynamic_geometry_data.icosahedron_d.define_object();
	dynamic_object_ID_mapper[DYNAMIC_OBJECT_ICOSAHEDRON] = dynamic_objects.size();
	dynamic_objects.push_back(dynamic_geometry_data.icosahedron_d);





}


void Scene::create_camera_list(int w, int h, float aspect) {
	camera_list.clear();

	camera_data.cam_main.define_camera(w, h, aspect);
	camera_ID_mapper[CAMERA_MAIN] = camera_list.size();
	camera_list.push_back(camera_data.cam_main);

	camera_data.cam_front.define_camera(w, h, aspect);
	camera_ID_mapper[CAMERA_SIDE_FRONT] = camera_list.size();
	camera_list.push_back(camera_data.cam_front);

	camera_data.cam_top.define_camera(w, h, aspect);
	camera_ID_mapper[CAMERA_TOP] = camera_list.size();
	camera_list.push_back(camera_data.cam_top);

	camera_data.cam_side.define_camera(w, h, aspect);
	camera_ID_mapper[CAMERA_SIDE] = camera_list.size();
	camera_list.push_back(camera_data.cam_side);

	camera_data.cam_cctv_a.define_camera(w, h, aspect);
	camera_ID_mapper[CAMERA_CCTV_A] = camera_list.size();
	camera_list.push_back(camera_data.cam_cctv_a);

	camera_data.cam_cctv_b.define_camera(w, h, aspect);
	camera_ID_mapper[CAMERA_CCTV_B] = camera_list.size();
	camera_list.push_back(camera_data.cam_cctv_b);

	camera_data.cam_cctv_c.define_camera(w, h, aspect);
	camera_ID_mapper[CAMERA_CCTV_C] = camera_list.size();
	camera_list.push_back(camera_data.cam_cctv_c);

	camera_data.cam_cctv_d.define_camera(w, h, aspect);
	camera_ID_mapper[CAMERA_CCTV_D_REMOTE] = camera_list.size();
	camera_list.push_back(camera_data.cam_cctv_d);
}

void Scene::build_shader_list() {
	shader_data.shader_simple.prepare_shader();
	shader_ID_mapper[SHADER_SIMPLE] = shader_list.size();
	shader_list.push_back(shader_data.shader_simple);

	shader_data.shader_gouraud.prepare_shader();     // <-- NEW
	shader_ID_mapper[SHADER_GOURAUD] = shader_list.size();
	shader_list.push_back(shader_data.shader_gouraud);

	shader_data.shader_phong.prepare_shader();       // <-- NEW
	shader_ID_mapper[SHADER_PHONG] = shader_list.size();
	shader_list.push_back(shader_data.shader_phong);

	shader_data.shader_phong_texture.prepare_shader();
	shader_ID_mapper[SHADER_PHONG_TEXUTRE] = shader_list.size();
	shader_list.push_back(shader_data.shader_phong_texture);


	shader_data.shader_spot_phong.prepare_shader();
	shader_ID_mapper[SHADER_SPOT_PHONG] = shader_list.size();
	shader_list.push_back(shader_data.shader_spot_phong);



	shader_data.shader_lava.prepare_shader();          // NEW
	shader_ID_mapper[SHADER_LAVA] = shader_list.size();
	shader_list.push_back(shader_data.shader_lava);


	shader_data.shader_neon_fres.prepare_shader();       // NEW
	shader_ID_mapper[SHADER_NEON_FRESNEL] = shader_list.size();
	shader_list.push_back(shader_data.shader_neon_fres);
}


void Scene :: apply_user_filter()
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, scene.g_cur_min_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, scene.g_cur_mag_filter);
	GLint minF, magF;
glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &minF);
glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &magF);
//printf("[TEX] MIN=0x%X  MAG=0x%X\n", minF, magF);

}
/* -------------------------------------------------------------------- */
/*  Building1_vnt.geom ─> 벽 AABB 목록 추출                              */
/* -------------------------------------------------------------------- */
static void build_wall_rects(const char* filename)
{
	FILE* fp = fopen(filename, "rb");
	if (!fp) { fprintf(stderr, "cannot open %s\n", filename); exit(EXIT_FAILURE); }

	int n_tri;
	fread(&n_tri, sizeof(int), 1, fp);                 // 삼각형 개수

	/* 삼각형 3개 == 1쿼드(직사각형) *아님*  → 4개씩 묶어서 처리       */
	const int BYTES_PER_VERT = 8 * sizeof(float);     // v n t (8 floats)
	const int BYTES_PER_TRI = 3 * BYTES_PER_VERT;
	const int BYTES_PER_QUAD = 4 * BYTES_PER_TRI;     // 4 tri = 1 wall face

	/* 파일 전체를 한 번에 읽어 들인 뒤 XY 좌표만 뽑아낸다               */
	std::vector<float> buffer(n_tri * 3 * 8);
	fread(buffer.data(), BYTES_PER_TRI, n_tri, fp);
	fclose(fp);

	for (int q = 0; q < n_tri / 4; ++q) {
		float minx = 1e9f, maxx = -1e9f;
		float miny = 1e9f, maxy = -1e9f;

		for (int t = 0; t < 4; ++t)           // 4 tri
			for (int v = 0; v < 3; ++v) {     // 3 vert
				size_t idx = (q * 4 + t) * 3 * 8 + v * 8;
				float x = buffer[idx + 0];
				float y = buffer[idx + 1];
				minx = std::min(minx, x);  maxx = std::max(maxx, x);
				miny = std::min(miny, y);  maxy = std::max(maxy, y);
			}

		/* XY 둘 중 하나라도 두께(≈5) 보다 작다면 ‘벽’이 아님 → skip      */
		if ((maxx - minx) < 5.f || (maxy - miny) < 5.f) continue;

		scene.g_wall_rects.push_back({ minx, maxx, miny, maxy });
	}

	fprintf(stdout, "[BUILD] %zu wall-rects collected.\n", scene.g_wall_rects.size());
}




void Scene::initialize() {
	axis_object.define_axis();
	build_static_world();
	build_dynamic_world();
	create_camera_list(window.width, window.height, window.aspect_ratio);
	build_shader_list();
	build_wall_rects("Data/Building1_vnt.geom");

	/* 과제 2: PNG 텍스처 두 장 로딩 */
	glGenTextures(N_USER_TEXTURES, texture_names);

	load_png_to_texture("Data/dynamic_objects/my_spider_diff.png",
		texture_names[TEXTURE_ID_SPIDER], g_cur_filter);

	load_png_to_texture("Data/static_objects/my_woodtower_diff.png",
		texture_names[TEXTURE_ID_WOOD_TOWER], g_cur_filter);

	load_png_to_texture("Data/static_objects/my_lava_ring.png",
		texture_names[TEXTURE_ID_DRAGON_LAVA], g_cur_filter);


	/* ---------- (A) 월드 고정 램프 3개 ---------- */
	auto initWorld = [&](int idx, glm::vec3 pos) {
		light[idx].light_on = 1;
		light[idx].position = glm::vec4(pos, 1.0f);
		light[idx].ambient = { 0.2f, 0.2f, 0.2f, 1 };
		light[idx].diffuse = { 0.8f, 0.8f, 0.8f, 1 };
		light[idx].specular = { 0.8f, 0.8f, 0.8f, 1 };

		light[idx].spot_dir = { 0,0,-1 };
		light[idx].spot_cut = 30.f;
		light[idx].spot_exp = 20.f;
		};
	/*               중앙          서쪽        동쪽  */
	initWorld(0, { 120,100,49 });
	initWorld(3, { 0, 40,49 });
	initWorld(4, { 200, 50,49 });


	// 1 : 카메라 플래시(눈 좌표) – EC (0,0,10)


	/* ───────── CCTV-D Spotlight (#3) ───────── */
	light[1].light_on = 0;                    // 항상 ON (4키로 토글해도 됨)
	light[1].ambient = { 0.05f,0.05f,0.05f,1 };
	light[1].diffuse = { 1.0f ,1.0f ,0.9f ,1 };
	light[1].specular = { 1.0f ,1.0f ,0.9f ,1 };
	light[1].spot_cut = 20.f;                 // 좁은 원뿔
	light[1].spot_exp = 25.f;                 // 강한 집중


// 2 : 타이거 스포트라이트
light[2].light_on   = 0;              // ‘5’ 키로 토글
light[2].position   = { 0,0,30,1 };  // MC 좌표 (위에서 다시 변환됨)
light[2].diffuse    = { 1,0.9,0.7,1 };
light[2].specular   = { 1,0.9,0.7,1 };
light[2].spot_dir   = { 0,0,-1 };   // MC 기준
light[2].spot_cut   = 20.f;
light[2].spot_exp   = 15.f;


	for (int i = 0; i < 3; ++i) {
		const auto& L = light[i];
		fprintf(stderr,
			"[INIT] L%d on=%d  pos=(%.1f,%.1f,%.1f,%.1f)  cutoff=%.1f\n",
			i, L.light_on, L.position.x, L.position.y, L.position.z, L.position.w,
			L.spot_cut);
	}

}

/* ===============================================================
 *  Scene_Definitions.cpp
 *  draw_static_world()  –  우드타워 전용: Fill+양면+Depth OFF
 * ===============================================================*/
 /* ===============================================================
  * Scene_Definitions.cpp
  * Scene::draw_static_world()  –  Wood-Tower 전용: 항상 FILL,
  *                               다른 오브젝트의 폴리곤 모드를 건드리지 않음
  * ===============================================================*/
  /* ===============================================================
   * Scene::draw_static_world()   –   우드타워 전용 처리 개선
   *   · LINE 모드여도 항상 FILL
   *   · Cull OFF (양면)           – 필요 시만
   *   · 깊이 비교·기록은 그대로 유지  🔸
   * ===============================================================*/
void Scene::draw_static_world()
{
	/* 0) 전역 렌더 상태 백업 ------------------------------- */
	GLint     prevPolyMode[2];   glGetIntegerv(GL_POLYGON_MODE, prevPolyMode);
	GLboolean prevCullEnabled = glIsEnabled(GL_CULL_FACE);

	/* 1) 정적 오브젝트 루프 ------------------------------- */
	for (auto& obj_ref : static_objects) {
		Static_Object& obj = obj_ref.get();
		if (!obj.flag_valid) continue;

		const bool isWT = (obj.object_id == STATIC_OBJECT_WOOD_TOWER);

		/* ── (A) 우드타워 전용 상태 세팅 ─────────────────── */
		if (isWT) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);          // 항상 면
			glDisable(GL_CULL_FACE);                            // 양면
			/* 깊이 비교/기록은 그대로 둔다 🔸 */
		}

		/* ── (B) 실제 드로우 ─────────────────────────────── */
		obj.draw_object(ViewMatrix, ProjectionMatrix,
			shader_kind, shader_list);

		/* ── (C) 상태 복원 (우드타워에만 해당) ────────────── */
		if (isWT) {
			/* Cull 복구 */
			if (prevCullEnabled) glEnable(GL_CULL_FACE);
			else                 glDisable(GL_CULL_FACE);
			/* 폴리곤 모드 복구 (LINE 모드 등) */
			glPolygonMode(GL_FRONT_AND_BACK, prevPolyMode[0]);
		}
	}
}

// Scene_Definitions.cpp
void Scene::draw_dynamic_world() {
	/* ① 먼저 타이거의 모델행렬을 계산 – Dynamic_Objects와 동일한 식 */
	unsigned int t_ms = glutGet(GLUT_ELAPSED_TIME);

	glm::vec3 dir, posMC = path_pos_dir(t_ms,
		TIGER_PATH, N_TIGER_SEG, &dir);
	float heading = atan2f(dir.y, dir.x);

	
	/* ⑤ 원래 하던 것처럼 모든 동적 오브젝트 그리기 */
	for (auto& dobj_ref : dynamic_objects) {
		if (!dobj_ref.get().flag_valid) continue;
		dobj_ref.get().draw_object(ViewMatrix, ProjectionMatrix,
			shader_kind, shader_list, time_stamp);
	}
}


void Scene::draw_axis() {
	axis_object.draw_axis(static_cast<Shader_Simple*>(&shader_list[shader_ID_mapper[SHADER_SIMPLE]].get()),
		ViewMatrix, ProjectionMatrix);
}

void Scene::draw_axis_with_model(const Camera& cam) {
	axis_object.draw_axis_with_model(static_cast<Shader_Simple*>(&shader_list[shader_ID_mapper[SHADER_SIMPLE]].get()),
		ViewMatrix, ProjectionMatrix,cam.ModelMatrix_axis);
}


void Scene::draw_world() {

	/* 월드 고정 라이트(0) */
	glm::vec4 posEC = ViewMatrix * scene.light[0].position;
	glUniform4fv(scene.loc_light[0].position, 1, &posEC[0]);
	//printf("[frame] L0 posEC=(%.1f,%.1f,%.1f) on=%d",
	//	posEC.x, posEC.y, posEC.z, scene.light[0].light_on);

/* --- CCTV-D Spotlight (#3) --------------------------------- */
	glm::vec4 posEC1 = ViewMatrix * scene.light[3].position;
	glm::vec3 dirEC1 = glm::mat3(ViewMatrix) * scene.light[3].spot_dir;

	glUniform4fv(scene.loc_light[1].position, 1, &posEC1.x);
	glUniform3fv(scene.loc_light[1].spot_dir, 1, &dirEC1.x);

	


	draw_axis();
	if (show_camframe) {      
		// 1 키로 토글
		draw_axis_with_model(camera_data.cam_main);
		draw_axis_with_model(camera_data.cam_cctv_d);
		draw_axis_with_model(camera_data.cam_cctv_a);
		draw_axis_with_model(camera_data.cam_cctv_b);
		draw_axis_with_model(camera_data.cam_cctv_c);
	}
	draw_static_world();
	draw_dynamic_world();

}
const int Scene::floor_mask[Scene::H][Scene::W] = {
	/* y =  0 */ { /*0-7*/ 1,1,1,1,1,1,1,1,   /*8-15*/ 1,1,1,1,1,1,1,1,   /*16-22*/ 1,1,1,1,1,1,1 },
	/* y =  1 */ { /*0-7*/ 1,1,1,1,1,1,1,1,   /*8-15*/ 1,1,1,1,1,1,1,1,   /*16-22*/ 1,1,1,1,1,1,1 },
	/* y =  2 */ { /*0-7*/ 1,1,1,0,0,0,0,0,   /*8-15*/ 0,1,1,0,1,0,0,0,   /*16-22*/ 1,1,1,0,0,1,1 },
	/* y =  3 */ { /*0-7*/ 1,1,1,0,0,1,1,0,   /*8-15*/ 0,1,1,1,1,1,0,0,   /*16-22*/ 1,1,0,0,0,1,1 },
	/* y =  4 */ { /*0-7*/ 1,1,1,0,0,1,1,0,   /*8-15*/ 0,1,0,0,1,0,0,1,   /*16-22*/ 1,1,0,1,1,1,1 },
	/* y =  5 */ { /*0-7*/ 1,1,1,0,0,1,1,0,   /*8-15*/ 0,1,0,0,1,1,1,1,   /*16-22*/ 1,1,0,0,0,1,1 },
	/* y =  6 */ { /*0-7*/ 1,1,1,1,1,1,1,0,   /*8-15*/ 0,1,1,0,0,0,0,0,   /*16-22*/ 1,1,0,0,0,0,1 },
	/* y =  7 */ { /*0-7*/ 1,1,1,0,0,0,0,0,   /*8-15*/ 0,1,1,1,0,0,1,1,   /*16-22*/ 1,1,1,0,0,0,1 },
	/* y =  8 */ { /*0-7*/ 1,1,1,0,0,0,0,0,   /*8-15*/ 0,0,0,0,0,0,1,1,   /*16-22*/ 1,1,1,0,0,0,1 },
	/* y =  9 */ { /*0-7*/ 1,1,1,0,0,0,0,0,   /*8-15*/ 0,0,0,0,0,1,1,1,   /*16-22*/ 1,1,1,0,0,0,1 },
	/* y = 10 */ { /*0-7*/ 1,1,0,0,0,0,1,1,   /*8-15*/ 0,0,0,0,0,0,0,0,   /*16-22*/ 0,0,0,0,0,0,1 },
	/* y = 11 */ { /*0-7*/ 1,1,0,1,1,1,0,1,   /*8-15*/ 1,1,1,1,1,1,1,0,   /*16-22*/ 0,0,1,0,0,0,1 },
	/* y = 12 */ { /*0-7*/ 1,1,0,0,0,0,1,1,   /*8-15*/ 1,1,1,1,0,0,1,0,   /*16-22*/ 0,0,1,1,1,0,1 },
	/* y = 13 */ { /*0-7*/ 1,1,1,0,0,0,1,1,   /*8-15*/ 0,0,0,0,0,0,1,0,   /*16-22*/ 0,0,1,1,1,0,1 },
	/* y = 14 */ { /*0-7*/ 1,1,1,0,0,0,1,1,   /*8-15*/ 0,0,0,0,0,0,0,0,   /*16-22*/ 0,1,1,1,0,0,1 },
	/* y = 15 */ { /*0-7*/ 1,1,1,1,1,1,1,1,   /*8-15*/ 1,1,1,1,1,1,1,1,   /*16-22*/ 1,1,1,1,1,1,1 },
	/* y = 16 */ { /*0-7*/ 1,1,1,1,1,1,1,1,   /*8-15*/ 1,1,1,1,1,1,1,1,   /*16-22*/ 1,1,1,1,1,1,1 }
};


