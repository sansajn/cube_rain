#include <iostream>
#include "opengl.hpp"
#include "module.hpp"

namespace glt::shader {

using std::string,
	std::cerr,
	std::endl;

namespace detail {

void dump_compile_log(GLuint shader, string const & name)
{
	GLint len;
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
	string log;
	log.resize(len);
	glGetShaderInfoLog(shader, len, nullptr, (GLchar *)log.data());
	cerr << "compile output ('" << name << "'):\n" << log << endl;
}

}  // detail

}  // glt::shader
