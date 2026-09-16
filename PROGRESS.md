# Polomodoro — Implementation Progress

Last updated: 2026-09-16 (session 1)

## Status overview

| Step | Description | Status |
|------|-------------|--------|
| 1 | Scaffold (CMake, main, QML shell, qrc) | **done** |
| 2 | DatabaseManager + SQLite schema | **done** |
| 3 | TimerEngine + TimerController | **done** |
| 4 | TaskTree + TaskTreeModel + persistence | **done** |
| 5 | TaskMenuDrawer + ActiveTasksBar | **done** |
| 6 | ShutdownGuard + heartbeat | **done** (aboutToQuit + 10s heartbeat; logind D-Bus deferred) |
| 7 | Timer → tasks session logging | **done** |
| 8 | BackgroundManager + BackgroundView | **done** (basic gradient/wallpaper; no bundled images yet) |
| 9 | Spotify drawer + SpotifyController | **done** |
| 10 | WindowLayoutManager + 3 view modes | **done** |
| 11 | UI polish + shortcuts | partial (buttons in chrome; global shortcuts not wired in C++) |
| 12 | README | **done** |

## Build status

- **Compiles**: yes (`./build/polomodoro`)
- **Location**: `/home/taps/Projects/polomodoro`

### Phase 1 integration (2026-09-16)
- Ported deliverable QML shell: Theme, main.qml, StackLayoutLike, all three view modes
- Components: TimerDial, ModeSwitcher, SpotifyDrawer (edge tab), WindowChrome, IconButton+SVG icons
- Extended C++ API: TimerController, WindowLayoutManager (int viewMode), SpotifyController, TaskController, SettingsController, BackgroundController
- Stubs: DayTimeline, simplified TaskEditor (title only)
- **Visual polish batch:** drawer overlay, blurred wallpaper + phase tint, Geist fonts, frameless chrome, day timeline mock, bar/PiP theme tokens, larger timer dial, bundled default wallpaper, extra shortcuts

### Deliverable QML review (2026-09-16)
- Reviewed 40 files in `Renders/Deliverable and surface prioritization/qml/`
- Wrote integration plan: **[INTEGRATION-PRIORITIES.md](INTEGRATION-PRIORITIES.md)**

### UI interim pass (2026-09-16)
- Spotify **hidden by default** — click **Music** or `Ctrl+M` to slide panel open
- Timer **centered** in main area when Spotify closed
- Replaced emoji icons with **ChromeButton** text labels (Tasks, Music, Pin, Full/Bar/Mini)
- Solid window background `#12121f`, frosted panels, cleaner active-task bar
- Waiting for user mockup/render for final visual design

### Runtime fixes (2026-09-16)
- `OverallProgressBar.qml`: renamed `clicked` signal → `barClicked` (conflicted with MouseArea)
- `SpotifyDrawer.qml`: added missing `import QtQuick.Layouts`
- `DatabaseManager`: skip `removeDatabase` at teardown to avoid "still in use" warning

## Session log

### Session 1
- Created full project structure with PIMPL C++ backend
- SQLite schema v1, settings seed, task lifecycle, timer engine
- QML UI: MainWindow (expanded), CompactView, ProgressBarView, TaskMenuDrawer
- Spotify WebEngine embed + art/metadata polling + media controls in bar mode
- Build fixed: MOC headers in CMake, QTimer in PIMPL, pointer refs

## Known gaps / next session

1. ~~Wire global keyboard shortcuts~~ — already done (`qml/main.qml` `Shortcut` items: play/pause, reset, skip, mode switch, view modes, PiP, always-on-top, background cycle, music toggle, task drawer, new task, settings, tab cycle)
2. Add logind D-Bus `PrepareForShutdown` flush (needs QtDBus in CMake — QtDBus now linked for notifications, so this is easier to add)
3. ~~Desktop notifications for target reached~~ — done: `NotificationManager` (org.freedesktop.Notifications via QtDBus), fires on task target reached and pomodoro segment completion
4. ~~TaskEditor integration in drawer (schedule date/time pickers)~~ — done: ported `DateTimeField`, `TargetTimeEditor`, full `TaskEditor` (title/parent/starts/due/target) from the deliverable; `TaskController::save()` + `parentChoices` + `reparentTask()` added
5. Bundled default wallpaper images in `resources/backgrounds/` (only one default wallpaper exists)
6. ~~Nested tree display in TaskTreeModel (currently flat per bucket)~~ — done: `TaskTree::flattenBucket()` walks the real hierarchy, keeping ancestor context for any matching descendant; `depth`/`hasPrevSibling` roles now reflect real tree position
7. ~~Target-reached detection + `targetReachedAt` persistence~~ — done: `TaskController::checkTargets()` runs every tick, persists `targetReachedAt`, emits `targetReached` signal
8. `loggedWorkMs` progress basis UI in settings (basis is stored/read but no settings toggle exposed yet)
9. Test on Hyprland/Wayland with real Spotify login
10. ~~Fix ActiveTasksBar to use dedicated active-only model filter~~ — already used `TaskController.activeChips` (dedicated active-task list), not the shared tree model

