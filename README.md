# Knack

Free clipper plug-in by Dani Koenig for macOS (Apple Silicon): VST3, CLAP, Audio Unit and Standalone.

One big CLIP knob, a SOFT / HARD slider, INPUT with CALIBRATE (auto input), MIX for parallel clipping and an OUTPUT limiter (0 = 0 dBFS). The curve display and a red glow show how hard the signal is pushed.

## Build
Built on GitHub Actions (macOS, Apple Silicon). Locally:

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target Knack_VST3
```

JUCE 8.0.4 is downloaded automatically.

## Test (DSP, no GUI)
```
cmake -B build -DKNACK_TEST_SRC=tests/EngineTest.cpp && cmake --build build --target KnackTest
```

## Install
See [INSTALL.txt](INSTALL.txt) or the [releases](https://github.com/danikoenig/Knack/releases).
