# BlipBlop-Modern Progress

## Current build status

The game configures and builds successfully as a native x64 Release executable with Visual Studio 2022, CMake 3.29.5, MSVC 19.42.34435, SDL2 2.32.10, and SDL2_mixer 2.8.2. Dependencies are declared in `vcpkg.json`; its registry baseline is pinned for repeatability and the `mpg123` feature is enabled because the shipped data includes MP3 music.

Verified commands (PowerShell, from the repository root):

```powershell
git clone --depth 1 https://github.com/microsoft/vcpkg.git out/vcpkg
cmd.exe /c out\vcpkg\bootstrap-vcpkg.bat -disableMetrics
& 'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' -S . -B out/build-vs2022 -G 'Visual Studio 17 2022' -A x64 -DCMAKE_TOOLCHAIN_FILE='D:/Cs/C++/Home Projects/blip-blop/out/vcpkg/scripts/buildsystems/vcpkg.cmake'
& 'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build out/build-vs2022 --config Release --parallel 4
```

Result: `out/build-vs2022/vc-projects/Blip_n_Blop_3/Release/blipblop.exe` and the required dependency DLLs were produced. The build emitted one warning: `pause_menu.cpp(30): C4715`, because `PauseMenu::ProcessEvent` does not return a value on every path.

The Visual Studio-bundled vcpkg client was also tried. Its pinned older ports referenced a removed MSYS2 pkgconf archive, while current ports require a newer vcpkg client. A current workspace-local vcpkg checkout succeeded. Build directories are ignored and are not repository content.

## Current runtime status

A bounded smoke launch was performed from `vc-projects/Blip_n_Blop_3`, where the relative `data/` directory is available. The process remained alive for eight seconds and was then stopped intentionally. This verifies that the executable starts and does not immediately crash, but the hidden, non-interactive check does **not** verify the main menu, gameplay, rendering correctness, input, audio, level loading, fullscreen, or clean shutdown.

Launching from an arbitrary working directory is not yet supported because assets, configuration, high scores, and logs use relative paths.

## Architecture audit

- **Entry point and startup:** `blip_n_blop_3.cpp` contains `main`. `InitApp` initializes the SDL_mixer-backed FMOD compatibility API, reads `data/bb.cfg` and `data/bb.scr`, loads localized text, initializes graphics/input, creates the 640x480 surfaces, initializes the LGX decoder, and loads fonts/interface banks. `main` calls `Game::go`, then writes high scores and configuration.
- **Main game flow:** `Game::go` in `game.cpp` owns the intro/title/character-selection loop and starts `jouePartie`. Each level runs `gameLoop` until death, completion, skip, or quit. `UpdateRegulator` preserves the original fixed-step simulation at an approximately 11 ms step and may perform zero or multiple simulation updates per rendered frame.
- **Rendering:** `graphics.cpp/.h`, `dd_gfx.cpp/.h`, and `sdl_surface.h` form an SDL2 implementation of the old DirectDraw-shaped API. Gameplay renders to 640x480 SDL surfaces; `Graphics::Flip` creates a texture from the back surface every frame and copies it to an SDL renderer. Fullscreen currently uses exclusive `SDL_WINDOW_FULLSCREEN`; window placement incorrectly uses the requested width/height as x/y coordinates. There is no aspect-preserving modern output policy yet.
- **Image/assets:** `picture_bank`, `picture`, `fonte`, and `lgx_packer` decode the custom GFX/LGX and font containers. `LGXpacker` directly writes pixels into locked SDL surfaces and contains 16/32-bit and pitch assumptions that are high-risk on modern formats.
- **Platform abstraction:** SDL2 supplies windows, surfaces, rendering, events, timing, and joysticks. The code retains DirectDraw/DirectInput/FMOD-shaped compatibility interfaces and many global objects. Windows-specific live code is mainly `MessageBox` diagnostics and the GUI subsystem; old Win32 window procedure code is disabled.
- **Input:** `input.cpp/.h` polls SDL events into legacy key/joystick buffers and aliases configured controls through `config.cpp`. It uses raw SDL joystick APIs, fixed arrays, unchecked string copying, and suspicious joystick event indexing. Keyboard behavior must remain the regression reference.
- **Audio/music:** `fake_fmod.cpp` maps the legacy FMOD-facing calls to SDL2_mixer. `SoundBank` reads embedded WAV samples from SFX banks; `MusicBank` loads music paths from MBK banks. Mixer initialization requests OGG/MOD support while shipped music also includes MP3, hence the vcpkg mpg123 feature.
- **Data and level loading:** Runtime assets are a flat `data/` directory containing GFX, SFX, MBK, MP3, fonts, scripts, level lists, and binary LVL files. `Game::loadList` reads `data/bb.lst`; `Game::chargeNiveau` parses fixed-size binary fields and loads the referenced graphics, audio, RPG, enemy, and event data. Embedded backslashes in level/list paths remain a portability concern.
- **Configuration/saves:** `config.cpp` reads and writes a raw binary `data/bb.cfg`; `HiScores` similarly uses `data/bb.scr`. Both mutate the shipped data directory and depend on the current working directory. There is no versioning, validation, migration, or per-user writable location.
- **Build system:** the previous CMake file existed only inside the game directory, used ad-hoc library/path searches, referenced a nonexistent `Engine` directory, did not request C++17, and placed the `WIN32` executable flag on the non-Windows branch. The new root project uses imported vcpkg CMake targets, C++17, an x64-capable VS generator, and a pinned dependency manifest. Editors and historical Visual Studio projects remain out of scope.

