#ifndef _MODULECAMERA_H_
#define _MODULECAMERA_H_

#include "Module.h"
#include "Globals.h"
#include "BoundingBox.h"

#include "Geometry/Frustum.h"
#include "MathGeoLib.h"

class ModuleCamera : public Module
{
public:
	ModuleCamera() = default;
	~ModuleCamera() = default;
	
	bool Init();
	update_status PreUpdate();
	update_status Update();
	update_status PostUpdate();
	bool CleanUp();
	
	void SetFOV(const float fov);
	void SetAspectRatio(const float aspect_ratio);
	void SetNearDistance(const float distance);
	void SetFarDistance(const float distance);
	void SetPosition(const float3 position);
	void SetOrientation(const float3 orientation);
	void LookAt(const float3 focus);
	void LookAt(const float x, const float y, const float z);

	void MoveUp();
	void MoveDown();
	void MoveFoward();
	void MoveBackward();
	void MoveLeft();
	void MoveRight();

	void RotatePitch(const float angle);
	void RotateYaw(const float angle);

	void OrbitX(const float angle);
	void OrbitY(const float angle);

	void SetSpeedUp(const bool is_speeding_up);
	void SetOrbit(const bool is_orbiting);

	void Center(const BoundingBox &bounding_box);
	void Focus(const BoundingBox &bounding_box);

	void SetMovement(const bool movement_enabled);
	bool MovementEnabled() const;

	void ShowCameraOptions();

private:
	void generateMatrices();

public:
	# define SPEED_UP_FACTOR 2

	# define FAR_PLANE_FACTOR 25
	# define BOUNDING_BOX_DISTANCE_FACTOR 3
	# define INITIAL_HEIGHT_FACTOR 0.5
	# define CAMERA_MOVEMENT_SPEED_BOUNDING_BOX_RADIUS_FACTOR 0.005
	# define CAMERA_ZOOMING_SPEED_BOUNDING_BOX_RADIUS_FACTOR 0.0625 

	float camera_movement_speed = 1.0f;
	float camera_zooming_speed = 1.0f;
	float camera_rotation_speed = 0.000625f;

	float4x4 proj;
	float4x4 view;

private:
	Frustum camera_frustum;
	
	float aspect_ratio = 1.f;

	bool movement_enabled = false;

	bool is_orbiting = false;
	float speed_up;

	bool is_focusing = false;
	float3 desired_focus_position;
};

#endif //_MODULECAMERA_H_