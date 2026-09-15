# Polomodoro QML tree

Drop-in components for the Qt 6 Quick build, matching *Polomodoro — Visual
Specification v1.0*. Paths mirror the layout in the plan.

## Files

    qml/
      qmldir                     Theme registered as a singleton
      Theme.qml                  all tokens: neutrals, derived palette, space, motion
      main.qml                   ApplicationWindow, view modes, all shortcuts
      windows/
        MainWindow.qml           expanded layout
        ProgressBarView.qml      bar mode (window height animates)
        CompactView.qml          PiP
        StackLayoutLike.qml      keeps all three views instantiated
      components/
        TimerDial.qml            Shape ring arc + centred digits
        TaskRow.qml              all lifecycle states, indent rail
        TaskRowMenu.qml          right-click actions
        TaskMenuDrawer.qml       overlay drawer, four proxies, one model
        TaskTreeView.qml         nested list + per-bucket empty states
        TaskEditor.qml           title/parent/starts/due/target
        TargetTimeEditor.qml     h + m spinboxes with presets
        DateTimeField.qml        MonthGrid popup, local display / UTC storage
        ActiveTasksBar.qml       main-window chips
        OverallProgressBar.qml   Σms/Σtarget, pulse fallback
        SubtaskProgressDropdown.qml
        DayTimeline.qml          72px/hour, planned vs logged
        TimelineBlock.qml
        SpotifyDrawer.qml        persistent WebEngine profile
        SpotifyMediaControls.qml optimistic state + reconcile
        NowPlayingLabel.qml      dwell-scroll-dwell marquee
        BackgroundView.qml       blur, crossfade, phase tint
        BackgroundSourceToggle.qml
        WindowChrome.qml         fixed right-hand control order
        PoloTabBar.qml           Active / Pending / Future / All
        SettingsView.qml         no save button, writes through
        SettingRow.qml
        IconButton.qml PillButton.qml PoloToggle.qml PoloSegmented.qml
        PoloTextField.qml PoloComboBox.qml ProgressTrough.qml
        FieldLabel.qml Divider.qml ModeSwitcher.qml

## What the C++ side must provide

Registered as QML singletons (context properties also work):

    TimerController      mode, phase, phaseLabel, progress (qreal), minutes,
                         seconds, formattedTime, isRunning, isPaused,
                         cycleIndex, cycleLabel; toggle(), reset(),
                         skipPhase(), toggleMode()
    TaskController       activeCount/pendingCount/futureCount,
                         activeChips, activeOverflowCount,
                         soleTargetedActiveTask, overallRatio,
                         overallTargetMs, combinedActiveLabel,
                         activeSubtreeProxy, proxyFor(bucket),
                         parentChoices, showCompleted;
                         startTask/pauseTask/resumeTask/stopTask/
                         completeTask/deleteTask/promote/demote/save/
                         requestEdit/requestCreate/loadInto/openDrawerOnActive
    SettingsController   typed properties per settings key + exportCsv(),
                         integrityReport, palettePresets
    BackgroundController source, currentImageUrl, previousImageUrl,
                         cycleSource()
    SpotifyController    ready, isPlaying, nowPlayingLabel, premiumRequired,
                         attach(view), play/pause/togglePlayPause/next/previous
    WindowLayoutManager  viewMode, x/y/width/height, setMode(i),
                         togglePip(), rememberGeometry(x,y,w,h)
    DayTimelineModel     weekStrip, dayLabel, summaryLabel, plannedBlocks,
                         loggedBlocks, nowMs, nowLabel, isToday, anchorMs,
                         selectDay(date)

Model roles consumed by TaskRow / dropdown delegates: `id, title, depth,
status, hasTarget, progressRatio, overflowRatio, overTarget, progressLabel,
badgeText, startable, startsAtLabel, overdue, hasChildren, hasPrevSibling`.

## Notes that matter

- **One timer, not one per row.** `liveElapsedMs` is recomputed by a single
  app-wide 1 s timer in the model; emit `dataChanged` with an explicit roles
  list or a 200-task tree stutters.
- **Accent animation.** Every accent-bound colour carries
  `Behavior on color { ColorAnimation { duration: Theme.dAccent } }`. A
  Behavior cannot be declared once in the singleton and shared — declare it at
  each binding site (the property in `Theme.qml` is a placeholder/reminder).
- **Fonts.** Register Geist and Geist Mono with
  `QFontDatabase::addApplicationFont()` from `resources/fonts/`; never rely on
  a system install. Duration labels need `font.features: { "tnum": 1 }`.
- **Icons.** SVG paths under `resources/icons/` (`play, pause, stop, prev,
  next, expand, close, pin, settings, reset, tasks`), not an icon font.
- **UTC everywhere.** Store and compare `QDateTime::currentDateTimeUtc()`;
  format only at the display boundary.
- **Palette derivation off the GUI thread.** Downscale to 64x64, k-means in
  OKLCH in a worker, clamp L to 0.62-0.82 and C to 0.09-0.22, then emit the
  five roles as one struct.
