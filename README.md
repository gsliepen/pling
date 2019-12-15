# About

This project is intended to provide a easy and fun way to convert a MIDI
controller into a digital audio workstation, where (almost) all of the DAW
functionality will be accessible via the MIDI controller. It is intended to be
used with MIDI controllers that have lots of controls, such as:

* AKAI MPK
* Arturia Keylab
* Behringer MÖTOR
* M-Audio Axiom/Code/Oxygen
* Nektar Impulse
* Novation Impulse/Launchkey

Alternatively, it could be used using multiple smaller MIDI controllers, such
as the SubZero MINI series 3, or by mixing and matching together different
controllers.

The software is intended to run well on small single-board computers, such as
the Raspberry Pi, ASUS Tinkerboard, Odroid-XU4, and so on.

# Synthesis

The goal is to provide a variety of synthesis engines, each of which can be
controlled via the pots and sliders of the MIDI controller as much as possible.
Planned engine types are:

* Simple oscillators + filters
* FM synthesis
* Waveguide synthesis
* Soundfonts (SF2, SFZ)

The engines should be efficient so we can get a high degree of polyphony
even on small devices such as the Raspberry Pi.

The goal is not to reproduce the sound of existing synthesizers.

# Graphical User interface

A graphical user interface using OpenGL ES 2.0 will be available which will
mostly render what is going on, whereas control will be as much as possible via
the MIDI controllers. An oscilloscope and a view of the currently pressed keys
will be present at all times. Furthermore, context-dependent widgets will be
shown, such as an ADSR curve when sliders are used to modify envelope
parameters, or a Bode plot when filter parameters are changed.

The goal is to keep the display simple at all times, and not have hundreds of
variables and controls visible simultaneously. Furthermore, there should be no
skeuomorphisms.  Also, values should be shown as physical quantities with
proper units, instead of something that goes arbitrarily from 1 to 10.

# Implementation

The goal is to have a very CPU-efficient implementation of the synthesis
engines that can make use of multiple cores. Internally everything will be
rendered as chunks of 128 floats. This should be very L1-cache friendly, and
might also benefit from vectorization. At a sample rate of 48 kHz, this means a
latency of 2.6 ms, which should be acceptible.

SDL is used both for GUI rendering as well as audio output. The reason for this
is that it supports a wide variety of audio backends, including raw ALSA. It
also does not introduce any unnecessary latency itself. Since SDL does not
support MIDI, we do require the ALSA library for MIDI I/O.

OpenGL ES 2.0 is chosen for the GUI rendering, since it is supported on many
devices, including the Raspberry Pi. It also means a lot of rendering details
can be offloaded to shaders that run on the GPU. The GUI widgets are provided
by Dear ImGui, as this integrates very well with real-time OpenGL rendering.

# Dependencies

* ALSA
* C++17-compliant compiler
* Dear ImGui
* FFTW3
* fmtlib
* GLM
* Meson
* SDL2
* yaml-cpp

On some platforms, you might also need to install CMake, in order for Meson to be able to find some dependencies.
On Debian-based systems, you can install all the dependencies with the following command:

    sudo apt install build-essential libasound2-dev libfftw3-dev libfmt-dev libglm-dev meson libsdl2-dev cmake libyaml-cpp-dev

Dear ImGui is a submodule.  To ensure it is checked out, run the following
commands in Pling's root directory:

    git submodule init
    git submodule update

# Building

To build, run these commands in the root directory of the source code:

    meson build
    ninja -C build

To install the binaries on your system, run:

    ninja -C build install

The latter command might ask for a root password if it detects the installation
directory is not writable by the current user.

By default, a debug version will be built, and the resulting binaries will be
install inside `/usr/local/`. If you want to create a release build and install
it in `/usr/`, use the following commands:

    meson --buildtype=release --prefix=/usr build-release
    ninja -C build-release install
