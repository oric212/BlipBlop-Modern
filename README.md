# BlipBlop-Modern

BlipBlop-Modern is a source-port and compatibility-modernization effort for the original Blip'n Blop. Its goal is to make the existing game run reliably on modern Windows PCs, x64 hardware, and modern displays while preserving the original gameplay, mechanics, timing, levels, assets, and overall feel.

This repository builds upon the original Blip'n Blop source code and the work of its developers and previous contributors. The upstream project is available at [github.com/benkaraban/blip-blop](https://github.com/benkaraban/blip-blop). This modernization does not claim authorship of the original game or its content.

See [PROG.md](PROG.md) for the current verified status and [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md) for the staged modernization roadmap.

## Modernization approach

This remains the original game rather than a remake. Gameplay runs in its original 640x480 coordinate system with the original assets, levels, physics, timing, and rules. The modernization is concentrated in the Windows build, SDL platform layer, display presentation, resource loading, input, audio compatibility, and safe persistence.

## Windows build and standalone runtime

The current target is a native x64 Windows application built with Visual Studio 2022, CMake, SDL2, and SDL2_mixer. Build configuration and dependency versions are recorded in the repository's CMake files and `vcpkg.json`. The executable embeds a multi-resolution icon cropped from the original Blip and Blop menu artwork. The CMake `deploy` target assembles the ignored `game-build/` directory with `BlipnBlop.exe`, the original `data/` tree, and only the SDL, codec, and MSVC runtime DLLs the game needs. This folder can be moved and launched normally without a source-tree working directory or development tools; it is the working distribution layout, not yet a final release archive.

Verified local build and deployment commands are maintained in [PROG.md](PROG.md).

## Modern display support

