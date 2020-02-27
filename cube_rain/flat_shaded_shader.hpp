#pragma once
#include "glt/gles2.hpp"
#include "glt/program.hpp"
#include "phys/matrices.h"

namespace gles2 {

using phys::vec3,
	phys::mat4;

using program = glt::shader::program<glt::shader::module<
	glt::shader::gles2_shader_type>>;

/*! Shader program with model color and diffuse lighting support.
\note Set world_to_screen transformation befor local_to_world transformation,
otherwise identity matrix will be used as world_to_screen transformation. */
class flat_shaded_shader
{
public:
	flat_shaded_shader();
	void use();
	int position_location() const;
	int normal_location() const;

	// setters
	void model_color(vec3 const & rgb);
	void light_position(vec3 const & lpos);
	void light_direction(vec3 const & ldir);  // normalized vector
	void local_to_world(mat4 const & M);
	void world_to_screen(mat4 const & VP);

private:
	program _prog;
	program::uniform_type _color_u,
		_light_dir_u,
		_local_to_screen_u,
		_normal_to_world_u;
	int _position,
		_normal;
	mat4 _world_to_screen;
};

}  // gles2
