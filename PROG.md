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

Result: `out/build-vs2022/vc-projects/Blip_n_Blop_3/Release/BlipnBlop.exe` and the required dependency DLLs were produced. Prompt 3 fixed the `PauseMenu::ProcessEvent` missing-return warning; the Prompt 4 Release build completed without warnings from the changed rendering files.

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

## Prompt 4 modern display presentation

The game continues to render all simulation and artwork into the original 640x480 software surface. `Graphics::Flip` now uploads that surface to one reusable 640x480 streaming texture using the surface's actual pixel format and pitch. The texture is no longer created and destroyed every frame, and nearest-neighbor filtering preserves the original pixel artwork.

Presentation uses `SDL_GetRendererOutputSize`, so destination calculations use drawable pixels rather than DPI-scaled window coordinates. The largest 4:3 rectangle that fits the drawable is centered and the remainder is cleared to black, producing pillarboxing or letterboxing without exposing more world or changing any gameplay coordinate. Windows are centered, resizable, and created with `SDL_WINDOW_ALLOW_HIGHDPI`. SDL is asked for per-monitor-v2 DPI awareness before initialization.

Fullscreen now uses `SDL_WINDOW_FULLSCREEN_DESKTOP` on the existing window and renderer instead of recreating the graphics stack or requesting an exclusive display mode. The last windowed position and client size are saved on entry and restored on exit.

VS2022 x64 Release built successfully and the deploy target regenerated `game-build/`. The deployed executable was launched from unrelated `%TEMP%` working directories. On the available 4K high-DPI display, fullscreen reported a 3840x2160 drawable and a centered 2880x2160 game destination at `(480,0)`. Windowed resize requests of 1280x720, 1920x1080, 2560x1440, and 3840x2160 were exercised; because the test controller itself used DPI-scaled Win32 client coordinates, SDL reported corresponding physical drawables of 2241x1261, 3361x1891, 4481x2521, and 5284x2524. Every logged destination retained 4:3 and remained centered. The last oversized window was constrained by the desktop, as expected.

A bounded visual run skipped the intro and reached the main menu. Captures showed coherent intro/menu artwork and black side bars without obvious stretching or corruption. The fullscreen option entered borderless fullscreen and returned to the saved 1280x720 window client size without crashing. The process remained alive throughout and was stopped by the test harness. Character selection, the first level, multi-monitor movement, changing monitor DPI while running, and mouse-coordinate translation were not interactively tested; current gameplay menus do not consume mouse coordinates.

## Prompt 5 audio and input compatibility

The SDL2_mixer compatibility layer previously never detected missing codecs because its initialization expression tested the wrong mask with incorrect operator precedence. It also returned non-null wrappers after failed WAV/music loads, leaked stream wrappers, ignored stream loop flags, and used an assignment in the sample-loop test. Codec initialization now explicitly requires and reports the MP3 and OGG decoders used by the shipped data, reports the negotiated audio format, propagates load/play failures, preserves intended looping, and releases streams, samples, channels, banks, the mixer, SDL input devices, surfaces, and the graphics object in dependency order.

`gameover.zik` and `tambour.zik` contain 417 zero-padding bytes before valid MP3 frames. SDL2_mixer rejected these original files from offset zero. The compatibility loader now skips only leading zero padding when opening music; the files and bank formats remain unchanged. Music-bank and SFX-bank loading now validate reads, reject invalid entries, retain the old bank on a failed replacement, and log successful resource counts. The legacy volume entry points have no callers in the shipped game and remain behaviorally unchanged rather than guessing a new attenuation mapping.

The input layer previously used SDL joystick instance IDs as fixed-array indexes, wrote buttons without bounds checks, opened more than `MAX_JOY`, duplicated devices when queued add events followed startup enumeration, never closed devices, and used a precedence-broken joystick-key test. Keyboard handling could index before the key buffer for negative keycodes, and its special-key buffer was one byte smaller than its mask allowed. Input now maps instance IDs to bounded legacy slots, safely opens/closes and hot-plugs devices, validates buttons and aliases, clears all held state on focus loss, closes devices at shutdown, and exits blocking key waits when the application is closing. Original aliases, dead zone, mappings, and gameplay polling semantics are unchanged.

