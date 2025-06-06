#pragma once

#include <glm/gtc/matrix_transform.hpp>

#define CAM_SPIDER_OFFSET_Z  40.f

enum Camera_Projection_TYPE {
	CAMERA_PROJECTION_PERSPECTIVE = 0, CAMERA_PROJECTION_ORTHOGRAPHIC
};

enum Camera_ID {
	CAMERA_MAIN = 0, CAMERA_SIDE_FRONT, CAMERA_TOP, CAMERA_SIDE,
	CAMERA_CCTV_A, CAMERA_CCTV_B, CAMERA_CCTV_C,               // ���� CCTV 3��
	CAMERA_CCTV_D_REMOTE                                       // ���� CCTV 1��
};

struct Camera_View {
	glm::vec3 pos;
	glm::vec3 uaxis, vaxis, naxis;
};

struct Camera_Projection {
	Camera_Projection_TYPE projection_type;
	union {
		struct {
			float fovy, aspect, n, f;
		} pers;
		struct {
			float left, right, botton, top, n, f;
		} ortho;
	} params;
};

struct View_Port {
	float x, y, w, h;
};

struct Camera {
	Camera_ID camera_id;
	Camera_View cam_view;
	glm::mat4 ViewMatrix;
	Camera_Projection cam_proj;
	glm::mat4 ProjectionMatrix;
	View_Port view_port;
	glm::mat4  ModelMatrix_axis = glm::mat4(1.0f);
	Camera(Camera_ID _camera_id) : camera_id(_camera_id) {}
	bool flag_valid;
	bool flag_move; // what else?
	bool flag_show_frame = false;
};

struct Perspective_Camera : public Camera {
	Perspective_Camera(Camera_ID _camera_id) : Camera(_camera_id) {}
	void define_camera(int win_width, int win_height, float win_aspect_ratio);
};

struct Orthographic_Camera : public Camera {
	Orthographic_Camera(Camera_ID _camera_id) : Camera(_camera_id) {}
	void define_camera(int win_width, int win_height, float win_aspect_ratio);
};

struct Camera_Data {
	/* ��-1  :  ���� ī�޶� */
	Perspective_Camera cam_main{ CAMERA_MAIN };
	Perspective_Camera cam_cctv_a{ CAMERA_CCTV_A };
	Perspective_Camera cam_cctv_b{ CAMERA_CCTV_B };
	Perspective_Camera cam_cctv_c{ CAMERA_CCTV_C };
	Perspective_Camera cam_cctv_d{ CAMERA_CCTV_D_REMOTE };

	/* ��-2  :  ���� ī�޶� */
	Orthographic_Camera cam_front{ CAMERA_SIDE_FRONT };
	Orthographic_Camera cam_side{ CAMERA_SIDE };
	Orthographic_Camera cam_top{ CAMERA_TOP };
};

inline void sync_cam_axes_from_view(Camera& cam);