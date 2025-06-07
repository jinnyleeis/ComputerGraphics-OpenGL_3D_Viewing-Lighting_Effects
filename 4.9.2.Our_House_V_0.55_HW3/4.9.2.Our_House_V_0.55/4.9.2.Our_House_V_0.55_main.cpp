#define _CRT_SECURE_NO_WARNINGS
#include <GL/glew.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/freeglut.h>
#include "Shaders/LoadShaders.h"
#include "Scene_Definitions.h"
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>     
#include <glm/gtx/normalize_dot.hpp>       

const float WOLF_STEP = 5.f;   // 한 번 누를 때 이동 거리

// 현재 조작 대상 카메라 – 기본은 MAIN, '8' 을 누르면 CCTV_D 로 전환
static Camera_ID   g_active_cam_id = CAMERA_MAIN;
#define CAM_AXIS_LENGTH 1.0f
static Camera_ID g_cur_cam_id = CAMERA_MAIN;
static Camera_ID g_prev_cam_id = CAMERA_MAIN;   // 최초엔 동일

static void debug_axis_mm(const Camera& cam, const char* tag)
{
	fprintf(stderr,
		"[%s] POS=(%.1f, %.1f, %.1f)  | scale ≈ %.1f\n",
		tag,
		cam.ModelMatrix_axis[3][0],
		cam.ModelMatrix_axis[3][1],
		cam.ModelMatrix_axis[3][2],
		glm::length(glm::vec3(cam.ModelMatrix_axis[0])));
}

inline void update_axis_mm(Camera& cam)
{
	constexpr float AXIS_LEN = CAM_AXIS_LENGTH;     
	glm::mat4 M_camWorld = glm::inverse(cam.ViewMatrix);
	cam.ModelMatrix_axis = M_camWorld *
		glm::scale(glm::mat4(1.0f), glm::vec3(AXIS_LEN));
}

inline bool is_walkable(float x, float y)
{
	int gx = glm::clamp(int(x / 10.f), 0, W - 1);
	int gy = glm::clamp(int(y / 10.f), 0, H - 1);
	return scene.floor_mask[gy][gx] == 0;
}

// ------------------------------------------------------------
// (and world‐space cell corner on, as well as whether it's a '0' (walk) or '1' (wall).
// ------------------------------------------------------------
void move_wolf(float dx, float dy)
{
	// Compute the candidate new world‐position
	glm::vec3 cand = scene.g_wolf.pos + glm::vec3(dy, -dx, 0.f);

	// Convert to grid indices [0..W-1], [0..H-1]
	int gx = glm::clamp(int(cand.x / 10.f), 0, Scene::W - 1);
	int gy = glm::clamp(int(cand.y / 10.f), 0, Scene::H - 1);

	// Look up the mask cell ('0' = walkable, '1' = wall)
	int cell = scene.floor_mask[gy][gx];

	// Compute the world‐space origin of that grid cell
	float cell_world_x = gx * 10.0f;
	float cell_world_y = gy * 10.0f;

	//   - cand.x, cand.y : the precise wolf position in world‐space
	//   - gx, gy         : the grid indices being checked
	//   - cell_world_x,y : the bottom‐left corner of that grid cell
	//   - cell           : '0' or '1'
	//   - walk vs wall
	/*
	printf(
		"[WOLF] Attempted move to world(%.1f, %.1f) → grid indices (gx=%2d, gy=%2d)\n"
		"       Cell origin at world(%.1f, %.1f), mask='%c' → %s\n",
	cand.x,     // world X
		cand.y,     // world Y
		gx,         // grid column
		gy,         // grid row
		cell_world_x, // world X of cell's lower-left corner
		cell_world_y, // world Y of cell's lower-left corner
		cell,        // '0'=walk, '1'=wall
		(cell == 0) ? "WALKABLE" : "WALL"
	);
	*/
	// If it's a wall, ignore the move
	if (cell == 1)
		return;

	// Otherwise, commit the move and update heading
	scene.g_wolf.pos = cand;
	if (fabs(dx) > 1e-3f || fabs(dy) > 1e-3f) {
		scene.g_wolf.heading = atan2f(dy, dx);
	}
}

void make_viewport_center(Camera& cam, int W, int H) {
	cam.view_port = { 200.f, 200.f,
					 200.0f,
					  200.f };
}

inline Camera& ACTIVE_CAM() {
	return scene.camera_list[camera_ID_mapper[g_cur_cam_id]];
}
inline Camera& CUR_CAM() { return scene.camera_list[camera_ID_mapper[g_cur_cam_id]]; }
inline Camera& PREV_CAM() { return scene.camera_list[camera_ID_mapper[g_prev_cam_id]]; }
const float MOVE_STEP = 10.0f;                // world units
const float ROT_STEP = 5.0f * TO_RADIAN;     // 5° → rad
const float ZOOM_STEP = 2.0f * TO_RADIAN;     // 2°



void rebuild_view(Camera& cam) {
	cam.ViewMatrix = glm::lookAt(
		cam.cam_view.pos,
		cam.cam_view.pos - cam.cam_view.naxis,
		cam.cam_view.vaxis
	);
	update_axis_mm(cam);
}

void rebuild_perspective(Camera& cam) {
	if (cam.cam_proj.projection_type != CAMERA_PROJECTION_PERSPECTIVE) return;
	cam.ProjectionMatrix = glm::perspective(
		cam.cam_proj.params.pers.fovy,
		cam.cam_proj.params.pers.aspect,
		cam.cam_proj.params.pers.n,
		cam.cam_proj.params.pers.f
	);
	//fprintf(stdout, "[DEBUG] rebuild_perspective (fovy : %.1f, asp : %.1f,  near : %.1f, far : (%.1f) view NewPos=(%.1f,%.1f,%.1f)\n",
	//	cam.cam_proj.params.pers.fovy, cam.cam_proj.params.pers.aspect, cam.cam_proj.params.pers.n, cam.cam_proj.params.pers.f,
	//	cam.cam_view.pos.x, cam.cam_view.pos.y, cam.cam_view.pos.z);
	update_axis_mm(cam);


}

// 카메라 이동 (u,v,n)
void translate_camera(Camera& cam, const glm::vec3& dir, float step) {
	cam.cam_view.pos += dir * step;
	fprintf(stdout, "[DEBUG] translate (%.1f,%.1f,%.1f)  NewPos=(%.1f,%.1f,%.1f)\n",
		dir.x, dir.y, dir.z,
		cam.cam_view.pos.x, cam.cam_view.pos.y, cam.cam_view.pos.z);
	rebuild_view(cam);
}

/* ────────────────────────────────────────────────────────────── */
/*  카메라 로컬축 기준 회전                                       */
/*    · localAxis : cam.cam_view.uaxis / vaxis / naxis 중 하나   */
/*    · angle     : +CCW (라디안)                                 */
/* ────────────────────────────────────────────────────────────── */
void rotate_camera(Camera& cam, const glm::vec3& localAxis, float angle) {
	/* 1) 회전 행렬 생성 (world-space) */
	glm::mat3 R = glm::mat3(glm::rotate(glm::mat4(1.0f), angle, glm::normalize(localAxis)));

	/* 2) u / v / n 축 회전 후 재정규화 */
	cam.cam_view.uaxis = glm::normalize(R * cam.cam_view.uaxis);
	cam.cam_view.vaxis = glm::normalize(R * cam.cam_view.vaxis);
	cam.cam_view.naxis = glm::normalize(glm::cross(cam.cam_view.uaxis, cam.cam_view.vaxis));
	cam.cam_view.vaxis = glm::normalize(glm::cross(cam.cam_view.naxis, cam.cam_view.uaxis)); // 직교성 보정

	/* 3) ViewMatrix 갱신 */
	rebuild_view(cam);
}


void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	for (auto camera = scene.camera_list.begin(); camera != scene.camera_list.end(); camera++) {
		if (camera->get().flag_valid == false) continue;
		glViewport(camera->get().view_port.x, camera->get().view_port.y,
			camera->get().view_port.w, camera->get().view_port.h);
		scene.ViewMatrix = camera->get().ViewMatrix;
		scene.ProjectionMatrix = camera->get().ProjectionMatrix;
	
		if (scene.show_axes)
		scene.axis_object.draw_axis(
			static_cast<Shader_Simple*>(
					&scene.shader_list[shader_ID_mapper[SHADER_SIMPLE]]
				  .get()), scene.ViewMatrix, scene.ProjectionMatrix);

		scene.draw_world();
	}
	glutSwapBuffers();
}


	void keyboard(unsigned char key, int x, int y) {
		static int flag_cull_face = 0, polygon_fill_on = 0, depth_test_on = 0;
	    float STEP = ROT_STEP;


		Camera& cam = ACTIVE_CAM();

		// 1) ESC
		if (key == 27) {
			glutLeaveMainLoop();
			return;
		}

		// 2) MAIN 카메라용 WSADQE
		if (g_cur_cam_id == CAMERA_MAIN) {
			switch (key) {
				STEP = 5.f * TO_RADIAN;
			case 'a': scene.g_orbit.leftRight -= STEP; break;  // 왼쪽 둘러보기
			case 'd': scene.g_orbit.leftRight += STEP; break;  // 오른쪽 둘러보기
			case 'w': scene.g_orbit.upDown += STEP; break;  // 위로 보기
			case 's': scene.g_orbit.upDown -= STEP; break;  // 아래로 보기
			case 'q': scene.g_orbit.headTilt -= STEP; break;  // 왼쪽으로 기울이기
			case 'e': scene.g_orbit.headTilt += STEP; break;  // 오른쪽으로 기울이기
			default: break;
			}
			// clamp & update
			scene.g_orbit.upDown = glm::clamp(
				scene.g_orbit.upDown,
				-glm::half_pi<float>() * 0.15f + 0.05f,
				glm::half_pi<float>() - 0.05f);

			scene.g_orbit.leftRight= glm::clamp(
				scene.g_orbit.leftRight,
				-glm::half_pi<float>() + 0.05f,
				glm::half_pi<float>() - 0.05f);

			scene.g_orbit.headTilt = glm::clamp(
				scene.g_orbit.headTilt,
				-glm::half_pi<float>() + 0.05f,
				glm::half_pi<float>() - 0.05f);


			scene.update_main_camera_follow_wolf();
			fprintf(stdout,
				"[CAM_UPDATE] MAIN cam leftRight=%.2f upDown=%.2f headTilt=%.2f\n",
			scene.g_orbit.leftRight,
			scene.g_orbit.upDown,
				scene.g_orbit.headTilt);
		}
		// 3) CCTV-D 카메라용 WSADQE
		else if (g_cur_cam_id == CAMERA_CCTV_D_REMOTE) {
			switch (key) {
			case 'q': rotate_camera(cam, cam.cam_view.naxis, -STEP); break;
			case 'e': rotate_camera(cam, cam.cam_view.naxis, +STEP); break;
			case 'w': rotate_camera(cam, cam.cam_view.uaxis, STEP); break;
			case 's': rotate_camera(cam, cam.cam_view.uaxis, -STEP); break;
			case 'a': rotate_camera(cam, cam.cam_view.vaxis, +STEP); break;
			case 'd': rotate_camera(cam, cam.cam_view.vaxis, -STEP); break;
			default: break;
			}
			fprintf(stdout, "[CAM_UPDATE] CCTV-D camera view matrix updated.\n");
		}

		// 4) 공통 키 (토글, 줌, 카메라 전환 등)
		switch (key) {
		case 'y':
			scene.show_camframe = !scene.show_camframe;
			break;
		case '0':
			g_prev_cam_id = g_cur_cam_id;
			g_cur_cam_id = (g_cur_cam_id == CAMERA_MAIN
				? CAMERA_CCTV_D_REMOTE
				: CAMERA_MAIN);
			break;

			/* --- 줌 (MAIN + CCTV_D) --------------------------------------------- */
		case 'z':
			if (cam.cam_proj.projection_type == CAMERA_PROJECTION_PERSPECTIVE) {
				cam.cam_proj.params.pers.fovy = glm::max(5.f * TO_RADIAN,
					cam.cam_proj.params.pers.fovy - ZOOM_STEP);
				rebuild_perspective(cam);
			} break;
		case 'x':
			if (cam.cam_proj.projection_type == CAMERA_PROJECTION_PERSPECTIVE) {
				cam.cam_proj.params.pers.fovy = glm::min(80.f * TO_RADIAN,
					cam.cam_proj.params.pers.fovy + ZOOM_STEP);
				rebuild_perspective(cam);
			} break;
			/* --- CCTV_D FOV 세밀 조정 ------------------------------------------- */
		case 'v': if (g_cur_cam_id == CAMERA_CCTV_D_REMOTE) {
			cam.cam_proj.params.pers.fovy =
				glm::clamp(cam.cam_proj.params.pers.fovy - ZOOM_STEP,
					5.f * TO_RADIAN, 80.f * TO_RADIAN);
			rebuild_perspective(cam);
		} break;
		case 'b': if (g_cur_cam_id == CAMERA_CCTV_D_REMOTE) {
			cam.cam_proj.params.pers.fovy =
				glm::clamp(cam.cam_proj.params.pers.fovy + ZOOM_STEP,
					5.f * TO_RADIAN, 80.f * TO_RADIAN);
			rebuild_perspective(cam);
		} break;
				/* --- 카메라 축 토글 (Axis 표시) -------------------------------------- */


		case 'c':
			flag_cull_face = (flag_cull_face + 1) % 3;
			switch (flag_cull_face) {
			case 0:
				glDisable(GL_CULL_FACE);
				glutPostRedisplay();
				fprintf(stdout, "^^^ No faces are culled.\n");
				break;
			case 1: // cull back faces;
				glCullFace(GL_BACK);
				glEnable(GL_CULL_FACE);
				glutPostRedisplay();
				fprintf(stdout, "^^^ Back faces are culled.\n");
				break;
			case 2: // cull front faces;
				glCullFace(GL_FRONT);
				glEnable(GL_CULL_FACE);
				glutPostRedisplay();
				fprintf(stdout, "^^^ Front faces are culled.\n");
				break;
			}
			break;
		case 'p':
			polygon_fill_on = 1 - polygon_fill_on;
			if (polygon_fill_on) {
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				fprintf(stdout, "^^^ Polygon filling enabled.\n");
			}
			else {
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				fprintf(stdout, "^^^ Line drawing enabled.\n");
			}
			glutPostRedisplay();
			break;
		case 'm':
			depth_test_on = 1 - depth_test_on;
			if (depth_test_on) {
				glEnable(GL_DEPTH_TEST);
				fprintf(stdout, "^^^ Depth test enabled.\n");
			}
			else {
				glDisable(GL_DEPTH_TEST);
				fprintf(stdout, "^^^ Depth test disabled.\n");
			}
			break;
		default: break;
		}
	

	/* ← switch 밖 (즉 항상 실행) */
	static const char* cam_name[8] = {
		"MAIN","FRONT","SIDE","TOP","CCTV-A","CCTV-B","CCTV-C","CCTV-D" };
	fprintf(stdout, "[DEBUG] Active-Cam switched → %s (%d)\n",
		cam_name[g_cur_cam_id], g_cur_cam_id);


	glutPostRedisplay();
}

