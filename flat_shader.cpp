#include "flat_shader.hpp"

namespace gles2 {

constexpr char program_shader_code[] = R"(
// #version 100
#ifdef _VERTEX_
attribute vec3 position;
uniform mat4 local_to_screen;
void main()	{
	gl_Position = local_to_screen * vec4(position, 1.0);
}
#endif

#ifdef _FRAGMENT_
precision mediump float;
uniform vec3 color;
void main() {
	gl_FragColor = vec4(color, 1);
}
#endif)";

flat_shader::flat_shader()
{
	_prog.from_memory(program_shader_code, 100);
	_color_u = _prog.uniform_variable("color");
	_local_to_screen_u = _prog.uniform_variable("local_to_screen");
	_position = _prog.attribute_location("position");
}

void flat_shader::use()
{
	if (!_prog.used())
		_prog.use();
}

int flat_shader::position_location() const
{
	return _position;
}

void flat_shader::model_color(vec3 const & rgb)
{
	_color_u = rgb;
}

void flat_shader::local_to_world(mat4 const & M)
{
	_local_to_screen_u = M * _world_to_screen;
}

void flat_shader::world_to_screen(mat4 const & VP)
{
	_world_to_screen = VP;
}

}  // gles2