VS2022 x64 Release built successfully. A deployed run from an unrelated `%TEMP%` directory opened the real SDL audio device at 44100 Hz, stereo, format `0x8010`; reported both MP3 and OGG codecs available; loaded all four interface-bank tracks including both padded MP3s; and reported successful looping music playback. One attached `Keychron Link` joystick was enumerated safely. The process remained stable during the bounded run. Audio was not acoustically monitored, representative in-level SFX were not reached, and the attempted deeper automated keyboard/focus test was not authorized; those interactive checks are not claimed.

## Windows executable icon

The x64 Windows executable now embeds a multi-resolution icon containing 16, 32, 48, 64, 128, and 256 pixel images. It uses a native-resolution square crop of Blip and Blop from the original main-menu artwork in `data/inter.gfx`, omitting the title and menu text so the characters remain legible at shell-icon sizes; no replacement artwork or unrelated editor/fan icon was used. The lossless crop is retained as `resources/windows/BlipnBlop-source.png`, and `BlipnBlop.ico` is compiled into `BlipnBlop.exe` by `BlipnBlop.rc` through the existing CMake target.

The upstream source for the original artwork is https://github.com/benkaraban/blip-blop/blob/master/vc-projects/Blip_n_Blop_3/data/inter.gfx. The 2002 LOADED Studio Windows release and its cover/screenshots were cross-checked at https://oldgamesdownload.com/game/blip-blop-balls-of-steel-m3w/ and https://gamesdb.launchbox-app.com/games/images/30130-blip-blop-balls-of-steel. Those external images were not copied into the repository.

VS2022 x64 Debug and Release builds both succeeded with the resource compiler producing a 108,140-byte `BlipnBlop.res` for each configuration. Windows' `Icon.ExtractAssociatedIcon` successfully extracted the new 32x32 icon from both generated executables; the extracted PNGs had identical SHA-256 hashes and the Release extraction was visually inspected against the source artwork. The `.ico` was also inspected programmatically and contains all six intended sizes. Windows Explorer and taskbar presentation were not visually tested, so shell icon-cache behavior is not claimed.

## Architecture audit

- **Entry point and startup:** `blip_n_blop_3.cpp` contains `main`. `InitApp` initializes the SDL_mixer-backed FMOD compatibility API, reads `data/bb.cfg` and `data/bb.scr`, loads localized text, initializes graphics/input, creates the 640x480 surfaces, initializes the LGX decoder, and loads fonts/interface banks. `main` calls `Game::go`, then writes high scores and configuration.
- **Main game flow:** `Game::go` in `game.cpp` owns the intro/title/character-selection loop and starts `jouePartie`. Each level runs `gameLoop` until death, completion, skip, or quit. `UpdateRegulator` preserves the original fixed-step simulation at an approximately 11 ms step and may perform zero or multiple simulation updates per rendered frame.
- **Rendering:** `graphics.cpp/.h`, `dd_gfx.cpp/.h`, and `sdl_surface.h` form an SDL2 implementation of the old DirectDraw-shaped API. Gameplay renders to 640x480 SDL surfaces; `Graphics::Flip` uploads that surface to a reusable streaming texture and presents it through a centered 4:3 destination in the renderer's drawable area. The window is centered/resizable/high-DPI-aware and fullscreen uses the desktop mode without recreating the graphics stack.
- **Image/assets:** `picture_bank`, `picture`, `fonte`, and `lgx_packer` decode the custom GFX/LGX and font containers. `LGXpacker` writes pixels into locked SDL surfaces and now honors each surface's actual format and row pitch; embedded decoder input-length validation remains incomplete.
- **Platform abstraction:** SDL2 supplies windows, surfaces, rendering, events, timing, and joysticks. The code retains DirectDraw/DirectInput/FMOD-shaped compatibility interfaces and many global objects. Windows-specific live code is mainly `MessageBox` diagnostics and the GUI subsystem; old Win32 window procedure code is disabled.
- **Input:** `input.cpp/.h` polls SDL events into legacy key/joystick buffers and aliases configured controls through `config.cpp`. It uses raw SDL joystick APIs, fixed arrays, unchecked string copying, and suspicious joystick event indexing. Keyboard behavior must remain the regression reference.
- **Audio/music:** `fake_fmod.cpp` maps the legacy FMOD-facing calls to SDL2_mixer. `SoundBank` reads embedded WAV samples from SFX banks; `MusicBank` loads music paths from MBK banks. Mixer initialization requests OGG/MOD support while shipped music also includes MP3, hence the vcpkg mpg123 feature.
- **Data and level loading:** Runtime assets are a flat `data/` directory containing GFX, SFX, MBK, MP3, fonts, scripts, level lists, and binary LVL files. `Game::loadList` reads `data/bb.lst`; `Game::chargeNiveau` parses fixed-size binary fields and loads the referenced graphics, audio, RPG, enemy, and event data. Embedded backslashes in level/list paths remain a portability concern.
- **Configuration/saves:** `config.cpp` reads and writes a raw binary `data/bb.cfg`; `HiScores` similarly uses `data/bb.scr`. Both mutate the shipped data directory and depend on the current working directory. There is no versioning, validation, migration, or per-user writable location.
- **Build system:** the previous CMake file existed only inside the game directory, used ad-hoc library/path searches, referenced a nonexistent `Engine` directory, did not request C++17, and placed the `WIN32` executable flag on the non-Windows branch. The new root project uses imported vcpkg CMake targets, C++17, an x64-capable VS generator, and a pinned dependency manifest. Editors and historical Visual Studio projects remain out of scope.

