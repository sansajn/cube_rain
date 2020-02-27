// flat shader program test
#include <thread>
#include <chrono>
#include <random>
#include <iostream>
#include <vector>
#include <cassert>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "glt/program.hpp"
#include "glt/gles2.hpp"
#include "phys/matrices.h"
#include "phys/Camera.h"
#include "flat_shaded_shader.hpp"

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
void draw(GLuint position_vbo, GLuint normal_vbo, GLint position_loc, GLint normal_loc, size_t triangle_count);
GLuint push_data(void const * data, size_t size_in_bytes);
void calc_triangle_normals(float const * positions, size_t triangle_count, float * normals);

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
	
	bool err = glewInit() != GLEW_OK;

	gles2::flat_shaded_shader shader_program;
	shader_program.use();

	vec3 plane_color = vec3{1,0,0};
	shader_program.model_color(plane_color);
		
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

	// prepare normal data
	GLfloat normals[2*3*3];  // for two triangles
	calc_triangle_normals(xy_plane_verts, 2, normals);
	GLuint normal_vbo = push_data(normals, sizeof(normals));
	
	steady_clock::time_point last_tp = steady_clock::now();

	mat4 world_to_screen = cam.GetViewMatrix() * cam.GetProjectionMatrix();
	shader_program.world_to_screen(world_to_screen);

	float light_angle = 0;
	
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		// dt
		steady_clock::time_point now = steady_clock::now();
		float dt = duration_cast<duration<float>>(now - last_tp).count();
		last_tp = now;
		
		// change light direction
		float const angular_velocity = DEG2RAD(30.f);  // rad/s
		light_angle += angular_velocity * dt;
		if (light_angle >= DEG2RAD(90.f))
			light_angle = 0.f;
		vec3 light_direction = Normalized(
			vec3{0, sinf(light_angle), -cosf(light_angle)});
		shader_program.light_direction(light_direction);

		// draw plane
		mat4 M = Scale(vec3{2, 2, 1});
		shader_program.local_to_world(M);
		
		draw(plane_vbo, normal_vbo, shader_program.position_location(),
			shader_program.normal_location(), 2);
		
		glfwSwapBuffers(window);
		
		std::this_thread::sleep_for(10ms);
	}
	
	glDeleteBuffers(1, &normal_vbo);
	glDeleteBuffers(1, &plane_vbo);
	glfwTerminate();
	
	return 0;
}

void calc_triangle_normals(float const * positions, size_t triangle_count, float * normals)
{
	for (size_t i = 0; i < triangle_count; ++i)
	{
		size_t idx = 3*3*i;
		vec3 v1{positions[idx], positions[idx+1], positions[idx+2]},
			v2{positions[idx+3], positions[idx+4], positions[idx+5]},
			v3{positions[idx+6], positions[idx+7], positions[idx+8]};

		vec3 n = Normalized(Cross(v3 - v1, v2 - v1));  // do oposite cross for left hand coordinate system

		GLfloat * p = &normals[idx];
		for (size_t j = 0; j < 3; ++j)  // for all three vertices
		{
			*p++ = n.x;
			*p++ = n.y;
			*p++ = n.z;
		}
	}
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

void draw(GLuint position_vbo, GLuint normal_vbo, GLint position_loc, GLint normal_loc, size_t triangle_count)
{
	glEnableVertexAttribArray(position_loc);
	glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
	glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	glEnableVertexAttribArray(normal_loc);
	glBindBuffer(GL_ARRAY_BUFFER, normal_vbo);
	glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	glDrawArrays(GL_TRIANGLES, 0, triangle_count * 3);
}
