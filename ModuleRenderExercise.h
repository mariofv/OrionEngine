#pragma once
#include "Module.h"
#include "Globals.h"
#include <GL/glew.h>
#include "MathGeoLib/include/Geometry/Frustum.h"
#include "MathGeoLib/include/MathGeoLib.h"

struct SDL_Texture;
struct SDL_Renderer;
struct SDL_Rect;

class ModuleRenderExercise : public Module
{
public:
	ModuleRenderExercise();
	~ModuleRenderExercise();

	bool Init();
	update_status PreUpdate();
	update_status Update();
	update_status PostUpdate();
	bool CleanUp();

private:
	void initCamera();

private:
	GLuint vbo;
	GLuint vao;
	GLuint ebo;
	GLuint texture;

	float4x4 proj;
	float4x4 view;
	float4x4 model;
};
