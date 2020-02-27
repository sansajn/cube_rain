// right hand (opengl) vs left hand plane
#include <thread>
#include <chrono>
#include <random>
#include <iostream>
#include <vector>
#include <cassert>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "phys/matrices.h"
#include "phys/Camera.h"

using std::vector;
using std::chrono::steady_clock,
	std::chrono::duration,
	std::chrono::duration_cast;
using std::random_device,
	std::default_random_engine;
using std::cout, std::endl;
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
	
GLchar const * vertex_shader_source = R"(
#version 100
attribute vec3 position;
uniform mat4 local_to_screen;
void main() {
	gl_Position = local_to_screen * vec4(position, 1.0);
})";

GLchar const * fragment_shader_source = R"(
#version 100
precision mediump float;
uniform vec3 color;
void main() {
	gl_FragColor = vec4(color, 1.0);
})";

// one triangle
constexpr float right_hand_verts[] = {
	-1.f, 0.f, 1.f,
	1.f, 0.f, 1.f,
	0.f, 1.f, 0.f
};

constexpr float left_hand_verts[] = {
	-1.f, 0.f, -1.f,
	1.f, 0.f, -1.f,
	0.f, 1.f, 0.f
};

GLuint push_data(void const * data, size_t size_in_bytes);
void draw(GLuint position_vbo, GLint position_loc, size_t triangle_count);

template <typename T>
void push(T const & val, GLint loc);

template <>
void push<mat4>(mat4 const & val, GLint loc)
{
	glUniformMatrix4fv(loc, 1, GL_FALSE, &val.asArray[0]);
}

template <>
void push<mat3>(mat3 const & val, GLint loc)
{
	glUniformMatrix3fv(loc, 1, GL_FALSE, &val.asArray[0]);
}

template <>
void push<vec3>(vec3 const & val, GLint loc)
{
	glUniform3fv(loc, 1, &val.asArray[0]);
}

GLint get_shader_program(char const * vertex_shader_source, char const * fragment_shader_source);

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

	GLuint shader_program = get_shader_program(vertex_shader_source, fragment_shader_source);
	glUseProgram(shader_program);
	GLint position_loc = glGetAttribLocation(shader_program, "position"),
		local_to_screen_loc = glGetUniformLocation(shader_program, "local_to_screen"),
		color_loc = glGetUniformLocation(shader_program, "color");
	
	vec3 plane_color = vec3{1,0,0};
	push(plane_color, color_loc);
		
	OrbitCamera cam;
	cam.Perspective(60, WIDTH/(float)HEIGHT, 0.01f, 1000.0f);
	cam.SetTarget(vec3{0,0,0});
	cam.SetZoom(10);
	cam.Update(0);
	
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	// positions
	GLuint rh_plane_vbo = push_data(right_hand_verts, sizeof(right_hand_verts)),
		lh_plane_vbo = push_data(left_hand_verts, sizeof(left_hand_verts));
	
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
		
		// draw left hand plane
		mat4 M = Translation(vec3{-2, 0, 0});
		mat4 local_to_screen = M * world_to_screen;
		push(local_to_screen, local_to_screen_loc);

		draw(lh_plane_vbo, position_loc, 1);
		
		// draw right hand plant
		M = Translation(vec3{2, 0, 0});
		local_to_screen = M * world_to_screen;
		push(local_to_screen, local_to_screen_loc);
		
		draw(rh_plane_vbo, position_loc, 1);
		
		glfwSwapBuffers(window);
		
		std::this_thread::sleep_for(10ms);
	}
	
	glDeleteBuffers(1, &lh_plane_vbo);
	glDeleteBuffers(1, &rh_plane_vbo);
	glfwTerminate();
	glDeleteProgram(shader_program);
	
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

void draw(GLuint position_vbo, GLint position_loc, size_t triangle_count)
{
	glEnableVertexAttribArray(position_loc);
	glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
	glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	
	glDrawArrays(GL_TRIANGLES, 0, triangle_count * 3);
}

GLint get_shader_program(char const * vertex_shader_source, char const * fragment_shader_source)
{
	enum Consts {INFOLOG_LEN = 512};
	GLchar infoLog[INFOLOG_LEN];
	GLint fragment_shader;
	GLint shader_program;
	GLint success;
	GLint vertex_shader;

	/* Vertex shader */
	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(vertex_shader, INFOLOG_LEN, NULL, infoLog);
		cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" 
			<< infoLog << endl;
	}

	/* Fragment shader */
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderInfoLog(fragment_shader, INFOLOG_LEN, NULL, infoLog);
		cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" 
			<< infoLog << endl;
	}

	/* Link shaders */
	shader_program = glCreateProgram();
	glAttachShader(shader_program, vertex_shader);
	glAttachShader(shader_program, fragment_shader);
	glLinkProgram(shader_program);
	glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
	if (!success) {
		glGetProgramInfoLog(shader_program, INFOLOG_LEN, NULL, infoLog);
		cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" 
			<< infoLog << endl;
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	return shader_program;
}
