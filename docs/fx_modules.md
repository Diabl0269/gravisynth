# FX Modules Reference

Technical documentation for the Gravisynth effects suite.

## Distortion Module
- **Algorithm**: Nonlinear soft-clipping using a `tanh`-based curve: `f(x) = x / (1 + |x|)`.
- **Oversampling**: 2x oversampling using polyphase IIR half-band filters to reduce aliasing in the high-frequency spectrum.
- **Parameters**: Drive (Intensity), Mix (Wet/Dry).

## Delay Module
- **Type**: Stereo feedback delay.
- **Technique**: Fractional delay line with linear interpolation for smooth "time" parameter changes.
- **Parameters**: Time (ms), Feedback, Mix.
- **Smoothing**: Glide-on-time prevents pitch glitches during modulation.

## Reverb Module
- **Type**: Algorithmic stereo reverb.
- **Implementation**: Uses standard Schroeder/Freeverb-inspired techniques for lush acoustic simulation.
- **Parameters**: Room Size, Damping, Width, Mix.
