# Tone generators

- Ramp generator, can be used to:
  - turn into sine, tri, square and some other basic shapes
  - index into wavetable
- Random noise
- Unlimitted FM?
- Karplus-Strong

# Karplus-Strong

Use a POT buffer size, do interpolation to get desired exact frequency?
Initial excitation using output from other tone generators
Allow multiple taps, just like filters?
Add a non-linear excitation tap simulating stick-slip from a bow

# LFO

- Just allow any tone generator with low frequency

# Envelope generators

- Straight-forward ADSR
- ADR for Karplus-Strong?
- Arbitrary length generators?

# Filters

- Short delay lines with arbitrary taps
  - Create lp, bp, hp, comb, notch etc.
  - More complex ones that add resonance
  - Both IIR and FIR?
  - Allow tap gain and/or position to vary using LFOs?
  - Key follow?

# Effects

- Long delay lines with arbitrary taps
  - Create echo, reverb, chorus
  - Allow tap gain and/or position to vary using LFOs?
  - Lock to beat freq?

