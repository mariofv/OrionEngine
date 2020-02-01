#include "MeshImporter.h"
#include "ResourceManagement/Resources/Mesh.h"
#include "MaterialImporter.h"
#include "Main/Application.h"

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/material.h>
#include <assimp/mesh.h>
#include "assimp/DefaultLogger.hpp"
#include "Brofiler/Brofiler.h"

MeshImporter::MeshImporter()
{
	Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
	Assimp::DefaultLogger::get()->attachStream(new AssimpStream(Assimp::Logger::Debugging), Assimp::Logger::Debugging);
	Assimp::DefaultLogger::get()->attachStream(new AssimpStream(Assimp::Logger::Info), Assimp::Logger::Info);
	Assimp::DefaultLogger::get()->attachStream(new AssimpStream(Assimp::Logger::Err), Assimp::Logger::Err);
	Assimp::DefaultLogger::get()->attachStream(new AssimpStream(Assimp::Logger::Warn), Assimp::Logger::Warn);
}

MeshImporter::~MeshImporter()
{
	Assimp::DefaultLogger::kill();
}

std::pair<bool, std::string> MeshImporter::Import(const File & file) const
{
	if (file.filename.empty())
	{
		APP_LOG_ERROR("Importing mesh error: Couldn't find the file to import.")
		return std::pair<bool, std::string>(false, "");
	}
	std::shared_ptr<File> already_imported = GetAlreadyImportedResource(LIBRARY_MESHES_FOLDER,file);
	if (already_imported != nullptr) {
		return std::pair<bool, std::string>(true, already_imported->file_path);
	}

	File output_file = App->filesystem->MakeDirectory(LIBRARY_MESHES_FOLDER+"/"+ file.filename_no_extension);
	APP_LOG_INIT("Importing model %s.", file.file_path.c_str());

	performance_timer.Start();

	const aiScene* scene = aiImportFile(file.file_path.c_str(), aiProcessPreset_TargetRealtime_MaxQuality);
	if (scene == NULL || output_file.file_path.empty())
	{
		const char *error = aiGetErrorString();
		APP_LOG_ERROR("Error loading model %s ", file.file_path.c_str());
		APP_LOG_ERROR(error);
		App->filesystem->Remove(&output_file);
		return std::pair<bool, std::string>(false, "");
	}
	performance_timer.Stop();
	float time = performance_timer.Read();
	APP_LOG_SUCCESS("Model %s loaded correctly from assimp in %f ms.", file.file_path.c_str(), time);

	
	aiNode * root_node = scene->mRootNode;
	std::string base_path = file.file_path.substr(0, file.file_path.find_last_of("//"));
	aiMatrix4x4 identity_transformation = aiMatrix4x4();
	ImportNode(root_node, identity_transformation, scene, base_path.c_str(),output_file.file_path);

	aiReleaseImport(scene);
	return std::pair<bool, std::string>(true, output_file.file_path);
}

void MeshImporter::ImportNode(const aiNode* root_node, const aiMatrix4x4& parent_transformation, const aiScene* scene, const char* file_path, const std::string& output_file) const
{
	aiMatrix4x4& current_transformtion = parent_transformation * root_node->mTransformation;

	for (size_t i = 0; i < root_node->mNumMeshes; ++i)
	{
		size_t mesh_index = root_node->mMeshes[i];
		std::vector<std::string> loaded_meshes_materials;
		App->material_importer->ImportMaterialFromMesh(scene, mesh_index, file_path, loaded_meshes_materials);

		std::string mesh_file = output_file + "/" + std::string(root_node->mName.data) + std::to_string(i) + ".ol";

		// Transformation
		aiVector3t<float> pScaling, pPosition;
		aiQuaterniont<float> pRotation;
		aiMatrix4x4 node_transformation = current_transformtion;
		node_transformation.Decompose(pScaling, pRotation, pPosition);
		pScaling *= SCALE_FACTOR;
		pPosition *= SCALE_FACTOR;

		node_transformation = aiMatrix4x4(pScaling, pRotation, pPosition);
		ImportMesh(scene->mMeshes[mesh_index], loaded_meshes_materials, node_transformation, mesh_file);
	}

	for (size_t i = 0; i < root_node->mNumChildren; i++)
	{
		ImportNode(root_node->mChildren[i], current_transformtion, scene, file_path,output_file);
	}
}

