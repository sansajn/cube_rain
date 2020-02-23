#pragma once
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <utility>
#include <cassert>
#include "opengl.hpp"
#include "module.hpp"

namespace glt::shader {

constexpr unsigned INVALID_PROGRAM_ID = 0;

/*! Uniform variable.
\note Default constructed uniform have undefined behaviour if used, use
Program::uniform_variable() factory instead.

\code
// vector assign
vector<int> data{1,2,3,4,5};
u = data;
\endcode */
template <typename Program>
class uniform
{
public:
	uniform();
	uniform(int location, Program * prog);

	template <typename T>
	uniform & operator=(T const & v);

	template <typename T>
	operator T();

private:
	int _loc;
	Program * _prog;
};

//! GLSL program representation.
template <typename Module>
class program
{
public:
	using module_type = Module;
	using module_ptr = std::shared_ptr<module_type>;
	using uniform_type = uniform<program>;

	static constexpr int invalid_location = -1;

	program();
	program(std::string const & fname, unsigned version);
	program(module_ptr m);
	~program();

	void from_file(std::string const & fname, unsigned version);
	void from_memory(std::string const & source, unsigned version);

	void attach(module_ptr m);
	void attach(std::vector<module_ptr> const & mods);

	int id() const {return _pid;}
	void use();
	bool used() const;

	int attribute_location(char const * name) const;
	uniform_type uniform_variable(std::string const & name);

	void free();

	program(program &) = delete;
	void operator=(program &) = delete;

private:
	void create_program_lazy();
	void init_uniforms();
	void append_uniform(std::string const & name, int index);
	void link();
	bool link_check();

	unsigned _pid;  //!< progrm id
	std::vector<module_ptr> _modules;
	std::map<std::string, uniform_type> _uniforms;

