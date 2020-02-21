# libglfw3-dev (3.2.1, ubuntu 18.04)

cpp17 = Environment(
	CCFLAGS=['-std=c++17', '-Wall', '-O0', '-g'],
	CPPDEFINES=['GLFW_INCLUDE_ES2'])

cpp17.ParseConfig('pkg-config --cflags --libs glfw3 glesv2')

phys = cpp17.Object(Glob('phys/*.cpp'))

cpp17.Program(['cube_rain.cpp', phys])
cpp17.Program(['normals.cpp', phys])
cpp17.Program(['plane_normals.cpp', phys])
cpp17.Program(['cube_normals.cpp', phys])
cpp17.Program(['left_vs_right_hand_plane.cpp', phys])
