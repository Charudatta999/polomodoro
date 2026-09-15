# Polomodoro

Linux desktop Pomodoro + stopwatch app with nested task tracking, SQLite persistence, Spotify embed, and three view modes (expanded, progress bar, PiP).

## Dependencies (Arch/CachyOS)

```bash
sudo pacman -S qt6-base qt6-declarative qt6-webengine cmake ninja gcc
```

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
- **Three view modes**: Expanded, Progress Bar (with subtask dropdown + Spotify controls), PiP
- **Backgrounds**: wallpaper rotation or Spotify album art
- **Embedded Spotify** (requires Premium for playback)

## Data

Database: `~/.config/polomodoro/polomodoro.db`

Backup: `cp ~/.config/polomodoro/polomodoro.db ~/.config/polomodoro/polomodoro.db.bak`

## Keyboard shortcuts

| Shortcut | Action |
|----------|--------|
| `Ctrl+Shift+E` | Open task menu |
| `Ctrl+Shift+1/2/3` | Expanded / Bar / PiP |
| `Ctrl+Shift+T` | Toggle always on top |
| `Ctrl+Shift+B` | Cycle background source |
| `Ctrl+M` | Toggle Spotify panel |

## Notes

- Qt WebEngine is dynamically linked (Chromium); full static binary is not practical with WebEngine.
- Spotify playback controls use MediaSession + DOM fallback; may break if Spotify changes their web UI.
- On Wayland/Hyprland, try `QT_QPA_PLATFORM=wayland` or `xcb` if WebEngine has rendering issues.
