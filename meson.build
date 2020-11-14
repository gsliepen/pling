project('Pling', 'cpp',
	version: '0.1',
	license: 'LGPL3+',
	default_options: [
		'cpp_std=c++17',
	]
)

cpp = meson.get_compiler('cpp')

alsa = dependency('alsa')
fftw3f = dependency('fftw3f')
fmtlib = dependency('fmt')
gl = dependency('gl')
glm = dependency('glm')
sdl2 = dependency('SDL2')
threads = dependency('threads')
yaml_cpp = dependency('yaml-cpp')

config_data = configuration_data()
config_data.set_quoted('DATADIR', get_option('prefix') / get_option('datadir'))
config_data.set_quoted('VERSION', meson.project_version())

subdir('src')
