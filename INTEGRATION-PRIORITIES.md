# Deliverable QML → App Integration Priorities

Source: [`Renders/Deliverable and surface prioritization/qml/`](Renders/Deliverable%20and%20surface%20prioritization/qml/)  
Spec: *Polomodoro — Visual Specification v1.0* (see `README-qml.md`)

This document maps the **40-file deliverable QML tree** to the current app (`qml/`, 17 files) and ranks what to port first.

---

## Executive summary

The deliverable is a **complete design system + UI shell**, not a drop-in replacement yet. It assumes a **richer C++ API** than we have today. Integration should proceed in **4 phases**: foundation → core surfaces → task UX → advanced features.

**Biggest wins for your Spotify/layout complaints (Phase 1):**
- `SpotifyDrawer.qml` — collapsed **34px edge tab** (`MUSIC ◂`), expands to 340px; WebEngine **never unloads**
- `MainWindow.qml` — timer in **frosted panel**, Spotify **pushes** layout (not overlay)
- `WindowChrome.qml` — fixed control order, wordmark, proper mode switcher
- `Theme.qml` + `IconButton.qml` + SVG icons — replaces emoji/ChromeButton

---

## File inventory (deliverable)

| Category | Files | In current app? |
|----------|-------|-----------------|
| **Foundation** | `Theme.qml`, `qmldir`, `main.qml` | Partial (`Theme.qml` stub only) |
| **Windows** | `MainWindow`, `ProgressBarView`, `CompactView`, `StackLayoutLike` | Yes (3/4, different impl) |
| **Timer** | `TimerDial`, `ModeSwitcher` | `TimerRing` only |
| **Tasks** | `TaskRow`, `TaskRowMenu`, `TaskTreeView`, `TaskMenuDrawer`, `TaskEditor`, `TargetTimeEditor`, `DateTimeField` | Partial stubs |
| **Active strip** | `ActiveTasksBar` | Basic version |
| **Bar mode** | `OverallProgressBar`, `SubtaskProgressDropdown`, `NowPlayingLabel`, `SpotifyMediaControls` | Basic versions |
| **Spotify** | `SpotifyDrawer` | Different (hide/show, no edge tab) |
| **Background** | `BackgroundView`, `BackgroundSourceToggle` | Partial |
| **Chrome** | `WindowChrome` | Different |
| **Settings** | `SettingsView`, `SettingRow` | **Missing** |
| **Timeline** | `DayTimeline`, `TimelineBlock` | **Missing** |
| **Primitives** | `IconButton`, `PillButton`, `PoloTabBar`, `PoloToggle`, `PoloSegmented`, `PoloTextField`, `PoloComboBox`, `ProgressTrough`, `FieldLabel`, `Divider` | **Missing** (only `ChromeButton`) |

---

## C++ API gaps (must extend controllers)

The deliverable QML expects properties/methods we **do not expose yet**:

### TimerController
| Expected | Current |
|----------|---------|
| `progress` (qreal 0–1 for ring arc) | ❌ |
| `phaseLabel`, `cycleLabel`, `cycleIndex` | ❌ |
| `minutes`, `seconds` | ❌ (only `formattedTime`) |
| `isPaused` | ❌ |
| `toggle()`, `toggleMode()` | ❌ (separate start/pause) |

### TaskController
| Expected | Current |
|----------|---------|
| `activeCount`, `pendingCount`, `futureCount` | partial (`activeTaskCount` only) |
| `activeChips`, `activeOverflowCount` | ❌ |
| `proxyFor(bucket)` / per-bucket models | single model + `setBucketFilter` |
| `overallRatio`, `overallTargetMs`, `combinedActiveLabel` | partial (`overallProgressRatio`) |
| `promote`, `demote`, `parentChoices` | ❌ |
| `requestEdit`, `requestCreate`, `loadInto` | ❌ |
| Model roles: `progressLabel`, `badgeText`, `startable`, `overdue`, `overflowRatio`, etc. | minimal roles |

### SpotifyController
| Expected | Current |
|----------|---------|
| `attach(view)` on load success | `attachWebPage` via QML |
| `ready`, `premiumRequired` | ❌ |
| Optimistic play state + reconcile | basic `isPlaying` poll |

### WindowLayoutManager
| Expected | Current |
|----------|---------|
| `viewMode` as **int** 0/1/2 | **string** `"expanded"/"bar"/"compact"` |
| `setMode(int)`, `togglePip()` | `setViewMode(string)`, `cycleViewMode()` |
| `rememberGeometry(x,y,w,h)` | `saveGeometry(mode,x,y,w,h)` |
| `x`, `y`, `width`, `height` properties | `geometryForMode()` |

### New models (not started)
- **`DayTimelineModel`** — week strip, planned/logged blocks, `nowMs`
- **`PaletteDeriver`** — accent from album art (worker thread, OKLCH)

### Assets (not in repo)
- `resources/fonts/` — Geist + Geist Mono (bundled, not system)
- `resources/icons/*.svg` — play, pause, stop, prev, next, pin, settings, etc.

---

## Integration phases

### Phase 1 — Foundation + layout fix (highest impact, ~1–2 days)

**Goal:** Match the screenshot problems: stuck Spotify, bad icons, wrong layout.

