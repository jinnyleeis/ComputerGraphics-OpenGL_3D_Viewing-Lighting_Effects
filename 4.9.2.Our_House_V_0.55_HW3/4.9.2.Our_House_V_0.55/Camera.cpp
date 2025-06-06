/**********************************************************************
 *모든 카메라(View · Projection · Viewport) 정의
 *********************************************************************/
#define _CRT_SECURE_NO_WARNINGS
#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#define CAM_AXIS_LENGTH 40.0f

#define TO_RADIAN 0.01745329252f
#define TO_DEGREE 57.295779513f
inline void update_axis_mm(Camera& cam)
{
	constexpr float AXIS_LEN = CAM_AXIS_LENGTH;     // Scene_Definitions.cpp에 이미 존재
	glm::mat4 M_camWorld = glm::inverse(cam.ViewMatrix);
	cam.ModelMatrix_axis = M_camWorld *
		glm::scale(glm::mat4(1.0f), glm::vec3(AXIS_LEN));
}
 /*-------------------------------------------------------------------------*/
 /*ViewMatrix ➜ Camera_View(u,v,n,pos) 추출                        */
 /*-------------------------------------------------------------------------*/
static void extract_axes(Camera& cam) {
	glm::mat3 R = glm::mat3(cam.ViewMatrix);
	glm::mat3 RT = glm::transpose(R);
	cam.cam_view.uaxis = glm::vec3(R[0][0], R[1][0], R[2][0]);
	cam.cam_view.vaxis = glm::vec3(R[0][1], R[1][1], R[2][1]);
	cam.cam_view.naxis = glm::vec3(R[0][2], R[1][2], R[2][2]);

	glm::mat4 T = glm::mat4(RT) * cam.ViewMatrix;
	cam.cam_view.pos = -glm::vec3(T[3][0], T[3][1], T[3][2]);
}

/*-------------------------------------------------------------------------*/
/*  1) Perspective_Camera                                                   */
/*-------------------------------------------------------------------------*/
void Perspective_Camera::define_camera(int win_width, int win_height,
	float win_aspect_ratio) {
	glm::mat3 R33_t;
	glm::mat4 T;

	cam_proj.projection_type = CAMERA_PROJECTION_PERSPECTIVE;
	cam_proj.params.pers.fovy = 15.0f * TO_RADIAN;
	cam_proj.params.pers.aspect = win_aspect_ratio;
	cam_proj.params.pers.n = 1.0f;
	cam_proj.params.pers.f = 50000.0f;

	switch (camera_id) {
	case CAMERA_MAIN:
		flag_valid = true;
		flag_move = true; // yes. the main camera is permitted to move

		// let's use glm funtions to set up the initial camera pose
		ViewMatrix = glm::lookAt(glm::vec3(-600.0f, -600.0f, 400.0f), glm::vec3(125.0f, 80.0f, 25.0f),
			glm::vec3(0.0f, 0.0f, 1.0f)); // initial pose for main camera
		cam_view.uaxis = glm::vec3(ViewMatrix[0][0], ViewMatrix[1][0], ViewMatrix[2][0]);
		cam_view.vaxis = glm::vec3(ViewMatrix[0][1], ViewMatrix[1][1], ViewMatrix[2][1]);
		cam_view.naxis = glm::vec3(ViewMatrix[0][2], ViewMatrix[1][2], ViewMatrix[2][2]);
		R33_t = glm::transpose(glm::mat3(ViewMatrix));
		T = glm::mat4(R33_t) * ViewMatrix;
		cam_view.pos = -glm::vec3(T[3][0], T[3][1], T[3][2]); // why does this work?



		ProjectionMatrix = glm::perspective(cam_proj.params.pers.fovy, cam_proj.params.pers.aspect,
			cam_proj.params.pers.n, cam_proj.params.pers.f);
		view_port.x = 200; view_port.y = 200; view_port.w = win_width - 200; view_port.h = win_height - 200;
		break;

		/*------------------------------------ 고정 CCTV 3대 ------------------*/
	case CAMERA_CCTV_A: {        /* 정면 오른쪽 상단 */

		flag_valid = true;

		cam_proj.params.pers.fovy = 30.f * TO_RADIAN;

		ViewMatrix = glm::lookAt(glm::vec3(30.f, 20.f, 160.f),
			glm::vec3(100.f, 80.f, 20.f),
			glm::vec3(0.f, 0.f, 1.f));
		extract_axes(*this);

		ProjectionMatrix = glm::perspective(cam_proj.params.pers.fovy,
			cam_proj.params.pers.aspect,
			cam_proj.params.pers.n,
			cam_proj.params.pers.f);

		view_port = { 0.f, (float)win_height - 240.f, 240.f, 240.f };
		break;
	}
	case CAMERA_CCTV_B: {        /* 후면 */

		flag_valid = true;


		cam_proj.params.pers.fovy = 32.f * TO_RADIAN;

		ViewMatrix = glm::lookAt(glm::vec3(220.f, 20.f, 160.f),
			glm::vec3(125.f, 80.f, 25.f),
			glm::vec3(0.f, 0.f, 1.f));
		extract_axes(*this);

		ProjectionMatrix = glm::perspective(cam_proj.params.pers.fovy,
			cam_proj.params.pers.aspect,
			cam_proj.params.pers.n,
			cam_proj.params.pers.f);

		view_port = { 0.f, (float)win_height - 480.f, 240.f, 240.f };
		break;
	}
	case CAMERA_CCTV_C: {        /* 좌측 구석 */

		flag_valid = true;

		cam_proj.params.pers.fovy = 28.f * TO_RADIAN;

		ViewMatrix = glm::lookAt(glm::vec3(125.f, 150.f, 155.f),
			glm::vec3(125.f, 80.f, 25.f),
			glm::vec3(0.f, 0.f, 1.f));
		extract_axes(*this);

		ProjectionMatrix = glm::perspective(cam_proj.params.pers.fovy,
			cam_proj.params.pers.aspect,
			cam_proj.params.pers.n,
			cam_proj.params.pers.f);

		view_port = { 0.f, (float)win_height - 720.f, 240.f, 240.f };
		break;
	}
					  /*------------------------------- 동적 CCTV (FOV/방향 조절) -----------*/
	case CAMERA_CCTV_D_REMOTE: {


		flag_valid = true;
		flag_move = true;
		cam_proj.params.pers.fovy = 25.f * TO_RADIAN;

		ViewMatrix = glm::lookAt(glm::vec3(200.f, 10.f, 150.f),
			glm::vec3(125.f, 80.f, 30.f),
			glm::vec3(0.f, 0.f, 1.f));
		extract_axes(*this);

		ProjectionMatrix = glm::perspective(cam_proj.params.pers.fovy,
			cam_proj.params.pers.aspect,
			cam_proj.params.pers.n,
			cam_proj.params.pers.f);

		view_port = { 260.f, (float)win_height - 160.f, 260.f, 160.f };
		break;
	}
							 /*---------------------------------- 정의되지 않은 ID ------------------*/
	default:
		flag_valid = false;
	}
}




