# Polomodoro

Linux desktop Pomodoro + stopwatch app with nested task tracking, SQLite persistence, MPRIS-based Spotify control, and three view modes (expanded, progress bar, PiP).

## Dependencies (Arch/CachyOS)

```bash
sudo pacman -S qt6-base qt6-declarative cmake ninja gcc spotifyd qtkeychain-qt6
```

`spotifyd` is the local Spotify Connect target that transport controls talk to
over MPRIS — start it (or run any other MPRIS-capable player, or the official
Spotify client) before expecting the now-playing strip to show anything.
`qtkeychain-qt6` is optional but recommended: without it, signing in to
Spotify's Web API does not survive a restart (the refresh token is kept in
memory only, deliberately never written to the settings database).

## Build

```bash
cd ~/Projects/polomodoro
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/polomodoro
```

Optional static C++ runtime:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DPOLOMODORO_STATIC_STDCXX=ON
```

## Features

- **Pomodoro** and **stopwatch** timers
- **Nested tasks** with start/pause/resume/stop lifecycle
- **Multiple active tasks** at once; active time survives reboot (SQLite + heartbeat)
- **Scheduling**: start date, end date, target duration per task
- **Task menu** (☰): Active / Pending / Future / All tabs
- **Three view modes**: Expanded, Progress Bar (with subtask dropdown + compact transport), PiP
- **Backgrounds**: wallpaper rotation or Spotify album art (from `mpris:artUrl`)
- **Spotify via MPRIS + Web API**: transport (play/pause/next/previous/seek) works against whatever MPRIS player is on the session bus, no account needed; playlists, search and device switching need a one-time Spotify sign-in (OAuth PKCE)

## Setting up Spotify

Transport (play/pause/seek) works out of the box against any MPRIS player —
run `spotifyd`, the official Spotify client, or anything else that exposes
`org.mpris.MediaPlayer2.*`, and the now-playing strip picks it up automatically.

Playlists, search and switching devices need a one-time sign-in:

1. Register an app at [developer.spotify.com](https://developer.spotify.com/dashboard) (free). Add `http://127.0.0.1:8888/callback` as a redirect URI (or pick a different port and set it in Settings → Spotify).
2. Copy the app's **Client ID** into Settings → Spotify → Client ID. No client secret is needed — Polomodoro uses OAuth PKCE, which is designed for a public client ID.
3. Click **Sign in** in the Music library overlay (`Ctrl+M`). Your browser opens Spotify's login page; after approving, it redirects back to a local port Polomodoro is listening on.

## Data

Database: `~/.config/polomodoro/polomodoro.db`

Backup: `cp ~/.config/polomodoro/polomodoro.db ~/.config/polomodoro/polomodoro.db.bak`

The Spotify refresh token is **not** in this database — it lives in the
system keyring (via qtkeychain) so the database stays copyable as a plain
backup. Copying the database elsewhere does not leak Spotify access.

## Keyboard shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+Shift+E` | Open task menu |
| `Ctrl+Shift+1/2/3` | Expanded / Bar / PiP |
| `Ctrl+Shift+T` | Toggle always on top |
| `Ctrl+Shift+B` | Cycle background source |
| `Ctrl+M` | Toggle Music library overlay |

## Notes

- Qt WebEngine was removed (2026-09-17) along with the embedded Spotify web
  player — see `PROGRESS.md` for why. Playback is spotifyd/MPRIS, browsing is
  the Spotify Web API.
- MPRIS player selection: if more than one player is on the bus, Polomodoro
  prefers one whose service name contains "spotify"; otherwise it follows
  whichever one appeared first, and switches if a spotify-named one appears
  later.
