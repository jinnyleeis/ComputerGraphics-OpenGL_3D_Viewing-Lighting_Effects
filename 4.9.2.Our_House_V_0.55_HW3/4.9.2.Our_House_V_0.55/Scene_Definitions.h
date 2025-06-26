#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "Shaders/LoadShaders.h"
#include "Camera.h"
#include <bitset>
#include <iostream>

#define BUFFER_OFFSET(offset) ((GLvoid *) (offset))
#define TO_RADIAN 0.01745329252f  
#define TO_DEGREE 57.295779513f


#define N_MAX_STATIC_OBJECTS		10
#define N_MAX_DYNAMIC_OBJECTS		11
#define N_MAX_CAMERAS		10
#define N_MAX_SHADERS		10

#define N_MAX_TEXTURES 16       

static const int W = 23, H = 17;

extern unsigned int static_object_ID_mapper[N_MAX_STATIC_OBJECTS];
extern unsigned int dynamic_object_ID_mapper[N_MAX_DYNAMIC_OBJECTS];
extern unsigned int camera_ID_mapper[N_MAX_CAMERAS];
extern unsigned int shader_ID_mapper[N_MAX_SHADERS];

#define NUMBER_OF_LIGHTS_SUPPORTED 6   

constexpr int WORLD_LIGHT_IDS[3] = { 0, 3, 4 };

struct Light_Parameters {
	int   light_on = 0;                  // 0: off, 1: on
	glm::vec4 position = { 0,0,1,0 };    // WC or EC (w=1: point, w=0: dir)
	glm::vec4 ambient = { 0,0,0,1 };
	glm::vec4 diffuse = { 0,0,0,1 };
	glm::vec4 specular = { 0,0,0,1 };
	glm::vec3 spot_dir = { 0,0,-1 };     // only for spot
	float     spot_exp = 0.0f;
	float     spot_cut = 180.0f;       // 180 ⇒ no spot
	glm::vec4 atten = { 1,0,0,0 };    // .w≠0 ⇒ no attenuation
};



struct loc_Light_Parameters {
	GLint light_on, position;
	GLint ambient, diffuse, specular;
	GLint spot_dir, spot_exp, spot_cut, atten;
};
/* ─────────────────────────────────────────────────────────── */

enum STATIC_OBJECT_ID {
	STATIC_OBJECT_BUILDING = 0, STATIC_OBJECT_TABLE,
	STATIC_OBJECT_WOOD_TOWER,
	STATIC_OBJECT_TEAPOT, 
	STATIC_OBJECT_FRAME, 
	STATIC_OBJECT_BIKE, STATIC_OBJECT_CAT, 
	STATIC_OBJECT_IRONMAN, STATIC_OBJECT_DRAGON, 
	STATIC_OBJECT_LIGHT,
};

enum DYNAMIC_OBJECT_ID {
	DYNAMIC_OBJECT_TIGER = 0,
	DYNAMIC_OBJECT_SPIDER, DYNAMIC_OBJECT_WOLF, 
	DYNAMIC_OBJECT_ICOSAHEDRON

};

/* 사용자-지정 텍스처 인덱스 */
enum USER_TEXTURE_ID {
	TEXTURE_ID_FLOOR = 0,        // 이미 예제에서 사용
	TEXTURE_ID_SPIDER = 1,       // 동적 오브젝트
	TEXTURE_ID_WOLF = 1,       // 동적 오브젝트
	TEXTURE_ID_WOOD_TOWER = 2,   // 정적 오브젝트
	TEXTURE_ID_DRAGON_LAVA,
	N_USER_TEXTURES
};
extern GLuint texture_names[N_MAX_TEXTURES];

enum SHADER_ID { SHADER_SIMPLE = 0, SHADER_GOURAUD, SHADER_PHONG, SHADER_PHONG_TEXUTRE, SHADER_SPOT_PHONG,
	SHADER_LAVA,             //  ← NEW (7번 : 드래곤)
	SHADER_NEON_FRESNEL           //  ← NEW (8번 : 아이언맨)
};
enum SHADING_MODE { SHADE_SIMPLE = 0, SHADE_GOURAUD, SHADE_PHONG };
extern SHADING_MODE g_shading_mode;      // <-- NEW
/* ───────── 효과 토글 플래그 ────────── */
extern bool g_flag_dissolve;      // ‘7’ key
extern bool g_flag_fresnel;       // ‘8’ key

struct Shader {
	ShaderInfo shader_info[3];
	GLuint h_ShaderProgram; // handle to shader program
	GLint loc_ModelViewProjectionMatrix;
	GLint  loc_u_flag_blending;
	GLint loc_u_fragment_alpha;


