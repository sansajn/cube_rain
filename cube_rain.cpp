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
#include "flat_shader.hpp"
#include "flat_shaded_shader.hpp"

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
	phys::vec2,
	phys::Projection,
	phys::Translation,
	phys::Scale,
	phys::YRotation,
	phys::Inverse;
using phys::DEG2RAD, phys::RAD2DEG;
using phys::OrbitCamera;

constexpr GLuint WIDTH = 800,
	HEIGHT = 600;

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
	-1.f, -1.f, -1.f,  // front
	1.f, -1.f, -1.f,
	1.f, 1.f, -1.f,

	1.f, 1.f, -1.f,
	-1.f, 1.f, -1.f,
	-1.f, -1.f, -1.f,

	1.f, -1.f, -1.f,  // right
	1.f, -1.f, 1.f,
	1.f, 1.f, 1.f,

	1.f, 1.f, 1.f,
	1.f, 1.f, -1.f,
	1.f, -1.f, -1.f,

	-1.f, -1.f, 1.f,  // back
	-1.f, 1.f, 1.f,
	1.f, 1.f, 1.f,

	1.f, 1.f, 1.f,
	1.f, -1.f, 1.f,
	-1.f, -1.f, 1.f,

	-1.f, -1.f, -1.f,  // left
	-1.f, 1.f, -1.f,
	-1.f, 1.f, 1.f,

	-1.f, 1.f, 1.f,
	-1.f, -1.f, 1.f,
	-1.f, -1.f, -1.f,

	-1.f, -1.f, -1.f,  // top
	-1.f, -1.f, 1.f,
	1.f, -1.f, 1.f,

	1.f, -1.f, 1.f,
	1.f, -1.f, -1.f,
	-1.f, -1.f, -1.f,

	-1.f, 1.f, -1.f,  // bottom
	1.f, 1.f, -1.f,
	1.f, 1.f, 1.f,

	1.f, 1.f, 1.f,
	-1.f, 1.f, 1.f,
	-1.f, 1.f, -1.f
};

// three lines
constexpr float axis_verts[] = {
	0,0,0, 1,0,0,  // x
	0,0,0, 0,1,0,  // y
	0,0,0, 0,0,1  // z
};

GLuint push_cube();
GLuint push_axes();
GLuint push_xz_plane();
void draw(GLuint position_vbo, GLuint normal_vbo, GLint position_loc, GLint normal_loc, size_t triangle_count);
void draw_triangles(GLuint position_vbo, GLint position_loc, size_t triangle_count);
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

vec3 random_cube_position();

void scroll_handler(GLFWwindow * window, double xoffset, double yoffset);
void mouse_button_handler(GLFWwindow * window, int button, int action, int mods);
void cursor_position_handler(GLFWwindow * window, double xpos, double ypos);
void key_handler(GLFWwindow * window, int key, int scancode, int action, int mods);

// user input helpers
float g_camera_zoom = 10;
bool g_rotate_camera = false,
	g_pan_camera = false;
vec2 g_cursor_move = vec2{0,0};  // normalized
bool g_animation = true;

class orbit_camera : public OrbitCamera
{
public:
	orbit_camera()
	{
		rotationSpeed = vec2{5, 5};
		panSpeed = vec2{5, 5};
	}
};

class axes_model
{
public:
	axes_model(GLuint axes_vbo);
	void draw(gles2::flat_shader & program, mat4 const & local_to_world);

private:
	GLuint _axes_vbo;
};

axes_model::axes_model(GLuint axes_vbo)
	: _axes_vbo{axes_vbo}
{}

