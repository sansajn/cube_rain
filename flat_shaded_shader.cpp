#include "flat_shaded_shader.hpp"

namespace gles2 {

using phys::mat3,
	phys::Inverse,
	phys::Transpose;

constexpr char shader_program_code[] = R"(
// #version 100
#ifdef _VERTEX_
attribute vec3 position;
attribute vec3 normal;
uniform mat4 local_to_screen;
uniform mat3 normal_to_world;
varying vec3 n;
void main() {
	n = normalize(normal_to_world * normal);
	gl_Position = local_to_screen * vec4(position, 1.0);
}
#endif

#ifdef _FRAGMENT_
precision mediump float;
uniform vec3 color;
uniform vec3 light_direction;  // from surface to light in world space
varying vec3 n;
void main() {
	gl_FragColor = vec4(max(dot(n, light_direction), 0.2) * color, 1.0);
}
#endif
)";

flat_shaded_shader::flat_shaded_shader()
{
	_prog.from_memory(shader_program_code, 100);
	_color_u = _prog.uniform_variable("color");
	_light_dir_u = _prog.uniform_variable("light_direction");
	_local_to_screen_u = _prog.uniform_variable("local_to_screen");
	_normal_to_world_u = _prog.uniform_variable("normal_to_world");
	_position = _prog.attribute_location("position");
	_normal = _prog.attribute_location("normal");
}

void flat_shaded_shader::use()
{
	if (!_prog.used())
		_prog.use();
}

int flat_shaded_shader::position_location() const
{
	return _position;
}

int flat_shaded_shader::normal_location() const
{
	return _normal;
}

void flat_shaded_shader::model_color(vec3 const & rgb)
{
	_color_u = rgb;
}

void flat_shaded_shader::light_position(vec3 const & lpos)
{
	_light_dir_u = Normalized(lpos);
}

void flat_shaded_shader::light_direction(vec3 const & ldir)
{
	_light_dir_u = ldir;
}

void flat_shaded_shader::local_to_world(mat4 const & M)
{
	_local_to_screen_u = M * _world_to_screen;
	_normal_to_world_u = Transpose(Inverse(mat3{
		M._11, M._12, M._13,
		M._21, M._22, M._23,
		M._31, M._32, M._33}));
}

void flat_shaded_shader::world_to_screen(mat4 const & VP)
{
	_world_to_screen = VP;
}

}  // gles2
