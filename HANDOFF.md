# Polomodoro — Handoff

Read this first. It's the current, accurate snapshot — `PROGRESS.md` is the
full chronological session log underneath it if you need the "why" behind a
decision; the design spec/rulebook lives at
`claude-designfiles/updated-docs/Deliverable and surface prioritization/`
(`RULEBOOK.md`, `PROMPT.md`, `DECISIONS.md`) and is the binding source of
truth for visual/behavioral rules.

## What this is

Linux-only Pomodoro + stopwatch desktop app. Qt6 Quick (QML) UI, C++17 PIMPL
backend, SQLite persistence, three view modes (expanded / progress-bar /
picture-in-picture), nested task tracking with live time tracking, and
Spotify integration via MPRIS + the Spotify Web API (no embedded browser —
Qt WebEngine was removed for good, see rulebook R7).

## Build & run

```bash
cd ~/Projects/polomodoro
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/polomodoro
```

Confirmed clean as of this handoff (`ninja: no work to do` after a full
rebuild, zero QML warnings via the `POLOMODORO_SCREENSHOT` self-grab hook —
see "Debug affordances" below).

Dependencies: `qt6-base qt6-declarative cmake ninja gcc spotifyd
qtkeychain-qt6` (Arch/CachyOS names). `qtkeychain-qt6` is optional but
without it the Spotify refresh token doesn't survive a restart.

## ⚠️ Current git state — read before touching anything

