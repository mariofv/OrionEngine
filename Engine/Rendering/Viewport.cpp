#include "Viewport.h"

#include "Component/ComponentCamera.h"
#include "Component/ComponentMeshRenderer.h"

#include "Main/Application.h"
#include "Main/GameObject.h"
#include "Module/ModuleCamera.h"
#include "Module/ModuleDebug.h"
#include "Module/ModuleDebugDraw.h"
#include "Module/ModuleEffects.h"
#include "Module/ModuleEditor.h"
#include "Module/ModuleProgram.h"
#include "Module/ModuleRender.h"
#include "Module/ModuleUI.h"
#include "Module/ModuleSpacePartitioning.h"
#include "Helper/Utils.h"

#include "FrameBuffer/DepthFrameBuffer.h"
#include "FrameBuffer/FrameBuffer.h"
#include "LightFrustum.h"

Viewport::Viewport(int options) : viewport_options(options)
{
	framebuffers.emplace_back(main_fbo = new FrameBuffer());
	framebuffers.emplace_back(postprocess_fbo = new FrameBuffer());
	framebuffers.emplace_back(blit_fbo = new FrameBuffer());

	depth_full_fbo = new DepthFrameBuffer();
	depth_near_fbo = new DepthFrameBuffer();
	depth_mid_fbo = new DepthFrameBuffer();
	depth_far_fbo = new DepthFrameBuffer();
}

Viewport::~Viewport()
{
	for (auto& framebuffer : framebuffers)
	{
		delete framebuffer;
	}

	delete depth_full_fbo;
	delete depth_near_fbo;
	delete depth_mid_fbo;
	delete depth_far_fbo;
}

void Viewport::Render(ComponentCamera* camera)
{
	this->camera = camera;
	camera->SetAspectRatio(width / height);
	culled_mesh_renderers = App->space_partitioning->GetCullingMeshes(camera, App->renderer->mesh_renderers);

	LightCameraPass();
	App->lights->BindLightFrustumsMatrices();

	BindCameraFrustumMatrices(camera->camera_frustum);
	glViewport(0, 0, width, height);

	MeshRenderPass();
	EffectsRenderPass();
	UIRenderPass();
	DebugPass();
	DebugDrawPass();
	EditorDrawPass();
	PostProcessPass();
	BlitPass();

	SelectLastDisplayedTexture();
}

