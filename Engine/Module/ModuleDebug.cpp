#include "ModuleDebug.h"

#include "Filesystem/Path.h"
#include "Helper/TemplatedGameObjectCreator.h"

#include "Main/Application.h"
#include "Main/GameObject.h"

#include "Module/ModuleResourceManager.h"
#include "Module/ModuleScene.h"
#include "Module/ModuleCamera.h"
#include "Module/ModuleDebugDraw.h"
#include "Module/ModuleRender.h"
#include "Module/ModuleUI.h"

#include "ResourceManagement/Importer/Importer.h"
#include "ResourceManagement/ResourcesDB/CoreResources.h"
#include "ResourceManagement/Resources/Prefab.h"

#include <ctime>
#include <GL/glew.h>
#include <random>

// Called before render is available
bool ModuleDebug::Init()
{
	APP_LOG_SECTION("************ Module Debug Init ************");

	
	APP_LOG_SUCCESS("Module Debug initialized correctly.")

	return true;
}

// Called before quitting
bool ModuleDebug::CleanUp()
{
	APP_LOG_INFO("Destroying Debug");	
	return true;
}

void ModuleDebug::Render(ComponentCamera* cam)
{
	BROFILER_CATEGORY("Render Debug Draws", Profiler::Color::Lavender);

	if (App->debug->show_canvas && App->ui->main_canvas != nullptr)
	{
		App->debug_draw->RenderRectTransform(App->ui->main_canvas->owner);
	}

	if (show_navmesh)
	{
		App->debug_draw->RenderNavMesh(*cam);
	}

	if (show_quadtree)
	{
		App->debug_draw->RenderQuadTree();
	}

	if (show_octtree)
	{
		App->debug_draw->RenderOcTree();
	}

	if (show_aabbtree)
	{
		App->debug_draw->RenderAABBTree();
	}

	if (show_bounding_boxes)
	{
		App->debug_draw->RenderBoundingBoxes();
	}

	if (show_global_bounding_boxes)
	{
		App->debug_draw->RenderGlobalBoundingBoxes();
	}

	if (show_pathfind_points)
	{
		App->debug_draw->RenderPathfinding();
	}

	if (show_axis && App->renderer->meshes_to_render.size() != 0)
	{
		App->debug_draw->RenderTangentsAndBitangents();
	}

#if !GAME
	App->debug_draw->RenderSelectedGameObjectHelpers();
	App->debug_draw->RenderBillboards();

	if (show_grid)
	{
		App->debug_draw->RenderGrid();
	}

	App->debug_draw->RenderDebugDraws(*cam);
#endif
}

void ModuleDebug::CreateFrustumCullingDebugScene() const
{
	std::srand(static_cast<unsigned int>(std::time(nullptr))); // use current time as seed for random generator
	
	GameObject *cubes = App->scene->CreateGameObject();
	
	for (int i = 0; i < num_houses; ++i)
	{
		GameObject* loaded_cube = TemplatedGameObjectCreator::CreatePrimitive(CoreResource::CUBE);
		loaded_cube->SetParent(cubes);

		float x = static_cast<float>(std::rand() % max_dispersion_x);
		float z = static_cast<float>(std::rand() % max_dispersion_z);
		loaded_cube->transform.SetTranslation(float3(x, 0, z));
	}
	cubes->SetStatic(true);
}