	Shader() {
		h_ShaderProgram = NULL;
		loc_ModelViewProjectionMatrix = -1;
		shader_info[0] = shader_info[1] = shader_info[2] = { NULL, NULL };
	}
	virtual void prepare_shader() = 0;
};

struct Shader_Simple : Shader {
	GLint loc_primitive_color;
	void prepare_shader();
};

struct Shader_Phong_Texture : Shader {
	GLint loc_ModelViewProjectionMatrix;
	GLint loc_ModelViewMatrix;
	GLint loc_ModelViewMatrixInvTrans;
	GLint loc_texture;

	void prepare_shader();
};

/* ─── Gouraud per-vertex ─────────────────────────────────────────── */
struct Shader_Gouraud : Shader {
	GLint loc_ModelViewMatrix, loc_ModelViewMatrixInvTrans;
	void  prepare_shader() override;
};

/* ─── Phong per-fragment ─────────────────────────────────────────── */
struct Shader_Phong : Shader {
	GLint loc_ModelViewMatrix, loc_ModelViewMatrixInvTrans;
	GLint loc_Kd;        // 🔹추가
	void  prepare_shader() override;
};

/* ─── Spot-Phong (새 셰이더) ──────────────────────────────────────── */
struct Shader_Spot_Phong : Shader {
	GLint loc_ModelViewMatrix, loc_ModelViewMatrixInvTrans;
	
	// 재질 필드별 위치
	GLint loc_mat_ambient, loc_mat_diffuse, loc_mat_specular;
	GLint loc_mat_emissive, loc_mat_shininess;
	void  prepare_shader() override;

};


struct Shader_Lava : Shader {
	GLint loc_MVP, loc_MV, loc_time;
	GLint loc_uParams, loc_vParams, loc_tex;
	void prepare_shader() override;
};

struct Shader_FresnelNeon : Shader {
	GLint loc_ModelViewProjectionMatrix, loc_ModelViewMatrix, loc_MVN;
	GLint loc_time, loc_emissionColor;
	void prepare_shader() override {
		shader_info[0] = { GL_VERTEX_SHADER,   "Shaders/fresnel.vert" };
		shader_info[1] = { GL_FRAGMENT_SHADER, "Shaders/fresnel.frag" };
		shader_info[2] = { GL_NONE, nullptr };

		h_ShaderProgram = LoadShaders(shader_info);
		glUseProgram(h_ShaderProgram);
		loc_ModelViewProjectionMatrix = glGetUniformLocation(h_ShaderProgram, "u_MVP");
		loc_ModelViewMatrix = glGetUniformLocation(h_ShaderProgram, "u_MV");
		loc_MVN = glGetUniformLocation(h_ShaderProgram, "u_MVN");
		loc_time = glGetUniformLocation(h_ShaderProgram, "u_time");
		loc_emissionColor = glGetUniformLocation(h_ShaderProgram, "u_emiCol");
		glUseProgram(0);
	}
};



struct Shader_Data {
	Shader_Simple shader_simple;
	 Shader_Phong shader_phong;
	 Shader_Gouraud shader_gouraud;
	// Shader_Phong_Texture Shader_Phong_texture;
	Shader_Phong_Texture  shader_phong_texture;
	Shader_Spot_Phong    shader_spot_phong;
	Shader_Lava          shader_lava;   
	Shader_FresnelNeon shader_neon_fres;
};

struct Material {
	glm::vec4 emission, ambient, diffuse, specular;
	GLfloat exponent;
};

struct Instance {
	glm::mat4 ModelMatrix;
	Material material;
};

struct Axis_Object {
	GLuint VBO, VAO;
	GLfloat vertices_axes[6][3] = {
		{ 0.0f, 0.0f, 0.0f },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 0.0f },{ 0.0f, 1.0f, 0.0f },
		{ 0.0f, 0.0f, 0.0f },{ 0.0f, 0.0f, 1.0f }
	};
	GLfloat axes_color[3][3] = { { 1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f, 0.0f },{ 0.0f, 0.0f, 1.0f } };

	void define_axis();
	void draw_axis(Shader_Simple* shader_simple, glm::mat4& ViewMatrix, glm::mat4& ProjectionMatrix);
	void draw_axis_with_model(Shader_Simple* shader_simple,
		glm::mat4& ViewMatrix,
		glm::mat4& ProjectionMatrix,
		const glm::mat4& ModelMatrix);
};

