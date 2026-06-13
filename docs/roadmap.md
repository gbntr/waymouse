# Roadmap

Public feature roadmap for `waymouse`. Want to contribute? Pick a task below, read `CONTRIBUTING.md`, and open a PR!

## Current Status

| Phase | Feature | Priority | Status | Good First Issue |
|-------|---------|----------|--------|------------------|
| **1 — UX & Polish** | Pointer Manager (theme & size) | **High** | `qa` | ✅ |
| | Shake to Find | **High** | `not started` | ❌ |
| **2 — Core Functionality** | Basic Mouse Config (speed, accel, profiles) | **High** | `not started` | ✅ |
| | Device Separation (mouse vs touchpad) | **Medium** | `not started` | ✅ |

## Feature Details

### Phase 1: UX & Polish

#### Pointer Manager
**What:** Allow users to change the cursor theme and size from a GUI.
**Why:** Wayland users currently have to manually edit env vars or compositor configs.
**Scope:** Theme selection (from `/usr/share/icons` and `~/.icons`), size slider, live preview.
**Backend challenge:** Wayland has no standard cursor theme protocol. We need to investigate Mango (wlroots/dwl) and Niri IPC support, or fallback to `XCURSOR_THEME`/`XCURSOR_SIZE` with session restart hints.

#### Shake to Find
**What:** When the user shakes the mouse rapidly, the cursor temporarily enlarges or gets highlighted to help locate it.
**Why:** Losing the cursor on large/multi-monitor setups is a common UX pain. macOS has this; Linux/Wayland lacks it.
**Scope:** Shake detection via `evdev`/`libinput`, overlay drawing (Wayland Layer Shell), configurable sensitivity/duration/scale.
**Backend challenge:** Wayland clients cannot read absolute cursor position. We need to accumulate relative deltas and draw an overlay surface. This is the project's flagship feature.

### Phase 2: Core Functionality

#### Basic Mouse Config
**What:** Core pointer settings: acceleration speed, acceleration profile (flat/adaptive), natural scroll, left-handed mode.
**Why:** These are the most frequently adjusted mouse settings.
**Scope:** Per-device configuration, apply via compositor IPC (Mango/Niri), fallback to config export if runtime apply is not supported.
**Backend challenge:** Compositors handle input differently. We need IPC/protocols for each compositor. A generic `libinput` fallback is a myth — libinput configs are per-compositor-context.

#### Device Separation
**What:** Distinguish between mouse and touchpad devices. Show different settings for each.
**Why:** Touchpad and mouse have different needs (tap-to-click, palm detection, etc.).
**Scope:** Detect type via `udev` (`ID_INPUT_TOUCHPAD`), filtered UI tabs, touchpad-specific settings (tap, palm detection, disable-while-typing).
**Backend challenge:** Same as Basic Mouse Config — requires compositor IPC or config export.

## How to Pick a Task

1. Check the table above. Anything marked `not started` and `✅ Good First Issue` is fair game.
2. Comment on the issue (or open a draft PR) saying you're working on it to avoid duplicate work.
3. Read the corresponding brief in the issue description (we translate internal briefs to public issues).
4. For backend work, check `rules/architecture.md` and `rules/backend-guide.md`.
5. For UI work, check `src/gui/` and `rules/coderules.md`.

## Future Ideas

These are not yet scoped. Open a discussion if you want to champion one:

- Trackball support
- Drawing tablet / Wacom support
- Custom acceleration curves (per-pixel polynomial)
- Mouse button remapping
- Polling rate configuration (requires root/kernel)
- Sound effects on Shake to Find
- Animated cursor previews

## Legend

- **Status:** `not started` → `in progress` → `pr open` → `qa` → `done`
- **Priority:** `High` = blocking or core value / `Medium` = important, not urgent / `Low` = nice-to-have
- **Good First Issue:** `✅` = relatively isolated, clear scope, good for newcomers / `❌` = complex, requires deep Wayland knowledge
