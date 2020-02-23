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

using phys::mat4,
	phys::mat3,
	phys::vec3,
	phys::Projection,
	phys::Translation,
	phys::Scale,
	phys::YRotation,
	phys::Inverse;
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
uniform vec3 light_direction;  // from surface to light
varying vec3 n;
void main() {
	gl_FragColor = vec4(max(dot(n, light_direction), 0.2) * color, 1.0);
})";

// 2 triangles
constexpr float xz_plane_verts[] = {
	// triangle 1
	-1.f, 0.f, -1.f,
	1.f, 0.f, -1.f,
	1.f, 0.f, 1.f,
	
	// triangle 2
	1.f, 0.f, 1.f,
	-1.f, 0.f, 1.f,
	-1.f, 0.f, -1.f
};

// 12 triangles
constexpr float cube_verts[] = {
	-1.f, -1.f, -1.f,  // 1
	1.f, -1.f, -1.f,
	1.f, 1.f, -1.f,
	
	1.f, 1.f, -1.f,  // 2
	-1.f, 1.f, -1.f,
	-1.f, -1.f, -1.f,
	
	1.f, -1.f, -1.f,  // 3
	1.f, -1.f, 1.f,
	1.f, 1.f, 1.f,
	
	1.f, 1.f, 1.f,  // 4
	1.f, 1.f, -1.f,
	1.f, -1.f, -1.f,
	
	-1.f, -1.f, 1.f,  // 5
	-1.f, 1.f, 1.f,
	1.f, 1.f, 1.f,
	
	1.f, 1.f, 1.f,  // 6
	1.f, -1.f, 1.f,
	-1.f, -1.f, 1.f,
		
	-1.f, -1.f, -1.f,  // 7
	-1.f, 1.f, -1.f,
	-1.f, 1.f, 1.f,
	
	-1.f, 1.f, 1.f,  // 8
	-1.f, -1.f, 1.f,
	-1.f, -1.f, -1.f,
	
	-1.f, -1.f, -1.f,  // 9
	-1.f, -1.f, 1.f,
	1.f, -1.f, 1.f,
	
	1.f, -1.f, 1.f,  // 10
	1.f, -1.f, -1.f,
	-1.f, -1.f, -1.f,
	
	-1.f, 1.f, -1.f,  // 11
	1.f, 1.f, -1.f,
	1.f, 1.f, 1.f,
	
	1.f, 1.f, 1.f,  // 12
	-1.f, 1.f, 1.f,
	-1.f, 1.f, -1.f
};

GLuint push_cube();
GLuint push_xz_plane();
void draw(GLuint position_vbo, GLuint normal_vbo, GLint position_loc, GLint normal_loc, size_t triangle_count);
GLuint push_data(void const * data, size_t size_in_bytes);
void calc_triangle_normals(float const * positions, size_t triangle_count, float * normals);

template <typename T>
void push(T const & val, GLint loc);

template <>
void push<mat3>(mat3 const & val, GLint loc)
{
	glUniformMatrix3fv(loc, 1, GL_FALSE, &val.asArray[0]);
}

template <>
void push<mat4>(mat4 const & val, GLint loc)
{
	glUniformMatrix4fv(loc, 1, GL_FALSE, &val.asArray[0]);
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
	
	vec3 cube_color = vec3{1,0,0};
	push(cube_color, color_loc);
	
	// light
	vec3 const light_position = vec3{10, 10, -10},
		light_direction = Normalized(light_position);
	push(light_direction, light_direction_loc);
	
	OrbitCamera cam;
	cam.Perspective(60, WIDTH/(float)HEIGHT, 0.01f, 1000.0f);
	cam.SetTarget(vec3{0,0,0});
	cam.SetZoom(10);
	cam.Update(0);
	
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	// positions
	GLuint cube_position_vbo = push_cube();
	
	// prepare normal data
	GLfloat normals[12*3*3];  // for 12 triangles
	calc_triangle_normals(cube_verts, 12, normals);
	GLuint cube_normal_vbo = push_data(normals, sizeof(normals));
		
	steady_clock::time_point last_tp = steady_clock::now();
	float cube_angle = 0;
	
	vector<vec3> positions(100);
	for (vec3 & pos : positions)
		pos = random_cube_position();
	
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
// 		cube_angle += angular_velocity * dt;  // deg
		mat4 M = YRotation(cube_angle);
		
		mat4 local_to_screen = M * world_to_screen;
		push(local_to_screen, local_to_screen_loc);
		
		mat3 normal_to_world = Transpose(Inverse(mat3{
			M._11, M._12, M._13,
			M._21, M._22, M._23,
			M._31, M._32, M._33}));
		push(normal_to_world, normal_to_world_loc);
		
		draw(cube_position_vbo, cube_normal_vbo, position_loc, normal_loc, 12);
		
		// draw falling cubes
		for (vec3 & pos : positions)
		{
			constexpr float fall_speed = 3.f;
			pos.y -= fall_speed * dt;
			
			// reuse fallen cubes
			if (pos.y < -10.f)
			{
				pos = random_cube_position();
				continue;
			}
		
			mat4 M = Scale(0.2f, 0.2f, 0.2f) * Translate(pos);
			local_to_screen = M * world_to_screen;
			push(local_to_screen, local_to_screen_loc);
			
			normal_to_world = Transpose(Inverse(mat3{
				M._11, M._12, M._13,
				M._21, M._22, M._23,
				M._31, M._32, M._33}));
			push(normal_to_world, normal_to_world_loc);
		
			draw(cube_position_vbo, cube_normal_vbo, position_loc, normal_loc, 12);
		}
		
		glfwSwapBuffers(window);
		
		std::this_thread::sleep_for(10ms);
	}
	
	glDeleteBuffers(1, &cube_position_vbo);
	glDeleteBuffers(1, &cube_normal_vbo);
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
	return push_data(cube_verts, sizeof(cube_verts));
}

GLuint push_xz_plane()
{
	return push_data(xz_plane_verts, sizeof(xz_plane_verts));
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
		
// 		cout << "n=" << n << "\n";
	}	
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
