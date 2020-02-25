// glfw mouse move test
#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

using std::cout,
	std::endl;
using glm::vec2;

constexpr GLuint WIDTH = 800,
	HEIGHT = 600;

vec2 g_cursor_position = vec2{0,0};

void cursor_position_handler(GLFWwindow * window, double xpos, double ypos)
{
	g_cursor_position = vec2{xpos, ypos};
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
	
	glfwSetCursorPosCallback(window, cursor_position_handler);

	cout << "GL_VENDOR: " << glGetString(GL_VENDOR) << "\n" 
		<< "GL_VERSION: " << glGetString(GL_VERSION) << "\n"
		<< "GL_RENDERER: " << glGetString(GL_RENDERER) << "\n"
		<< "GLSL_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
	
	vec2 prev_cursor_position = g_cursor_position;
	
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		
		vec2 mouse_move = g_cursor_position - prev_cursor_position;
		prev_cursor_position = g_cursor_position;
		
		if (mouse_move != vec2{0,0})
			cout << "(" << mouse_move.x << ", " << mouse_move.y << ") mouse move" << endl;
			
		glfwSwapBuffers(window);
		
	}
	
	glfwTerminate();
	
	return 0;
}
