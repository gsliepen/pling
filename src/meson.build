imgui_incdir = include_directories('imgui')

executable('pling',
	'envelopes/exponential-adsr.cpp',
	'imgui/imgui.cpp',
	'imgui/imgui_draw.cpp',
	'imgui/imgui_widgets.cpp',
	'imgui/examples/imgui_impl_opengl3.cpp',
	'imgui/examples/imgui_impl_sdl.cpp',
	'midi.cpp',
	'pling.cpp',
	'program-manager.cpp',
	'programs/simple.cpp',
	'shader.cpp',
	'ui.cpp',
	'widgets/oscilloscope.cpp',
	'widgets/spectrum.cpp',
	dependencies: [
		alsa,
		fftw3f,
		fmtlib,
		gl,
		glm,
		sdl2,
		threads,
	],
	cpp_args: [
		'-DGL_GLEXT_PROTOTYPES=1',
		'-DIMGUI_IMPL_OPENGL_ES2',
	],
	include_directories: [
		imgui_incdir,
        ],
	install: true
)