### Session 2 (2026-09-16, continued)
- **Nested task tree**: `TaskTree::flattenBucket()` replaces the flat `tasksInBucket()` walk for model display — includes ancestor chain of any matching descendant so hierarchy is visible, with correct `depth` and `hasPrevSibling` per row.
- **Target-reached + notifications**: added `polomodoro::NotificationManager` (QtDBus, `org.freedesktop.Notifications`), wired to `TaskController::targetReached` and `TimerController::workSegmentCompleted`. `TaskController::checkTargets()` persists `targetReachedAt` once a task's progress crosses its target.
- **TaskEditor scheduling UI**: ported `DateTimeField` (MonthGrid popup, local display/UTC storage), `TargetTimeEditor` (h/m spinboxes + presets, `setFromMs()`), and the full `TaskEditor` (title/parent/starts/due/target validation) from the deliverable. Added `TaskController::save()`, `parentChoices`, and `TaskTree::reparentTask()` (moves a task, with cycle/self-parent guards) to back it. Wired `TaskController::editRequested`/`createRequested` to `TaskMenuDrawer`'s editor via `Connections` (previously unconnected — right-click "Edit…"/"Add subtask" did nothing).
- **Global shortcuts**: reviewed `qml/main.qml` — already complete from a prior pass; no changes needed.
- Fixed a `font.pixelSize: 10.5` in the ported `TaskEditor.qml` (pixelSize needs an int; produced a silent QML warning that prevented root object creation).
- Added a `QQmlApplicationEngine::warnings` handler in `main.cpp` to print QML errors to stderr — this environment doesn't surface them via `qWarning` by default, which made the above bug invisible until explicitly hooked.

### Session 3 (2026-09-16, live UI debugging)

Drove the actual running app on the real desktop (GNOME screenshot portal for
screenshots, `ydotool` for input) to reproduce the user's report: "PiP/bar
mode — tasks added but not shown, calendar not interactive, progress bar not
visible."

- **Root cause found and fixed — task rows rendered blank.** In
  `qml/components/TaskTreeView.qml`, the `ListView` delegate did
  `task: model` / `depth: model.depth`, relying on the bare `model`
  identifier to mean "this row's data." The `ListView` itself also has its
  own `model` *property* (`model: TaskController.proxyFor(bucket)`), and on
  this Qt/QML build that outer property won the name lookup instead of the
  per-row delegate context. Every `TaskRow` was silently bound to the
  *entire* `TaskTreeModel` object instead of one row, so every role
  (`title`, `hasTarget`, `progressRatio`, `startable`, …) evaluated to
  `undefined` — QML then resets each property to its type default (empty
  string → invisible titles; `true` for `ProgressTrough.visible` → a stray
  gray line always showing regardless of `hasTarget`). This is what read as
  "tasks added but not shown."
  **Fix**: declare `required property var model` directly on the delegate
  instance so Qt injects the correct per-row object there instead of
  falling through to the outer property. One-line change,
  `qml/components/TaskTreeView.qml`.
- **Secondary fix — full model reset every second.** `TaskController`'s 1s
  live timer called `refreshAllModels()` (full `beginResetModel`/
  `endResetModel` across all 6 task models) unconditionally, even though
  bucket membership never changes from a plain tick. This tears down and
  recreates every delegate every second — wasteful, and a likely source of
  transient blank/flicker frames independent of the bug above. Added
  `TaskTreeModel::tick()` (targeted `dataChanged` for the live-time-derived
  roles only) and switched the live timer to call that instead; full resets
  are now reserved for actual structural changes (create/start/stop/delete/
  reparent/etc.).
