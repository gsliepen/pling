# Graphical user interface design rules

## Don't show what is already shown elsewhere

For example, don't replicate the state of sliders and knobs of the MIDI
controller on the screen. The user can just look at the MIDI controller for
this information.

## Only show what is currently interesting to the user

Let's divide this into two parts.

### Things that are almost always interesting

Since we are producing sound, it's good to always show some things on the
screen to give immediate feedback about the sound we are producing. These are:

- Current amplitude of the sound, with clear indications for when we start limiting and/or clipping.
- Active program.
- Currently active keys or voices, to show that we are indeed responding to keypresses.
- Oscilloscope and/or spectrum.

### Things that depend on context

When we are changing envelope parameters, we want to show the shape of the
envelope. At that moment, the user is probably not interested in seeing other
things, except the effect the envelope has on the produced sound (so spectrum
is usually a good thing to keep visible). Also, in case multiple envelopes are
used by a program, it probably is a good idea to show the others as well in a
distinct way, so the user can compare the shapes, and quickly match up
envelopes if desired.

A list of context-dependent things to show:

- Envelopes
- Filter curves
- LFO shape
- Reverb parameters
- Chorus parameters
- Oscillator configuration
- ...

## Be consistent

### Scales

Use fixed scales where possible. For example, use +12..-48 dB range for
amplitudes everywhere we have an amplitude. This allows the user to easily
compare things, and without autoscaling, it is much easier to infer the
amplitude from the on-screen height of a curve. A list of scales used:

- Absolute amplitude (+12..-48 dB)
- Frequency (8..12500 Hz to match the full MIDI range with 12-TET tuning)
- Time (0..10 seconds)

### Tick marks

When drawing graphs, apart from using a consistent scale, we also want the tick
marks to be consistent.

- Absolute amplitude: power-of-two multiples/dividers of 12 dB
- Frequency: octaves of C
- Time (power-of-ten multiples/dividers of 1 second)

### Alignment

Whenever possible, align widgets so that their scales and tick marks line up.

### Position

Things should be layed out consistently on the screen, so that the user always
knows where on the screen the desired information will be shown.

### Colors

Use subtle background colors to hint what kind of information is shown:

- Green: waveform
- Blue: spectrum
- ...

Also, when showing multiple sets of data in one graph, use colors to distinguish them.
For example, use colors to distinguish between:

- Program
- Voice
- Oscillator
- Controller
- ...

Always try to ensure there is enough contrast so that even a color-blind person
can still use the interface.

## Show real units

Whenever some value represents time, frequency, amplitude and so on, display
the value in real units, and make sure the unit is shown as well. Use SI units
where appropriate. For example:

- Time in seconds (s, ms)
- Frequency in hertz (Hz, kHz)
- Amplitude in decibels (dB)
- ...

Never show something as going arbitrarily from 0 to 10, or in MIDI units 0..127.

## Be calm

Don't show moving or blinking things if it's not needed. In particular, if
nothing is playing, the screen should be static.
