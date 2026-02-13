# Core Modules Reference

Detailed specifications for Gravisynth's primary synthesis modules.

## Oscillator Module
- **Waveforms**: Sine, Square, Saw, Triangle.
- **Features**: 
    - PolyBLEP anti-aliasing on Square and Saw.
    - MIDI-to-Frequency tracking.
    - Integrated visual buffer for real-time waveform display.

## Filter Module
- **Type**: Resonant Low-Pass.
- **Parameters**: Cutoff (Freq), Resonance (Q).
- **Quality**: Zero-delay feedback (ZDF) style filtering for analog-like response.

## ADSR (Envelope) Module
- **Stages**: Attack, Decay, Sustain, Release.
- **Output**: Generates a mono control signal (0.0 to 1.0).
- **Uses**: Modulation of VCA gain or Filter cutoff.

## VCA (Amplifier) Module
- **Inputs**: 
    - Input 0: Audio.
    - Input 1: CV (Modulation).
- **Features**: Parameter smoothing for "click-free" gain changes.

## Poly MIDI Module
- **Capacity**: 8 simultaneous voices.
- **Allocation**: Least Recently Used (LRU) algorithm.
- **Outputs**: Provides 8 separate Pitch and Gate channels for polyphonic routing.
