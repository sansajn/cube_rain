// how to transform model normals
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include "glmprint.hpp"

using std::cout;

using glm::vec4,
glm::vec3,
glm::mat4,
glm::translate,
glm::scale,
glm::inverseTranspose;

int main(int argc, char * argv[])
{
	vec4 v = vec4{0,1,0,1};
	mat4 M = translate(scale(mat4{1}, vec3{0.2f, 0.2f, 0.2f}), vec3{3,0,0});
	mat4 N = inverseTranspose(M);
	vec4 v_ = N*v;  // no longer unit vector
	cout << with_label("v'", v_) << "\n"
		<< "v_expected =\n" << vec4{0,1,0,1} << "\n";

	return 0;
}
