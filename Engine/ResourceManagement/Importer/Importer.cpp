#include "Importer.h"

#include "Main/Application.h"
#include "Module/ModuleFileSystem.h"
#include "Helper/Config.h"

std::pair<bool, std::string> Importer::Import(const File & file, bool force) const
{
	ImportOptions already_imported = GetAlreadyImportedResource(file);
	if (already_imported.uid != 0 && !force) {
		return std::pair<bool, std::string>(true, already_imported.exported_file);
	}
	std::string uid = "default";
	SaveMetaFile(file, uid);
	return std::pair<bool, std::string>(false, uid);
}


ImportOptions Importer::GetAlreadyImportedResource(const File & file_to_look_for) const
{
	std::string meta_file_path = GetMetaFilePath(file_to_look_for);

	File meta_file(meta_file_path);
	if (App->filesystem->Exists(meta_file_path.c_str()) && meta_file.modification_timestamp >= file_to_look_for.modification_timestamp) {
	
		ImportOptions options;
		GetOptionsFromMeta(meta_file,options);
		if (options.version != IMPORTER_VERSION) {
			options.uid = 0;
			return options;
		}
		return options;
	}

	return ImportOptions();
}


void Importer::SaveMetaFile(const File & imported_file, const std::string & exported_path) const
{

	std::string meta_file_path = GetMetaFilePath(imported_file);

	Config scene_config;
	ImportOptions options;
	options.uid = std::hash<std::string>{}(imported_file.file_path);
	options.version = IMPORTER_VERSION;
	options.exported_file = exported_path;
	options.original_file = imported_file.file_path;
	options.Save(scene_config);

	std::string serialized_scene_string;
	scene_config.GetSerializedString(serialized_scene_string);

	App->filesystem->Save(meta_file_path.c_str(), serialized_scene_string.c_str(), serialized_scene_string.size() + 1);
}


void Importer::GetOptionsFromMeta(const File& file, ImportOptions & options)
{
	size_t readed_bytes;
	char* meta_file_data = App->filesystem->Load(file.file_path.c_str(), readed_bytes);
	std::string serialized_string = meta_file_data;
	free(meta_file_data);

	Config meta_config(serialized_string);
	options.Load(meta_config);
}

std::string Importer::GetMetaFilePath(const File& file)
{
	int extension_index = file.file_path.find_last_of(".");
	extension_index = extension_index != std::string::npos ? extension_index : file.file_path.size();
	return file.file_path.substr(0, extension_index) + ".meta";
}