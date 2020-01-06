#include "Mesh.h"

Mesh::Mesh(std::vector<Vertex> && vertices, std::vector<uint32_t> && indices) : 
	vertices(vertices), 
	indices(indices)
{
	InitMesh();
}

Mesh::Mesh(std::vector<Vertex> && vertices, std::vector<uint32_t> && indices, std::string mesh_file_path) : vertices(vertices),
indices(indices),
mesh_file_path(mesh_file_path)
{
	InitMesh();
}
Mesh::Mesh(std::vector<Vertex> && vertices, std::vector<uint32_t> && indices, std::vector<std::string> && meshes_textures_path ,std::string mesh_file_path) : vertices(vertices),
indices(indices),
meshes_textures_path(meshes_textures_path),
mesh_file_path(mesh_file_path)
{
	InitMesh();
	
}
Mesh::~Mesh() {
	glDeleteBuffers(1, &vbo);
	glDeleteBuffers(1, &ebo);
	glDeleteVertexArrays(1, &vao);
}


void Mesh::Render() const
{
	glBindVertexArray(vao);
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void Mesh::InitMesh()
{
	for (size_t i = 0; i <indices.size(); i += 3)
	{
		float3 first_point = vertices[indices[i]].position;
		float3 second_point =vertices[indices[i + 1]].position;
		float3 third_point = vertices[indices[i + 2]].position;
		triangles.push_back(Triangle(first_point, second_point, third_point));
	}

	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ebo);

	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Mesh::Vertex), &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices[0], GL_STATIC_DRAW);

	// VERTEX POSITION
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (void*)0);

	// VERTEX TEXTURE COORDS
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex, tex_coords));

	// VERTEX NORMALS
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Mesh::Vertex), (void*)offsetof(Mesh::Vertex, normals));

	glBindVertexArray(0);
}