/*-------------------------------------------------------------------------*/
/*  2) Orthographic_Camera                                                 */
/*-------------------------------------------------------------------------*/
void Orthographic_Camera::define_camera(int win_width, int win_height,
	float /*win_aspect_ratio*/) {
	flag_valid = true;
	flag_move = false;

	/* 동일한 정규 투영 박스 */
	cam_proj.projection_type = CAMERA_PROJECTION_ORTHOGRAPHIC;
	cam_proj.params.ortho.left = -260.f;
	cam_proj.params.ortho.right = 260.f;
	cam_proj.params.ortho.botton = -160.f;
	cam_proj.params.ortho.top = 160.f;
	cam_proj.params.ortho.n = -500.f;
	cam_proj.params.ortho.f = 500.f;

	ProjectionMatrix = glm::ortho(cam_proj.params.ortho.left,
		cam_proj.params.ortho.right,
		cam_proj.params.ortho.botton,
		cam_proj.params.ortho.top,
		cam_proj.params.ortho.n,
		cam_proj.params.ortho.f);

	switch (camera_id) {
		/*---------------------------------- (정면도) -------------------------*/
	case CAMERA_SIDE_FRONT: {
		ViewMatrix = glm::lookAt(glm::vec3(125.f, -300.f, 25.f),
			glm::vec3(125.f, 80.f, 25.f),
			glm::vec3(0.f, 0.f, 1.f));
		extract_axes(*this);
		view_port = { (float)win_width - 260.f, (float)win_height - 160.f, 260.f, 160.f };
		break;
	}
						  /*---------------------------------- (측면도) -------------------------*/
	case CAMERA_SIDE: {
		ViewMatrix = glm::lookAt(glm::vec3(-200.f, 80.f, 25.f),
			glm::vec3(125.f, 80.f, 25.f),
			glm::vec3(0.f, 0.f, 1.f));
		extract_axes(*this);
		view_port = { (float)win_width - 260.f, (float)win_height - 320.f, 260.f, 160.f };
		break;
	}
					/*---------------------------------- (상면도) -------------------------*/
	case CAMERA_TOP: {
		ViewMatrix = glm::lookAt(glm::vec3(125.f, 80.f, 400.f),
			glm::vec3(125.f, 80.f, 0.f),
			glm::vec3(1.f, 0.f, 0.f)); // +x 가 up
		extract_axes(*this);
		view_port = { (float)win_width - 260.f, (float)win_height - 480.f, 260.f, 160.f };
		break;
	}
	default:
		flag_valid = false;
	}
}