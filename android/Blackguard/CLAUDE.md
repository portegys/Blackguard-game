# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository scope

This directory (`android/Blackguard`) is the Android Studio project for **Blackguard**, a first-person 3D rogue-like game. It is one of several platform ports living in the larger `Blackguard` repo (siblings: `basic/` OpenGL+C++/FMOD, `ios/` Marmalade, `jogl_jni/` Java+JOGL/JNI, `metro/` Windows Store) — those are not part of this Gradle project and generally don't need to be touched when working here. All ports share the same core: the classic **Rogue** game engine in C, located two levels up at `../../rogue/` (relative to this file), which this Android app compiles and links against via JNI/NDK.

The game manual is at `app/src/main/assets/doc/blackguard.txt` — read it for gameplay/command semantics when working on input handling or game logic.

## Build

Standard Gradle/Android Studio project (Gradle 8.7, AGP 9.1.1). NDK/ndk-build is used for the native side (no CMake).

```bash
# Windows
gradlew.bat assembleDebug
gradlew.bat assembleRelease

# From a shell with the wrapper
./gradlew assembleDebug
```

- `applicationId`: `com.dialectek.blackguard`
- `compileSdkVersion`/`targetSdkVersion`: 37, `minSdkVersion`: 29
- Native build config: `app/src/main/jni/Android.mk` (invoked via `externalNativeBuild { ndkBuild { ... } }` in `app/build.gradle`), building ABIs `armeabi-v7a`, `arm64-v8a`, `x86`, `x86_64`.
- There are no unit/instrumented tests in this project (no `src/test` or `src/androidTest` sources) and no lint/format tooling configured beyond the default Android Gradle plugin — there is nothing to run beyond a build.

## Native build details

`app/src/main/jni/Android.mk` compiles a single shared library (`libBlackguard.so`) from:
1. `Blackguard.cpp` — the JNI bridge (see below).
2. The shared Rogue engine sources at `../../../../../../rogue/*.c` (i.e., the repo-root `rogue/` directory).
3. A bundled ncurses 5.7 source tree (`../../../ncurses-5.7/...`) providing the terminal/curses layer the Rogue engine expects — this is **not checked into this directory**; the Android.mk expects it to exist as a sibling at the `android/` level. If you're missing it, native builds will fail on curses headers.

Header files under `app/src/main/jni/com_dialectek_blackguard_*.h` are javah-generated JNI headers for the Java classes below (see `app/src/main/jni/javah.sh` for how they're regenerated after changing native method signatures in Java).

## Architecture: how the pieces fit together

The app embeds the terminal-based Rogue game engine (designed for stdin/stdout + curses) inside a modern Android OpenGL ES view, translating between the two worlds via a producer/consumer threading model:

1. **Rogue engine thread** (`rogue_main`, native, C) — runs the actual unmodified Rogue game loop from `rogue/*.c` against a curses `WINDOW`. It blocks on input and writes to the curses screen buffer exactly as the original terminal game would.
2. **JNI bridge** (`app/src/main/jni/Blackguard.cpp`) — starts the Rogue thread via `pthread_create`, and exposes the curses screen buffer and input queue to Java through JNI functions named `Java_com_dialectek_blackguard_<Class>_<method>`. Synchronization between the Rogue thread and the UI/render threads uses two mutex/condvar pairs: `RenderMutex` (guards reads of the screen buffer) and `InputMutex`/`InputCond1`/`InputCond2` (hands typed input to the blocked Rogue thread and waits for it to render the next frame).
3. **`Blackguard.java`** (`Activity`) — app entry point; also owns the `SpeechRecognizer`/`RecognizerIntent` voice-command flow (results are translated into keystrokes fed to the engine).
4. **`BlackguardView.java`** (`GLSurfaceView`) — owns the native Rogue thread lifecycle (`initBlackguard`/`saveBlackguard`/`deleteBlackguard`), turns touch gestures (`GestureDetector.OnGestureListener`/`OnDoubleTapListener`) and soft-keyboard taps into characters pushed to the engine's input buffer (`inputChar`/`inputBuf`/`inputReq`), and tracks player facing (`playerDir`).
5. **`BlackguardRenderer.java`** (`GLSurfaceView.Renderer`) — each frame, locks the render mutex, reads the curses screen buffer cell-by-cell (`getLines`/`getCols`/`getWindowChar`/`getScreenChar`), and turns it into the first-person 3D scene: walls/floor/doors as textured geometry, monsters/items as billboards positioned relative to the player, plus the on-screen soft keyboard (`SoftKeyboardToggle`) and voice-input button (`VoicePrompter`). Text messages from the engine surface via `displaymsg()`.
6. **`Scene.java`** — the 3D scene graph/geometry builder used by the renderer: `Animator` (monster movement/animation state fed from native via `getAnimator`), `ObjectTransform`, `Wall`, `Floor`, `Billboard`. Coordinates/enums (`DIRECTION`, `LOCALE`, `TEXTURE_TYPE`) are mirrored between here and the native side via the generated JNI headers.
7. **`LabelMaker.java`** — builds OpenGL textures for text labels (used for on-screen messages/HUD text and the soft keyboard glyphs).
8. **`Grid.java`** — small helper for tessellated grid geometry (floor/wall tiling).
9. **`SoundManager.java`** — plays the `.ogg` sound effects in `app/src/main/assets/sounds/` (native mute toggle via `SoundManager_mute`).
10. **`Eula.java`** — first-run EULA dialog.

When changing gameplay behavior, prefer editing the shared `rogue/*.c` engine (it affects every port) unless the change is Android-presentation-specific (rendering, input mapping, voice/touch UI), in which case it belongs in the Java classes or `Blackguard.cpp`.

Because the Rogue engine was written as a single-process terminal program (global state, `exit()` calls, etc.), the native side works around this by running it on its own pthread rather than adapting it to an event-driven model — keep this in mind before "cleaning up" the threading/globals in `Blackguard.cpp` or `rogue/*.c`.