void MeshImporter::ImportMesh(const aiMesh* mesh, const std::vector<std::string> & loaded_meshes_materials, const aiMatrix4x4& mesh_transformation, const std::string& output_file) const
{
	std::vector<uint32_t> indices;
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	//We only accept triangle formed meshes
	if (indices.size() % 3 != 0)
	{
		APP_LOG_ERROR("Mesh %s have incorrect indices", mesh->mName.C_Str());
		return;
	}
	std::vector<Mesh::Vertex> vertices;
	vertices.reserve(mesh->mNumVertices);
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Mesh::Vertex new_vertex;
		aiVector3D transformed_position = mesh_transformation * mesh->mVertices[i];
		new_vertex.position = float3(transformed_position.x, transformed_position.y, transformed_position.z);
		new_vertex.tex_coords = float2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
		new_vertex.normals = float3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
		vertices.push_back(new_vertex);
	}

	std::vector<uint32_t> materials_path_size;
	uint32_t total_meshes_size = 0;
	for (auto & path_size : loaded_meshes_materials)
	{
		materials_path_size.push_back(path_size.size());
		total_meshes_size += path_size.size();
	}

	uint32_t num_indices = indices.size();
	uint32_t num_vertices = vertices.size();
	uint32_t num_materials = loaded_meshes_materials.size();
	uint32_t ranges[3] = { num_indices, num_vertices, num_materials };

	uint32_t size = sizeof(ranges) + sizeof(uint32_t) * num_indices + sizeof(Mesh::Vertex) * num_vertices + sizeof(uint32_t) * num_materials + total_meshes_size;

	char* data = new char[size]; // Allocate
	char* cursor = data;
	size_t bytes = sizeof(ranges); // First store ranges
	memcpy(cursor, ranges, bytes);

	cursor += bytes; // Store indices
	bytes = sizeof(uint32_t) * num_indices;
	memcpy(cursor, &indices.front(), bytes);

	cursor += bytes; // Store vertices
	bytes = sizeof(Mesh::Vertex) * num_vertices;
	memcpy(cursor, &vertices.front(), bytes);

	cursor += bytes; // Store sizes
	bytes = sizeof(uint32_t) * num_materials;
	if (bytes != 0)
	{
		memcpy(cursor, &materials_path_size.front(), bytes);
	}
	for (size_t i = 0; i < num_materials; i++)
	{
		cursor += bytes; // Store materials
		bytes = materials_path_size.at(i);
		memcpy(cursor, loaded_meshes_materials[i].c_str(), bytes);
	}


	App->filesystem->Save(output_file.c_str(), data, size);
	delete data;
}

 std::shared_ptr<Mesh> MeshImporter::Load(const std::string& file_path) const
 {
	 BROFILER_CATEGORY("Load Mesh", Profiler::Color::Brown);
	 if (!App->filesystem->Exists(file_path.c_str()))
	 {
		 return nullptr;
	 }
	//Check if the mesh is already loaded
	auto it = std::find_if(mesh_cache.begin(), mesh_cache.end(), [file_path](const std::shared_ptr<Mesh> mesh) 
	{
		return mesh->mesh_file_path == file_path;
	});
	if (it != mesh_cache.end())
	{
		APP_LOG_INFO("Model %s exists in cache.", file_path.c_str());
		return *it;
	}

	APP_LOG_INFO("Loading model %s.", file_path.c_str());
	performance_timer.Start();
	size_t mesh_size;
	char * data = App->filesystem->Load(file_path.c_str(), mesh_size);
	char* cursor = data;

	uint32_t ranges[3];
	//Get ranges
	size_t bytes = sizeof(ranges); // First store ranges
	memcpy(ranges, cursor, bytes);

	std::vector<uint32_t> indices;
	std::vector<Mesh::Vertex> vertices;
	std::vector<uint32_t> meshes_materials_size;
	std::vector<std::string> meshes_textures_path;

	indices.resize(ranges[0]);

	cursor += bytes; // Get indices
	bytes = sizeof(uint32_t) * ranges[0];
	memcpy(&indices.front(), cursor, bytes);

	vertices.resize(ranges[1]);

	cursor += bytes; // Get vertices
	bytes = sizeof(Mesh::Vertex) * ranges[1];
	memcpy(&vertices.front(), cursor, bytes);


	cursor += bytes; // Get sizes
	bytes = sizeof(uint32_t) * ranges[2];
	meshes_materials_size.resize(ranges[2]);
	if (bytes != 0)
	{
		memcpy(&meshes_materials_size.front(), cursor, bytes);
	}

	meshes_textures_path.resize(ranges[2]);
	for (size_t i = 0; i < ranges[2]; i++)
	{
		cursor += bytes; // Get materials
		bytes = meshes_materials_size[i];
		std::vector<char> path;
		path.resize(bytes);
		memcpy(&path.front(),cursor,bytes);
		meshes_textures_path[i] = std::string(path.begin(), path.end());
	}

	std::shared_ptr<Mesh> new_mesh = std::make_shared<Mesh>(std::move(vertices), std::move(indices), std::move(meshes_textures_path),file_path);
	mesh_cache.push_back(new_mesh);
	performance_timer.Stop();
	float time = performance_timer.Read();
	free(data);
	APP_LOG_SUCCESS("Model %s loaded correctly from own format in %.2f ms.", file_path.c_str(), time);

	return new_mesh;
}

 //Remove the mesh from the cache if the only owner is the cache itself
 void MeshImporter::RemoveMeshFromCacheIfNeeded(const std::shared_ptr<Mesh> & mesh) 
 {
	 auto it = std::find(mesh_cache.begin(), mesh_cache.end(), mesh);
	 if (it != mesh_cache.end() && (*it).use_count() <= 2)
	 {
		 mesh_cache.erase(it);
	 }
 }