# List of NRPN parameters supported by Pling

Currently this is just a draft of what this list might look like.

## Oscillator control

Up to 64 oscillators, each with:

- Type (waveform)
- Frequency
- Frequency key follow
- Amplitude
- Modulator source
- Modulation type
- Envelope source

## Envelope control

Up to 64 envelopes, each with:

- Type (adsr, adr, N-segment)
- Attack key follow
- Decay key follow
- Up to 8 segments, each with:
  - Level
  - Duration

## Filter control

Up to 64 filters, each with:

- Type (lf, hf, bp, ...)
- Cutoff frequency
- Frequency key follow
- Cutoff envelope source
- Q
