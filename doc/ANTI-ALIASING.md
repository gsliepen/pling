# Introduction

With digital synthesis, we have the issue that we are digitizing the sound at a
fixed frequency, and this can cause aliasing. In particular, if an ideal
waveform is sampled, any frequency component of that waveform that is above the
Nyquist frequency (half of the sample frequency) will be mirrored back into the
frequency range below the Nyquist frequency. This is called aliasing, and it
can easily cause very audible artifacts, which we should try to avoid.

Below we will discuss several ways to limit the effects of aliasing, but first
we discuss some properties of waveforms to get a better understanding of the
problem.

# Pure waveforms

## Pure sine waves

Any pure sine wave that is below the Nyquist frequency, sounds pure even when
digitized.  The reason is that the difference between the ideal pure sine wave
and the *already digitized* version contains only frequency components that are
above the Nyquist frequency. Assuming that the Nyquist frequency is above the
maximum frequency the human ear can detect, this means the difference is
inaudible, and hence the digitized sine wave is perceived as a pure sine wave.

A pure sine wave that is above the Nyquist frequency, when digitized, will have
its frequency mirrored; that is, when the pure sine wave frequency is f = fN + df,
where fN is the Nyquist frequency, then the digitized waveform will be
identical to the digitized waveform of a pure sine wave with frequency fN - df,
and will thus also be perceived by the human ear as a sine wave with frequency
fN - df.

Sine waves can be simply added together linearly. This means that digitizing
the sum of two pure sine waves is identical to the sum of two individually
digitized pure sine waves. Also, the sum of two individually digitized pure
sine waves that are both below the Nyquist frequency is perceived by the human
ear as the two original pure sine waves.

## Other pure waveforms

Other waveforms, such as triangle, square and sawtooth waveforms, can be
constructed by summing pure sine waves of different frequency and amplitude.
For example, a pure square wave of frequency f is identical to the sum of all
pure sine waves with frequencies that are an odd multiple of f, with the
amplitude of each sine wave decreasing as the frequency increases. The formula
is:

```
square(t) = \sum_{n=1,3,5,...} sin(2 pi n t) / n
```

Other waveforms have different sets of frequency multipliers, and the sines can
have different amplitudes and/or phases. However, there are in general three main rules for these pure waveforms:

1. Each sine's frequency is an integer multiple of the base frequency.
2. Each sine's amplitude is 1 over the multiple of the base frequency.
3. The sum goes to infinity.

Calculating a square wave by summing to infinity is normally not what anyone
would do, in fact one can easily generate a pure square wave using the simple
formula:

```
square(t) = t mod 1/f < 0.5 ? 1 : -1
```

The point is that both formulas give an identical result. This means that when
we are *digitizing* a pure square wave, we are also digitizing an infinite
number of pure *sine waves*, most of which are above the Nyquist frequency, and
so they will alias. If their amplitude is large enough, these aliased sine
waves are then audible. The human ear can hear overtones that have an amplitude
of -40 dB compared to the base frequency component which is assumed to be at 0 dB.

# Anti-aliasing techniques

## Increasing the samplerate

One way to make aliasing less of a problem is by increasing the samplerate.
With most waveforms, the amplitude of frequency components halves with each
octave, or put in another way, it decreases by 6 dB per octave. The highest
note on a piano keyboard has a base frequency of about 4 kHz. This means that
at a samplerate of 48 kHz, there are only 3.6 octaves before hitting the
Nyquist frequency. This means the amplitude of the aliased frequency components
is -22 dB. However, to make these higher frequency components inaudible, they
should have an amplitude lower dan -40 dB.

By doubling the samplerate, we gain an extra octave before hitting the Nyquist
frequency. This means that by going to 96 kHz, the aliases frequency components
will have an amplitude of -28 dB. Going to 192 kHz, they will have reduced to
-34 dB; unfortunately that is still just above the level that human ears can
pick them up. One would have to go to 384 kHz to ensure a naively sampled
highest piano note does not contain any audible aliasing artifacts.

