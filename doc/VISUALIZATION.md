# Basic idea

The screen should be split into a few regions that always show the same information (ie, selected program, timebase, etc, some buttons for actions/menus). This should cover only <= 20% of the screen.

The rest should be context-sensitive information. For example, when changing envelope parameters, a visual representation of the envelope should be visible.

Possible context-sensitive elements:

## Envelope

Show time on x-axis, log amplitude on y-axis.
Draw graph of the envelope. Show rulers for A, D, S and R parameters.
For simple tone generators or LFO modulation:

- A: time to reach 0 db from start of sound
- D: time to reach sustain level
- S: level of sustain
- R: time to reach -50 db from release of sound

For Karplus-Strong, control more physical parameters?


## LFO

Show time on x-axis, strength on y-axis.
Draw oscillation shape.
Parameters to visualize:

- frequency
- amplitude
- waveform
- phase?

## Filters

Show log frequency on x-axis, log amplitude on y-axis.
Draw bode plot for amplitude and phase.

Alternatively: show Nyquist or Nichols plots?


## Oscilloscope

Show time on the x-axis, amplitude on the y-axis.
Use the base frequency (or the base ramp generator?) to ensure the waveform is nicely centered.
If multiple keys are pressed, use the lowest frequency for sync.
Use rulers to show the start and end of one full cycle.
Keep the time scale constant. Fit a 100 Hz signal?

Selectable wet/dry signal?

## Spectrograph

Show log frequency on the x-axis, log amplitude on the y-axis. 50 dB range?
Use rulers to show the fundamentals of the currently active keys.
Use a good window function; Hanning or Blackman-Harris.

## Mixer

Show different channels on the x-axis, log amplitude on the y-axis.
While active, VU-meter inside each bar should show that channel's current sound.