void axes_model::draw(gles2::flat_shader & program, mat4 const & local_to_world)
{
	program.local_to_world(local_to_world);

	GLint position_loc = program.position_location();

	glEnableVertexAttribArray(position_loc);
	glBindBuffer(GL_ARRAY_BUFFER, _axes_vbo);

	// x
	program.model_color(vec3{1,0,0});
	glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glDrawArrays(GL_LINES, 0, 2);

	// y
	program.model_color(vec3{0,1,0});
	glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glDrawArrays(GL_LINES, 2, 2);

	// z
	program.model_color(vec3{0,0,1});
	glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);
	glDrawArrays(GL_LINES, 4, 2);
}


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
	
	glfwSetScrollCallback(window, scroll_handler);
	glfwSetMouseButtonCallback(window, mouse_button_handler);
	glfwSetCursorPosCallback(window, cursor_position_handler);
	glfwSetKeyCallback(window, key_handler);

	gles2::flat_shader flat;
	gles2::flat_shaded_shader shaded;

	vec3 cube_color = vec3{1,0,0},
		axis_color = vec3{1,0,0},
		light_source_color = vec3{1,1,0};
	
	// light
	vec3 const light_position = vec3{10, 10, -10},
		light_direction = Normalized(light_position);
	
	orbit_camera cam;
	cam.Perspective(60, WIDTH/(float)HEIGHT, 0.01f, 1000.0f);
	cam.SetTarget(vec3{0,0,0});
	
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	// positions
	GLuint cube_position_vbo = push_cube(),
		axes_position_vbo = push_axes();
	axes_model axes{axes_position_vbo};
	
	// prepare normal data
	GLfloat normals[12*3*3];  // for 12 triangles
	calc_triangle_normals(cube_verts, 12, normals);
	GLuint cube_normal_vbo = push_data(normals, sizeof(normals));
		
	steady_clock::time_point last_tp = steady_clock::now();
	
	vector<vec3> positions(100);
	for (vec3 & pos : positions)
		pos = random_cube_position();
	
	float light_angle = 0,
		cube_angle = 0;

	while (!glfwWindowShouldClose(window))
	{
		// input
		glfwPollEvents();

		// update

		// dt
		steady_clock::time_point now = steady_clock::now();
		float dt = duration_cast<duration<float>>(now - last_tp).count();
		last_tp = now;

		cam.SetZoom(g_camera_zoom);
		if (g_cursor_move != vec2{0,0})
		{
			if (g_rotate_camera)
				cam.Rotate(g_cursor_move, dt);
			else if (g_pan_camera)
				cam.Pan(g_cursor_move, dt);
		}

		cam.Update(dt);

		// draw

		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		
		mat4 world_to_screen = cam.GetViewMatrix() * cam.GetProjectionMatrix();

		// axis
		flat.use();
		flat.model_color(axis_color);
		flat.world_to_screen(world_to_screen);
		mat4 M_axes = Translation(vec3{0,0,0});
		axes.draw(flat, M_axes);

		// change light direction
		float const angular_velocity = DEG2RAD(30.f);  // rad/s
		light_angle += angular_velocity * dt;
		if (light_angle >= DEG2RAD(90.f))
			light_angle = 0.f;
		vec3 light_direction = Normalized(
			vec3{0, sinf(light_angle), -cosf(light_angle)});

		// light source
		constexpr float light_distance = 5.f;
		flat.model_color(light_source_color);
		mat4 M_light = Scale(vec3{0.1f, 0.1f, 0.1f}) * Translation(light_direction * light_distance);
		flat.local_to_world(M_light);
		draw_triangles(cube_position_vbo, flat.position_location(), 12);

		// draw cube
		shaded.use();
		shaded.model_color(cube_color);
		shaded.light_direction(light_direction);
		shaded.world_to_screen(world_to_screen);

		constexpr float cube_angular_velocity = 360/8.f;  // deg/s
		if (g_animation)
			cube_angle += cube_angular_velocity * dt;

		mat4 M_cube = YRotation(cube_angle) * Translation(vec3{3,0,0});
		shaded.local_to_world(M_cube);

		draw(cube_position_vbo, cube_normal_vbo, shaded.position_location(),
			shaded.normal_location(), 12);
		
		// draw falling cubes
		for (vec3 & pos : positions)
		{
			constexpr float fall_speed = 3.f;
			if (g_animation)
				pos.y -= fall_speed * dt;
			
			// reuse fallen cubes
			if (pos.y < -10.f)
			{
				pos = random_cube_position();
				continue;
			}
		
			mat4 M = Scale(0.2f, 0.2f, 0.2f) * Translate(pos);
			shaded.local_to_world(M);

			draw(cube_position_vbo, cube_normal_vbo, shaded.position_location(),
				shaded.normal_location(), 12);
		}
		
		glfwSwapBuffers(window);
		
		std::this_thread::sleep_for(10ms);
	}
	
	glDeleteBuffers(1, &cube_position_vbo);
	glDeleteBuffers(1, &cube_normal_vbo);
	glDeleteBuffers(1, &axes_position_vbo);
	glfwTerminate();
	
	return 0;
}

void scroll_handler(GLFWwindow * window, double xoffset, double yoffset)
{
	g_camera_zoom -= yoffset/4;
}

void mouse_button_handler(GLFWwindow * window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
		g_rotate_camera = action == GLFW_PRESS;
	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
		g_pan_camera = action == GLFW_PRESS;
}

void cursor_position_handler(GLFWwindow * window, double xpos, double ypos)
{
	static vec2 last_pos = vec2{xpos, ypos};
	if (g_rotate_camera || g_pan_camera)
		g_cursor_move = vec2{xpos, ypos} - last_pos;
	else
		g_cursor_move = vec2{0,0};

	last_pos = vec2{xpos, ypos};
}

void key_handler(GLFWwindow * window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_SPACE)
			g_animation = !g_animation;
	}
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

GLuint push_axes()
{
	return push_data(axis_verts, sizeof(axis_verts));
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

void draw_triangles(GLuint position_vbo, GLint position_loc, size_t triangle_count)
{
	glEnableVertexAttribArray(position_loc);
	glBindBuffer(GL_ARRAY_BUFFER, position_vbo);
	glVertexAttribPointer(position_loc, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid*)0);

	glDrawArrays(GL_TRIANGLES, 0, triangle_count * 3);
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
