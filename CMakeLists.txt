cmake_minimum_required(VERSION 3.21)
project(polomodoro VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

option(POLOMODORO_STATIC_STDCXX "Statically link libgcc/libstdc++" OFF)

find_package(Qt6 REQUIRED COMPONENTS Quick Qml Sql QuickControls2 WebEngineQuick QuickEffects DBus)

qt_add_executable(polomodoro
    include/polomodoro/TaskTreeModel.h
    include/polomodoro/ShutdownGuard.h
    include/polomodoro/WindowLayoutManager.h
    include/polomodoro/SpotifyArtBridge.h
    include/polomodoro/TimerController.h
    include/polomodoro/TaskController.h
    include/polomodoro/SettingsController.h
    include/polomodoro/SpotifyController.h
    include/polomodoro/BackgroundController.h
    include/polomodoro/NotificationManager.h
    src/main.cpp
    src/DatabaseManager.cpp
    src/SettingsStore.cpp
    src/TimerEngine.cpp
    src/TaskTree.cpp
    src/TaskTreeModel.cpp
    src/ShutdownGuard.cpp
    src/WindowLayoutManager.cpp
    src/BackgroundManager.cpp
    src/SpotifyArtBridge.cpp
    src/NotificationManager.cpp
    src/controllers/TimerController.cpp
    src/controllers/TaskController.cpp
    src/controllers/SettingsController.cpp
    src/controllers/SpotifyController.cpp
    src/controllers/BackgroundController.cpp
    resources/qml.qrc
)

target_include_directories(polomodoro PRIVATE include)

target_link_libraries(polomodoro PRIVATE
    Qt6::Quick
    Qt6::Qml
    Qt6::Sql
    Qt6::QuickControls2
    Qt6::WebEngineQuick
    Qt6::QuickEffects
    Qt6::DBus
)

if(POLOMODORO_STATIC_STDCXX)
    target_link_options(polomodoro PRIVATE -static-libgcc -static-libstdc++)
endif()

install(TARGETS polomodoro RUNTIME DESTINATION bin)
