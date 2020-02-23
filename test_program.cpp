// plane with normals and lighting
#include <thread>
#include <chrono>
#include <random>
#include <iostream>
#include <vector>
#include <cassert>
#include <GLFW/glfw3.h>
#include "phys/matrices.h"
#include "phys/Camera.h"
#include "glt/program.hpp"
#include "glt/gles2.hpp"

using std::string;
using std::vector;
using std::chrono::steady_clock,
	std::chrono::duration,
	std::chrono::duration_cast;
using std::random_device,
	std::default_random_engine;
using std::cout,
	std::endl;
using namespace std::chrono_literals;

using phys::vec3,
	phys::Normalized,
	phys::Cross;
using phys::mat4,
	phys::mat3,
	phys::Projection, 
	phys::Translation,
	phys::Scale,
	phys::YRotation,
	phys::Inverse,
	phys::Transpose;
using phys::DEG2RAD, phys::RAD2DEG;
using phys::OrbitCamera;

constexpr GLuint WIDTH = 800,
	HEIGHT = 600;
	
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

constexpr float xy_plane_verts[] = {
	// triangle 1
	-1.f, -1.f, 0.f,
	1.f, -1.f, 0.f,
	1.f, 1.f, 0.f,
	
	// triangle 2
	1.f, 1.f, 0.f,
	-1.f, 1.f, 0.f,
	-1.f, -1.f, 0.f
};

GLuint push_xy_plane();
void draw(GLuint position_vbo, GLint position_loc, size_t triangle_count);
GLuint push_data(void const * data, size_t size_in_bytes);

namespace glt::shader {

template <>
void set_uniform<mat4>(GLint loc, mat4 const & val)
{
	glUniformMatrix4fv(loc, 1, GL_FALSE, &val.asArray[0]);
}

template <>
void set_uniform<mat3>(GLint loc, mat3 const & val)
{
	glUniformMatrix3fv(loc, 1, GL_FALSE, &val.asArray[0]);
}

template <>
void set_uniform<vec3>(GLint loc, vec3 const & val)
{
	glUniform3fv(loc, 1, &val.asArray[0]);
}

}  // gtl::shader

int main(int argc, char * argv[]) 
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
	
	GLFWwindow * window = glfwCreateWindow(WIDTH, HEIGHT, __FILE__, NULL, NULL);
	assert(window);
	glfwMakeContextCurrent(window);

	cout << "GL_VENDOR: " << glGetString(GL_VENDOR) << "\n" 
		<< "GL_VERSION: " << glGetString(GL_VERSION) << "\n"
		<< "GL_RENDERER: " << glGetString(GL_RENDERER) << "\n"
		<< "GLSL_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	
	using program = glt::shader::program<glt::shader::module<glt::shader::gles2_shader_type>>;
	program shader_program;
	shader_program.from_memory(shader_program_code, 100);
	shader_program.use();

	GLint position_loc = shader_program.attribute_location("position");
	program::uniform_type local_to_screen_u = shader_program.uniform_variable("local_to_screen"),
		color_u = shader_program.uniform_variable("color");

	vec3 plane_color = vec3{1,0,0};
	color_u = plane_color;
		
	OrbitCamera cam;
	cam.Perspective(60, WIDTH/(float)HEIGHT, 0.01f, 1000.0f);
	cam.SetTarget(vec3{0,0,0});
	cam.SetZoom(10);
	cam.Update(0);
	
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	// positions
	GLuint plane_vbo = push_xy_plane();
	
	steady_clock::time_point last_tp = steady_clock::now();
	
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		mat4 world_to_screen = cam.GetViewMatrix() * cam.GetProjectionMatrix();

		// dt
		steady_clock::time_point now = steady_clock::now();
		float dt = duration_cast<duration<float>>(now - last_tp).count();
		last_tp = now;
		
		// draw plane
		mat4 M = Scale(vec3{2, 2, 1});
		mat4 local_to_screen = M * world_to_screen;
		local_to_screen_u = local_to_screen;
		
		draw(plane_vbo, position_loc, 2);
		
		glfwSwapBuffers(window);
		
		std::this_thread::sleep_for(10ms);
	}
	
	glDeleteBuffers(1, &plane_vbo);
	glfwTerminate();
	
	return 0;
}

GLuint push_data(void const * data, size_t size_in_bytes)
{
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, size_in_bytes, data, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind
	return vbo;
}

GLuint push_xy_plane()
{
	return push_data(xy_plane_verts, sizeof(xy_plane_verts));
}

void draw(GLuint position_vbo, GLint position_loc, size_t triangle_count)
{
	glEnableVertexAttribArray(position_loc);
	glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
	glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	
	glDrawArrays(GL_TRIANGLES, 0, triangle_count * 3);
}
