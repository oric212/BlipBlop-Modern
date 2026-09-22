# BlipBlop-Modern Progress

## Current build status

The game configures and builds successfully as a native x64 Release executable with Visual Studio 2022, CMake 3.29.5, MSVC 19.42.34435, SDL2 2.32.10, and SDL2_mixer 2.8.2. Dependencies are declared in `vcpkg.json`; its registry baseline is pinned for repeatability and the `mpg123` feature is enabled because the shipped data includes MP3 music.

Verified commands (PowerShell, from the repository root):

```powershell
git clone --depth 1 https://github.com/microsoft/vcpkg.git out/vcpkg
cmd.exe /c out\vcpkg\bootstrap-vcpkg.bat -disableMetrics
& 'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' -S . -B out/build-vs2022 -G 'Visual Studio 17 2022' -A x64 -DCMAKE_TOOLCHAIN_FILE='D:/Cs/C++/Home Projects/blip-blop/out/vcpkg/scripts/buildsystems/vcpkg.cmake'
& 'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build out/build-vs2022 --config Release --parallel 4
& 'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build out/build-vs2022 --config Release --target deploy
```

Result: `out/build-vs2022/vc-projects/Blip_n_Blop_3/Release/BlipnBlop.exe` and the required dependency DLLs were produced. A clean build still emits the known `pause_menu.cpp(30): C4715` warning because `PauseMenu::ProcessEvent` does not return a value on every path; Prompt 2 did not change that code.

The Visual Studio-bundled vcpkg client was also tried. Its pinned older ports referenced a removed MSYS2 pkgconf archive, while current ports require a newer vcpkg client. A current workspace-local vcpkg checkout succeeded. Build directories are ignored and are not repository content.

## Current runtime status

A bounded development smoke launch was performed from `vc-projects/Blip_n_Blop_3`. Because the build-tree executable has no adjacent `data/`, the documented legacy working-directory fallback selected the source game directory. The process remained alive for eight seconds and was then stopped intentionally.

The deployed `game-build/BlipnBlop.exe` was also launched for eight seconds both from `game-build/` and from `%TEMP%/blipblop-prompt2-cwd`. Both processes remained alive. The unrelated-directory log recorded `game-build/` as both the application directory and executable-relative resource root, proving normal standalone startup did not use source-tree data. These hidden, non-interactive checks do **not** verify menu visibility, gameplay, rendering correctness, input, audio output, level loading, fullscreen, or clean shutdown.

## Runtime resource paths

`runtime_paths.cpp/.h` obtains the executable directory with `SDL_GetBasePath`, records the original working directory, and selects a runtime resource root before initialization. The lookup order is:

1. executable-directory-relative logical path;
2. original-working-directory-relative logical path as a development compatibility fallback;
3. if neither exists, return the preferred executable-relative path so errors and writable legacy files have deterministic paths.

If the executable has an adjacent `data/`, startup changes the process working directory to the executable directory. This lets untouched legacy call sites and paths embedded in scripts continue to resolve under the packaged runtime root. If executable-adjacent `data/` is absent but the original working directory has `data/`, startup selects and reports that fallback. If neither directory exists, startup stops with both attempted paths.

Directly resolved call sites now cover startup config/high scores, localized text, fonts, interface graphics/music, the level list, level files and their referenced graphics/SFX/MBK/RPG resources, and filenames read from music banks. Backslashes read from level lists and music-bank metadata are normalized in memory; original data files are not changed. Other legacy `data/...` calls resolve because startup selects the resource root as the process working directory.

Missing localized text now reports the logical resource and full resolved path through the log and Windows error dialog. A test temporarily made `game-build/data/uk.dat` unavailable; the log reported `data/uk.dat` resolved to the exact missing path under `game-build/data`, and the file was restored immediately.

The remaining current-working-directory-dependent output is `BlipBlop.log`, whose global stream opens before `main` can initialize executable-relative paths. Moving logs and writable user data is deferred to Prompt 6. Config and high scores retain their original formats and semantics under executable-relative `data/` as required.

## Local runtime deployment

The CMake `deploy` target recreates the ignored `game-build/` directory from a Release build. It copies `BlipnBlop.exe`, the original 128 runtime data files, direct SDL runtime DLLs, the five transitive SDL2_mixer codec DLLs identified with `dumpbin`, and the MSVC runtime DLLs. It does not copy source, project, documentation, cache, object, or source-control files.

The clean generated root contains:

