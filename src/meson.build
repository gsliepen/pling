imgui_incdir = include_directories('imgui')

configure_file(
	output: 'config.h',
	configuration: config_data
)

executable('pling',
	'clock.cpp',
	'config.cpp',
	'controller.cpp',
	'curves/keyboard-scaling-dx7.cpp',
	'curves/velocity-scaling-dx7.cpp',
	'envelopes/exponential-adsr.cpp',
	'envelopes/exponential-dx7.cpp',
	'filters/state-variable.cpp',
	'imgui/imgui.cpp',
	'imgui/imgui_draw.cpp',
	'imgui/imgui_tables.cpp',
	'imgui/imgui_widgets.cpp',
	'imgui/backends/imgui_impl_opengl3.cpp',
	'imgui/backends/imgui_impl_sdl.cpp',
	'learn.cpp',
	'midi.cpp',
	'pling.cpp',
	'program-manager.cpp',
	'programs/karplus-strong.cpp',
	'programs/octalope.cpp',
	'programs/simple.cpp',
	'shader.cpp',
	'state.cpp',
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
		stdcppfs,
		threads,
		yaml_cpp,
	],
	cpp_args: [
		'-DGL_GLEXT_PROTOTYPES=1',
		'-DIMGUI_IMPL_OPENGL_ES2',
		'-DIMGUI_IMPL_OPENGL_LOADER_CUSTOM',
	],
	include_directories: [
		imgui_incdir,
        ],
	install: true
)