## Upstream pull-request findings

### PR #9 — critical crashes and Linux compatibility

PR #9 has two commits and changes LGX decoding, halftone/pause rendering, path normalization, SDL window/scaling behavior, and Linux CMake linking. Its most valuable findings were the 32-bit SDL pixel/pitch mismatch in `LGXpacker`, the missing surface unlock/bounds handling in `halfTone`, and Windows separators embedded in level data. Those runtime fixes have now been reimplemented narrowly and tested rather than copied wholesale.

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
8. Cleanup now runs from normal shutdown and initialization failure, but clean interactive exit and teardown still need broader runtime verification.

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
- Added reusable-texture presentation with centered 4:3 scaling, resizable high-DPI windows, and desktop fullscreen switching.
- Verified a 4K fullscreen drawable, representative window resizes, main-menu presentation, and fullscreen round-trip in the deployed build.
- Hardened SDL2_mixer initialization, legacy padded-MP3 loading, music/SFX bank validation, loop semantics, ownership, and shutdown.
- Hardened keyboard focus handling and bounded legacy joystick enumeration, event mapping, hot-plug, and cleanup without changing mappings.
- Completed Prompt 5 and merged the Prompt 1-5 core-modernization milestone to `main` while preserving its commit history.

## Known problems

- Broader interactive gameplay behavior beyond the main menu and display controls remains unverified.
- `BlipBlop.log` is still opened relative to the initial working directory before `main`.
- In-memory LGX decoding cannot fully reject truncated input until callers provide buffer lengths.
- Multi-monitor behavior and moving a live window between monitors with different DPI settings remain unverified.
- Mouse-to-logical-coordinate conversion is not implemented; no current gameplay/menu path consumes mouse coordinates.
- Character selection, first-level presentation, and pause overlays were not interactively verified during Prompt 4.
- Audio playback was accepted by SDL2_mixer but was not acoustically verified; representative gameplay SFX remain untested.
- The Windows executable embeds a multi-resolution icon derived from the original main-menu Blip and Blop artwork; editor icons were deliberately not substituted.
- There is no automated test suite or current CI workflow.

## Current task

Prompt 5 is complete: audio/resource ownership and legacy input safety are hardened, standalone startup is verified, and the core Prompt 1-5 milestone is merged to `main`.

## Next task

Prompt 6 is the controller/remapping and configuration modernization work described in `IMPLEMENTATION_PLAN.md`. Future development continues directly on `main`; Prompt 6 has not been started.
