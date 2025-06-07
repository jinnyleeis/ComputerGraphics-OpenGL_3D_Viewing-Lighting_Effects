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

enum CamFrameMode { FRM_NONE = 0, FRM_MAIN_ONLY, FRM_CCTV_ONLY, FRM_ALL };
static CamFrameMode g_frame_mode = FRM_ALL;   // 시작은 ‘주카메라만’

static void debug_axis_mm(const Camera& cam, const char* tag)
{
	// 1회만 찍고 싶으면 static bool once = true; 조건으로 감싸도 됨
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
	constexpr float AXIS_LEN = CAM_AXIS_LENGTH;     // Scene_Definitions.cpp에 이미 존재
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
// Updated move_wolf(...) with enhanced debug printing
// so you can manually steer the wolf and see exactly which grid cell
// (and world‐space cell corner) you're on, as well as whether it's a '0' (walk) or '1' (wall).
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

	// Print everything you need to know for manual tweaking:
	//   - cand.x, cand.y : the precise wolf position in world‐space
	//   - gx, gy         : the grid indices being checked
	//   - cell_world_x,y : the bottom‐left corner of that grid cell
	//   - cell           : '0' or '1'
	//   - walk vs wall
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
	fprintf(stdout, "[DEBUG] rebuild_perspective (fovy : %.1f, asp : %.1f,  near : %.1f, far : (%.1f) view NewPos=(%.1f,%.1f,%.1f)\n",
		cam.cam_proj.params.pers.fovy, cam.cam_proj.params.pers.aspect, cam.cam_proj.params.pers.n, cam.cam_proj.params.pers.f,
		cam.cam_view.pos.x, cam.cam_view.pos.y, cam.cam_view.pos.z);
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
		//scene.draw_cam_frame(camera->get());

		scene.draw_world();
	}
	glutSwapBuffers();
}


	void keyboard(unsigned char key, int x, int y) {
		static int flag_cull_face = 0, polygon_fill_on = 0, depth_test_on = 0;

		Camera& cam = ACTIVE_CAM();

		switch (key) {
		case 27: glutLeaveMainLoop(); break;                  // ESC

			/* ────────── n-축(roll) 회전 : Q / E ────────── */
		case 'q': if (cam.flag_move) rotate_camera(cam, cam.cam_view.naxis, ROT_STEP);  break;
		case 'e': if (cam.flag_move) rotate_camera(cam, cam.cam_view.naxis, -ROT_STEP);  break;

			/* ────────── 카메라-프레임 토글 : Y ────────── */
		case 'y': scene.show_camframe = !scene.show_camframe; glutPostRedisplay(); break;

		case 't':   // 소문자 t : **현재(Active) 카메라만** 토글
		{
			bool anyOn = false;
			for (auto& cref : scene.camera_list)
				anyOn |= cref.get().flag_show_frame;

			// 하나라도 켜져 있으면 → 모두 끔, 전부 꺼져 있으면 → 모두 켬
			bool newState = !anyOn;
			for (auto& cref : scene.camera_list)
				cref.get().flag_show_frame = newState;

			glutPostRedisplay();
			break;
		}

		case 'd': move_wolf(WOLF_STEP, 0);  break;
		case 'a':move_wolf(-WOLF_STEP, 0); break;    // 동(+x)
		case 'w': move_wolf(0, WOLF_STEP); break;   // 북(+y)
		case 's':move_wolf(0, -WOLF_STEP); break;   // 남(-y)

		case 'r': if (g_cur_cam_id == CAMERA_MAIN)
			translate_camera(cam, cam.cam_view.vaxis, MOVE_STEP); break;
		case 'f': if (g_cur_cam_id == CAMERA_MAIN)
			translate_camera(cam, -cam.cam_view.vaxis, MOVE_STEP); break;
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

				// keyboard() ─ case 't' 수정
		case '1':
			scene.show_camframe = !scene.show_camframe;
			glutPostRedisplay();
			break;



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
		case '0':          // MAIN <-> CCTV-D 토글
			g_prev_cam_id = g_cur_cam_id;                                // 기록
			g_cur_cam_id = (g_cur_cam_id == CAMERA_MAIN ?
				CAMERA_CCTV_D_REMOTE : CAMERA_MAIN);        // 전환
			break;
		default: break;
		}
	
//Camera& new_main = ACTIVE_CAM();
//Camera& old_main = scene.camera_data.cam_main;

	//std::swap(new_main.view_port, old_main.view_port);
//std::swap(new_main.camera_id, old_main.camera_id);



	/* ← switch 밖 (즉 항상 실행) */
	static const char* cam_name[8] = {
		"MAIN","FRONT","SIDE","TOP","CCTV-A","CCTV-B","CCTV-C","CCTV-D" };
	fprintf(stdout, "[DEBUG] Active-Cam switched → %s (%d)\n",
		cam_name[g_cur_cam_id], g_cur_cam_id);


	glutPostRedisplay();
}
/* 이동 계수 – 화면 픽셀 → 월드 단위 */
const float PIXEL2WORLD = 0.5f;
static int   last_x = -1, last_y = -1;

void passive_motion(int x, int y) {
	if (last_x < 0) { last_x = x; last_y = y; return; }
	float dx = (x - last_x) * PIXEL2WORLD;
	float dy = (last_y - y) * PIXEL2WORLD; // 윈도우 y축 반대
	last_x = x; last_y = y;

	/* ① 이동 후보 계산 */
	glm::vec3 cand = scene.g_wolf.pos + glm::vec3(dx, dy, 0.f);

	/* ② 충돌 확인 – 못 지나가면 무시 */
	if (!is_walkable(cand.x, cand.y)) return;

	/* ③ 반영 */
//	scene.g_wolf.pos = cand;
	//if (glm::length(glm::vec2(dx, dy)) > 0.01f)
	//	scene.g_wolf.heading = atan2f(dy, dx);   // 바라보는 방향 갱신
}

/*───────────────────────────────*/
/*  Arrow-key interaction        */
/*───────────────────────────────*/
/*───────────────────────────────*/
/*   Arrow-key interaction       */
/*───────────────────────────────*/
/*───────────────────────────────*/
/*  Arrow-key interaction with English console logs        */
/*───────────────────────────────*/
void special(int key, int, int)
{
	Camera& cam = ACTIVE_CAM();

	/* MAIN camera: accumulate yaw/pitch; CCTV-D: local-axis rotation */
	if (cam.camera_id == CAMERA_MAIN) {
		const float STEP = 5.f * TO_RADIAN;

		switch (key) {
		case GLUT_KEY_LEFT:
			scene.g_orbit.yaw += STEP;
			fprintf(stdout,
				"[CAM_CONTROL] MAIN camera: LEFT arrow → yaw increased (yaw: %.2f, pitch: %.2f)\n",
				scene.g_orbit.yaw, scene.g_orbit.pitch);
			break;
		case GLUT_KEY_RIGHT:
			scene.g_orbit.yaw -= STEP;
			fprintf(stdout,
				"[CAM_CONTROL] MAIN camera: RIGHT arrow → yaw decreased (yaw: %.2f, pitch: %.2f)\n",
				scene.g_orbit.yaw, scene.g_orbit.pitch);
			break;
		case GLUT_KEY_UP:
			scene.g_orbit.pitch += STEP;
			fprintf(stdout,
				"[CAM_CONTROL] MAIN camera: UP arrow → pitch increased (yaw: %.2f, pitch: %.2f)\n",
				scene.g_orbit.yaw, scene.g_orbit.pitch);
			break;
		case GLUT_KEY_DOWN:
			scene.g_orbit.pitch -= STEP;
			fprintf(stdout,
				"[CAM_CONTROL] MAIN camera: DOWN arrow → pitch decreased (yaw: %.2f, pitch: %.2f)\n",
				scene.g_orbit.yaw, scene.g_orbit.pitch);
			break;
		default:
			return;
		}

		/* clamp pitch */
		scene.g_orbit.pitch = glm::clamp(
			scene.g_orbit.pitch,
			-glm::half_pi<float>() + 0.05f,
			glm::half_pi<float>() - 0.05f);

		/* update view matrix */
		scene.update_main_camera_follow_wolf();

		fprintf(stdout,
			"[CAM_UPDATE] MAIN camera view matrix updated. Camera position: (%.1f, %.1f, %.1f)\n",
			cam.cam_view.pos.x, cam.cam_view.pos.y, cam.cam_view.pos.z);
	}
	else if (cam.camera_id == CAMERA_CCTV_D_REMOTE) {
		const float STEP = ROT_STEP;
		switch (key) {
		case GLUT_KEY_LEFT:
			rotate_camera(cam, cam.cam_view.vaxis, STEP);
			fprintf(stdout,
				"[CAM_CONTROL] CCTV-D camera: LEFT arrow → rotate around V axis (STEP: %.2f)\n",
				STEP);
			break;
		case GLUT_KEY_RIGHT:
			rotate_camera(cam, cam.cam_view.vaxis, -STEP);
			fprintf(stdout,
				"[CAM_CONTROL] CCTV-D camera: RIGHT arrow → rotate around V axis (negative STEP: %.2f)\n",
				STEP);
			break;
		case GLUT_KEY_UP:
			rotate_camera(cam, cam.cam_view.uaxis, STEP);
			fprintf(stdout,
				"[CAM_CONTROL] CCTV-D camera: UP arrow → rotate around U axis (STEP: %.2f)\n",
				STEP);
			break;
		case GLUT_KEY_DOWN:
			rotate_camera(cam, cam.cam_view.uaxis, -STEP);
			fprintf(stdout,
				"[CAM_CONTROL] CCTV-D camera: DOWN arrow → rotate around U axis (negative STEP: %.2f)\n",
				STEP);
			break;
		default:
			return;
		}

		fprintf(stdout,
			"[CAM_UPDATE] CCTV-D camera view matrix updated.\n");
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
	//glutPassiveMotionFunc(passive_motion);

	//	glutCloseFunc(cleanup_OpenGL_stuffs or else); // Do it yourself!!!
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

#define N_MESSAGE_LINES 4
void main(int argc, char* argv[]) {
	char program_name[256] = "Sogang CSE4170/AIE4120 Our_House_GLSL_V_0.55";
	char messages[N_MESSAGE_LINES][256] = {
		   "  - 이동   : W/S, A/D, R/F",
		   " - 회전 : ← ↑ ↓ →, Q / E ",
		   "  - 줌     : Z / X   (CCTV_D : V/B)",
		   "  - 카메라 : 1~8  | 축토글 T"
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