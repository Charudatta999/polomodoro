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

## Known gaps / next session

1. **expanded→bar does not shrink the window width.** Going expanded→bar leaves
   the width at the expanded width (e.g. 1280x48 instead of 800x48), and that
   width is then persisted — it is a legal bar width so validation cannot reject
   it. Not a timing issue: re-asserting the geometry repeatedly does not help.
   Notably expanded→**compact** (1280→300) shrinks fine, compact→bar (300→800)
   grows fine, and starting directly in bar mode gives a correct 800x48 — it is
   specifically the expanded→bar shrink that sticks. Suspect a Wayland
   size-constraint/window-recreation interaction. Repro:
   `POLOMODORO_VIEW_MODE=1` from a profile whose stored `viewMode` is `expanded`.
2. **The real profile still holds the old degraded geometry.** The ratchet is
   fixed so it will not get worse, but `expandedGeometry` is stuck at its
   legacy 640x420 (a legal size, so it is preserved rather than reset). To get
   the intended defaults back:
   ```bash
   sqlite3 ~/.config/polomodoro/polomodoro.db \
     "delete from settings where key like '%Geometry%';"
   ```
3. Add logind D-Bus `PrepareForShutdown` flush (QtDBus already linked for
   notifications, so this is easier to add now)
4. Bundled default wallpaper images in `resources/backgrounds/` (only one
   default wallpaper exists)
5. `loggedWorkMs` progress basis UI in settings (basis is stored/read but no
   settings toggle exposed yet)
6. Test on Hyprland/Wayland with real Spotify login
7. The `DateTimeField` SpinBoxes still render in light Fusion colors against the
   dark popup — cosmetic, unstyled.

## Resume instructions

```bash
cd ~/Projects/polomodoro
cmake --build build
./build/polomodoro
```

Continue from "Known gaps" above — start with re-testing PiP/bar mode and
the calendar popup now that the task-list rendering bug is fixed.