- **Debugging note for next time**: this environment's Qt build has
  `QT_NO_DEBUG_OUTPUT` in effect (or an equivalent), so `console.log`/
  `console.warn`/`console.error` from QML produce **no output at all**
  unless launched with `QT_LOGGING_RULES="*=true"` — the `qml` logging
  category is off by default and silently swallows every level, including
  `error`. Confirmed the app's own `QQmlApplicationEngine::warnings` signal
  (wired to stderr in `main.cpp` since session 2) is a *different* channel
  from `console.*` and unaffected by this — it kept working the whole time,
  which is what pointed at the bug in the first place via
  "Unable to assign [undefined] to ..." warnings.
- Did **not** get to: PiP/bar-mode progress-bar visibility and calendar
  (`DateTimeField` MonthGrid) interactivity — the task-list bug above turned
  out to be the first domino, worth re-testing those two once this fix is
  confirmed live, since a blank/broken task-role binding could plausibly
  have been masking or contributing to both.

### Session 4 (2026-09-16, PiP/bar + calendar verification)

Re-tested the two items left open by session 3. Neither was downstream of the
task-row bug; they were separate, and one of them was not really the bug it
looked like.

- **Screenshot tooling.** This desktop denies both the GNOME Shell screenshot
  D-Bus interface and the xdg portal, and there is no `grim`/`xwininfo`. Added
  an env-gated self-grab to `main.cpp`: `POLOMODORO_SCREENSHOT=<path>` plus
  optional `POLOMODORO_SCREENSHOT_DELAY=<ms>` and `POLOMODORO_VIEW_MODE=<0|1|2>`
  makes the app render, `grabWindow()` itself, and exit. The mode switch is
  fired from inside the running event loop, not before `exec()` — pre-exec the
  window is not mapped yet and size constraints behave differently.
- **`console.log`/`warn`/`error` are dead even with `QT_LOGGING_RULES="*=true"`.**
  Session 3 assumed that env var restored them; it does not. Both
  `QT_NO_DEBUG_OUTPUT` and warning output are compiled out. The only working
  QML-side channels are the `QQmlApplicationEngine::warnings` hook and
  `Qt.exit(<code>)` as a smuggled-out integer. Several probes here used the
  exit code to report measured values.
- **The progress bars were never broken.** In both bar and PiP mode they render
  correctly (verified at 43% for a seeded 25min/60min task). They are gated on
  there being an active task *with a target* — the real DB has two `stopped`
  tasks with `target_ms` NULL, so there was nothing to draw. Bar mode correctly
  showed "— no active tasks".
- **PiP mode was genuinely broken, but the cause was window geometry, not the
  view.** The compact window was 300x**800**. Root cause was a ratchet bug:
  `WindowLayoutManager::setMode()` emitted `geometryChanged` *before*
  `viewModeChanged`, so QML applied the incoming mode's geometry while the
  *outgoing* mode's `minimumWidth`/`minimumHeight` were still in force. Qt
  silently clamps such an assignment, and the clamped value immediately echoed
  back through `onWidthChanged` → `rememberGeometry()` and was persisted. Every
  stored geometry had degraded to its minimum:
  `expanded 640x420` (default 1280x800), `bar 560x48` (default 800x48),
  `compact 300x800`. Fixes:
  - emit `viewModeChanged` before `loadGeometryForCurrentMode()`;
  - minimum sizes are no longer bindings on `viewMode` — a binding reacting to
    the same signal gives no ordering guarantee against the handler that writes
    the geometry. They are now set imperatively in `_applyWindowGeometryNow()`
    immediately before the assignments;
  - `_applyingGeometry` starts `true`, so pre-first-apply window transients are
    not mistaken for user resizes;
  - a 250ms settle timer re-asserts the geometry after the compositor ack and
    after the `flags` change (expanded has a titlebar, bar/PiP are frameless)
    recreates the platform window, and only then reopens the persist guard;
  - `rememberGeometry()` validates against the current mode before writing;
  - compact geometry validation gained a max height (`h <= 400`), so the
    corrupt 300x800 is rejected on read and self-heals to the default.
  Verified stable across repeated full mode cycles.
- **The calendar popup was not actually dead.** Isolated-harness testing (real
  `Theme`, Fusion style, offscreen + `grabToImage`) confirmed the `MonthGrid` is
  enabled, visible, correctly sized, and that `DateTimeField`'s `onClicked`
  handler does store the picked date. What was missing was any way to *use* it:
  no month navigation and no weekday header, so only the current month was ever
  reachable and the columns were unlabeled. Added `‹ Month Year ›` navigation
  (with year rollover), a themed `DayOfWeekRow`, a themed day delegate that
  accents today and dims adjacent-month days, and `onAboutToShow` now opens on
  the month of the current value rather than today's.