![Blip & Blop Modern gameplay](https://github.com/oric212/BlipBlop-Modern/releases/download/v1.0.0/Screenshot_1.png)

The game still renders one complete 640x480 frame. The presentation layer scales that final frame to the largest centered 4:3 rectangle that fits the actual drawable area, with black pillarboxing or letterboxing where necessary. It does not expand the visible world or convert individual artwork and gameplay coordinates to the desktop resolution.

Nearest-neighbor sampling keeps the original pixel-art character instead of smoothing it. Scaling is allowed to be fractional so 4:3 uses as much of the display as possible. For example, a 3840x2160 (4K) display presents the game as 2880x2160 at 4.5x, centered with 480-pixel black bars on the left and right. This preserves the original assumptions and composition, works automatically at modern resolutions, and avoids rewriting the UI, camera, gameplay coordinates, or artwork. It does not create additional graphical detail, fractional nearest-neighbor output is not pixel-perfect integer scaling, and widescreen displays retain unused side space. Strict integer scaling is not currently offered.

The window is resizable and high-DPI aware. A first launch with no valid configuration defaults to borderless desktop fullscreen; a saved windowed or fullscreen preference is respected on later launches. Fullscreen returns to the saved windowed size and position. These presentation changes do not alter the 640x480 simulation or gameplay timing.

## Keyboard and controller input

The original keyboard controls remain active and configurable. Recognized SDL game controllers work at the same time, so players can switch between devices without restarting:

| Action | Controller input |
| --- | --- |
| Move | D-pad or left stick |
| Fire | X |
| Jump / menu confirm (player 1) | A |
| Super attack | B |
| Pause / skip intro and cutscenes | Start (player 1) |

The first two recognized controllers are assigned to players 1 and 2. Keyboard Escape also skips the intro and cutscenes. Hot-plug and removal are handled by SDL, and devices that are not in SDL's controller mapping database retain the legacy raw-joystick path. Analog sticks intentionally produce the same digital movement states as the keyboard; they do not change acceleration, physics, or movement speed.

Menus show a compact prompt beside the current selection and switch between keyboard, mapped-controller, and raw-joystick wording according to the most recently used device. In menus with a Return or Resume action, the mapped controller's B button goes back; A selects the focused item. The level briefing uses the same device context for its existing start prompt. Small stick drift is filtered for prompt switching without changing the original gameplay movement dead zone. The key-remapping screen omits the confirm hint because it waits for a key rather than the normal menu-confirm action.

## Configuration and writable data

Configuration, high scores, and `BlipBlop.log` are stored in SDL's per-user preference directory (`%APPDATA%\BlipBlopModern\BlipnBlop\` on Windows). On first use, existing `data/bb.cfg` and `data/bb.scr` files are copied forward when present, but existing per-user files are never overwritten by migration. The original binary formats and semantics remain compatible; malformed files fall back to defaults, and successful saves use a temporary file followed by replacement to reduce the risk of losing the previous file during an interrupted write.

Runtime assets remain read-only in the executable-relative `data/` directory. Startup reports useful paths for missing resources and retains a source-tree working-directory fallback for development builds.

## Compatibility and robustness

The modernization includes pitch- and pixel-format-safe image decoding, centered aspect-correct SDL rendering, checked controller/joystick lifetime handling, focus-loss input cleanup, and stricter SDL2_mixer initialization, codec checks, and resource-error handling. Original padded MP3 assets are supported without modifying the shipped files. Cleanup and error reporting have been strengthened around initialization and normal shutdown.

Verification is deliberately reported conservatively in [PROG.md](PROG.md). Successful compilation or a bounded launch is not treated as proof of untested gameplay, controller hardware, audio output, or Windows shell behavior.

---

# Original README

Linux: ![linux](https://travis-ci.org/Vermeille/blip-blop.svg?branch=master)
Windows: [![Build status](https://ci.appveyor.com/api/projects/status/n8rv6hstgmlx4j0a/branch/master?svg=true)](https://ci.appveyor.com/project/Vermeille/blip-blop/branch/master)

# Forker's notes

I am mainly trying to modernize the code. Both in terms of C++ features, and
features (mainly trying not to lock the game in 640x480, adding a few shaders
and stuff).

As the code is kinda old, there's a lot of revamping to do, and a lot of
optimizations done back in the days shouldn't be relevant anymore. The code
contains a FUCKTON of global variables, to the point where I'm in awe that
somehow the game works flawlessly. The quantity of global variables declared
and handled all over the places is truly unmanageable.

As a french, I can work my way through the code, and although I'll be
translating comments as I skim through it, there are some really funny gems
that I'd rather not translate as they'd break the stupidity that made Blip'n
Blop famous. I would rather have a separate lexicon for the fellas that can't
french.

This project is HUGE for two students. Kudos to you guys, it's a LOT of work,
and even if the quality is, uh, debatable, the game actually works flawlessly
and you undeniably got the work done. I can hardly believe every single error
has been handled, even memory allocation failures, so that the game wouldn't
crash (some, including me, would say that it's easier for a developper to
handle a crash, rather than propagating / circumventing silently an error that
would still disallow the gamer to properly play the game). Congrats and thanks!

## Lexicon

- Couille: testicle. That's what the players are, and the code is in
  couille.{cpp,h}
- schnuff: "thing". It's not french, it's a made up word, don't look at me like
  that
- snuf: Smurf?
- Tête de turc: a person that gets all the hate. In the game engine, the
  enemies are fighting one of the two couilles at a time, who is then called
  "la tête de turc". See `Game::tete_turc`

## When I laughed

- "couille" in the code
- "schnuff"
- Tête de turc. That's kinda informal and slang.
- `SuperListe` (=AwesomeList). And it's actually really not awesome, it's a
  `void*` based linked list with an embedded STATEFUL iterator. I'm sad I have
  to remove it to use actual vectors.
- `SuperListe::vide_porc`: empty (like a?) pork, meaning "in an unclean way".
  IDK why, the code is exactly the same as the unporked version.
- `SuperListe::supprime_porc`: removes an element of the list without freeing
  its memory (ie, it's a non-owning list)
- 'Gère les messages. Eh oui, Windows pue du cul' (Handles messages. Yeah,
  Windows' ass stinks, nota: Ben Karaban apparently works for MS now)
- A variable is named 'glorf'.
- "Gère le scrolling avec le super buffer qui marche bizarrement sur cette
  merde de GeForce." (Handles scrolling with the awesome buffer that works
  weirdly on this shitty GeForce)
- After a failed `new`, in a log message: "Nani? Not enough memory???"
- "Affiche les têtes de con" (Show the fucktard' faces)
- "Destructeur -> On est pas des Brujahs..."

## Significant differences

- The menus were a big pile of switch case in a single class. The pause menu
  and the title screen menu were just a big copy paste. I cleaned up all that.
  Take a look at `menus/`. Pause menu and title screen menu are now the same
  thing
- Added a "TOGGLE FULLSCREEN" option.
- Fixed a bug that prevented unix like os from playing. Some level descriptions
  contained path containing backslashes that failed. Now backslashes are
  handled.
- MusicBank has been reworked a lot
- For some reasons, there were (and still are) too many nonsensical bridges in
  this code. There's a Direct3D simulated API implented in SDL, a FMOD fake API
  implemented in SDL mixer, and a windows API implemented with Linux syscalls.
  I guess those came from the first revamping of the code in 2014; the
  developper must have thought that implementing a bridge would be easier than
  manipulating the code.

# Blip'n Blop
This is the source code of Blip'n Blop, a free video game for the PC released in 2002. Years after the game got released, some enthusiastic programmers asked us to open source it and here we are!

A few things to keep in mind:
- this was writen when we were still students so the code quality and the (lack of) architecture can be disturbing
- the code is mostly written in a terrible mix of french and english, which should be kind of akward to read for non french speaking people (actually, it's kind of awkward even for french people! :p )
- the various editors can be quite complicated to get working because they relied on various cryptic INI files

