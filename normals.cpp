// row major matrices xy plane sample with glew3 and OpenGL ES 2.0
#include <thread>
#include <chrono>
#include <random>
#include <iostream>
#include <vector>
#include <cassert>
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
using phys::OrbitCamera;

constexpr GLuint WIDTH = 800,
	HEIGHT = 600;

GLchar const * vertex_shader_source = R"(
#version 100
attribute vec3 position;
attribute vec3 normal;
uniform mat4 local_to_screen;
uniform mat3 normal_to_world;
varying vec3 n;  // TODO: ujasni si co sa v podobe n dostane do fragment shaderu
void main() {
	n = normal_to_world * normal;
	gl_Position = local_to_screen * vec4(position, 1.0);
})";

GLchar const * fragment_shader_source = R"(
#version 100
precision mediump float;
uniform vec3 color;
uniform vec3 light_direction;
varying vec3 n;
void main() {
	gl_FragColor = vec4(max(dot(n, light_direction), 0.2) * color, 1.0);
})";

// Our vertices. Three consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
// A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices
static GLfloat const cube_verts[] = {
	-1.0f,-1.0f,-1.0f, // triangle 1 : begin
	-1.0f,-1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f, // triangle 1 : end
	1.0f, 1.0f,-1.0f, // triangle 2 : begin
	-1.0f,-1.0f,-1.0f,
	-1.0f, 1.0f,-1.0f, // triangle 2 : end
	1.0f,-1.0f, 1.0f,
	-1.0f,-1.0f,-1.0f,
	1.0f,-1.0f,-1.0f,
	1.0f, 1.0f,-1.0f,
	1.0f,-1.0f,-1.0f,
	-1.0f,-1.0f,-1.0f,
	-1.0f,-1.0f,-1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f,-1.0f,
	1.0f,-1.0f, 1.0f,
	-1.0f,-1.0f, 1.0f,
	-1.0f,-1.0f,-1.0f,
	-1.0f, 1.0f, 1.0f,
	-1.0f,-1.0f, 1.0f,
	1.0f,-1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f,-1.0f,-1.0f,
	1.0f, 1.0f,-1.0f,
	1.0f,-1.0f,-1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f,-1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	1.0f, 1.0f,-1.0f,
	-1.0f, 1.0f,-1.0f,
	1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f,-1.0f,
	-1.0f, 1.0f, 1.0f,
	1.0f, 1.0f, 1.0f,
	-1.0f, 1.0f, 1.0f,
	1.0f,-1.0f, 1.0f
};

GLuint push_cube();
void draw_cube(GLuint position_vbo, GLuint normal_vbo, GLint position_loc, GLint normal_loc);

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

vec3 random_cube_position();

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
	
	GLuint shader_program = get_shader_program(vertex_shader_source, fragment_shader_source);
	glUseProgram(shader_program);
	GLint position_loc = glGetAttribLocation(shader_program, "position"),
		normal_loc = glGetAttribLocation(shader_program, "normal"),
		local_to_screen_loc = glGetUniformLocation(shader_program, "local_to_screen"),
		normal_to_world_loc = glGetUniformLocation(shader_program, "normal_to_world"),
		color_loc = glGetUniformLocation(shader_program, "color"),
		light_direction_loc = glGetUniformLocation(shader_program, "light_direction");
	
	vec3 cube_color = vec3{1,0,0},
		light_direction = Normalized(vec3{1,-1,3});
	
	push(cube_color, color_loc);
	push(light_direction, light_direction_loc);
		
	OrbitCamera cam;
	cam.Perspective(60, WIDTH/(float)HEIGHT, 0.01f, 1000.0f);
	cam.SetTarget(vec3{0,0,0});
	cam.SetZoom(10);
	cam.Update(0);
	
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	GLuint cube_vbo = push_cube();
	
	// prepare normal data
	GLfloat normals[36 * 3];
	
	// for each triangle
	for (size_t i = 0; i < 12 /*36/3*/; ++i)
	{
		size_t idx = 3*3*i;
		vec3 v1{cube_verts[idx], cube_verts[idx+1], cube_verts[idx+2]},
			v2{cube_verts[idx+3], cube_verts[idx+4], cube_verts[idx+5]},
			v3{cube_verts[idx+6], cube_verts[idx+7], cube_verts[idx+8]};
			
		vec3 n = Normalized(Cross(v2 - v1, v3 - v1));
		
		GLfloat * n_ptr = &normals[idx];
		for (size_t j = 0; j < 3; ++j)  // for all three vertices
		{
			*n_ptr++ = n.x;
			*n_ptr++ = n.y;
			*n_ptr++ = n.z;
		}
	}
	
	// push normals to GPU
	GLuint normal_vbo;
	glGenBuffers(1, &normal_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, normal_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(normals), normals, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);  // unbind
		
	steady_clock::time_point last_tp = steady_clock::now();
	float cube_angle = 0;
	
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		mat4 world_to_screen = cam.GetViewMatrix() * cam.GetProjectionMatrix();

		// dt
		steady_clock::time_point now = steady_clock::now();
		float dt = duration_cast<duration<float>>(now - last_tp).count();
		last_tp = now;
		
		// draw cube
		constexpr float angular_velocity = 360/5.f;
		cube_angle += angular_velocity * dt;  // deg
// 		mat4 M_cube = YRotation(cube_angle);
		mat4 M_cube;
		mat4 local_to_screen = M_cube * world_to_screen;
		push(local_to_screen, local_to_screen_loc);
		
// 		mat3 normal_to_world = Transpose(Inverse(mat3{
// 			M_cube._11, M_cube._12, M_cube._13,
// 			M_cube._21, M_cube._22, M_cube._23,
// 			M_cube._31, M_cube._32, M_cube._33}));
		mat3 normal_to_world;
		push(normal_to_world, normal_to_world_loc);
		
		draw_cube(cube_vbo, normal_vbo, position_loc, normal_loc);
		
		glfwSwapBuffers(window);
		
		std::this_thread::sleep_for(10ms);
	}
	
	glDeleteBuffers(1, &cube_vbo);
	glfwTerminate();
	glDeleteProgram(shader_program);
	
	return 0;
}

vec3 random_cube_position()
{
	static random_device rd;
	static default_random_engine rand{rd()};
	
	return vec3{
		1 + (rand() % 10) - 5.f, 
		10.f + (rand() % 20), 
		1 + (rand() % 10) - 5.f};
}

GLuint push_cube()
{	
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_verts), cube_verts, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	
	return vbo;
}

void draw_cube(GLuint position_vbo, GLuint normal_vbo, GLint position_loc, GLint normal_loc)
{
	glEnableVertexAttribArray(position_loc);
	glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
	glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	
	glEnableVertexAttribArray(normal_loc);
	glBindBuffer(GL_ARRAY_BUFFER, normal_vbo);
	glVertexAttribPointer(normal_loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	
	glDrawArrays(GL_TRIANGLES, 0, 36);
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
