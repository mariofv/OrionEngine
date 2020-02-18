#include "ModuleActions.h"
#include "ModuleEditor.h"

#include "Main/Application.h"

#include "Actions/EditorActionEnableDisableComponent.h"
#include "Actions/EditorActionModifyCamera.h"
#include "Actions/EditorActionSetTexture.h"
#include "Actions/EditorActionModifyLight.h"
#include "Actions/EditorActionAddComponent.h"
#include "Actions/EditorActionDeleteComponent.h"
#include "Actions/EditorActionAddGameObject.h"
#include "Actions/EditorActionDeleteGameObject.h"
#include "Actions/EditorActionTranslate.h"
#include "Actions/EditorActionRotation.h"
#include "Actions/EditorActionScale.h"
#include "Actions/EditorAction.h"


bool ModuleActions::Init()
{
	//Delete all actions (go are deleted here)
	ClearUndoRedoStacks();
	return true;
}

update_status ModuleActions::PreUpdate()
{
	return update_status::UPDATE_CONTINUE;
}

update_status ModuleActions::Update()
{
	return update_status::UPDATE_CONTINUE;
}

bool ModuleActions::CleanUp()
{
	return true ;
}

void ModuleActions::ClearRedoStack()
{
	//TODO: delete all actions when engine closes (delete/add go/comp could end up in memory leak, be careful)
	while (!redoStack.empty())
	{
		EditorAction* action = redoStack.back();
		delete action;
		redoStack.pop_back();
	}
}

void ModuleActions::ClearUndoStack()
{
	while (!undoStack.empty())
	{
		EditorAction* action = undoStack.back();
		delete action;
		undoStack.pop_back();
	}
}

void ModuleActions::Undo()
{
	if (!undoStack.empty())
	{
		EditorAction* action = undoStack.back();
		action->Undo();

		redoStack.push_back(action);
		undoStack.pop_back();
	}
}

void ModuleActions::Redo()
{
	if (!redoStack.empty())
	{
		EditorAction* action = redoStack.back();
		action->Redo();

		undoStack.push_back(action);
		redoStack.pop_back();
	}
}

void ModuleActions::AddUndoAction(UndoActionType type)
{
	//StackUndoRedo set size maximum
	if (undoStack.size() >= MAXIMUM_SIZE_STACK_UNDO)
	{
		EditorAction* action = undoStack.front();
		delete action;
		undoStack.erase(undoStack.begin());
	}

	EditorAction* new_action = nullptr;

	switch (type)
	{
	case UndoActionType::TRANSLATION:
		new_action = new EditorActionTranslate(
			previous_transform,
			App->editor->selected_game_object->transform.GetTranslation(),
			App->editor->selected_game_object
		);
		break;

	case UndoActionType::ROTATION:
		new_action = new EditorActionRotation(
			previous_transform,
			App->editor->selected_game_object->transform.GetRotationRadiants(),
			App->editor->selected_game_object
		);
		break;

	case UndoActionType::SCALE:
		new_action = new EditorActionScale(
			previous_transform,
			App->editor->selected_game_object->transform.GetScale(),
			App->editor->selected_game_object
		);
		break;

	case UndoActionType::ADD_GAMEOBJECT:
		new_action = new EditorActionAddGameObject(
			action_game_object,
			action_game_object->parent,
			action_game_object->GetHierarchyDepth()
		);
		break;

	case UndoActionType::DELETE_GAMEOBJECT:
		new_action = new EditorActionDeleteGameObject(
			action_game_object
		);
		break;

	case UndoActionType::ADD_COMPONENT:
		new_action = new EditorActionAddComponent(action_component);
		break;

	case UndoActionType::DELETE_COMPONENT:
		new_action = new EditorActionDeleteComponent(action_component);
		break;

	case UndoActionType::EDIT_COMPONENTLIGHT:
		new_action = new EditorActionModifyLight(
			(ComponentLight*)action_component,
			previous_light_color,
			previous_light_intensity
		);
		break;

	case UndoActionType::EDIT_COMPONENTMATERIAL:
		new_action = new EditorActionSetTexture(
			(ComponentMaterial*)action_component,
			type_texture
		);
		break;

	case UndoActionType::EDIT_COMPONENTCAMERA:
		new_action = new EditorActionModifyCamera(
			(ComponentCamera*)action_component
		);
		break;

	case UndoActionType::ENABLE_DISABLE_COMPONENT:
		new_action = new EditorActionEnableDisableComponent(action_component);
		break;

	default:
		break;
	}
	if (new_action != nullptr)
	{
		undoStack.push_back(new_action);
		ClearRedoStack();
	}
}

void ModuleActions::DeleteComponentUndo(Component * component)
{
	//UndoRedo
	action_component = component;
	AddUndoAction(UndoActionType::DELETE_COMPONENT);
	component->Disable();
	auto it = std::find(component->owner->components.begin(), component->owner->components.end(), component);
	component->owner->components.erase(it);
}

void ModuleActions::ClearUndoRedoStacks()
{
	ClearRedoStack();
	ClearUndoStack();
}
