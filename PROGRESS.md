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

1. Wire global keyboard shortcuts (`QShortcut` in C++ or `Shortcut` in QML)
2. Add logind D-Bus `PrepareForShutdown` flush (needs QtDBus in CMake)
3. Desktop notifications for target reached (`org.freedesktop.Notifications`)
4. TaskEditor integration in drawer (schedule date/time pickers)
5. Bundled default wallpaper images in `resources/backgrounds/`
6. Nested tree display in TaskTreeModel (currently flat per bucket)
7. Target-reached detection + `targetReachedAt` persistence
8. `loggedWorkMs` progress basis UI in settings
9. Test on Hyprland/Wayland with real Spotify login
10. Fix ActiveTasksBar to use dedicated active-only model filter

## Resume instructions

```bash
cd ~/Projects/polomodoro
cmake --build build
./build/polomodoro
```

Continue from "Known gaps" items above, prioritizing shortcuts, nested tree, and notifications.
