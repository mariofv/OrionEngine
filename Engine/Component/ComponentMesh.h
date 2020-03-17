#ifndef _COMPONENTMESH_H_
#define _COMPONENTMESH_H_

#include "Component.h"
#include "EditorUI/Panel/InspectorSubpanel/PanelComponent.h"
#include "ResourceManagement/Resources/Mesh.h"

class ComponentMesh : public Component
{
public:
	ComponentMesh(const std::shared_ptr<Mesh> & mesh_to_render);
	ComponentMesh(const std::shared_ptr<Mesh> & mesh_to_render, GameObject * owner);
	ComponentMesh();

	//Copy and move
	ComponentMesh(const ComponentMesh& component_to_copy) = default;
	ComponentMesh(ComponentMesh&& component_to_move) = default;

	ComponentMesh & operator=(const ComponentMesh & component_to_copy) = default;
	ComponentMesh & operator=(ComponentMesh && component_to_move) = default;

	~ComponentMesh();

	void Delete() override;

	void Save(Config& config) const override;
	void Load(const Config& config) override;
	Component* Clone(bool create_on_module = true) const override;
	void Copy(Component * component_to_copy) const override;

	void Render() const;
	void RenderModel() const;

	void SetMesh(const std::shared_ptr<Mesh> & mesh_to_render);

public:
	std::shared_ptr<Mesh> mesh_to_render;

private:
	friend class PanelComponent;
};

#endif //_COMPONENTMESH_H_