```text
game-build/
|-- BlipnBlop.exe
|-- SDL2.dll
|-- SDL2_mixer.dll
|-- mpg123.dll
|-- wavpackdll.dll
|-- vorbisfile.dll
|-- vorbis.dll
|-- ogg.dll
|-- msvcp140.dll
|-- vcruntime140.dll
|-- vcruntime140_1.dll
`-- data/ (original runtime data layout)
```

`SDL2_mixer.dll` directly requires `vorbisfile.dll`, `mpg123.dll`, and `wavpackdll.dll`; Vorbis additionally requires `vorbis.dll` and `ogg.dll`. The executable's inspected imports require the three selected MSVC C++ runtime DLLs. No unrelated vcpkg or MSVC runtime DLLs are copied. Running the game may create `BlipBlop.log`; regenerating with `deploy` returns the folder to the clean distribution layout.

## Prompt 3 runtime/x64 hardening

The LGX decoder had two confirmed incompatible output paths: its pitch test assumed two-byte pixels while its writes used four bytes, and the tight-pitch version-0 path cast the same output to 16-bit pixels. Padded rows were advanced as though they were tightly packed. On the current SDL surfaces (`BytesPerPixel == 4`), these contradictions could corrupt decoded images and adjacent memory.

LGX output now uses the destination surface's actual `format->BytesPerPixel` and `pitch`. A single row-aware decoder writes 1-, 2-, 3-, or 4-byte mapped pixel values without unaligned typed-pointer writes. Version-1 runs are limited to the remaining surface pixel count, and a zero-length run cannot hang decoding. Pixel conversion tables and the LGX binary format are unchanged. The standalone LGX file overload now uses a checked `std::streamoff` size and owned byte buffer, fixing its leaked allocation and narrowing to `int`.

`halfTone` previously treated every surface as a tightly packed array of 32-bit pixels, ignored bounds, and calculated rows from width rather than pitch. It now clips the requested rectangle to surface bounds, addresses each row through the locked pitch, reads/writes the actual pixel width, halves RGB through `SDL_GetRGBA`/`SDL_MapRGBA`, preserves alpha, and unlocks on every path following a successful lock. This preserves the intended darkening operation across supported SDL formats.

`PauseMenu::ProcessEvent` now returns `MenuType::Main` when no selection is activated. `MenuGame::Update` defines `Main` as continuing with the pause menu, matching the equivalent idle-menu behavior and removing the undefined missing-return path rather than introducing a numeric sentinel.

VS2022 x64 Release and Debug builds both succeeded. The previous `C4715` pause-menu warning is gone, and no compiler warnings were emitted by the changed files. Release deployment regenerated `game-build/`. A standalone launch from an unrelated `%TEMP%` working directory remained alive for 20 seconds and logged the executable-relative `game-build/` resource root. Captured intro frames showed representative decoded artwork with coherent colors, transparency, and geometry and no obvious corruption. Automated input did not reach the menu during the bounded intro sequence, so character selection, first-level loading, pause/resume, and interactive gameplay remain unverified.

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

1. LGX output and halftone access now honor surface pixel width and pitch, but buffer-length validation for LGX data embedded inside larger custom banks remains incomplete because the legacy memory decoder receives no source length.
2. Resource lookup is now executable-relative, but changing the process working directory remains a compatibility bridge for untouched legacy call sites. The pre-main log stream still opens relative to the caller's working directory.
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
- Added centralized executable-relative resource resolution with a documented development fallback.
- Normalized historical separators from level-list and music-bank metadata without changing assets.
- Added focused startup diagnostics for the resource root and missing localized text.
- Added and verified the reproducible CMake `deploy` target and standalone `game-build/` layout.
- Verified development, deployed, and unrelated-working-directory startup smoke tests.
- Hardened LGX output and halftone access for actual SDL formats, pitch, bounds, and lock lifetime.
- Removed the pause-menu missing-return undefined behavior and warning.
- Verified x64 Release and Debug builds plus a 20-second deployed-runtime visual smoke test.

## Known problems

- Interactive runtime behavior is unverified.
- `BlipBlop.log` is still opened relative to the initial working directory before `main`.
- In-memory LGX decoding cannot fully reject truncated input until callers provide buffer lengths.
- PR #9's crash, pitch, unlock, and path fixes are not yet integrated.
- `PauseMenu::ProcessEvent` has a missing-return warning.
- Modern scaling, aspect-ratio handling, high-DPI behavior, and borderless fullscreen are absent.
- There is no automated test suite or current CI workflow.

## Current task

Prompt 3 is complete: LGX surface-format/pitch safety, balanced halftone locking and bounds, adjacent standalone LGX ownership safety, and the pause-menu return fix.

## Next task

Prompt 4 is the rendering and modern-display work described in `IMPLEMENTATION_PLAN.md`. It has not been started.