| Priority | Deliverable file | Action |
|----------|------------------|--------|
| P0 | `Theme.qml` + `qmldir` | Replace stub; register `Polomodoro` module in CMake/qrc |
| P0 | `IconButton.qml` + `resources/icons/*.svg` | Add SVG assets; remove `ChromeButton` |
| P0 | `SpotifyDrawer.qml` | **Replace** current drawer — edge tab + expand |
| P0 | `windows/MainWindow.qml` | Frosted timer panel, `DayTimeline` slot (can hide until Phase 4) |
| P0 | `WindowChrome.qml` | Fixed right-side order: pin → bg → modes → settings |
| P1 | `TimerDial.qml` | Replace `TimerRing`; needs `TimerController.progress` |
| P1 | `ModeSwitcher.qml` + `PillButton.qml` | Replace Start/Pause/Reset row |
| P1 | `StackLayoutLike.qml` + `main.qml` | Keep WebEngine alive across mode switches |
| P1 | `Divider.qml`, `FieldLabel.qml` | Bar mode separators |

**C++ work in Phase 1:**
- `TimerController`: add `progress`, `toggle()`, `toggleMode()`, `isPaused`, `cycleLabel`
- `WindowLayoutManager`: int `viewMode`, `setMode(int)`, `togglePip()`, geometry properties
- Register `Theme` singleton

---

### Phase 2 — Task surfaces (~2–3 days)

| Priority | Deliverable file | Action |
|----------|------------------|--------|
| P0 | `TaskMenuDrawer.qml` | Qt `Drawer` overlay (not full-screen Rectangle) |
| P0 | `PoloTabBar.qml` | Tab counts from `TaskController` |
| P0 | `TaskTreeView.qml` | Nested list + empty states per bucket |
| P0 | `TaskRow.qml` + `ProgressTrough.qml` | Full lifecycle UI + indent rail |
| P1 | `ActiveTasksBar.qml` | Chip layout, max 4 + overflow |
| P1 | `TaskRowMenu.qml` | Right-click promote/demote/complete |
| P2 | `TaskEditor.qml` | Wire to C++ save |

**C++ work in Phase 2:**
- `TaskController`: bucket proxy models OR `QSortFilterProxyModel` per tab
- `TaskTreeModel`: all roles from README (`progressLabel`, `startable`, `overdue`, …)
- `activeChips` as `QVariantList` for Repeater
- `promote`/`demote` in `TaskTree`

---

### Phase 3 — Bar mode + Spotify polish (~1–2 days)

| Priority | Deliverable file | Action |
|----------|------------------|--------|
| P0 | `windows/ProgressBarView.qml` | **Window height** animates (not inner clip) |
| P0 | `OverallProgressBar.qml` | Σprogress/Σtarget + pulse fallback |
| P0 | `SubtaskProgressDropdown.qml` | Nested active subtree |
| P1 | `NowPlayingLabel.qml` | Marquee scroll |
| P1 | `SpotifyMediaControls.qml` | Optimistic state + SVG icons |
| P1 | `BackgroundView.qml` | Blur, crossfade, phase tint |
| P1 | `BackgroundSourceToggle.qml` | Icon in chrome |

**C++ work in Phase 3:**
- `SpotifyController.premiumRequired` detection
- `SettingsController.barDropdownExpanded` drives window height from QML

---

### Phase 4 — Settings, scheduling, timeline (~3+ days)

| Priority | Deliverable file | Action |
|----------|------------------|--------|
| P1 | `SettingsView.qml` + `SettingRow.qml` | Modal popup, instant save |
| P1 | `DateTimeField.qml` | MonthGrid, UTC storage |
| P1 | `TargetTimeEditor.qml` | h/m presets |
| P2 | `DayTimeline.qml` + `TimelineBlock.qml` | Requires `DayTimelineModel` |
| P3 | Palette derivation from Spotify art | Worker + `Theme.accent` update |

**C++ work in Phase 4:**
- Full `SettingsController` surface
- `DayTimelineModel` + session/planned block queries
- Notifications (freedesktop) for target/end date

---

## What to copy vs adapt

| Copy verbatim | Adapt (API mismatch) |
|---------------|----------------------|
| `Theme.qml`, primitives (`Divider`, `FieldLabel`, `ProgressTrough`) | `main.qml` (wire our context properties) |
| `IconButton`, `PillButton`, `PoloTabBar` | `TaskRow` (needs model roles) |
| `SpotifyDrawer`, `StackLayoutLike` | `WindowLayoutManager` calls |
| `WindowChrome` layout | `TimerDial` bindings |

---

## Recommended next step

**Start Phase 1** by copying the deliverable folder structure into `qml/`:

1. Add `resources/fonts/` and `resources/icons/`
2. Port `Theme.qml` + `qmldir`
3. Swap `SpotifyDrawer`, `MainWindow`, `WindowChrome`, `TimerDial`
4. Extend `TimerController` + `WindowLayoutManager` to match expected API

Keep deliverable sources in `Renders/.../qml/` as reference; merge into `qml/` incrementally with `qml.qrc` updates.

---

## Current app vs deliverable — quick diff

```
Deliverable (40 files)          Current app (17 files)
─────────────────────          ─────────────────────
Theme (full tokens)      →     Theme (minimal stub)
TimerDial + progress     →     TimerRing (static arc)
Spotify edge tab 34px    →     Spotify hidden/340px toggle
Drawer (Tasks)           →     Rectangle overlay
TaskTreeView nested      →     flat ListView
IconButton + SVG         →     ChromeButton text
SettingsView             →     (none)
DayTimeline                →     (none)
```
