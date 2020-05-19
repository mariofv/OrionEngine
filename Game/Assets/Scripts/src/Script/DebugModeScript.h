#ifndef  __DEBUGMODESCRIPT_H__
#define  __DEBUGMODESCRIPT_H__

#include "Script.h"
#include "Module/ModuleInput.h"
#include "Component/ComponentCamera.h"
#include "Module/ModuleCamera.h"

class ComponentImage;
class ComponentText;
class SceneCamerasController;
class EnemyManager;

class DebugModeScript : public Script
{
public:
	DebugModeScript();
	~DebugModeScript() = default;

	void Awake() override;
	void Start() override;
	void Update() override;
	void UpdateWithImGui(ImGuiContext*) override;

	void OnInspector(ImGuiContext*) override;
	void InitPublicGameObjects();
	//void Save(Config& config) const override;
	//void Load(const Config& config) override;
	bool debug_enabled = false;
	bool render_wireframe = false;
	bool is_player_invincible = false;
	bool is_warping_player = false;

	GameObject* enemy_manager_obj;
	GameObject* player_obj;
private:
	std::string base_str_tris = "#Tris: ";
	std::string base_str_verts = "#Verts: ";
	std::string base_str_fps = "FPS: ";

	GameObject* camera_manager = nullptr;
	SceneCamerasController* scene_cameras = nullptr;
	EnemyManager* enemy_manager;
	bool has_warped_player_recently = false;
	float warp_cooldown = 0.0f;
};
extern "C" SCRIPT_API DebugModeScript* DebugModeScriptDLL(); //This is how we are going to load the script
#endif