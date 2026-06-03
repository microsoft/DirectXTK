@set VCPKG_BINARY_SOURCES=clear
@set VCPKG_ROOT=%cd%
@if %1.==clang. goto clang
.\vcpkg install directxtk[core]:x86-windows
@if errorlevel 1 goto error
.\vcpkg install directxtk[tools]:x86-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[xaudio2-8]:x86-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[xaudio2-9]:x86-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[windows-gaming-input]:x86-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[xaudio2redist]:x86-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[spectre]:x86-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk:x86-windows-static
@if errorlevel 1 goto error
.\vcpkg install directxtk:x86-windows-static-md
@if errorlevel 1 goto error
.\vcpkg install directxtk[xaudio2redist]:x86-windows-static-md --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[core]:x64-windows
@if errorlevel 1 goto error
.\vcpkg install directxtk[tools]:x64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[xaudio2-8]:x64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[xaudio2-9]:x64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[windows-gaming-input]:x64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[xaudio2redist]:x64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[xaudio2redist]:x64-windows-static-md --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[gameinput]:x64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[gameinput]:x64-windows-static-md --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[spectre]:x64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk:x64-windows-static
@if errorlevel 1 goto error
.\vcpkg install directxtk:x64-windows-static-md
@if errorlevel 1 goto error
.\vcpkg install directxtk[core]:arm64-windows
@if errorlevel 1 goto error
.\vcpkg install directxtk[tools]:arm64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[windows-gaming-input]:arm64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[xaudio2redist]:arm64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[spectre]:arm64-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk:arm64-windows-static
@if errorlevel 1 goto error
.\vcpkg install directxtk:arm64-windows-static-md
@if errorlevel 1 goto error
.\vcpkg install directxtk[xaudio2redist]:arm64-windows-static-md --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk:arm64ec-windows
@if errorlevel 1 goto error
.\vcpkg install directxtk[tools]:arm64ec-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[spectre]:arm64ec-windows --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk:x86-uwp
@if errorlevel 1 goto error
.\vcpkg install directxtk:x64-uwp
@if errorlevel 1 goto error
.\vcpkg install directxtk:arm64-uwp
@if errorlevel 1 goto error
@where /Q x86_64-w64-mingw32-g++.exe
@if errorlevel 1 goto skipgcc64
.\vcpkg install directxtk:x64-mingw-dynamic
@if errorlevel 1 goto error
.\vcpkg install directxtk[core]:x64-mingw-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[gameinput]:x64-mingw-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[xaudio2redist]:x64-mingw-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[tools]:x64-mingw-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[core]:x64-mingw-static
@if errorlevel 1 goto error
.\vcpkg install directxtk[xaudio2redist]:x64-mingw-static --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[gameinput]:x64-mingw-static --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[tools]:x64-mingw-static --recurse
@if errorlevel 1 goto error
:skipgcc64
@where /Q i686-w64-mingw32-g++.exe
@if errorlevel 1 goto finish
.\vcpkg install directxtk:x86-mingw-dynamic
@if errorlevel 1 goto error
.\vcpkg install directxtk[xaudio2redist]:x86-mingw-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[tools]:x86-mingw-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk:x86-mingw-static
@if errorlevel 1 goto error
.\vcpkg install directxtk[xaudio2redist]:x86-mingw-static --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[tools]:x86-mingw-static --recurse
@if errorlevel 1 goto error
@goto finish
:clang
.\vcpkg install directxtk:x64-clangcl-dynamic
@if errorlevel 1 goto error
.\vcpkg install directxtk[tools]:x64-clangcl-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[xaudio2-8]:x64-clangcl-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[xaudio2-9]:x64-clangcl-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[xaudio2redist]:x64-clangcl-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk:x64-clangcl-static
@if errorlevel 1 goto error
.\vcpkg install directxtk:arm64-clangcl-dynamic
@if errorlevel 1 goto error
.\vcpkg install directxtk[tools]:arm64-clangcl-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk[xaudio2redist]:arm64-clangcl-dynamic --recurse
@if errorlevel 1 goto error
.\vcpkg install directxtk:arm64-clangcl-static
@if errorlevel 1 goto error
.\vcpkg install directxtk:x64-clangcl-uwp
@if errorlevel 1 goto error
.\vcpkg install directxtk:arm64-clangcl-uwp
@if errorlevel 1 goto error
:finish
@echo SUCCEEDED
@if %1.==clang. goto eof
@where /Q x86_64-w64-mingw32-g++.exe
@if NOT errorlevel 1 goto gcc64
@echo .
@echo . Run for MinGW64
@echo .   .\vcpkg install directxtk:x64-mingw-dynamic
@echo .   .\vcpkg install directxtk[gameinput]:x64-mingw-dynamic --recurse
@echo .   .\vcpkg install directxtk[xaudio2redist]:x64-mingw-dynamic --recurse
@echo .   .\vcpkg install directxtk[tools]:x64-mingw-dynamic --recurse
@echo .   .\vcpkg install directxtk:x64-mingw-static
@echo .   .\vcpkg install directxtk[gameinput]:x64-mingw-static --recurse
@echo .   .\vcpkg install directxtk[xaudio2redist]:x64-mingw-static --recurse
@echo .   .\vcpkg install directxtk[tools]:x64-mingw-static --recurse
:gcc64
@where /Q i686-w64-mingw32-g++.exe
@if NOT errorlevel 1 goto gcc32
@echo .
@echo . Run for MinGW32
@echo .   .\vcpkg install directxtk:x86-mingw-dynamic
@echo .   .\vcpkg install directxtk[xaudio2redist]:x86-mingw-dynamic--recurse
@echo .   .\vcpkg install directxtk[tools]:x86-mingw-dynamic --recurse
@echo .   .\vcpkg install directxtk:x86-mingw-static
@echo .   .\vcpkg install directxtk[xaudio2redist]:x86-mingw-static --recurse
@echo .   .\vcpkg install directxtk[tools]:x86-mingw-static --recurse
:gcc32
@goto eof
:error
@echo FAILED
:eof
