# Effecient sound generation

Signal generators write to buffers.
Buffer size is at least the size of a single audio buffer.

Signal generators might use other buffers as input (ie, for FM or LFO purposes).

Multiple signal generators can be combined to form one voice.
One voice ~= one key pressed.

Voices are mixed into a channel output buffer.
Channels can further be mixed into a global output buffer.

Final output is all active channels mixed, final filters, compressed.

Memory usage:
- ~0.5 kiB per signal generator
- ~1 kiB per voice
- ~1 kiB per channel

Some effects, such as echo, might require much larger buffers to hold multiple seconds of audio data. Could be up to 1 MiB.
If soundfonts are used, these will take up the size of all the samples contained, could be up to 1 GiB?
Waveguide synthesis might require a buffer per key, with a buffer size depending on the key frequency. Could be ~100 kiB for 88 keys for simple waveguides.

Memory bandwidth:
- 384 kiB/s write per mono buffer (assuming cache is used for readback)

CPU usage:
- 8 FLOP per sample per signal generator
- 32 FLOP per sample per channel

32 voice polyphony with 8 signal generators a piece, spread over 5 channels:
- 5 MiB
- 106 MiB/s bandwidth
- 202 FLOPs

# MIDI command routing

The MIDI manager is responsible for interpreting MIDI commands, and routing them to the right destination.
Normally, note-on/off commands have to be routed to the programs that produce that actual sound.
We want to support 16 channels per port, and possibly have multiple programs active for a given channel, since we might do keyboard splitting/layering.

For knobs/buttons/faders, we want to do the appropriate context-sensitive action.
Whenever an active program is selected on a port, we query the controls this program exports,
such as ADSR envelopes, filter controls, oscillators and so on.
Then we map MIDI CC messages for these knobs/buttons/faders to the exposed controls.
This both triggers an update of the on-screen representation of that control (for example, the envelope shape),
and an update of the program's parameters.

Certain buttons/knobs/faders might not be directly associated with a program.
For example, transport buttons are routed to the sequencer,
master volume goes to the master volume control.
Certain buttons might change the way other buttons are interpreted, for example a shift-modifier, or bank selection buttons.
Some buttons might change the active program.

## Programs

A MIDI program maps to a Program object that can render sound, and default
settings for that Program object.

## Program (de)activation

- Set Port active program to 1.
  - Create a new Program instance, put it in programs[0], mark as player_active, return pointer.
- Play some notes.
  - Sent to Program 1.
    - Sets audio_active.
- Change Port active program to 2.
  - Program 1 notes off? But still might still be in use by audio thread. Unmark player_active.
  - Create a new Program instance, put it in programs[1], return pointer.
- All voices in Program 1 are off.
  - Unsets audio_active.
    - Garbage collect?

## Note-on

Activate a voice, attack timer starts

## Note-off

Release timer starts, once envelope -50 dB is hit, voice is deactivated.

# Application structure and libraries

Use SDL2 for rendering graphics and for audio output. The latter because it can
use ALSA directly, with arbitrarily small buffers to ensure low latency.

Use ALSA for raw MIDI device detection and input. It's the only thing that
allows proper enumeration of the Nektar Impulse MIDI ports.

Render text using SDL2's TTF-to-texture routines, with a small wrapper to cache
texts that don't change.

# Timeline

We want to support both real-time sound rendering and as fast as possible rendering. Basically the sequence for rendering a single chunk of audio follow this sequence:

1. handle MIDI input so far
2. render audio chunk

To support rendering audio on multiple cores, it should look like:

1. handle MIDI input so far
2. `pthread_cond_broadcast()` worker threads
3. worker threads render audio
4. `pthread_barrier_wait()` for worker threads to finish
5. downsample per-core audio buffer to output chunk

# Rendering loop

Temp chunk of ~128 samples = 512 floats

Render global LFOs
For each active program:
  Render LFOs?
  For each active voice:
    Render sound and add it to temp chunkls
  Render program effects?
Render global effects

# Mapping stuff

