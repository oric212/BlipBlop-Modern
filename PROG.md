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

VS2022 x64 Debug and Release builds both succeeded with the resource compiler producing an 89,880-byte `BlipnBlop.res` for each configuration. Windows' `Icon.ExtractAssociatedIcon` successfully extracted the new 32x32 icon from both generated executables; the extracted PNGs had identical SHA-256 hashes and the Release extraction was visually inspected against the source artwork. The `.ico` was also inspected programmatically and contains all six intended sizes. Windows Explorer and taskbar presentation were not visually tested, so shell icon-cache behavior is not claimed.

## Prompt 6 controller and writable-data modernization

Recognized devices now use SDL2's `SDL_GameController` abstraction and built-in mapping database. The first recognized controller drives player 1 and the second drives player 2. D-pad and left-stick axes map to movement, X maps to fire, A maps to jump, B maps to the super attack, player-1 A confirms menu and character selections, and player-1 Start raises a one-shot pause action. Keyboard aliases remain active simultaneously. Input records whether the most recent activity came from the keyboard or a controller for future prompt rendering. Devices that SDL does not recognize as game controllers retain the bounded raw-joystick path from Prompt 5.

Startup enumerates each SDL joystick device once and opens it as a game controller when supported, otherwise as a raw joystick. SDL instance IDs identify removal events; disconnect closes the correct owning handle and clears its state, while a later add event fills the first free slot. Duplicate instance IDs are rejected and controller/open failures remain non-fatal to keyboard play. All handles close before SDL shutdown.

