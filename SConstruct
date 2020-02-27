# libglfw3-dev (3.2.1, ubuntu 18.04)

cpp17 = Environment(
	CCFLAGS=['-std=c++17', '-Wall', '-O0', '-g'],
	CPPDEFINES=['IMGUI_IMPL_OPENGL_ES3'],
	CPPPATH=['.', 'imgui/', 'imgui/examples/'])

cpp17.ParseConfig('pkg-config --cflags --libs glfw3 glew gl')

imgui = cpp17.StaticLibrary([
	Glob('imgui/*.cpp'),
	'imgui/examples/imgui_impl_glfw.cpp',  # backend for GLFW
	'imgui/examples/imgui_impl_opengl3.cpp',  # backend for opengl es3
])

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

cpp17.Program(['cube_rain.cpp', glt, objs, phys, imgui])

# helpers
cpp17.Program(['normals.cpp', phys])
cpp17.Program(['plane_normals.cpp', phys])
cpp17.Program(['cube_normals.cpp', glt, phys, objs])
cpp17.Program(['left_vs_right_hand_plane.cpp', phys])
cpp17.Program(['test_module.cpp', glt])
cpp17.Program(['test_program.cpp', glt, phys])
cpp17.Program(['test_flat_shaded_shader.cpp', glt, phys, objs])
cpp17.Program(['test_mouse_move.cpp'])
cpp17.Program(['test_transform_normals.cpp'])
