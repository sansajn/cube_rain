#pragma once
#include <string>

namespace glt::shader {

constexpr unsigned GLES2_GLSL_VERSION = 100;

enum class gles2_shader_type
{
	vertex,
	fragment,
	number_of_types
};

std::string to_string(gles2_shader_type type);
int opengl_cast(gles2_shader_type type);
std::string shader_type_define_constant(gles2_shader_type type);

}  // glt::shader
