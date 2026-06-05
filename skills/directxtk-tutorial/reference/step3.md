# Step 3: Adding DirectX Tool Kit

Now add DirectX Tool Kit to the project via vcpkg. Edit the `vcpkg.json` manifest in the project root to add the `directxtk` port with GameInput support.

Add the following entry to the `"dependencies"` array in `vcpkg.json`:

```json
{
  "name": "directxtk",
  "default-features": false,
  "features": [
    "gameinput"
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
        "gameinput"
      ]
    }
  ]
}
```

## CMake Projects

For CMake projects, also add the following to `CMakeLists.txt`:

```cmake
find_package(directxtk CONFIG REQUIRED)
find_package(gameinput CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Microsoft::DirectXTK)
```

## Build and Verify

Build the project to verify everything works. Use the same commands from [Step 2](step2.md).

### MSBuild

For x64 systems:

```powershell
msbuild <ProjectName>.vcxproj /p:Configuration=Debug /p:Platform=x64
```

For ARM64 systems:

```powershell
msbuild <ProjectName>.vcxproj /p:Configuration=Debug /p:Platform=ARM64
```

### CMake

For x64 systems:

```powershell
cmake --preset=x64-Debug
cmake --build out\build\x64-Debug
```

For ARM64 systems:

```powershell
cmake --preset=arm64-Debug
cmake --build out\build\arm64-Debug
```

> The first rebuild will take longer as vcpkg fetches and builds DirectXTK and its dependencies (including GameInput).

## Verifying the Integration

Once the build succeeds, verify that the DirectXTK headers are available by adding a test include to `pch.h`:

```cpp
#include <directxtk/SpriteBatch.h>
```

> **Important:** Use `<directxtk/...>` include style (not `"SpriteBatch.h"`) for proper MSBuild+vcpkg integration.

If the build still succeeds, DirectXTK is properly integrated.

## Technical Notes

**Ask the user:** Would you like to see the Technical Notes for this step, or skip to the next step?

- **Static linking** — The MSBuild template uses `-static-md` vcpkg triplets, so the DirectXTK library is statically linked. For CMake, the `DIRECTX_TOOLKIT_IMPORT` define is handled automatically by the CMake targets when using DLL triplets.

- **Header location** — The DirectXTK headers are installed in the `include/directxtk` directory of the `vcpkg_installed` folder for the given host/target triplet combination. The VCPKG+MSBuild integration only adds the root 'include' folder to the search paths for the build, so we have to specify the subdirectory explicitly. For CMake, the CMake targets automatically handle the specific header location, but with the use of the directxmath vcpkg port it also has the root folder in the search paths.

- **GameInput** — A modern input API that provides a unified interface for gamepads, keyboards, and mice. It replaces the older XInput and raw input APIs. The vcpkg `gameinput` feature links the GameInput redistributable library.

> **Important:** You may need to run GameInputRedist.msi which is installed into a tools folder in `vcpkg_installed` as part of the build process.

- **vcpkg features** — The `"default-features": false` disables the default XInput backend so that only GameInput is used for input handling. This avoids linking both input systems. It also disables DirectX Tool Kit for Audio which we can add back later.
