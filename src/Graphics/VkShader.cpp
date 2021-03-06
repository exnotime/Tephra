#include "VkShader.h"

#include <shaderc/shaderc.h>
#include <fstream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include "DescriptorSetLayout.h"
using namespace smug;

const char* StageString(SHADER_KIND stage) {
	if (stage == VERTEX)
		return "VERTEX";
	else if (stage == FRAGMENT)
		return "FRAGMENT";
	else if (stage == GEOMETRY)
		return "GEOMETRY";
	else if (stage == CONTROL)
		return "CONTROL";
	else if (stage == EVALUATION)
		return "EVALUATION";
	else if (stage == COMPUTE)
		return "COMPUTE";
	else
		return "";
}
#define USE_GLSLANG_VALIDATOR

#ifdef USE_SHADERC
VkShaderModule smug::LoadShader(const VkDevice& device, const eastl::string& filename, SHADER_KIND stage, const eastl::string& entryPoint, SHADER_LANGUAGE language) {
	//read file ending and figure out shader type
	size_t lastDot = filename.find_last_of('.');
	eastl::string fileEnding = filename.substr(lastDot + 1);
	shaderc_shader_kind shaderType;
	if (stage == VERTEX) {
		shaderType = shaderc_glsl_vertex_shader;
	} else if (stage == FRAGMENT) {
		shaderType = shaderc_glsl_fragment_shader;
	} else if (stage == GEOMETRY) {
		shaderType = shaderc_glsl_geometry_shader;
	} else if (stage == CONTROL) {
		shaderType = shaderc_glsl_tess_control_shader;
	} else if (stage == EVALUATION) {
		shaderType = shaderc_glsl_tess_evaluation_shader;
	} else if (stage == COMPUTE) {
		shaderType = shaderc_glsl_compute_shader;
	} else if (stage == PRECOMPILED) {
		//Load  precompiled shader
		VkShaderModuleCreateInfo shaderInfo;
		FILE* fin = fopen(filename.c_str(), "rb");
		fseek(fin, 0, SEEK_END);
		uint64_t fileSize = ftell(fin);
		shaderInfo.codeSize = fileSize;
		rewind(fin);
		char* code = new char[fileSize];
		fread(code, sizeof(char), fileSize, fin);
		fclose(fin);
		shaderInfo.pCode = reinterpret_cast<const uint32_t*>(code);
		VkShaderModule module = device.createShaderModule(shaderInfo);
		delete[] code;
		return module;
	}

	//check if there is an up to date shader cache
	eastl::string cacheName = SHADER_CACHE_DIR + filename.substr(filename.find_last_of('/') + 1) + "." + StageString(stage) + ".spv";
	struct stat cacheBuf, fileBuf;
	stat(cacheName.c_str(), &cacheBuf);
	stat(filename.c_str(), &fileBuf);
#if USE_SHADER_CACHE
	//either there is no shader at filename or it was created 1 jan 1970
	if (fileBuf.st_mtime == 0) {
		printf("Error opening shader %s\n", filename.c_str());
		return nullptr;
	}
	if (cacheBuf.st_mtime > fileBuf.st_mtime) {
		//there is an up to date cache
		VkShaderModuleCreateInfo shaderInfo;
		FILE* fin = fopen(cacheName.c_str(), "rb");
		shaderInfo.codeSize = cacheBuf.st_size;
		char* code = new char[cacheBuf.st_size];
		fread(code, sizeof(char), cacheBuf.st_size, fin);
		fclose(fin);
		shaderInfo.pCode = reinterpret_cast<const uint32_t*>(code);
		VkShaderModule module = device.createShaderModule(shaderInfo);
		delete[] code;
		return module;
	}
#endif
	//load shader file
	FILE* fin = fopen(filename.c_str(), "rb");
	char* code = new char[fileBuf.st_size];
	fread(code, sizeof(char), fileBuf.st_size, fin);
	fclose(fin);
	//compile into spir-v
	shaderc_compiler_t compiler = shaderc_compiler_initialize();
	shaderc_compile_options_t options = shaderc_compile_options_initialize();

	if(language == GLSL)
		shaderc_compile_options_set_source_language(options, shaderc_source_language_glsl);
	else if (language == HLSL)
		shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);

#ifdef DEBUG
	shaderc_compile_options_set_generate_debug_info(options);