void Viewport::BindCameraFrustumMatrices(const Frustum& camera_frustum) const
{
	glBindBuffer(GL_UNIFORM_BUFFER, App->program->uniform_buffer.ubo);

	static size_t projection_matrix_offset = App->program->uniform_buffer.MATRICES_UNIFORMS_OFFSET + sizeof(float4x4);
	float4x4 projection_matrix = camera_frustum.ProjectionMatrix();
	glBufferSubData(GL_UNIFORM_BUFFER, projection_matrix_offset, sizeof(float4x4), projection_matrix.Transposed().ptr());

	static size_t view_matrix_offset = App->program->uniform_buffer.MATRICES_UNIFORMS_OFFSET + 2 * sizeof(float4x4);
	float4x4 view_matrix = camera_frustum.ViewMatrix();
	glBufferSubData(GL_UNIFORM_BUFFER, view_matrix_offset, sizeof(float4x4), view_matrix.Transposed().ptr());

	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Viewport::BindDepthMaps(GLuint program) const
{
	glActiveTexture(GL_TEXTURE12);
	glBindTexture(GL_TEXTURE_2D, depth_full_fbo->GetColorAttachement());
	glUniform1i(glGetUniformLocation(program, "close_depth_map"), 12);

	glActiveTexture(GL_TEXTURE13);
	glBindTexture(GL_TEXTURE_2D, depth_mid_fbo->GetColorAttachement());
	glUniform1i(glGetUniformLocation(program, "mid_depth_map"), 13);

	glActiveTexture(GL_TEXTURE14);
	glBindTexture(GL_TEXTURE_2D, depth_full_fbo->GetColorAttachement());
	glUniform1i(glGetUniformLocation(program, "far_depth_map"), 14);
}

void Viewport::LightCameraPass() const
{
	if (!shadows_pass || App->cameras->main_camera == nullptr)
	{
		return;
	}

	DepthMapPass(App->lights->full_frustum, depth_full_fbo);
	DepthMapPass(App->lights->near_frustum, depth_near_fbo);
	DepthMapPass(App->lights->mid_frustum, depth_mid_fbo);
	DepthMapPass(App->lights->far_frustum, depth_far_fbo);
}

void Viewport::MeshRenderPass() const
{
	main_fbo->Bind();
	camera->Clear();

	float3 camera_position = camera->owner->transform.GetGlobalTranslation();

	std::vector<Utils::MeshRendererDistancePair> opaque_mesh_renderers;
	std::vector<Utils::MeshRendererDistancePair> transparent_mesh_renderers;
	Utils::SplitCulledMeshRenderers(culled_mesh_renderers, camera_position, opaque_mesh_renderers, transparent_mesh_renderers);

	for (auto& opaque_mesh_renderer : opaque_mesh_renderers)
	{
		if (opaque_mesh_renderer.mesh_renderer->mesh_to_render != nullptr 
			&& opaque_mesh_renderer.mesh_renderer->material_to_render != nullptr
			&& opaque_mesh_renderer.mesh_renderer->IsEnabled()
		)
		{
			GLuint mesh_renderer_program = opaque_mesh_renderer.mesh_renderer->BindShaderProgram();
			opaque_mesh_renderer.mesh_renderer->BindMeshUniforms(mesh_renderer_program);
			opaque_mesh_renderer.mesh_renderer->BindMaterialUniforms(mesh_renderer_program);
			BindDepthMaps(mesh_renderer_program);
			App->lights->Render(opaque_mesh_renderer.mesh_renderer->owner->transform.GetGlobalTranslation(), mesh_renderer_program);
			opaque_mesh_renderer.mesh_renderer->RenderModel();
			/*
			num_rendered_tris += mesh.second->mesh_to_render->GetNumTriangles();
			num_rendered_verts += mesh.second->mesh_to_render->GetNumVerts();
			*/
			glUseProgram(0);
		}
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glBlendEquation(GL_FUNC_ADD);
	for (auto& transparent_mesh_renderer : transparent_mesh_renderers)
	{
		if (transparent_mesh_renderer.mesh_renderer->mesh_to_render != nullptr
			&& transparent_mesh_renderer.mesh_renderer->material_to_render != nullptr
			&& transparent_mesh_renderer.mesh_renderer->IsEnabled()
		) {
			GLuint mesh_renderer_program = transparent_mesh_renderer.mesh_renderer->BindShaderProgram();
			transparent_mesh_renderer.mesh_renderer->BindMeshUniforms(mesh_renderer_program);
			transparent_mesh_renderer.mesh_renderer->BindMaterialUniforms(mesh_renderer_program);
			BindDepthMaps(mesh_renderer_program);
			App->lights->Render(transparent_mesh_renderer.mesh_renderer->owner->transform.GetGlobalTranslation(), mesh_renderer_program);
			transparent_mesh_renderer.mesh_renderer->RenderModel();
			/*
			num_rendered_tris += mesh.second->mesh_to_render->GetNumTriangles();
			num_rendered_verts += mesh.second->mesh_to_render->GetNumVerts();
			*/
			glUseProgram(0);
		}
	}

	FrameBuffer::UnBind();
}

void Viewport::EffectsRenderPass() const
{
	if (!effects_pass)
	{
		return;
	}

	main_fbo->Bind();
	App->effects->Render();
	FrameBuffer::UnBind();
}

void Viewport::UIRenderPass() const
{
	main_fbo->Bind();
	App->ui->Render(width, height, IsOptionSet(ViewportOption::SCENE_MODE));
	FrameBuffer::UnBind();
}

void Viewport::PostProcessPass() const
{
	postprocess_fbo->Bind();

	int shader_variation = 0;
	if (antialiasing)
	{
		shader_variation |= (int)ModuleProgram::ShaderVariation::ENABLE_MSAA;
	}
	if (hdr)
	{
		shader_variation |= (int)ModuleProgram::ShaderVariation::ENABLE_HDR;
	}
	GLuint program = App->program->UseProgram("PostProcessing", shader_variation);
	
	glActiveTexture(GL_TEXTURE0);
	if (antialiasing)
	{
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, main_fbo->GetColorAttachement());
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, main_fbo->GetColorAttachement());
	}
	glUniform1i(glGetUniformLocation(program, "screen_texture"), 0);
	
	App->ui->quad->Render();

	FrameBuffer::UnBind();
}