## Upstream pull-request findings

### PR #9 — critical crashes and Linux compatibility

PR #9 has two commits and changes LGX decoding, halftone/pause rendering, path normalization, SDL window/scaling behavior, and Linux CMake linking. Its most valuable findings are the 32-bit SDL pixel/pitch mismatch in `LGXpacker`, the missing surface unlock/bounds handling in `halfTone`, and Windows separators embedded in level data. These should be reimplemented and tested narrowly.

Do not copy it wholesale: it rewrites the README instead of preserving history, changes `bb.cfg`, forces the software renderer, combines several display-policy changes, performs in-place path mutation, and includes defensive early returns that can hide initialization errors. Its LGX changes need validation against surface formats and pitch rather than assuming every surface is 32-bit. The pause-menu missing return is valid but should return the correct named menu result after its contract is confirmed.

### PR #10 — build, CI, and packaging baseline

PR #10 adds a root CMake project, C++17, correct Windows/non-Windows executable subsystem selection, a GitHub Actions matrix, vcpkg-based Windows dependencies, an executable existence check, and a packaged artifact with runtime DLLs and data. It also identifies the need for mpg123 support and the MSVC GUI-entry-point issue.

The build ideas are useful and informed this baseline, but should not be copied blindly. Its dependency discovery still mixes `find_package`, `find_library`, and `find_path`; the manifest was absent; the workflow installs floating dependencies; all vcpkg DLLs are copied indiscriminately; and the README edit does not follow this repository's preservation policy. CI and packaging belong in the final roadmap phase after runtime/resource fixes.

## Important discoveries and compatibility risks

1. LGX decoding and halftone effects assume pixel sizes and sometimes surface width instead of pitch; this is the leading startup/corruption risk.
2. All resources and writable files are current-working-directory relative, so double-click and packaged launches are fragile.
3. The raw binary config format serializes implementation-sized `bool`/`int` values without validation and writes into `data/`.
4. The renderer recreates a texture every frame, lacks error propagation, and does not preserve 4:3 output on arbitrary windows/displays.
5. Input uses legacy fixed buffers and raw joystick indexes; controller bounds and device lifecycle need auditing.
6. The fixed-step regulator is gameplay-critical. Modernization must not change its effective timing without before/after behavioral measurement.
7. Numerous fixed-size buffers, C string operations, binary reads, global ownership patterns, and unchecked file contents create x64/runtime risks.
8. Cleanup is incomplete: `ReleaseAll` exists but is not called by `main`, and SDL/mixer/window ownership needs a focused audit.

## Completed work

- Audited the game architecture and both requested upstream PRs without merging them.
- Added a root CMake entry point, C++17 target requirements, imported SDL2 targets, and correct Windows GUI target setup.
- Added a pinned vcpkg manifest and generated-file exclusions.
- Successfully configured and compiled a VS2022 x64 Release executable.
- Performed a bounded startup smoke check.
- Added the modernization/upstream acknowledgement while preserving the original README below it.
- Created this status document and the staged implementation roadmap.

## Known problems

- Interactive runtime behavior is unverified.
- Launch remains dependent on the working directory.
- PR #9's crash, pitch, unlock, and path fixes are not yet integrated.
- `PauseMenu::ProcessEvent` has a missing-return warning.
- Modern scaling, aspect-ratio handling, high-DPI behavior, and borderless fullscreen are absent.
- There is no automated test suite or current CI workflow.

## Current task

Prompt 1 is complete: audit, modern build baseline, upstream assessment, documentation, and smoke-build verification.

## Next task

Prompt 2 is the startup and resource-loading work described in `IMPLEMENTATION_PLAN.md`. It has not been started.
