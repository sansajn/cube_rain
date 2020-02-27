#include "exception.hpp"
#include "opengl.hpp"
#include "gles2.hpp"

namespace glt::shader {

using std::string,
	std::to_string;

string to_string(gles2_shader_type type)
{
	switch (type)
	{
		case gles2_shader_type::vertex: return "vertex";
		case gles2_shader_type::fragment: return "fragment";
		default:
			throw exception("unsupported shader program type");
	}
}

int opengl_cast(gles2_shader_type type)
{
	switch (type)
	{
		case gles2_shader_type::vertex: return GL_VERTEX_SHADER;
		case gles2_shader_type::fragment: return GL_FRAGMENT_SHADER;
		default:
			throw exception{"unknown shader type (" + to_string((int)type) + ")"};
	}
}

string shader_type_define_constant(gles2_shader_type type)
{
	switch (type)
	{
		case gles2_shader_type::vertex: return "_VERTEX_";
		case gles2_shader_type::fragment: return "_FRAGMENT_";
		default:
			throw exception{"unknown shader type (" + to_string((int)type) + ")"};
	}
}

}  // glt::shader
