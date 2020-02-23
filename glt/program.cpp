#include <memory>
#include <string>
#include <fstream>
#include <iostream>
#include <boost/format.hpp>
#include <glm/fwd.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "program.hpp"

namespace glt::shader {

using std::string,
	std::stringstream,
	std::shared_ptr,
	std::make_shared;

namespace detail {

void gl_error_check()
{
#ifndef NDEBUG
	switch (glGetError())
	{
		case GL_NO_ERROR: return;
		case GL_INVALID_ENUM: throw exception{"glGetError(): invalid enum"};
		case GL_INVALID_VALUE: throw exception{"glGetError(): invalid value"};
		case GL_INVALID_OPERATION: throw exception{"glGetError(): invalid operation"};
		case GL_INVALID_FRAMEBUFFER_OPERATION: throw exception{"glGetError(): invalid framebuffer operation"};
		case GL_OUT_OF_MEMORY: throw exception{"glGetError(): out of memory"};
		default: throw exception{"glGetError(): unknown error"};
	}
#endif
}

}

//void module::compile(string const & code, shader_type type)
//{
//	char const * lines[3];
//	string vstr = string("#version ") + to_string(version) + string("\n");
//	lines[0] = vstr.c_str();

//	unsigned sid;
//	GLenum stype;

//	switch (type)
//	{
//		case shader_type::vertex:
//			sid = 0;
//			lines[1] = "#define _VERTEX_\n";
//			stype = GL_VERTEX_SHADER;
//			break;

//		case shader_type::fragment:
//			sid = 1;
//			lines[1] = "#define _FRAGMENT_\n";
//			stype = GL_FRAGMENT_SHADER;
//			break;

//		default:
//			assert(0 && "unsupported shader program type");
//	}

//	lines[2] = code.c_str();

//	_ids[sid] = glCreateShader(stype);
//	glShaderSource(_ids[sid], 3, lines, nullptr);
//	glCompileShader(_ids[sid]);

//	compile_check(_ids[sid], type);
//}

//void module::compile_check(unsigned sid, shader_type type)
//{
//	string name = _fname + string(":") + to_string(type);

//	GLint compiled;
//	glGetShaderiv(sid, GL_COMPILE_STATUS, &compiled);

//	if (compiled == GL_FALSE)  // TODO: zobrazit warningi ?
//	{
//		dump_compile_log(sid, name);
//		throw exception("program shader compilation failed");
//	}

//	assert(glGetError() == GL_NO_ERROR && "opengl error");
//}

namespace detail {

void dump_link_log(GLuint program, std::string const & name)
{
	GLint len;
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
	std::string log;
	log.resize(len);
	glGetProgramInfoLog(program, len, nullptr, (GLchar *)log.data());
	std::cerr << "link output ('" << name << "'):\n" << log << std::endl;
}

}  // detail

template <>
void set_uniform<glm::mat3>(int location, glm::mat3 const & v)
{
	glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(v));
}

template <>
void set_uniform<glm::mat4>(int location, glm::mat4 const & v)
{
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(v));
}

template <>
void set_uniform<glm::vec2>(int location, glm::vec2 const & v)
{
	glUniform2fv(location, 1, glm::value_ptr(v));
}

template <>
void set_uniform<glm::vec3>(int location, glm::vec3 const & v)
{
	glUniform3fv(location, 1, glm::value_ptr(v));
}

template <>
void set_uniform<glm::vec4>(int location, glm::vec4 const & v)
{
	glUniform4fv(location, 1, glm::value_ptr(v));
}

template <>
void set_uniform<glm::ivec2>(int location, glm::ivec2 const & v)
{
	glUniform2iv(location, 1, glm::value_ptr(v));
}

template <>
void set_uniform<glm::ivec3>(int location, glm::ivec3 const & v)
{
	glUniform3iv(location, 1, glm::value_ptr(v));
}

template <>
void set_uniform<glm::ivec4>(int location, glm::ivec4 const & v)
{
	glUniform4iv(location, 1, glm::value_ptr(v));
}

template <>
void set_uniform<int>(int location, int const & v)
{
	glUniform1i(location, v);
}

template <>
void set_uniform<unsigned>(int location, unsigned const & v)
{
//	glUniform1ui(location, v);
	throw std::logic_error{"unsigned uniform is not supported in opengl es 2"};
}

template<>
void set_uniform<float>(int location, float const & v)
{
	glUniform1f(location, v);
}

template <>  // pre pole float-ou
void set_uniform<float>(int location, float const * a, int n)
{
	glUniform1fv(location, n, a);
}

template <>  // pre pole vec4 vektorov
void set_uniform<glm::vec4>(int location, glm::vec4 const * a, int n)
{
	glUniform4fv(location, n, glm::value_ptr(*a));
}

template <>  // pre pole matic
void set_uniform<glm::mat4>(int location, glm::mat4 const * a, int n)
{
	glUniformMatrix4fv(location, n, GL_FALSE, glm::value_ptr(*a));
}

template <>
void get_uniform(unsigned program, int location, int & v)
{
	glGetUniformiv(program, location, &v);
}

}  // glt::shader