struct Static_Object { // an object that does not move
	STATIC_OBJECT_ID object_id;
	char filename[512]; // where to read geometry data
	int n_fields; // 3/6/8 where3 floats for vertex, 3 floats for normal, and 2 floats for texcoord
	int n_triangles;
	GLfloat* vertices; // pointer to vertex array data
	GLuint VBO, VAO; // handles to vertex buffer object and vertex array object
	GLenum front_face_mode; // clockwise or counter-clockwise

	std::vector<Instance> instances;
	bool flag_valid;
	int  tex_id = -1;            // ← 텍스처 ID (-1 이면 비텍스처)

	Static_Object() {
		filename[0] = '\0';
		vertices = nullptr;   // ★
		n_fields = 0;         // ★
		n_triangles = 0;         // ★
		VBO = VAO = 0;
		flag_valid = false;
	}
	Static_Object(STATIC_OBJECT_ID _object_id) : object_id(_object_id) {
		instances.clear();
	}
	void read_geometry(int bytes_per_primitive);
	void prepare_geom_of_static_object();
	void draw_object(glm::mat4& ViewMatrix, glm::mat4& ProjectionMatrix, SHADER_ID shader_kind,
		std::vector<std::reference_wrapper<Shader>>& shader_list);
};

struct Building : public Static_Object {
	Building(STATIC_OBJECT_ID _object_id) : Static_Object(_object_id) {}
	void define_object();
};

struct Table : public Static_Object {
	Table(STATIC_OBJECT_ID _object_id) : Static_Object(_object_id) {}
	void define_object();
};

struct Teapot : public Static_Object {
	Teapot(STATIC_OBJECT_ID _object_id) : Static_Object(_object_id) {}
	void define_object();
};


struct Frame : public Static_Object {
	Frame(STATIC_OBJECT_ID _object_id) : Static_Object(_object_id) {}
	void define_object();
};


struct Bike : public Static_Object {
	Bike(STATIC_OBJECT_ID _object_id) : Static_Object(_object_id) {}
	void define_object();
};

struct Cat : public Static_Object {
	Cat(STATIC_OBJECT_ID _object_id) : Static_Object(_object_id) {}
	void define_object();
};

struct Ironman : public Static_Object {
	Ironman(STATIC_OBJECT_ID _object_id) : Static_Object(_object_id) {}
	void define_object();
};

struct Dragon : public Static_Object {
	Dragon(STATIC_OBJECT_ID _object_id) : Static_Object(_object_id) {}
	void define_object();
};

struct Wood_Tower : public Static_Object {
	Wood_Tower(STATIC_OBJECT_ID _object_id) : Static_Object(_object_id) {}
	void define_object();
};


struct Light : public Static_Object {
	Light(STATIC_OBJECT_ID _object_id) : Static_Object(_object_id) {}
	void define_object();
};



struct Static_Geometry_Data {
	Building building{ STATIC_OBJECT_BUILDING };
	Table table{ STATIC_OBJECT_TABLE };
	Teapot teapot{ STATIC_OBJECT_TEAPOT };
	Frame frame{ STATIC_OBJECT_FRAME };
	Bike bike{ STATIC_OBJECT_BIKE };
	Cat cat{ STATIC_OBJECT_CAT };
	Ironman ironman{ STATIC_OBJECT_IRONMAN };
	Dragon dragon{ STATIC_OBJECT_DRAGON };
	Wood_Tower wood_tower{ STATIC_OBJECT_WOOD_TOWER };
	Light light{ STATIC_OBJECT_LIGHT };
	

};

struct Dynamic_Object { // an object that moves
	DYNAMIC_OBJECT_ID object_id;
	std::vector<Static_Object> object_frames;
	bool flag_valid;

	Dynamic_Object() {}
	Dynamic_Object(DYNAMIC_OBJECT_ID _object_id) : object_id(_object_id) {
		object_frames.clear();
	}

	void draw_object(glm::mat4& ViewMatrix, glm::mat4& ProjectionMatrix, SHADER_ID shader_kind,
		std::vector<std::reference_wrapper<Shader>>& shader_list, int time_stamp);
};

struct Tiger_D : public Dynamic_Object {
	Tiger_D(DYNAMIC_OBJECT_ID _object_id) : Dynamic_Object(_object_id) {}
	void define_object();
};


struct Spider_D : public Dynamic_Object {
	Spider_D(DYNAMIC_OBJECT_ID _object_id) : Dynamic_Object(_object_id) {}
	void define_object();
};

struct Wolf_D : public Dynamic_Object {
	Wolf_D(DYNAMIC_OBJECT_ID _object_id) : Dynamic_Object(_object_id) {}
	void define_object();
};

