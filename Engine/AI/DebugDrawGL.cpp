#include "DebugDrawGL.h"

#include "Main/Application.h"
#include "Module/ModuleProgram.h"
#include "Module/ModuleDebug.h"


void DebugDrawGL::Vertex(const float* pos, unsigned int color)
{
	vertices.push_back(math::float3(pos));
}


void DebugDrawGL::DrawMesh(ComponentCamera& camera)
{
	if (vertices.size() == 0)
		return;


	unsigned int shader = App->program->GetShaderProgramId("NavMesh");
	math::float4x4 model = math::float4x4::identity;

	glUseProgram(shader);
	glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_TRUE, &model[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_TRUE, &camera.view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shader, "proj"), 1, GL_TRUE, &camera.proj[0][0]);

	glEnableVertexAttribArray(0); // attribute 0
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(
		0, // attribute 0
		3, // number of componentes (3 floats)
		GL_FLOAT, // data type
		GL_FALSE, // should be normalized?
		0, // stride
		(void*)0 // array buffer offset
	);


	glDrawArrays(GL_TRIANGLES, 0, vertices.size()); // start at 0 and tris count
	glDisableVertexAttribArray(0);
	glUseProgram(0);
}

void DebugDrawGL::GenerateBuffers()
{
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(float) * vertices.size(), &vertices[0], GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);


	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void DebugDrawGL::CleanUp()
{
	vertices.clear();
	glDeleteBuffers(1, &vbo);
}