void Viewport::BlitPass() const
{
	main_fbo->Bind(GL_READ_FRAMEBUFFER);

	if (IsOptionSet(ViewportOption::BLIT_FRAMEBUFFER))
	{
		blit_fbo->Bind(GL_DRAW_FRAMEBUFFER);
	}
	else
	{
		FrameBuffer::UnBind(GL_DRAW_FRAMEBUFFER);
	}

	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	FrameBuffer::UnBind();
}

void Viewport::DebugPass() const
{
	if (!debug_pass || !IsOptionSet(ViewportOption::SCENE_MODE))
	{
		return;
	}

	App->debug->Render();

}

void Viewport::DebugDrawPass() const
{
	if (!debug_draw_pass)
	{
		return;
	}

	main_fbo->Bind();
	App->debug_draw->Render(width, height, camera->GetProjectionMatrix() * camera->GetViewMatrix());
	FrameBuffer::UnBind();
}

void Viewport::EditorDrawPass() const
{
	if (!IsOptionSet(ViewportOption::SCENE_MODE))
	{
		return;
	}

	main_fbo->Bind();
	App->debug_draw->RenderGrid();
	if (App->debug->show_navmesh)
	{
		App->debug_draw->RenderNavMesh(*camera);
	}
	App->debug_draw->RenderBillboards();
	if (App->editor->selected_game_object != nullptr)
	{
		App->debug_draw->RenderOutline();
	}
	FrameBuffer::UnBind();
}

void Viewport::SetSize(float width, float height)
{
	if (this->width == width && this->height == height)
	{
		return;
	}

	this->width = width;
	this->height = height;

	for (auto& framebuffer : framebuffers)
	{
		framebuffer->ClearAttachements();
		framebuffer->GenerateAttachements(width, height);
	}
}

void Viewport::SelectLastDisplayedTexture()
{
	switch (viewport_output)
	{
	case ViewportOutput::COLOR:
		last_displayed_texture = blit_fbo->GetColorAttachement();
		break;

	case ViewportOutput::DEPTH_NEAR:
		last_displayed_texture = depth_near_fbo->GetColorAttachement();
		break;

	case ViewportOutput::DEPTH_MID:
		last_displayed_texture = depth_mid_fbo->GetColorAttachement();
		break;

	case ViewportOutput::DEPTH_FAR:
		last_displayed_texture = depth_far_fbo->GetColorAttachement();
		break;

	case ViewportOutput::DEPTH_FULL:
		last_displayed_texture = depth_full_fbo->GetColorAttachement();
		break;

	default:
		break;
	}

}

bool Viewport::IsOptionSet(ViewportOption option) const
{
	return viewport_options & (int)option;
}

void Viewport::DepthMapPass(LightFrustum* light_frustum, FrameBuffer* depth_fbo) const
{
	depth_fbo->ClearAttachements();
	depth_fbo->GenerateAttachements(width, height);

	light_frustum->RenderMeshRenderersAABB();
	light_frustum->RenderLightFrustum();

	depth_fbo->Bind();
	BindCameraFrustumMatrices(light_frustum->light_orthogonal_frustum);

	App->cameras->main_camera->Clear();
	glViewport(0, 0, width, height);

	std::vector<ComponentMeshRenderer*> culled_shadow_casters = App->space_partitioning->GetCullingMeshes(
		App->cameras->main_camera, 
		App->renderer->mesh_renderers,
		ComponentMeshRenderer::MeshProperties::SHADOW_CASTER
	);

	for (ComponentMeshRenderer* culled_shadow_caster : culled_shadow_casters)
	{
		if (
			culled_shadow_caster->mesh_to_render != nullptr
			&& culled_shadow_caster->material_to_render != nullptr
			&& culled_shadow_caster->IsEnabled()
		) {
			GLuint mesh_renderer_program = culled_shadow_caster->BindDepthShaderProgram();
			culled_shadow_caster->BindMeshUniforms(mesh_renderer_program);
			culled_shadow_caster->RenderModel();
			glUseProgram(0);
		}
	}
	FrameBuffer::UnBind();
}

void Viewport::SetAntialiasing(bool antialiasing)
{
	this->antialiasing = antialiasing;
	main_fbo->SetMultiSampled(antialiasing);
	main_fbo->ClearAttachements();
	main_fbo->GenerateAttachements(width, height);
}

void Viewport::SetHDR(bool hdr)
{
	this->hdr = hdr;
	main_fbo->SetFloatingPoint(hdr);
	main_fbo->ClearAttachements();
	main_fbo->GenerateAttachements(width, height);
}

void Viewport::SetOutput(ViewportOutput output)
{
	viewport_output = output;
}