### Session 5 (2026-09-16, task actions + real day timeline)

- **Task actions were wired but invisible.** Reported as "cannot mark complete /
  discard / pause". Harness testing (stub `TaskController`, real `TaskRow`)
  confirmed the row was enabled, the context menu opened, and "Mark complete"
  reached the controller — nothing was broken. The problem was affordance: the
  action row was `opacity: 0` until hover, and complete/delete existed *only*
  behind an unhinted right-click menu. Actions are now dimmed-but-visible at
  rest, with an explicit check (complete) and ⋯ (menu) button and tooltips
  throughout. New `more.svg` / `check.svg` icons.
- **DayTimeline is now real.** Replaced the hardcoded mock with
  `DayTimelineModel` (`QAbstractListModel`): "planned" blocks from a task's
  scheduled window and "logged" blocks from the sessions table, for one local
  day. Day strip cells select a day, prev/next arrows step, a "Today" button
  appears when off today, and the NOW marker only shows on today. Planned
  blocks drag vertically to reschedule (5-minute snap, duration preserved);
  logged history refuses to move. Backed by new `TaskTree::sessionsBetween()`,
  `rescheduleTask()` and `allTasks()`. Verified end to end: prev/next
  navigation, a drag snapping 1022→1020 minutes, the DB write-back
  (10:00→11:30 UTC with the 1h window preserved), and logged rows rejecting
  `moveBlock`.
- **Overlap lanes.** A planned block and the session logged against it occupy
  the same time and covered each other. `assignLanes()` clusters overlapping
  blocks and splits the width into columns; non-overlapping blocks keep the
  full width.
- **Two gotchas worth remembering.** First: the initial hour range used
  09:00–15:00 as a *floor* rather than a fallback, which pinned the view to
  09:00 and pushed an evening's blocks below the scroll fold — they rendered
  correctly the whole time, just off-screen. Second: `QAbstractListModel`
  exposes no `count` to QML, so the empty-state check was silently `undefined`;
  added an explicit `count` property.
- **Real logging channel at last.** `POLOMODORO_VERBOSE=1` now calls
  `QLoggingCategory::setFilterRules("*=true")` *and* installs a
  `qInstallMessageHandler`. This is the only way to see `console.*` and
  `qmlWarning()` on this build — including delegate-creation failures, which
  are otherwise completely silent and cost real time this session. Prefer it
  over the `Qt.exit(code)` trick from session 4.

### Session 6 (2026-09-16, delete cascade + unscheduled tasks)

- **"Deleting one task deletes all" was ON DELETE CASCADE.** Verified the model
  path is correct: with three root tasks, deleting one leaves the other two
  (and a subtask) intact. The reported case was a parent with a subtask —
  `parent_id ... ON DELETE CASCADE` in the schema plus `PRAGMA foreign_keys=ON`
  means deleting the parent takes the subtree, which is intended but was
  completely unannounced. `TaskRowMenu` even carried a comment saying to
  confirm when children would cascade; no confirmation had ever been written.
  Added one: `TaskRowMenu` now raises a `deleteRequested` signal and `TaskRow`
  shows a modal confirm naming the task when `hasChildren`, deleting directly
  otherwise.
- **"Added tasks are not shown on the calendar" — they had no time.** Confirmed
  by test that a task saved *with* a scheduled start appears immediately
  ("11:00 planned · ..."), while one created without one does not, because
  there is nothing to place on a time axis. Rather than fabricate a time,
  unscheduled non-completed tasks are now listed as chips above the timeline
  ("UNSCHEDULED · CLICK TO PLACE ON THIS DAY"); clicking one schedules it on
  the selected day at `suggestedStartMinutes()` (next quarter hour today, else
  09:00) with its target as the length, falling back to an hour. Blocks gained
  a hover close button to send a task back to unscheduled, so placing one is
  reversible. Round trip verified: 3 unscheduled/0 blocks → schedule → 2/1 →
  unschedule → 3/0.

### Session 7 (2026-09-16, reconciled against the visual spec)

Read the handoff spec (`claude-designfiles/.../Polomodoro Visual Spec.dc.html`;
the shared claude.ai link is auth-walled and returns 403, the local copy is the
same document). It defines both surfaces I had been guessing at.

