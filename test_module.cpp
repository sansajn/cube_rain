#include "glt/gles2.hpp"
#include "glt/module.hpp"
using std::string;
using namespace glt::shader;

string shader_program_code = R"(
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
)";

int main(int argc, char * argv[])
{
	module<gles2_shader_type> m;
	m.from_memory(shader_program_code, 100);


	return 0;
}
