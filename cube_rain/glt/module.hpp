#pragma once
#include <string>
#include <boost/range/adaptor/filtered.hpp>
#include "io.hpp"
#include "exception.hpp"
#include "opengl.hpp"

namespace glt::shader {

namespace detail {

inline bool valid_shader_id(unsigned v) {return v > 0;}

struct valid_shader_pred
{
	bool operator()(unsigned const & v) const {return valid_shader_id(v);}
};

void dump_compile_log(GLuint shader, std::string const & name);

}  // detail

/*! Module represents GLSL program and can contain vertex, fragment, ...
shaders in one file like this

\code
#ifdef _VERTEX_
attribute vec3 position;
uniform mat4 local_to_screen;
void main()	{
	gl_Position = local_to_screen * vec4(position,1);
}
#endif
#ifdef _FRAGMENT_
precision mediump float;
uniform vec3 color;  // vec3(.7);
void main() {
	gl_FragColor = vec4(color, 1);
}
#endif
\endcode */
template <typename ShaderType>
class module
{
public:
	module();
	module(std::string const & fname, unsigned version);
	~module();

	void from_file(std::string const & fname, unsigned version);
	void from_memory(std::string const & source, unsigned version);

	boost::filtered_range<detail::valid_shader_pred, const unsigned[int(ShaderType::number_of_types)]>
		ids() const;

	module(module &) = delete;
	module & operator=(module &) = delete;

private:
	bool compile(std::string const & code, int gl_shader_type, unsigned version,
		std::string const & define_constant, unsigned & shader_id);
	void clear_ids();

	unsigned _ids[int(ShaderType::number_of_types)];

	// debug
	std::string _fname;
};

template <typename ShaderType>
module<ShaderType>::module()
{
	clear_ids();
}

template <typename ShaderType>
module<ShaderType>::module(std::string const & fname, unsigned version)
{
	clear_ids();
	from_file(fname, version);
}

template <typename ShaderType>
module<ShaderType>::~module()
{
	for (unsigned sid : _ids)
		if (sid > 0)
			glDeleteShader(sid);
}

template <typename ShaderType>
void module<ShaderType>::from_file(std::string const & fname, unsigned version)
{
	_fname = fname;
	std::string source = read_file(fname);
	from_memory(source, version);
}

template <typename ShaderType>
void module<ShaderType>::from_memory(std::string const & source, unsigned version)
{
	for (unsigned i = 0; i < (unsigned)ShaderType::number_of_types; ++i)
	{
		ShaderType type = (ShaderType)i;
		std::string define_constant = shader_type_define_constant(type);
		if (source.find(define_constant) != std::string::npos)
		{
			if (!compile(source, opengl_cast(type), version, define_constant, _ids[i]))
			{
				std::string name{
					_fname.empty() ? to_string(type) : _fname + to_string(type)};

				detail::dump_compile_log(_ids[i], name);
				throw exception("program shader compilation failed");
			}

			assert(glGetError() == GL_NO_ERROR && "opengl error");
		}
	}

	// empty module check
	for (auto id : _ids)
		if (detail::valid_shader_id(id))
			return;  // we have at least one valid shader
	throw exception("empty shader module");
}

template <typename ShaderType>
boost::filtered_range<detail::valid_shader_pred,
	const unsigned[int(ShaderType::number_of_types)]>
module<ShaderType>::ids() const
{
	detail::valid_shader_pred pred;
	return boost::filtered_range<detail::valid_shader_pred, const unsigned[int(ShaderType::number_of_types)]>(pred, _ids);
}

template <typename ShaderType>
bool module<ShaderType>::compile(std::string const & code, int gl_shader_type,
	unsigned version,	std::string const & define_constant, unsigned & shader_id)
{
	char const * lines[3];

	std::string version_line = "#version " + std::to_string(version) + "\n";
	lines[0] = version_line.c_str();

	std::string define_line = "#define " + define_constant + "\n";
	lines[1] = define_line.c_str();

	lines[2] = code.c_str();

	shader_id = glCreateShader(gl_shader_type);
	glShaderSource(shader_id, 3, lines, nullptr);
	glCompileShader(shader_id);

	GLint compiled;
	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled);

	return compiled == GL_TRUE;
}

template <typename ShaderType>
void module<ShaderType>::clear_ids()
{
	for (unsigned & id : _ids)
		id = 0;
}

}  //  glt::shader
