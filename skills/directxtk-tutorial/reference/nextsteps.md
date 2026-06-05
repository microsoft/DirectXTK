# Next Steps

## Graphics

To explore more about DirectX Graphics functionality, continue with the tutorials at [Getting Started](https://github.com/microsoft/DirectXTK/wiki/Getting-Started) starting at [Rendering a model](https://github.com/microsoft/DirectXTK/wiki/Rendering-a-model).

## Input

To implement game input using GamePad, see [Game controller input](https://github.com/microsoft/DirectXTK/wiki/Game-controller-input) which for this tutorial's project will use the GameInput APIs.

To implement game input using Mouse and Keyboard, see [Mouse and keyboard input](https://github.com/microsoft/DirectXTK/wiki/Mouse-and-keyboard-input).

## Math

For more on the SimpleMath library, which is a C++ wrapper for the DirectXMath SIMD library, see [Using the SimpleMath library](https://github.com/microsoft/DirectXTK/wiki/Using-the-SimpleMath-library).

Alternatively, advanced users can use the **directxmath-usage** skill from [DirectXMath](https://github.com/microsoft/DirectXMath).

## DirectX Tool Kit for Audio

The audio functionality is built on XAudio2 which is a game audio API for Windows which provides real-time mixing and audio processing capabilities.

To add *DirectX Tool Kit for Audio* to your project, update the `vcpkg.json` file:

```json
{
  "name": "directxtk",
  "default-features": false,
  "features": [
    "gameinput",
    "xaudio2-9"
  ]
}
```

For example, the full `vcpkg.json` should look something like:

```json
{
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json",
  "dependencies": [
    "directxmath",
    {
      "name": "directxtk",
      "default-features": false,
      "features": [
        "gameinput",
        "xaudio2-9"
      ]
    }
  ]
}
```

Then add to the pch.h:

```cpp
#include <directxtk/Audio.h>
```

From there, pick up at [Adding audio to your project](https://github.com/microsoft/DirectXTK/wiki/Adding-audio-to-your-project).
