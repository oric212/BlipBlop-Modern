# BlipBlop-Modern Implementation Plan

Each prompt is intentionally bounded. Preserve the 640x480 simulation, fixed-step timing, gameplay data, and keyboard behavior throughout. Update `PROG.md`, build/test what changed, and make one focused commit per prompt.

## Prompt 2 — Startup and resource loading

Make startup independent of the process working directory. Resolve read-only game data relative to the executable, retain compatible fallback behavior where useful, and provide clear diagnostics for missing/corrupt startup assets. Verify launches from the source directory and an unrelated directory. Do not change file formats, level parsing, gameplay, config locations, rendering policy, or packaging yet.

## Prompt 3 — Focused x64 and runtime safety fixes

Reproduce and fix the known startup/runtime hazards, beginning with LGX pixel-format and surface-pitch handling, locked-surface cleanup, halftone bounds/unlock behavior, and the pause-menu missing return. Audit only directly adjacent fixed-size reads/casts/ownership required for stable x64 execution. Use PR #9 as evidence, not as a patch to merge. Add targeted non-gameplay tests or fixtures where practical and compare decoded assets before/after.

## Prompt 4 — Rendering and modern displays

Keep a 640x480 logical render target while implementing reliable SDL2 presentation for resizable windows, fullscreen-desktop, high-DPI displays, and 1080p/1440p/4K output. Preserve 4:3 with centered pillarbox/letterbox regions; do not expose more game world or alter camera/simulation coordinates. Avoid per-frame texture recreation where safe. Verify windowed/fullscreen transitions, scaling, pause overlays, cinematics, and representative levels.

## Prompt 5 — Audio and keyboard/input reliability

Audit SDL2_mixer initialization, codec availability, music/SFX bank loading, channel ownership, volume, shutdown, and failure diagnostics. Validate music and sound triggers without changing their timing or selection. Harden SDL keyboard and joystick event handling, focus changes, bounds, hot-plug lifecycle, and cleanup while preserving all original keyboard mappings and gameplay response. Do not add remapping/controller UX yet.

## Prompt 6 — Configuration, saves, and controller modernization

Design a backward-compatible migration for config/high-score writes to an appropriate per-user directory, keeping existing files readable and preserving their semantics. Add validation and atomic/recoverable writes without silently inventing incompatible formats. Add SDL game-controller support and persistent remapping as an additive layer; retain keyboard defaults and digital gameplay behavior, with no analog movement or physics changes.

## Prompt 7 — Packaging and continuous integration

Add GitHub Actions for Windows VS2022 x64 configure/build and appropriate lightweight checks. Produce a standalone folder/archive containing the executable, only required runtime DLLs/codecs, and `data/`. Ensure a clean machine can extract and double-click the executable without developer tools or a special working directory. Document exact local build and packaging commands; do not claim runtime coverage CI cannot provide.

## Prompt 8 — Full compatibility and release verification

Run and record the complete manual verification matrix: launch, menus, character selection, first and additional levels, movement/jump/shooting, enemies, collision, audio, pause/resume, windowed/fullscreen, scaling/aspect ratio, configuration/high-score persistence, controllers, and clean exit on Windows 10/11 where available. Compare timing and gameplay behavior with the reference build, fix only confirmed compatibility regressions, finalize release notes/credits, and prepare the Blip'n Blop Modern 1.0 release candidate.