	static program * _CURRENT;  //!< currently used program
};

namespace detail {

void dump_link_log(GLuint program, std::string const & name);

}  // detail

template <typename Program>
uniform<Program>::uniform()
	: _loc{0}
	, _prog{nullptr}
{}

template <typename Program>
uniform<Program>::uniform(int location, Program * prog)
	: _loc(location)
	, _prog(prog)
{}

template <typename Module>
program<Module> * program<Module>::_CURRENT = nullptr;

template <typename Module>
program<Module>::program()
	: _pid(INVALID_PROGRAM_ID)
{}

template <typename Module>
program<Module>::program(std::string const & fname, unsigned version)
	: _pid(INVALID_PROGRAM_ID)
{
	attach(module_ptr{new module_type{fname, version}});
}

template <typename Module>
program<Module>::program(module_ptr m)
	: _pid(INVALID_PROGRAM_ID)
{
	attach(m);
}

template <typename Module>
void program<Module>::from_file(std::string const & fname, unsigned version)
{
	attach(module_ptr{new module_type{fname, version}});
}

template <typename Module>
void program<Module>::from_memory(std::string const & source, unsigned version)
{
	module_ptr m = std::make_shared<module_type>();
	m->from_memory(source, version);
	attach(m);
}

template <typename Module>
void program<Module>::attach(module_ptr m)
{
	create_program_lazy();

	for (unsigned sid : m->ids())
		glAttachShader(_pid, sid);

	link();
	init_uniforms();

	_modules.push_back(m);

	assert(glGetError() == GL_NO_ERROR && "opengl error");
}

template <typename Module>
void program<Module>::attach(std::vector<module_ptr> const & mods)
{
	create_program_lazy();

	for (auto m : mods)
	{
		for (unsigned sid : m->ids())
			glAttachShader(_pid, sid);
	}

	link();
	init_uniforms();

	for (auto m : mods)
		_modules.push_back(m);

	assert(glGetError() == GL_NO_ERROR && "opengl error");
}

template <typename Module>
void program<Module>::create_program_lazy()
{
	if (_pid == INVALID_PROGRAM_ID)
		_pid = glCreateProgram();
	assert(_pid != INVALID_PROGRAM_ID && "unable to create a program");
}

template <typename Module>
void program<Module>::link()
{
	glLinkProgram(_pid);

	bool result = link_check();
	assert(result && "program linking failed");
	if (!result)
		throw exception("unable to link a program");
}

template <typename Module>
void program<Module>::use()
{
	if (_CURRENT == this)
		return;

	glUseProgram(_pid);
	_CURRENT = this;
}

template <typename Module>
bool program<Module>::used() const
{
	return _CURRENT == this;
}

template <typename Module>
typename program<Module>::uniform_type program<Module>::uniform_variable(
	std::string const & name)
{
	auto it = _uniforms.find(name);

	if (it == _uniforms.end())
		return uniform_type{};

	assert(it != _uniforms.end() && "unknown uniform variable (or not active)");

	return it->second;
}

template <typename Module>
int program<Module>::attribute_location(char const * name) const
{
	return glGetAttribLocation(_pid, name);
}

template <typename Module>
program<Module>::~program()
{
	free();
}

template <typename Module>
void program<Module>::free()
{
	if (used())
	{
		glUseProgram(INVALID_PROGRAM_ID);
		_CURRENT = nullptr;
	}

	_uniforms.clear();
	_modules.clear();

	glDeleteProgram(_pid);
	_pid = INVALID_PROGRAM_ID;
}

template <typename Module>
void program<Module>::init_uniforms()
{
	GLint max_length = 0;
	glGetProgramiv(_pid, GL_ACTIVE_UNIFORM_MAX_LENGTH, &max_length);

	std::unique_ptr<GLchar[]> buf{new GLchar[max_length]};

	GLint nuniform = 0;
	glGetProgramiv(_pid, GL_ACTIVE_UNIFORMS, &nuniform);
	for (GLuint i = 0; i < (GLuint)nuniform; ++i)
	{
		GLint size;
		GLsizei length;
		GLenum type;
		glGetActiveUniform(_pid, i, max_length, &length, &size, &type, buf.get());

		std::string uname(buf.get());
		GLint location = glGetUniformLocation(_pid, uname.c_str());
		assert(location != -1 && "unknown uniform");
		if (size > 1 && uname.find_first_of('[') != std::string::npos)  // if array removes [0]
			uname = uname.substr(0, uname.find_first_of('['));

		append_uniform(uname, location);
	}

	assert(glGetError() == GL_NO_ERROR);
}

template <typename Module>
void program<Module>::append_uniform(std::string const & name, int index)
{
	_uniforms[name] = uniform_type{index, this};
}

template <typename Module>
bool program<Module>::link_check()
{
	GLint linked;
	glGetProgramiv(_pid, GL_LINK_STATUS, &linked);
	if (linked == GL_FALSE)
		detail::dump_link_log(_pid, "<unspecified>");
	return linked != GL_FALSE;
}


template <typename T>
void set_uniform(int location, T const & v);

template <typename T>
void set_uniform(int location, T const * a, int n);

template <typename T>
void set_uniform(int location, std::vector<T> const & arr)
{
	set_uniform(location, arr.data(), arr.size());
}

template <typename T>
void set_uniform(int location, std::pair<T *, int> const & a)  //!< for array where pair::first is array pointer and pair::second is number of array elements
{
	set_uniform(location, a.first, a.second);
}

template <typename T>
void get_uniform(unsigned program, int location, T & v);

std::string read_file(std::string const & fname);


namespace detail {

void gl_error_check();

}

template <typename Program>
template <typename T>
uniform<Program> & uniform<Program>::operator=(T const & v)
{
	assert(_prog->used() && "pokusam sa nastavit uniform neaktivneho programu");
	set_uniform(_loc, v);	
	assert(glGetError() == GL_NO_ERROR && "opengl error");
	return *this;
}

template <typename Program>
template <typename T>
uniform<Program>::operator T()
{
	T v;
	get_uniform(_prog->id(), _loc, v);
	return v;
}

}  // glt::shader