struct Icosahedron_D : public Dynamic_Object {
	Icosahedron_D(DYNAMIC_OBJECT_ID _object_id) : Dynamic_Object(_object_id) {}
	void define_object();
};


struct Dynamic_Geometry_Data {
	Tiger_D tiger_d{ DYNAMIC_OBJECT_TIGER };
	Spider_D spider_d{ DYNAMIC_OBJECT_SPIDER };
	Wolf_D wolf_d{ DYNAMIC_OBJECT_WOLF };
	Icosahedron_D icosahedron_d{ DYNAMIC_OBJECT_ICOSAHEDRON };
};

struct Window {
	int width, height;
	float aspect_ratio;
};

struct Scene {
	unsigned int time_stamp;
	Static_Geometry_Data static_geometry_data;
	std::vector<std::reference_wrapper<Static_Object>> static_objects;

	Dynamic_Geometry_Data dynamic_geometry_data;
	std::vector<std::reference_wrapper<Dynamic_Object>> dynamic_objects;

	Camera_Data camera_data;
	std::vector<std::reference_wrapper<Camera>> camera_list;

	Shader_Data shader_data;
	std::vector<std::reference_wrapper<Shader>> shader_list;
	SHADER_ID shader_kind;

	Window window; // for a better code, this must be defined in another structure!

	glm::mat4 ViewMatrix;
	glm::mat4 ProjectionMatrix;

	Axis_Object axis_object;
	static constexpr int W = 23;   // number of columns in each string
	static constexpr int H = 17;   // number of rows

	static const int floor_mask[H][W];


	GLenum g_cur_min_filter;
	GLenum g_cur_mag_filter;

	Light_Parameters  light[NUMBER_OF_LIGHTS_SUPPORTED];
	loc_Light_Parameters loc_light[NUMBER_OF_LIGHTS_SUPPORTED];
	GLint loc_global_ambient;

	void set_user_filter(unsigned int id);



	bool show_axes = false;
	bool show_camframe = false;   // (신규) 카메라 프레임 RGB

	bool   g_flag_ico_blend = false; // ‘6' 토글 여부를 나타냄 
	float  g_ico_alpha = 0.5f; // 0.0~1.0 (‘+’,‘-’)
	float  g_ico_angle = 0.0f; //  회전 누적각


	/* ── Mouse-driven wolf -------------------------------------------------- */
	struct WolfCtrl {
		glm::vec3 pos = { 30.f, 30.f, 9.f };  // 초기 위치
		float     heading = glm::half_pi<float>();   // +Y 방향
	} g_wolf;

	/*──────────────── 메인 카메라 Orbit ───────*/
	struct CamOrbit {
		float leftRight = 0.0f;  // A/D: 좌우 돌려보기
		float upDown = 0.0f;  // W/S: 위/아래 보기
		float headTilt = 0.0f;  // Q/E: 머리(화면) 기울이기
		float dist = 400.f;               // 늑대까지 거리
	} g_orbit;
	/* -------------------------------------------------------------------- */
/*  WallRect : XY 평면에서 벽 하나의 AABB                               */
/* -------------------------------------------------------------------- */
	struct WallRect {
		float minx, maxx, miny, maxy;
		bool contains(float x, float y) const {
			return (x >= minx && x <= maxx && y >= miny && y <= maxy);
		}
	};

	/* 벽 AABB 들을 담아 둘 전역 컨테이너 */
	static std::vector<WallRect> g_wall_rects;




	Scene()
		: g_cur_min_filter(GL_LINEAR)   // 초기값: LINEAR
		, g_cur_mag_filter(GL_LINEAR)   // 초기값: LINEAR
	{
		time_stamp = 0;
		static_objects.clear();
		shader_list.clear();
		//shader_kind = SHADER_SIMPLE;
		//shader_kind = SHADER_PHONG;
	    shader_kind = SHADER_SPOT_PHONG;

		ViewMatrix = ProjectionMatrix = glm::mat4(1.0f);
	//	g_shading_mode = SHADE_SIMPLE;       // <-- NEW
	}


	void clock(int clock_id);
	void build_static_world();
	void build_dynamic_world();
	void create_camera_list(int win_width, int win_height, float win_aspect_ratio);
	void build_shader_list();
	void initialize();
	void draw_static_world();
	void draw_dynamic_world();
	void draw_axis();
	void draw_axis_with_model(const Camera& cam);
	void draw_world();
	void update_main_camera_follow_wolf();
	void draw_cam_frame(const Camera& cam);   // <-- 아래 1-B 구현
	void apply_user_filter();
	void upload_lights_to_current_prog();


};


extern Scene scene;








	 
 