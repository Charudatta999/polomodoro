# Polomodoro

Linux desktop Pomodoro + stopwatch app with nested task tracking, SQLite persistence, MPRIS-based Spotify control, and three view modes (expanded, progress bar, PiP).

## Dependencies (Arch/CachyOS)

```bash
sudo pacman -S qt6-base qt6-declarative cmake ninja gcc spotifyd qtkeychain-qt6
```

`spotifyd` is the local Spotify Connect target. Polomodoro launches its own
child (cache under `~/.config/polomodoro/spotifyd`, device name "Polomodoro").
That process cannot play until it has **its own** librespot login — Settings →
Music → **Sign in to spotifyd**. That is a different login from the Web API
playlist/search account. Auto-launch can be turned off in Settings → Music if
you'd rather run your own `spotifyd`.
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

Two separate logins. One without the other looks like "Spotify is connected
but play 404s / nothing uses spotifyd."

**A. spotifyd (this machine actually plays audio)**

1. Settings → Music → **Sign in to spotifyd**. A browser window opens
   (`spotifyd authenticate`, redirect on `127.0.0.1:8890`).
2. Approve. Credentials are stored under `~/.config/polomodoro/spotifyd`.
3. Polomodoro restarts the daemon; "Polomodoro" should then show up as a
   Connect device and MPRIS player once something is playing.

**B. Web API (playlists, search, "play this URI on a device")**

1. Register an app at [developer.spotify.com](https://developer.spotify.com/dashboard). Add `http://127.0.0.1:8888/callback` as a redirect URI.
2. Copy the **Client ID** into Settings → Music → Client ID. No client secret — PKCE.
3. Click **Sign in** in Settings or the Music library overlay (`Ctrl+M`).

Play always targets the local "Polomodoro" device, not whatever phone or web
player Spotify last marked active (those stale IDs are what produced HTTP 404).

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
- `SpotifydManager` never launches a second `spotifyd` if one whose MPRIS
  service name contains "spotify" is already on the bus (checked once at
  startup) — it won't collide with an instance you're already running
  yourself. It always passes `--no-daemon` so the child stays attached to
  Polomodoro's own process tree and gets terminated (not orphaned) on exit.
