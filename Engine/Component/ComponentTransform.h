#ifndef _COMPONENTTRANSFORM_H_
#define _COMPONENTTRANSFORM_H_

#define ENGINE_EXPORTS

#include "Component.h"

#include "MathGeoLib.h"
#include <GL/glew.h>

class PanelComponent;

class ComponentTransform : public Component
{
public:
	ComponentTransform(ComponentType transform_type = ComponentType::TRANSFORM);
	ComponentTransform(GameObject* owner, ComponentType transform_type = ComponentType::TRANSFORM);
	ComponentTransform(GameObject* owner, const float3 translation, const Quat rotation, const float3 scale);

	//Copy and move
	ComponentTransform(const ComponentTransform& component_to_copy) = default;
	ComponentTransform(ComponentTransform&& component_to_move) = default;

	ComponentTransform & operator=(const ComponentTransform & component_to_copy);
	ComponentTransform & operator=(ComponentTransform && component_to_move) = default;

	~ComponentTransform() = default;

	Component* Clone(bool create_on_module = true) const override;
	void Copy(Component * component_to_copy) const override;

	void Delete() override {};

	void SpecializedSave(Config& config) const override;
	void SpecializedLoad(const Config& config) override;
	
	ENGINE_API float3 GetGlobalTranslation() const;
	ENGINE_API float3 GetTranslation() const;
	ENGINE_API void SetTranslation(const float3& translation);
	ENGINE_API void Translate(const float3& translation);

	ENGINE_API Quat GetGlobalRotation() const;
	ENGINE_API Quat GetRotation() const;
	ENGINE_API float3 GetRotationRadiants() const;
	ENGINE_API void SetRotation(const float3x3& rotation);
	ENGINE_API void SetRotation(const float3& rotation);
	ENGINE_API void SetRotation(const Quat& rotation);

	void Rotate(const Quat& rotation);
	void Rotate(const float3x3& rotation);

	ENGINE_API void LookAt(const float3& target);

	float3 GetScale() const;
	void SetScale(const float3& scale);

	ENGINE_API float3 GetUpVector() const;
	ENGINE_API float3 GetFrontVector() const;
	ENGINE_API float3 GetRightVector() const;

	void ChangeLocalSpace(const float4x4& new_local_space);

	float4x4 GetModelMatrix() const;
	
	void GenerateGlobalModelMatrix();
	float4x4 GetGlobalModelMatrix() const;
	void SetGlobalModelMatrix(const float4x4& new_global_matrix);
  
protected:
	virtual void OnTransformChange();

protected:
	float3 translation = float3::zero;
	Quat rotation = Quat::identity;
	float3 rotation_degrees = float3::zero;
	float3 rotation_radians = float3::zero;
	float3 scale = float3::one;

	float4x4 model_matrix = float4x4::identity;
	float4x4 global_model_matrix = float4x4::identity;

	friend class PanelComponent;
};

#endif //_COMPONENTTRANSFORM_H_