Assume the following devices are connected:
- two (small) keyboards, each capable of program change
- one button/fader/knob controller with transport controls
- one pad controller with transport controls
- one 8x8 launchpad

Some controls are global, for example the transport controls. If there is a slider designated to global volume level, then that is global as well of course.

We want the keyboards to have different active programs. For example, one for bass, one melody.
Also, if pads are used as drumpads, then that is its own program as well.

We need a way to associate button/fader/knob instrument controls to an active program.

## Each MIDI controller has its own set of active programs

Allow multiple keyboard being connected simultaneously.
Each keyboard has its own programs. Even if set to the same program number, they will have distinct instances of Program.
Support 16 MIDI channels per controller, each with its own program. It's a lot of state to maintain, but most of it is inactive anyway.
If we know a controller can only send on one channel at a time (ie, no layer/split etc), we can optimize and treat it as "omni" mode, and merge all channels together.

## Each track records one audio stream, and all MIDI played on all devices

Although we might make this more flexible in the future, for now just treat recording to a track as recording everything currently being played.

## Use an internal MIDI mapping that is independent of what controllers send

We want to be able to record MIDI from controllers, and later to replay and even change that,
but perhaps with different controllers connected. We also want to be able to handle the user changing the mapping on their MIDI controllers.
So for example:

- User turns a knob that sends a MIDI CC 42 message
- This is mapped to filter cutoff frequency
- Internal MIDI mapping for filter cutoff is CC 74

However, we are probably going to support a lot of controls, and we want to have some consistency between them. So we want to use NRPNs to be able to control internal parameters.
General MIDI CC messages we do want to support are those that affect global parameters:

- bank select
- sustain / sostenuto / soft pedals
- mod wheel
- breath controller
- portamento time
- volume
- balance / pan
- expression?

There is no 1-to-1 mapping between controller messages and internal MIDI messages.
When a user moves a fader, the effect of that fader, and thus what internal MIDI message it results in,
depends on context such as modifier buttons pressed and which bank is active, *and* on the active program.

There is a 1-to-1 mapping between internal NRPN parameters and the controls of the active program,
even though the resulting effect of such a control might depend on the state of other controls.

# GUI controls

While the goal of this project is to provide a way to use a MIDI controller without needing to interact with a computer with a mouse and keyboard,
there might be functions which just cannot be triggered from a MIDI controller, or it can be that the MIDI controller(s) available do not have enough controls to fully control Pling.
So we envision a number of functions that can be selected using a mouse or touchscreen from the main screen of Pling:

- Learn
- Program select
- Load/Save
- Panic
- Controls
- Transport
- others?

## Learn

This is meant to learn MIDI controls and map them to Pling functionality.
When pressing it, the learning screen should be shown.
Touching any key/button/pad/fader/knob etc. on a controller should show the MIDI message received,
the currently assigned functionality to that control,
and allows a way to reassign it to other functions.

Learning should be a bit intelligent, so if it sees a CC going to 127 and then
back to 0 without any other value, it's most likely a button. If it goes to a
value inbetween but then only ever immediately back to 0, it's most likely a
velocity-sensitive pad. If it has varying values between 1 and 126 without
going to 0, it's most likely a fader or knob.

If a CC is mapped to generic knob or fader 1, then the next control touched should, if it isn't already mapped, then trigger automatic mapping to the knob or fader 2, etc..

Some buttons should be assigned shift, bank select etc., that modify the way other controls are interpreted.

Automatically save the settings when exitting the learn screen, but also provide load/save functionality.

## Program select

Provide a way to browse the available programs, and assign it to the active device and channel.
If there are multiple MIDI controllers, allow assigning to each of them.
Split the screen into:

- Bank select
- Program select
- MIDI controller: shows available controller, pressing selects one
- MIDI channel: shows 16 MIDI channels, with program assigned to each of them, pressing select a channel

## Load/Save

Load and Save current settings into numbered presets. Or provide some on-screen keyboard as well?

## Panic

Stop all sound production immediately: all keys off, kill all active voices.

## Controls

Provide a visual representation of sliders, knobs, buttons normally used to control program parameters.

## Transport

Provide transport controls, track selection.

