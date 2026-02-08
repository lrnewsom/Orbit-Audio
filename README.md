# OrbitAudio

A macOS menu bar app for real-time 3D/8D-style binaural spatialization.

Real-time binaural-style stereo spatializer: pan with interaural time difference (ITD) and head-shadow filtering for 3D/8D-style listening.

## Why I made OrbitAudio

I really enjoy good audio that immerses me when I listen to it. My favorite movies, video games, tv shows, etc. all have one thing in common -- they have amazing soundtracks. So, one day I encountered 3D audio on Spotify or YouTube for the first time and was really intrigued by the effect. Over time, I began to wonder how those tracks were made and if I could make my own. Then, after doing some quick research, I came to the conclusion there'd be some legal issues, and a lot of work to do, if I were to make 3D versions of my favorite songs. That, then, led me to the idea behind OrbitAudio; If I can't make my own 3d tracks, then can I make something that lets me "3d" whatever I am listening to? Yes, yes I can, and OrbitAudio is the quickly-made tool that does just that. :)

## What it does

OrbitAudio takes stereo input and applies:

- **Pan** — Left/right balance (-1 = full left, +1 = full right).
- **ITD (interaural time difference)** — A short delay on the “far” ear so the sound feels like it’s coming from a direction.
- **Head shadow** — A low-pass filter on the far ear to mimic your head blocking high frequencies.
- **Orbit mode** — Manual, Orbit (3D), or Figure-8 (8D). The latter two drive pan from an LFO for swirling spatial motion.
- **Speed (Hz)** — LFO rate for Orbit/Figure-8 modes (0.02–0.5 Hz).
- **Depth** — HF rolloff to simulate distance (0 = close, 1 = far).
- **Width** — Stereo field scale (0 = narrow, 1 = full).
- **Reverb** — Optional stereo reverb with adjustable wet amount.

Together this gives a binaural-style sense of direction with 3D/8D-style orbit modes. Best experienced with headphones.

## Screenshots

![OrbitAudio control panel](docs/panel-screenshot.png)

## Menu bar app

OrbitAudio runs as a **menu bar (status bar) app**—no dock icon. On launch you'll see the OrbitAudio icon in the top menu bar. Click it to open the Control Center-style panel with all spatialization controls. Click again or use the close button to hide the panel. Right-click the icon for "Open OrbitAudio" or "Quit". Audio keeps running when the panel is hidden. Use **Audio settings** to expand device selection (input/output, buffer size, sample rate).

## Pipeline (e.g. BlackHole)

Typical use: route stereo from your DAW or system (e.g. via **BlackHole 16ch**) into OrbitAudio’s input, then set OrbitAudio’s output to your headphones or monitor. The app uses a single stereo in → stereo out path; the rest of the 16ch pipeline can feed other tools.

To use OrbitAudio as intended, I recommend downloading BlackHole to set up a "virtual" audio source. I went with the 16ch version because ChatGPT recommended that for some reason, but I am pretty sure it'd be fine with any version of BlackHole. OrbitAudio uses BlackHole as a middle-man to pass along system audio on your Mac; basically, it lets us throw any audio source through OrbitAudio's spatializer.

## Low latency

OrbitAudio is optimized for minimal latency:

- On first run (no saved device state), the app requests the smallest available buffer size (128 samples or lower when supported).
- The device selector shows buffer size and sample rate; choose **128 samples** (or lower) for lowest latency.
- Audio device selection is persisted to `~/Library/Application Support/OrbitAudio/audioDeviceState.xml` and restored on launch.
- Denormal protection and in-place processing keep the DSP path lean.

## Tech

- **Tech stack:** C++, JUCE, macOS.
- **JUCE** (C++), macOS GUI app.
- **Build**: Open `NewProject/Builds/MacOSX/OrbitAudio.xcodeproj` in Xcode and build. The built app is at `Builds/MacOSX/build/Debug/OrbitAudio.app` (or Release). Copy to Applications or run from the build folder.
- **DSP**: The spatializer lives in `Source/Spatializer.cpp` (delay + LPF + LFO + depth/width); the UI in `Source/MainComponent.cpp` passes parameters.

## License

See [LICENSE](LICENSE) in this repository.
