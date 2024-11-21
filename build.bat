@echo off
REM Build script for Emscripten with emcc

REM Set filenames for input and output
set INPUT_FILE=main.cpp
set OUTPUT=index.html
set SHELL_FILE=shell.html

REM Check if the source file exists
if not exist "%INPUT_FILE%" (
    echo Error: %INPUT_FILE% not found!
    exit /b 1
)

REM Define SDL and WebAssembly flags
set SDL_FLAGS=-s USE_SDL=2 -s USE_SDL_MIXER=2
set WASM_FLAGS=-s USE_WEBGL2=1 -s FULL_ES3=1 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s MAX_WEBGL_VERSION=2
set OPT_FLAGS=-O3
set RUNTIME_FLAGS=-s EXPORTED_RUNTIME_METHODS=['ccall'] -s NO_EXIT_RUNTIME=1

REM List additional files to preload
set PRELOAD_FILES=--preload-file anime-cat-girl-6731.wav --preload-file anime-cat-girl-105182.wav

REM Compile using emcc
echo Running emcc to compile %INPUT_FILE%...
emcc %INPUT_FILE% -o %OUTPUT% ^
    %SDL_FLAGS% ^
    %WASM_FLAGS% ^
    %OPT_FLAGS% ^
    %RUNTIME_FLAGS% ^
    %PRELOAD_FILES% ^
    --shell-file %SHELL_FILE%

REM Check if the compilation was successful
if %ERRORLEVEL% neq 0 (
    echo Compilation failed.
    exit /b %ERRORLEVEL%
)

echo Compilation successful! Output files generated:


