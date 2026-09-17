cmake_minimum_required(VERSION 3.21)
project(polomodoro VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(POLOMODORO_STATIC_STDCXX "Statically link libgcc/libstdc++" OFF)

# WebEngineQuick dropped with the Spotify architecture pivot (2026-09-17):
# playback is spotifyd + MPRIS over QtDBus, browsing is the Spotify Web API.
# Network added for SpotifyWebApi's OAuth/PKCE and REST calls.
find_package(Qt6 REQUIRED COMPONENTS Quick Qml Sql QuickControls2 QuickEffects DBus Network)

# Optional: the refresh token must go in the system keyring per the rulebook
# ("secrets go in the keyring, never the settings table — the SQLite file is
# meant to be copyable as a backup"). Not installed on this machine yet
# (`pacman -S qtkeychain-qt6`). Without it, SpotifyWebApi keeps the token
# in memory only for the session rather than write it somewhere insecure.
find_package(Qt6Keychain QUIET)
if(Qt6Keychain_FOUND)
    message(STATUS "qtkeychain found — Spotify refresh token will persist across restarts")
else()
    message(WARNING "qtkeychain NOT found — Spotify sign-in will not persist across restarts. "
                     "Install qtkeychain-qt6 to fix.")
endif()

qt_add_executable(polomodoro
    include/polomodoro/TaskTreeModel.h
    include/polomodoro/DayTimelineModel.h
    include/polomodoro/ShutdownGuard.h
    include/polomodoro/WindowLayoutManager.h
    include/polomodoro/PaletteDeriver.h
    include/polomodoro/MprisController.h
    include/polomodoro/SpotifydManager.h
    include/polomodoro/SpotifyWebApi.h
    include/polomodoro/TimerController.h
    include/polomodoro/TaskController.h
    include/polomodoro/SettingsController.h
    include/polomodoro/BackgroundController.h
    include/polomodoro/NotificationManager.h
    src/main.cpp
    src/DatabaseManager.cpp
    src/SettingsStore.cpp
    src/TimerEngine.cpp
    src/TaskTree.cpp
    src/TaskTreeModel.cpp
    src/DayTimelineModel.cpp
    src/ShutdownGuard.cpp
    src/WindowLayoutManager.cpp
    src/BackgroundManager.cpp
    src/PaletteDeriver.cpp
    src/MprisController.cpp
    src/SpotifydManager.cpp
    src/SpotifyWebApi.cpp
    src/NotificationManager.cpp
    src/controllers/TimerController.cpp
    src/controllers/TaskController.cpp
    src/controllers/SettingsController.cpp
    src/controllers/BackgroundController.cpp
    resources/qml.qrc
)

target_include_directories(polomodoro PRIVATE include)

target_link_libraries(polomodoro PRIVATE
    Qt6::Quick
    Qt6::Qml
    Qt6::Sql
    Qt6::QuickControls2
    Qt6::QuickEffects
    Qt6::DBus
    Qt6::Network
)

if(Qt6Keychain_FOUND)
    target_link_libraries(polomodoro PRIVATE Qt6Keychain::Qt6Keychain)
    target_compile_definitions(polomodoro PRIVATE POLOMODORO_HAVE_KEYCHAIN)
endif()

if(POLOMODORO_STATIC_STDCXX)
    target_link_options(polomodoro PRIVATE -static-libgcc -static-libstdc++)
endif()

install(TARGETS polomodoro RUNTIME DESTINATION bin)
