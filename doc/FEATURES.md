# Feature list

This is a list of *planned* features that Pling will support out-of-the-box.

## Playing notes

The basics:

- Handle note-on/note-off messages, be velocity sensitive.
- Handle channel aftertouch, possibly polyphonic aftertouch as well.
- Handle pitch-bend.
- Handle sustain.
- Handle modulation wheel.

Handle these per channel per MIDI port.

## Global controls

- Main volume
- Transport controls (play, stop, pause, record, rewind, ff, loop, see recording controls)
- Time signature

## Instrument control

This requires a number of buttons, sliders and knobs to be present.

### Envelopes

- ADSR curves (4 sliders per curve), for various parameters:
  - Main amplitude envelope
  - Filter cutoff frequency
  - Pitch change
  - Pitch/amplitude modulation depth

- Multi-segment curves? (one slider and one knob for start value + duration of each segment).

### Filter control

- Filter type (low pass, high pass, bandpass, etc)
- Cutoff frequency
- Q/resonance
- Cutoff key follow

### Oscillator control

- Oscillator waveform (off, sine, square, triangle, saw, random, sample-and-hold)
- Modulation type (none, AM, RM, FM)
- Modulation source

### Waveguide control

- Attack shape
- Excitation (breath, bow)
- Decay rate (both on/off)
- Decay type (low pass, high pass, etc?)
- Nonlinearity

### Sample control

- ???

## Mixer control

Support 8 or more tracks:

### Track control

- Volume
- Pan
- Effect send
- Mute/Solo

### Effect control

- Effect type (filter, echo, reverb, chorus, ...)
- Effect strength
- Effect depth
- Effect time

## Recording controls

Record both MIDI and audio. Try not to overwrite old data if possible. Prefer
to keep MIDI over audio data.

### Position controls

- Forward/backward a bar
- Go to start/end of track
- Go to start of loop/last recorded section
- Set start/end of loop

### Playback

- Play currently enabled tracks

## Recording controls

- Record (overwrite/add?) to currently selected track
  - For example, play notes first time, fiddle with controls second time.
- Undo recording
- Quantize recording
- Bake selected tracks
- Downmix selected tracks