#endif
	shaderc_compile_options_set_optimization_level(options, shaderc_optimization_level::shaderc_optimization_level_zero);
	//add macros
	const char* stageStr = StageString(stage);
	shaderc_compile_options_add_macro_definition(options, stageStr, strlen(stageStr), nullptr, 0);

	//include resolver lambda
	auto includeResolver = [](void* user_data, const char* requested_source, int type,
	const char* requesting_source, size_t include_depth) -> shaderc_include_result* {
		eastl::string filename;
		if (type == shaderc_include_type_relative) {
			eastl::string reqSrc = eastl::string(requesting_source);
			filename = reqSrc.substr(0, reqSrc.find_last_of('/')) + '/' + eastl::string(requested_source);
		} else if (type == shaderc_include_type_standard) {
			filename = "shaders/" + eastl::string(requested_source);
		}
		FILE* fin = fopen(filename.c_str(), "rb");
		fseek(fin, 0, SEEK_END);
		size_t size = ftell(fin);
		rewind(fin);
		char* src = new char[size];
		fread(src, sizeof(char), size,fin);
		fclose(fin);

		char* srcfile = new char[filename.size()];
		strcpy(srcfile, filename.c_str());
		shaderc_include_result* res = new shaderc_include_result();
		res->content = src;
		res->content_length = size;
		res->source_name = srcfile;
		res->source_name_length = filename.size();
		res->user_data = nullptr;
		return res;
	};

	auto includeResRelease = [](void* user_data, shaderc_include_result* include_result) {
		delete include_result;
	};
	shaderc_compile_options_set_include_callbacks(options, includeResolver, includeResRelease, nullptr);

	shaderc_compilation_result_t result = shaderc_compile_into_spv(compiler, code, fileBuf.st_size, shaderType, filename.c_str(), "main", options);
	if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) {
		const char* errors = shaderc_result_get_error_message(result);
		printf("Error compiling shader %s \n Errors %s\n", filename.c_str(), errors);
		shaderc_result_release(result);
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);
		return nullptr;
	}
	delete[] code;
	//create vulkan shader module
	VkShaderModuleCreateInfo shaderInfo;
	shaderInfo.codeSize = shaderc_result_get_length(result);
	shaderInfo.pCode = reinterpret_cast<const uint32_t*>(shaderc_result_get_bytes(result));
	VkShaderModule module = device.createShaderModule(shaderInfo);
	//TEST:
	DescriptorSetLayout layout;
	layout.InitFromSpirV(shaderInfo.pCode, (uint32_t)shaderInfo.codeSize);
	//save to cache
	FILE* fout = fopen(cacheName.c_str(), "wb");
	if (fout) {
		fwrite(shaderc_result_get_bytes(result), sizeof(char), shaderc_result_get_length(result), fout);
		fclose(fout);
	}
	//clean up
	shaderc_result_release(result);
	shaderc_compile_options_release(options);
	shaderc_compiler_release(compiler);
	return module;
}
#endif

#ifdef USE_GLSLANG_VALIDATOR
VkShaderModule smug::LoadShaderModule(const VkDevice& device, const eastl::string& filename, SHADER_KIND stage, const eastl::string& entryPoint, SHADER_LANGUAGE lang) {
	//use the program glslangvalidator
	eastl::string command;
	command += "%VULKAN_SDK%/Bin/glslangValidator.exe -V ";
	switch (stage) {
	case smug::VERTEX:
		command += "-S vert -DVERTEX";
		break;
	case smug::FRAGMENT:
		command += "-S frag -DFRAGMENT";
		break;
	case smug::GEOMETRY:
		command += "-S geom -DGEOMETRY";
		break;
	case smug::EVALUATION:
		command += "-S tese -DEVALUATION";
		break;
	case smug::CONTROL:
		command += "-S tesc -DCONTROL";
		break;
	case smug::COMPUTE:
		command += "-S comp -DCOMPUTE";
		break;
	case smug::MESH:
		command += "-S mesh -DMESH";
		break;
	case smug::TASK:
		command += "-S task -DTASK";
		break;
	case smug::RAY_GEN:
		command += "-S rgen -DRAY_GEN";
		break;
	case smug::RAY_ANY_HIT:
		command += "-S rahit -DRAY_ANY_HIT";
		break;
	case smug::RAY_CLOSEST_HIT:
		command += "-S rchit -DRAY_CLOSEST_HIT";
		break;
	case smug::RAY_MISS:
		command += "-S rmiss -DRAY_MISS";
		break;
	case smug::RAY_INTERSECTION:
		command += "-S rint -DRAY_INTERSECTION";
		break;
	case smug::RAY_CALLABLE:
		command += "-S rcall -DRAY_CALLABLE";
		break;
	}
	command += " -e " + entryPoint;


	command += " -I./shader";
	command += " -o ./temp.spv";
	command += " ./" + filename;

	int ret = system(command.c_str());
	VkShaderModule module = nullptr;
	FILE* fin = fopen("./temp.spv", "rb");
	if (fin) {
		fseek(fin, 0, SEEK_END);
		size_t size = ftell(fin);
		rewind(fin);
		uint8_t* buffer = (uint8_t*)malloc(size);
		fread(buffer, sizeof(uint8_t), size, fin);
		fclose(fin);

		VkShaderModuleCreateInfo shaderInfo = {};
		shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		shaderInfo.pNext = nullptr;
		shaderInfo.codeSize = size;
		shaderInfo.pCode = reinterpret_cast<const uint32_t*>(buffer);
		vkCreateShaderModule(device, &shaderInfo, nullptr, &module);
		free(buffer);
	}
	fclose(fin);
	system("del ./temp.spv");
	
	return module;
}


#endif