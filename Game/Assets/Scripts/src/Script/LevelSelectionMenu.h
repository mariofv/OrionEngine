#ifndef  __CHARATERSELECTIONMENU_H
#define  __CHARATERSELECTIONMENU_H

#include "Script.h"
class ComponentAudioSource;
class LevelSelectionMenu : public Script
{

public:
	LevelSelectionMenu();
	~LevelSelectionMenu() = default;

	void Awake() override;
	void Update() override;

	void OnInspector(ImGuiContext * context) override;

	void InitPublicGameObjects() override;
private:
	std::vector<GameObject*> buttons;
	unsigned current = 0;

	bool awaked = false;
	bool selecting_level = false;
	bool enabled = false;

	GameObject* previous_panel = nullptr;

	GameObject* level1 = nullptr;
	GameObject* level2 = nullptr;
	GameObject* level3 = nullptr;
	GameObject* back = nullptr;
	GameObject* back_cursor = nullptr;

	GameObject* audio_controller = nullptr;
	ComponentAudioSource* audio_source = nullptr;

	const size_t LEVEL1_POSITION = 0;
	const size_t LEVEL2_POSITION = 0;
	const size_t LEVEL3_POSITION = 0;

};
extern "C" SCRIPT_API LevelSelectionMenu* LevelSelectionMenuDLL(); //This is how we are going to load the script
#endif