- **Reverted TaskRow to spec.** The spec is explicit that lifecycle buttons
  "fade in from 0 → 1 (they are hidden at rest to keep the list quiet)" — the
  hover-only behaviour reported as a bug in session 5 was deliberate. Restored
  `opacity: 0` at rest and removed the ✓ and ⋯ buttons I had added, along with
  their now-unused `check.svg` / `more.svg`. Complete and delete are
  right-click only, as designed. The delete confirmation stays: the spec calls
  for it explicitly ("Delete confirms only when children cascade").
- **DayTimeline brought to spec.** Several things already matched by
  coincidence (72 px/hour, Flickable over an Item, Repeaters, one absolute-y
  helper, sessions by date range, overlap lanes, the 1 px accent now line).
  Closed the rest:
  - planned blocks are a **dashed** 1 px outline (`Shape` + `DashLine`), not solid;
  - fixed **24 × 72** column that opens scrolled to the now line on today, else
    an hour before the day's first block — replacing the dynamic hour window;
  - **in-progress** blocks: hatched, open-ended (no bottom edge), grown from
    `activeSince` and rebuilt on the shared 60 s tick;
  - contiguous sessions of one task **under 2 min apart are merged**;
  - lane split **capped at 3**, extras collapse to 4 px ticks fanned along the
    right edge;
  - min block height **18 px**, labels dropped below **34 px** with a tooltip;
  - summary reads "N ACTIVE · …", not "N PLANNED".
- **Lane assignment now uses the rendered extent, not the logical duration.**
  With an 18 px floor a 3-minute session is painted ~15 minutes tall, so
  assigning lanes by real duration left it underneath its neighbour and
  unclickable — which is precisely what the spec's min-height rule exists to
  prevent.
- **The unscheduled-task tray is a deliberate addition, not in the spec.** Kept
  because it fixes the real complaint that a task added without a time simply
  vanishes from this surface.

### Session 8 (2026-09-16, Spotify DRM)

- **"Spotify won't work if you block protected content" was a missing Widevine
  CDM.** QtWebEngine only searches `~/.config/chromium/WidevineCdm` and
  `~/.config/google-chrome/WidevineCdm`; this machine has neither browser, so
  no CDM was found and every EME request was refused. Three perfectly good
  copies of `libwidevinecdm.so` 4.10.3050.0 were already present (Brave,
  Netflix, the Spotify cache). `findWidevineCdm()` in `main.cpp` now scans
  those locations plus the Chrome/Chromium system paths, picks the highest
  version, and appends `--widevine-path=` to `QTWEBENGINE_CHROMIUM_FLAGS`
  **before** `QtWebEngineQuick::initialize()` — the flags are read when
  Chromium starts, not when a view is created.
- **Proven, not assumed.** A standalone QtWebEngine probe calling
  `navigator.requestMediaKeySystemAccess('com.widevine.alpha', ...)` and
  reporting through the exit code: without the flag the request is rejected
  (the user's exact symptom), with it the page reports `WIDEVINE_OK`.
  Harness kept at `scratchpad/eme/` for re-testing.
- **DRM notice added**, per the spec's degraded states ("Inline notice naming
  the Widevine package, with the README section linked"): `WidevineCdmPath` is
  exposed as a context property and `SpotifyDrawer` shows a notice naming the
  package when it is empty, rather than letting Spotify blame the browser.

### Session 9 (2026-09-17, bar/PiP frame, drag, and the expanded→bar width bug)

User report: PiP/bar windows couldn't be moved freely, showed a GNOME-style
titlebar on top of the app's own chrome, and the task drawer wasn't fully
visible when opened from bar mode. The visual spec (session 7's read of
`Polomodoro Visual Spec.dc.html`) turned out to name the exact fixes:

- **Titlebar fixed by making `flags` mode-independent.** Previously only bar
  and PiP got `Qt.FramelessWindowHint`; expanded did not, and the *change* in
  `flags` on every mode switch is what the spec's own QML notes warn about —
  "some Wayland compositors recreate the surface and drop the hint." Since
  every mode already draws its own chrome (`WindowChrome` in expanded, the bar
  and PiP backgrounds themselves), there is no reason to ever want a system
  titlebar. `flags` is now `Qt.Window | Qt.FramelessWindowHint` unconditionally,
  so a mode switch never touches it and the surface is never recreated on that
  account. Confirmed with a `POLOMODORO_FRAME_TEST` probe (added to
  `main.cpp`, kept as a permanent debug affordance since `grabWindow()`
  screenshots cannot show decoration state) reading real `QWindow` flags and
  frame margins: `frameless=1`, top margin `0`, `decorated=no` in all three
  modes and across every transition.
- **Dragging fixed by switching from `DragHandler` to a `MouseArea`,** per the
  spec's own instruction ("Drag via `DragHandler` on the background only" is
  what the mock used, but in practice `DragHandler.onActiveChanged` only fires
  *after* the drag threshold, presenting `startSystemMove()` with an input
  serial the compositor treats as stale and silently refuses). The background
  `MouseArea` in `ProgressBarView.qml` and `CompactView.qml` now calls
  `startSystemMove()` from `onPositionChanged` while pressed — on first motion,
  not after a threshold — so the serial is still current. PiP's double-click
  to expand was folded into the same `MouseArea` (`onDoubleClicked`) rather
  than a second overlapping handler competing for the same events.