void special(int key, int, int)
{
	Camera& cam = ACTIVE_CAM();

	/* MAIN camera: accumulate yaw/pitch; CCTV-D: local-axis rotation */
	if (g_cur_cam_id ==CAMERA_MAIN) {


		switch (key) {
		case  GLUT_KEY_RIGHT: move_wolf(WOLF_STEP, 0);  break;
		case GLUT_KEY_LEFT:move_wolf(-WOLF_STEP, 0); break;    // 동(+x)
		case GLUT_KEY_UP: move_wolf(0, WOLF_STEP); break;   // 북(+y)
		case GLUT_KEY_DOWN:move_wolf(0, -WOLF_STEP); break;   // 남(-y)

		default:
			return;
		}

	}
	glutPostRedisplay();   // redraw immediately



}




void reshape(int width, int height) {
	scene.window.width = width;
	scene.window.height = height;
	scene.window.aspect_ratio = (float)width / height;
	scene.create_camera_list(scene.window.width, scene.window.height, scene.window.aspect_ratio);
	glutPostRedisplay();
}

void timer_scene(int index) {
	scene.clock(0);
	glutPostRedisplay();
	glutTimerFunc(100, timer_scene, 0);
}

void register_callbacks(void) {
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutReshapeFunc(reshape);
	glutTimerFunc(100, timer_scene, 0);
	glutSpecialFunc(special);
	
}