Writable state moved from executable/source-tree `data/` to the SDL preference directory, which is `%APPDATA%\BlipBlopModern\BlipnBlop\` on the verified Windows environment. `bb.cfg`, `bb.scr`, and `BlipBlop.log` now live there. If a destination config or score file is absent, an existing executable/source-tree legacy file is copied once; existing per-user data is never overwritten by migration. Both the historical 61-byte config and current 62-byte config are accepted, malformed/truncated config falls back wholly to defaults, and malformed scores fall back to the original default table. Successful config and score saves use temporary files followed by a replace operation so an incomplete write does not destroy the previous file. `BLIPBLOP_USER_DATA_DIR` provides an explicit isolated-directory override for development and verification.

VS2022 x64 Debug and Release builds compiled successfully. In isolated temporary user-data tests, an empty directory received the existing 61-byte config and 244-byte score file, both loaded successfully, and a normal window-close saved the current 62-byte config and 244-byte score file. A second normal launch loaded those saved files and exited successfully. Separate three-byte malformed config and score fixtures produced explicit fallback diagnostics and were replaced on clean exit with valid 62-byte and 244-byte files. The attached `Keychron Link` was safely enumerated as a raw joystick; no SDL-recognized game controller was available, so standardized mappings, physical gameplay/menu input, and live controller disconnect/reconnect were not physically tested.

The Release `deploy` target regenerated `game-build/` with the current executable, 128 original data files, SDL2/SDL2_mixer, required codec DLLs, and MSVC runtime DLLs. SHA-256 comparison confirmed `game-build/BlipnBlop.exe` is byte-for-byte identical to the final Release executable (`D3277D4DE7052A6BF5FEBD60FED15262B6508CFFBB396F741798003EF890FDF2`). Extracting its associated icon produced the same verified icon hash as the pre-Prompt-6 icon build. A clean deployed run from an unrelated working directory selected `game-build/` as its executable-relative resource root, migrated writable state into an isolated user-data directory, and exited normally with config and scores saved.

## Prompt 7 contextual input prompts and documentation

The existing menu selection box now shows a compact device-aware confirm hint beside the focused item: `ENTER` for keyboard/mouse activity, `[A]` for an SDL-mapped game controller, and `JOY` for the legacy raw-joystick path. The hint uses the original small menu font and reserved space inside the existing shaded box, so it does not replace menu art or add a gameplay HUD element. The key-remapping menu suppresses the hint because that screen waits for a key rather than the normal confirm action. The existing level-briefing start message similarly changes between key, A-button, and generic joystick-button wording without adding another overlay.

Keyboard, mouse, mapped-controller, and raw-joystick events update the last-used-device context immediately, while both keyboard and controller gameplay inputs remain active simultaneously. Axis motion must cross a separate 12000-unit threshold before changing prompt context, preventing ordinary stick noise from repeatedly taking over the prompt. The original 4200-unit gameplay dead zone and all gameplay mappings remain unchanged.

The modernization introduction in `README.md` now documents the project's preservation philosophy, native x64 Windows and standalone layout, 640x480 final-frame presentation, centered largest-fit 4:3 scaling, fractional nearest-neighbor behavior and its tradeoff, controller and keyboard behavior, per-user persistence, and current compatibility hardening. The original README remains intact below the separated modernization material.

VS2022 x64 Debug and Release builds both succeeded, and the Release `deploy` target refreshed `game-build/`. Its executable is byte-for-byte identical to the Release build (`B190D1A81B6F84A2FC5933BD22F844502625C91C9577BB061DD1969F0C977AD1`), and Windows successfully extracted the embedded 32x32 associated icon from both configurations. A standalone run from an unrelated `%TEMP%` working directory remained alive for 12 seconds, accepted a normal window-close request, exited with code 0, and wrote valid config and score files to an isolated per-user test directory.

The native 640x480 title menu was captured during development and visually inspected: the keyboard `ENTER` prompt was legible within the existing shaded selection area and did not overlap the focused label, title art, characters, or other menu entries. The capture-only instrumentation and image were removed before the final builds. No prompt was added to the gameplay HUD or character-selection artwork because there was no existing unobtrusive hint location. Code inspection verified mapped-controller and raw-joystick prompt selection, but no SDL-recognized physical controller was available, so live controller prompt switching, mappings, and reconnect behavior remain unverified. Gameplay, controller input, and audio were not manually exercised in this bounded run.

## Prompt 8 focused stability and safety audit

The audit used targeted source searches for manual allocation, unchecked C-string operations, binary reads, and SDL/audio ownership; a full MSVC `/analyze` Debug rebuild with the Native Recommended Rules; a separate MSVC AddressSanitizer RelWithDebInfo build; and normal Debug/Release builds plus bounded runtime checks. Runtime paths, config/high-score validation and replacement writes, controller/joystick lifetime handling, audio ownership, GFX/SFX/font/level loaders, cinematic parsing, and standalone DLL deployment were inspected. This was a focused audit, not a claim that the entire legacy codebase is memory-safe.

Three concrete unsafe-input defects were fixed. Startup previously concatenated arbitrary command-line arguments into a fixed 512-byte stack buffer; switches are now examined directly from `argv`. The cinematic command reader could overrun its command name, argument count, and per-argument buffers and passed signed `char` values to `tolower`; it now rejects overlong lines, commands, argument lists, and arguments before copying and performs defined character conversion. `PictureBank` previously trusted signed picture/blob sizes, allocated from unchecked values, decoded truncated reads, partially replaced the active bank on failure, and released live restore surfaces before a replacement was validated. It now bounds counts against the file, rejects non-positive or overlong entries, uses owned byte buffers, commits a newly loaded bank only after complete success, verifies restore counts, and validates replacement surfaces before releasing old ones.

MSVC `/analyze` completed with 255 warnings and no errors. Most were broad uninitialized-member reports in legacy gameplay/entity classes, plus four third-party SDL fallthrough warnings; they were not mechanically changed because many objects receive fields through level/event construction and speculative initialization could alter behavior. Notable findings left for evidence-driven follow-up include a no-op video-buffer fallback loop, a dormant precache implementation that mistakes `fseek`'s return value for a length, two possible uninitialized laser coordinates, and loaders whose inner LGX/font/level structures still lack complete source-length validation. The normal compiler emitted no warnings from the changed files.

The separate x64 AddressSanitizer target compiled and linked successfully. ASan initialized, and a corrected hidden run remained alive for 15 seconds without an observed sanitizer termination; the harness then stopped it. This limited startup exercise did not reach gameplay and is not proof of leak freedom. No confirmed resource leak was found or claimed, and repeated level/death/pause/controller/audio-transition stress was not practical without reliable gameplay automation. The standalone DLL set remains the intentional SDL2, SDL2_mixer, five codec DLLs, and three MSVC runtime DLLs documented above; no new dependency or loading behavior was introduced.

Final VS2022 x64 Debug and Release builds succeeded, and `deploy` refreshed `game-build/`. A deployed run from an unrelated `%TEMP%` working directory remained alive for 20 seconds, accepted a normal close, exited with code 0, and wrote config and score files to an isolated user-data directory. The deployed and Release executables had the same SHA-256 hash (`89E96F6073605F48BFFFC7E21994C44DEA002E9AC527EF327F4A1AD98144BD20`). Audio/render/input initialization completed far enough for the process to remain stable, but output was not manually observed; no mapped physical controller was available.

## Prompt 8B menu/runtime follow-up

The normal deployed launch that appeared to contradict the fullscreen-first default used the real `%APPDATA%\BlipBlopModern\BlipnBlop\bb.cfg`: it was a valid 62-byte config whose fullscreen byte was `0` (windowed). Startup loads that file before window creation, and `Graphics::SetGfxMode` applies the loaded flag without inversion. Overriding this file would discard a saved preference. The Prompt 8B config change sets fullscreen for absent/malformed config and for the bundled 61-byte legacy config, which has no fullscreen field; the bundled `game-build/data/bb.cfg` is copied into an empty user-data directory on first use. An isolated deployed launch with no preexisting config produced a borderless desktop-fullscreen window and logged a 3840x2160 drawable with centered 2880x2160 game presentation. Explicit 62-byte windowed/fullscreen configs opened in matching window modes; malformed and 61-byte legacy configs opened fullscreen. After normal exit, all four test configs were 62 bytes with the expected fullscreen byte. Existing per-user files were not replaced by the bundled file. The two display modes were verified as actual Win32 window styles and renderer output, not inferred solely from config bytes.

Menu navigation is shared in `Input`: up/down and left/right use one-move-on-press with a 350 ms delay and 120 ms held repeat, and confirm is edge-triggered. Main, Start, Options, both player key-binding lists, and Pause use it. The controls submenu had an uninitialized browsing/waiting state; it now starts in browsing state. The mapped controller's B button is a shared edge-triggered back action where the UI already has Return/Resume: Start to Main, Options to its parent, either key-binding list to Options, and Pause to gameplay. The root Main menu has no back action. In the key-binding lists, controller A activates only Return, so it cannot accidentally become a keyboard binding; the existing wait-for-key behavior is unchanged. Left/right toggles VSync only while that row is selected. All regular menu prompts are drawn only for the focused entry; the key-binding lists retain their deliberate prompt suppression. The title background is cleared and redrawn each frame to prevent old prompts accumulating, without the rejected opaque prompt gutter.

The cinematic player now consumes the same player-one controller Start event used for gameplay pause to skip a scene. Keyboard Escape already skipped scenes and remains supported. Consuming the Start event at the skip check prevents it from carrying into a later gameplay pause. This applies consistently to the intro and other cinematic scenes.

The user manually verified main-menu D-pad, left stick, A, held repeat, keyboard switching, and the previously reported audio/prompt behavior in Prompt 8B. In the final deployed follow-up, the user reported that controller navigation and B/back worked in Start, Options, both player Controls lists, and Pause; the live fullscreen toggle in Options worked; and controller Start and keyboard Escape both skipped the intro. The separate automated Options-menu key sequence was inconclusive and is not counted as a verification. Debug x64 and Release x64 builds succeeded. `deploy` refreshed `game-build/`; the Release and deployed executable SHA-256 hashes matched (`9F421E9AEF4526199ECEBD6F8CA3EB6665E52AAE055CFC35CD00C8743D07CF4F`), and Windows extracted a 32x32 associated icon. A final deployed run from an unrelated `%TEMP%` working directory remained alive for eight seconds, accepted normal window close, exited with code 0, used the executable-relative `game-build` data root, and saved a valid 62-byte config. Acoustic output, gameplay, and the revised prompt appearance were not independently verified in this pass. Saved-value startup was verified with controlled config fixtures; a manual toggle-exit-restart round trip awaits confirmation.

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
2. Resource lookup is executable-relative, but changing the process working directory remains a compatibility bridge for untouched legacy call sites.
3. The raw binary config format remains implementation-sized for backward compatibility; checked reads accept the known historical/current layouts and reject malformed files.
4. The renderer recreates a texture every frame, lacks error propagation, and does not preserve 4:3 output on arbitrary windows/displays.
5. SDL-recognized controller mappings are implemented, but physical standardized-controller and hot-plug behavior still needs verification with suitable hardware.
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
- Added standardized SDL game-controller defaults while retaining simultaneous keyboard and raw-joystick compatibility.
- Moved config, high scores, and logs to SDL's per-user directory with conservative migration, checked legacy-format loading, and recoverable writes.
- Added contextual keyboard, mapped-controller, and raw-joystick prompts to existing menu and briefing UI locations.
- Expanded the modernization README with current display, input, persistence, standalone-runtime, and robustness behavior while preserving the original README.

## Known problems

- Broader interactive gameplay behavior beyond the main menu and display controls remains unverified.
- In-memory LGX decoding cannot fully reject truncated input until callers provide buffer lengths.
- Multi-monitor behavior and moving a live window between monitors with different DPI settings remain unverified.
- Mouse-to-logical-coordinate conversion is not implemented; no current gameplay/menu path consumes mouse coordinates.
- Character selection, first-level presentation, and pause overlays were not interactively verified during Prompt 4.
- Audio playback was accepted by SDL2_mixer but was not acoustically verified; representative gameplay SFX remain untested.
- The Windows executable embeds a multi-resolution icon derived from the original main-menu Blip and Blop artwork; editor icons were deliberately not substituted.
- No SDL-recognized controller was available for physical mapping or hot-plug verification; only raw-device enumeration was exercised.
- There is no automated test suite or current CI workflow.

## Prompt 8C deferred safety pass

The LGX memory decoder now receives the actual blob length from GFX/font callers, validates its header, dimensions and compressed runs, and rejects truncated streams without returning a partially decoded surface. Font loading checks glyph lengths against file bounds and commits a replacement only after all glyphs decode. Picture-bank restore similarly stages replacement surfaces before changing the live bank. Level loading checks the fixed file layout, bounded counts, names, decor indices, platform/wall reads and event records; partially populated level arrays are initialized for safe cleanup, and a failed level load now invokes that cleanup. The level-list count off-by-one and an initially uninitialized list value were fixed. These checks preserve all 12 shipped level layouts and eight shipped fonts, including original fonts whose height field is zero.

Confirmed deferred defects: the precache loop used `fseek`'s return code as a file length and therefore never read the intended data; it now reads the file in bounded 8 KiB chunks. The old video-buffer fallback loop's initial condition was false, so its allocation body was unreachable; it has been reduced to the same effective SDL system-buffer path. Laser directions 16/17 map to an unhandled half-direction 8, which could leave collision coordinates uninitialized; that path is now explicitly non-colliding. A HUD weapon-image switch also had a real uninitialized pointer path for an invalid weapon ID and now returns without drawing.

The full MSVC `/analyze` rebuild also reported a concrete `showPE` post-level results path where player-display flags and coordinates could be read without assignment if neither player branch ran. Only those locals were initialized. The final analysis build succeeded with 247 warnings; the `showPE` `C6001`/`C6011` and precache `C6262` warnings were absent. A `C6054` warning remains at the level-name copy because the analyzer does not establish the preceding NUL check in `readName`; the check is present, but malformed-level runtime testing remains incomplete. Other reported class-field `C26495` warnings were not mass-initialized: several belong to legacy initialization protocols and need separate path-specific evidence. The prior precache 64 KiB stack warning was removed by using an 8 KiB read chunk.

VS2022 Debug x64 and Release x64 builds succeeded. The RelWithDebInfo AddressSanitizer build succeeded and launched through SDL/audio/interface initialization and the first presentation, but did **not** reach gameplay; it cannot establish first-level or transition safety. A deployed Release startup remained alive for eight seconds with executable-relative data and normal initialization logged. The deployed executable SHA-256 matched the final Release binary. `dumpbin` showed the executable imports SDL2/SDL2_mixer and the expected MSVC runtime, and the mixer imports the packaged codec DLLs. Neither manual gameplay stress nor a sustained memory-growth study was completed; the user will test gameplay later. Legacy analyzer warnings outside the touched/runtime-relevant paths are not being mass-silenced.

## Current task

Prompt 8C code and build work is complete. Gameplay-level sanitizer coverage and repeated manual transitions remain unverified.

## Next task

Complete the deferred manual gameplay/stress checks before treating runtime stability as verified. Prompt 9 packaging/release work remains deferred.
