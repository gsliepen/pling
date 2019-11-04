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
sdl2_ttf = dependency('SDL2_ttf')
threads = dependency('threads')

subdir('src')
