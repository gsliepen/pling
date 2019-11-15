# Idea

The main UI is NOT the screen, but the MIDI controllers themselves. As much as possible should be configurable via the controllers. Preferrably without the user needing the screen for feedback. We assume various types of controllers, and the possibility of mode switching functionality.

# Mode switching

Some controllers have a way to switch modes. For example, the LX88 has the mixer/instrument/preset modes. The SubZero minicontrol has 4 banks. These provide a good feedback to the user which mode is currently selected.

## Mixer mode

Sliders control volume, pots control panning, buttons control muting/solo.

## Instrument mode

Sliders control envelopes, pots control LFO rates and depths, filters, buttons control?

## Preset mode

???

# Types of controls

## Shift key

Either there is a dedicated shift-key, or there is a button that can become the designated shift-key. For sliders and pots, pressing shift could mean that the slider controls the LSB instead of the MSB of a value, thereby allowing more precise controls.

## Transport buttons

These control playing, recording, forward/rewind.

## Drum pads

For live performance, these would probably be better suited for quickly changing programs than as drum pads.

## Launch pads

Could be used as a sequencer for drums, as a clip launcher, as program selector.

## Operator chaining

For AM/RM/FM synthesis, buttons can be used to enable/disable and chain operators.

Single press: select if not selected, toggle enable
Hold A + press B: toggle link between A and B

# UI feedback

Screen shows context-sensitive feedback of what the user is doing. For example, decay rate slider is moved: display the envelope shape on-screen. LFO parameter is changed: show LFO waveform on-screen.