This technique is very simple, however going from 48 kHz to 384 kHz requires 8
times the processing power, as well as 8 times the disk storage when recording
the audio. Also, this technique assumes that the sound card can actually output
at 384 kHz, which is something most common sound cards do not support.

Note that this technique is not the same as oversampling, which equivalent to
rendering at a higher frequency, but then downsampling it before sending it to
the sound card. The downsampling will introduce its own aliasing artifacts if
done naively. Libraries such as libsamplerate implement sophisticated
techniques to do bandwidth-limited downsampling, however that is expensive
itself, so this would require even more processing power.

## Bandwidth limited waveforms

A pure square wave would have an infinite amount of frequency components which
are inaudible to the human ear.  Naive digitization of this square wave would
however cause these frequencies to alias, and become audible. One way to avoid
this, and to get a digital waveform that, to the ear, is indistinguishable from
a pure square wave, is to construct a square wave by summing sines, but only
sines with frequencies that are lower than the Nyquist frequency. Since there
are no higher frequencies, there is nothing that can alias. Coming back to the
formula for the square wave:

```
square(t) = \sum_{n=1,3,5,...} sin(2 pi n t) / n
```

The trick is to sum only up to `n = floor(fN / f)`. For the middle A, when
sampling at 48 kHz, this means n = 109, which is quite a large number, but
worse is that the number will double for every octave you go down. So this
technique is very expensive.

Another issue is that while this works well for generating an unmodulated
waveform of a fixed frequency, it doesn't take modulation into account. As
such, it is not a practical technique for synthesis engines using frequency
modulation, ring modulation or other non-linear techniques of combining
different waveforms.

## BLITs, BLEPs and BLAMPs

Instead of building up a waveform by summing sine waves, one can also construct
a waveform by summing impulse trains, step functions or ramp functions. An
impulse is just a signal that is zero everywhere, except at one point where it
has a non-zero value. An impulse train is simply a lot of impulses in a row. 
A step function is a function that is zero up to a certain point, and one after that point.
A ramp is a function that linearly increases.

The idea is that it is possible to make (or approximate) bandwidth limited
versions of these basic functions in a way that is computationally much more
efficient than summing pure sine waves. The desired waveform is then generated
by a combination of just a few BLITs, BLEPs and/or BLAMPs.

This technique is much more efficient than the previous ones, however they also
generally work best when the waveform has a fixed frequency and is not being
modulated.

## Polynomial bandwidth-limited step functions (PolyBLEPs)

This technique uses a polynomial approximation of a BLEP to anti-alias just the
sample near the step.  This is very fast, and does a decent job of reducing the
amplitude of aliased frequencies. (TODO: dB/octave?)

The drawback is again that once the waveform is modulated, the approximation
doesn't hold anymore.

## Derivative of integrated waveform

The main issue with naive sampling of a pure waveform is that the value chosen
for a sample is just the instantenous value at the sample time in the original
waveform. A better value for the sample would be to take the average value of
the original function for one sample period. It is easy to see that this would
reduce the amplitude of frequencies much higher than the Nyquist frequency very
well, but it is not perfect. It will also attenuate frequencies that are just a
bit smaller than the Nyquist frequency.

To implement this efficiently, we use the integral of the original waveform,
and sample it at time t and t + dt (where dt = 1 / f is the sample period). We
subtract the two, and divide the result by dt to get the average value of the
original waveform between t and dt.

It is possible to use higher-order integrals and derivatives to get better
anti-aliasing, at the cost of an increase in computational complexity.

The advantage of this method is that it works both with pure waveforms (square,
triangle and sawtooth), since those can easily be integrated, as well as
wavetables. It also works rather well when the waveform is being modulated.
(TODO: dB/octave?)

# Summary

TODO: table of different techniques, including naive sampling, with dB
suppression of strongest aliased frequency component of a 440 kHz and a 4 kHz
note at 48 kHz sampling, plus CPU overhead compared to naive sampling.

TODO: references
