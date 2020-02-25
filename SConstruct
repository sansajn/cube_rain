# libglfw3-dev (3.2.1, ubuntu 18.04)

cpp17 = Environment(
	CCFLAGS=['-std=c++17', '-Wall', '-O0', '-g'],
	CPPDEFINES=['GLFW_INCLUDE_ES2'])

cpp17.ParseConfig('pkg-config --cflags --libs glfw3 glesv2')

phys = cpp17.Object(Glob('phys/*.cpp'))

glt = cpp17.Object([
	'glt/module.cpp',
	'glt/io.cpp',
	'glt/gles2.cpp',
	'glt/program.cpp'
])

objs = cpp17.Object([
	'flat_shader.cpp',
	'flat_shaded_shader.cpp'
])

cpp17.Program(['cube_rain.cpp', glt, objs, phys])

# helpers
cpp17.Program(['normals.cpp', phys])
cpp17.Program(['plane_normals.cpp', phys])
cpp17.Program(['cube_normals.cpp', glt, phys, objs])
cpp17.Program(['left_vs_right_hand_plane.cpp', phys])
cpp17.Program(['test_module.cpp', glt])
cpp17.Program(['test_program.cpp', glt, phys])
cpp17.Program(['test_flat_shaded_shader.cpp', glt, phys, objs])
cpp17.Program(['test_mouse_move.cpp'])