void initialize_OpenGL(void) {
	glEnable(GL_DEPTH_TEST); // Default state

	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glClearColor(0.12f, 0.18f, 0.12f, 1.0f);
}

void initialize_renderer(void) {
	register_callbacks();
	initialize_OpenGL();
	scene.initialize();
}

void initialize_glew(void) {
	GLenum error;

	glewExperimental = GL_TRUE;

	error = glewInit();
	if (error != GLEW_OK) {
		fprintf(stderr, "Error: %s\n", glewGetErrorString(error));
		exit(-1);
	}
	fprintf(stdout, "*********************************************************\n");
	fprintf(stdout, " - GLEW version supported: %s\n", glewGetString(GLEW_VERSION));
	fprintf(stdout, " - OpenGL renderer: %s\n", glGetString(GL_RENDERER));
	fprintf(stdout, " - OpenGL version supported: %s\n", glGetString(GL_VERSION));
	fprintf(stdout, "*********************************************************\n\n");
}

void print_message(const char* m) {
	fprintf(stdout, "%s\n\n", m);
}

void greetings(char* program_name, char messages[][256], int n_message_lines) {
	fprintf(stdout, "**************************************************************\n\n");
	fprintf(stdout, "  PROGRAM NAME: %s\n\n", program_name);
	fprintf(stdout, "    This program was coded for CSE4170/AIE4012 students\n");
	fprintf(stdout, "      of Dept. of Comp. Sci. & Eng., Sogang University.\n\n");

	for (int i = 0; i < n_message_lines; i++)
		fprintf(stdout, "%s\n", messages[i]);
	fprintf(stdout, "\n**************************************************************\n\n");

	initialize_glew();
}

#define N_MESSAGE_LINES 6
void main(int argc, char* argv[]) {
	char program_name[256] = "Sogang CSE4170/AIE4120 Our_House_GLSL_V_0.55";
	char messages[N_MESSAGE_LINES][256] = {
	    "[MAIN] 이동 : ←/↑/↓/→ (늑대(메인 카메라) 이동",
		"[CCTV-D] FOV 조정 :  V/B (CCTV_D 전용 FOV 조절)",
		"[MAIN/CCTV-D] 회전: A/D (좌우 둘러보기),W/S (위/아래 보기),Q/E (좌/우 기울이기)",
		"[MAIN/CCTV-D] 줌          : Z/X (메인·CCTV_D 공통)",
		"[MAIN/CCTV-D] 카메라 전환 : 0 (MAIN <-> CCTV_D)",
        "[MAIN/CCTVS] 카메라 전환 :축 표시 토글: Y",

	};

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glutInitWindowSize(1200, 800);
	glutInitContextVersion(4, 0);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutCreateWindow(program_name);

	greetings(program_name, messages, N_MESSAGE_LINES);
	initialize_renderer();

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
	glutMainLoop();
}