- **Bar-mode task drawer fixed by redirecting to expanded**, not by trying to
  fit a 400 px, full-window-height `Drawer` into a 48 px bar or 140 px PiP
  window. `toggleTaskDrawer()` now switches to expanded first when called from
  bar or PiP, waits out the geometry settle, and opens the drawer there —
  which is also where the spec's own drawer mockups show it.
- **The expanded→bar width bug from session 4 is fixed — root cause was a
  second, uncoordinated writer of `Window.window.height`.**
  `ProgressBarView.syncBarHeight()` ran on `onIsActiveChanged`, which fires
  synchronously the instant `Window.window.viewMode` flips — before
  `main.qml`'s own `Qt.callLater(_applyWindowGeometryNow)` has even run. That
  fired an isolated height-only resize on the frameless Wayland surface,
  immediately followed by `main.qml`'s width+height resize once the deferred
  call executed. Two `xdg_toplevel` resize requests issued back to back within
  the same handling of one mode change is not something to assume both land —
  Wayland resizes are a request/ack cycle, not a fire-and-forget property
  write — and the second one (carrying the width fix) reliably lost. Removed
  `syncBarHeight()`'s hookup to `onIsActiveChanged`; it now only fires from
  `onDropdownOpenChanged`, which happens well after a mode switch has settled.
  Entering bar mode's initial height is `main.qml`'s job alone. Verified
  against isolated per-transition fixtures (fresh profile, `viewMode` forced
  to the *from* mode, corrupted geometry cleared, `POLOMODORO_VIEW_MODE` set
  to the *to* mode) across all six transitions, twice each, zero QML warnings:
  expanded→bar and compact→bar both now give `800x48` (previously
  expanded→bar alone stuck at `1280x48`); →compact gives `300x140`; →expanded
  gives `1280x800`.
- **Caveat.** Everything above is verified through `QWindow` introspection
  (`POLOMODORO_FRAME_TEST`) and scripted mode switches, not a real mouse
  drag — this environment has no pointer injection. The `MouseArea` fix is
  reasoned from the spec's own stated Wayland gotcha and from `flags` no
  longer changing (which removes the surface-recreation hazard that would
  invalidate *any* input serial, drag or otherwise), but an actual drag on the
  real desktop is the one thing here that could still surprise.

## Known gaps / next session

1. **The real profile may still hold old degraded geometry** from before the
   session 4 ratchet fix. `expandedGeometry`/`barGeometry`/`compactGeometry`
   are legal-but-stale sizes, so validation won't reset them on its own. To
   force the shipped defaults back:
   ```bash
   sqlite3 ~/.config/polomodoro/polomodoro.db \
     "delete from settings where key like '%Geometry%';"
   ```
2. Add logind D-Bus `PrepareForShutdown` flush (QtDBus already linked for
   notifications, so this is easier to add now)
3. Bundled default wallpaper images in `resources/backgrounds/` (only one
   default wallpaper exists)
4. `loggedWorkMs` progress basis UI in settings (basis is stored/read but no
   settings toggle exposed yet)
5. Test on real Hyprland/Wayland (and especially the new drag-to-move) with a
   real pointer, and confirm Spotify login end to end — both untestable
   headlessly on this machine.
6. The `DateTimeField` SpinBoxes still render in light Fusion colors against the
   dark popup — cosmetic, unstyled.

## Resume instructions

```bash
cd ~/Projects/polomodoro
cmake --build build
./build/polomodoro
```

Continue from "Known gaps" above — start with a real-desktop pass on dragging
bar/PiP by their background and confirming no titlebar reappears, since that
is the one thing session 9 could not verify without pointer injection.
