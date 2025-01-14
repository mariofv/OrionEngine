#ifndef _MODULEDEBUG_H_
#define _MODULEDEBUG_H_

#include "Module.h"
#include "Component/ComponentCamera.h"

#include <GL/glew.h>

class ModuleDebug : public Module
{
public:
	
	ModuleDebug() = default;
	~ModuleDebug() = default;

	bool Init() override;
	void Render();
	
	void CreateFrustumCullingDebugScene() const;

public:
	bool show_imgui_demo = false;
	bool show_debug_metrics = true;
	bool show_bounding_boxes = false;
	bool show_global_bounding_boxes = false;
	bool show_transform_2d = true;
	bool show_canvas = true;
	bool show_camera_frustum = true;
	bool show_quadtree = false;
	bool show_octtree = false;
	bool show_aabbtree = false;
	bool show_navmesh = false;
	bool show_pathfind_points = true;
	bool show_axis = false;
	bool show_physics = false;

#if !GAME
	bool culling_scene_mode = false;
#else
	bool culling_scene_mode = true;
#endif
	float rendering_time = 0;

private:
	int num_houses = 20;
	int max_dispersion_x = 40;
	int max_dispersion_z = 40;

	friend class PanelDebug;
	friend class PanelConfiguration;
};

#endif //_MODULEDEBUG_H_