**HEAD is `20aca3b` on branch `fix/window-geometry-and-calendar`.** The
large uncommitted feature pass described below (PaletteDeriver, window
shell rewrite, spotifyd device login, task promote/demote) has been
reviewed and committed in four commits (`07ba05b`, `c34f288`, `a71148e`,
`20aca3b` — see `PROGRESS.md` session 12 for what's in each). `git log`
now reflects current behavior.

The `Renders/` directory shows as 63 deleted files in `git status` —
**leave that alone**, it's deliberately unstaged per standing instruction
from earlier in this project's history, not something to clean up.
`external-binaries/` and `claude-designfiles/` are untracked on purpose too
(the latter is gitignored).

## Architecture

**PIMPL for every core class** (rulebook rule): public headers are thin
QObject bridges, all logic lives in `src/`. QML never mutates a model —
delegates call controller methods.

Core pieces, backend:
- `DatabaseManager` / `SettingsStore` — SQLite (`~/.config/polomodoro/polomodoro.db`), WAL mode, a flat `settings` key/value table.
- `TaskTree` / `TaskTreeModel` / `TaskController` — nested tasks, active-time tracking survives reboot via a heartbeat (`ShutdownGuard`, now also flushing on logind's `PrepareForShutdown`, not just `aboutToQuit`).
- `TimerEngine` / `TimerController` — Pomodoro + stopwatch.
- `WindowLayoutManager` — persisted per-mode geometry; the window itself is now a **transparent, rounded, borderless surface** (`QQuickWindow::setDefaultAlphaBuffer(true)` in `main.cpp`) with custom drag (`WindowChrome`'s `MouseArea` + `startSystemMove()`/`resetSystemMove()`, `WindowDragFrame.qml`) and resize (`WindowResizeFrame.qml`) — this replaces an earlier `DragHandler`-based approach that was less reliable on Wayland.
- `BackgroundManager` / `BackgroundController` — wallpaper rotation (bundled defaults + optional user folder via a `FolderDialog`, asked once per rulebook R2) or Spotify album art; three **placement modes** now exist (full-window / blurred-frame-only / middle-blur — see `shell.wallpaperOn` / `placementMiddle` / `placementFrame` in `main.qml`).
- **`PaletteDeriver` (new, not in earlier docs)** — actually derives `Theme.accent`/`accentHover`/`breakColor`/`overflow`/`muted` from the live background image: downscales to 64×64, k-means clusters in OKLab space, clamps L/C to the spec's range, picks the most-chromatic cluster. Falls back to a fixed "forest" green when there's no image or in the four manual presets. This satisfies a rulebook §1 requirement that was previously unmet (Theme.qml's derived-palette properties existed but nothing wrote to them dynamically).
- `NotificationManager` — desktop notifications via `org.freedesktop.Notifications` over QtDBus.

Spotify/media, backend — **this is the part most likely to need explaining to someone new**:
- `MprisController` — watches the D-Bus session bus for any `org.mpris.MediaPlayer2.*` service (signal-driven, never polled, except track position which has no MPRIS push equivalent). Now also exposes a `players` list (for a picker, not just the auto-heuristic) and, when the active target is a *remote* Spotify Connect device rather than local `spotifyd`, **proxies transport commands to `SpotifyWebApi`** instead of D-Bus, so the now-playing strip doesn't need to branch on which kind of player is active.
- `SpotifyWebApi` — OAuth **PKCE** flow for the Spotify **Web API** (playlists, search, device list, telling a device what to play). Refresh token in the system keyring via qtkeychain, never the settings table.
- `SpotifydManager` — spawns and owns a dedicated `spotifyd` child process (own cache dir under `~/.config/polomodoro/spotifyd`, own device name, defaults to "Polomodoro"). Handles stale-PID cleanup from a previous crashed run, distinguishes an intentional stop from a real crash in its process-exit logging, and — new — has an `authenticate()` method that runs `spotifyd authenticate` (librespot's own OAuth device login) so `spotifyd` can log in **without a phone-tap and without a password**, which matters for accounts (like phone-number/OTP logins) that have no traditional password to give it.

**The two-auth-system distinction is the single most likely point of confusion for anyone new to this codebase, including future me — document it clearly if you touch this area:**

| | `SpotifyWebApi` (PKCE) | `SpotifydManager.authenticate()` (librespot OAuth) |
|---|---|---|
| Grants | Calling the Web API: browse, search, see devices, tell an *already-connected* device what to play | `spotifyd` itself becoming a real Spotify Connect device that can decode and stream audio |
| Where it lives | Settings → Music → "Playlist account" | Settings → Music → "spotifyd login" |
| Needed for | Library overlay, search, playlists | Playback actually working through `spotifyd` at all |
| One does NOT cover the other | — | — |

A user can complete the Web API sign-in and still have zero playable
devices, because `spotifyd` needs its own separate login. Both are now
clearly labeled in `SettingsView.qml` with disambiguating subtext so this
doesn't need re-explaining in the UI itself, but it's easy to forget when
reading the code cold.

## What landed in the last feature pass (now committed)

In one paragraph: the window shell was rewritten (transparent rounded window,
custom drag/resize frames, three background placement modes); `PaletteDeriver`
was added to make the "derived at runtime" accent system in the rulebook
actually work instead of sitting static; `SpotifydManager` gained the
`authenticate()` OAuth flow described above plus stale-PID/crash-vs-intentional-
stop handling; `MprisController` gained a player list and Web-API transport
proxying for remote devices; `Theme.qml`'s `textFaint` was corrected to the
rulebook's exact `#868FA0` (it had drifted to `#6A7280` — the value the
rulebook explicitly calls out as having failed contrast); `SettingsView.qml`
got the disambiguated dual-Spotify-auth UI; task promote/demote went from
no-op stubs to real reparenting; task-start and deadline-approaching
notifications were wired up; icons for the background-source toggle moved
from emoji to real SVGs. It **builds clean and renders with zero QML
warnings** and, as of 2026-09-18, the user has confirmed `spotifyd` device
login and playback actually work end to end.

Recommended next step: do a rulebook §0 pass (screenshot each touched
surface, name differences against the spec pages) — not yet done for this
batch, same discipline as every prior session in `PROGRESS.md`.

## Known open items

From `PROGRESS.md`'s gap-tracking (kept there historically; the two genuinely
open items as of the last committed session were):

1. **Real Wayland pointer input** (drag-to-move, resize, `DateTimeField`
   calendar clicks) has never been tested with a real pointer — only
   headlessly. The window-chrome rewrite above was specifically aimed at
   this, but "aimed at" is not "verified" — this still needs a hands-on
   pass on a real Hyprland/Wayland session.
2. ~~`spotifyd` device-level login~~ — confirmed working by the user
   (2026-09-18). Playback works end to end.

Everything else tracked in `PROGRESS.md`'s "Known gaps" sections up through
the last committed session (`f3bc00a`) was closed out — geometry defaults,
logind shutdown flush, bundled wallpaper rotation, the `SpinBox` dark-palette
fix, the `MprisController` multi-player heuristic, a real segfault in the
PKCE callback handler, playlist track-count and search bugs against the live
API, and the `play()` device-resolution deadlock. Don't re-derive these from
scratch — read the relevant `PROGRESS.md` session entries if you need the
detail (session log headers are dated; search for the topic).

## Debug affordances (permanent, safe to use)

- `POLOMODORO_VERBOSE=1` — forces QML logging categories open and installs a handler that prints everything (this Qt build otherwise silently swallows `console.*`/`qmlWarning` output).
- `POLOMODORO_SCREENSHOT=<path>` (+ optional `POLOMODORO_SCREENSHOT_DELAY_MS`, `POLOMODORO_VIEW_MODE`) — the app grabs its own window and exits; this desktop has no working screenshot portal, so this is the only way to see what's actually rendered. Combine with `2>&1 | grep "QML WARN"` for the rulebook §0 gate.
- `POLOMODORO_FRAME_TEST=<delay_ms>` — reports real window flags/frame margins to catch a compositor drawing an unwanted system titlebar.

## A note on working with `spotifyd` in this environment

This agent's permission layer blocks executing anything named `spotifyd` via
Bash directly, regardless of provenance (pacman-installed, downloaded
release binary, doesn't matter — it pattern-matches the name). It does
**not** block the compiled `polomodoro` binary internally spawning `spotifyd`
via `QProcess` (that's a different call path and has worked fine throughout
this project). If you need to test `spotifyd` behavior directly rather than
through the app, that requires the user's own terminal.
