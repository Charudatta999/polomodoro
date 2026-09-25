pragma Singleton
import QtQuick

QtObject {
    id: theme

    // ---- Neutrals (never derived) ----
    readonly property color bgBase:        "#0C0D10"
    readonly property color surface:       "#14161B"
    readonly property color surfaceRaised: "#1B1E25"
    readonly property color line:          "#272B34"
    readonly property color textPrimary:   "#EDEFF3"
    readonly property color textDim:       "#9AA1AE"
    readonly property color textFaint:     "#868FA0"

    // Rose preset — overdue/destructive only. Not a fifth derived hue.
    readonly property color destructive:   "#E8489B"

    // ---- Derived palette. PaletteDeriver writes these five. ----
    property color accent:      "#1ED760"
    property color accentHover: "#3BE377"
    property color breakColor:  "#58C8B0"
    property color overflow:    "#E8C547"
    property color muted:       "#6B7F73"

    // Ink that sits on top of an accent fill.
    readonly property color onAccent: Qt.darker(accent, 3.6)

    // The accent currently in force for the timer phase.
    function phaseColor(phase) {
        switch (phase) {
        case "shortBreak":
        case "longBreak": return breakColor
        case "stopwatch": return textDim
        default:          return accent
        }
    }

    // ---- Spacing (4px base) ----
    readonly property int xs: 4
    readonly property int sm: 8
    readonly property int md: 12
    readonly property int lg: 16
    readonly property int xl: 24
    readonly property int xxl: 32

    // ---- Radii ----
    readonly property int rTrough: 4
    readonly property int rRow: 8
    readonly property int rPanel: 12
    readonly property int rWindow: 28
    readonly property int rPill: 999
    readonly property int windowGrip: 28

    // ---- Panels ----
    readonly property real panelOpacity: 0.82
    readonly property color panelBorder: Qt.rgba(0.929, 0.937, 0.953, 0.10)
    readonly property int blurRadius: 40

    // ---- Type ----
    readonly property string fontFamily: "Geist"
    readonly property string monoFamily: "Geist Mono"
    readonly property int fBody: 14
    readonly property int fBodyS: 13
    readonly property int fNumeric: 12
    readonly property int fEyebrow: 10

    // ---- Motion ----
    readonly property int dStandard: 180
    readonly property int dEnter: 260
    readonly property int dAccent: 420
    readonly property int dCrossfade: 1200
    readonly property int dProgress: 320

    // Animate every accent change; a track change must not flash.
    readonly property Behavior accentBehavior: null // see note in README-qml.md
}
