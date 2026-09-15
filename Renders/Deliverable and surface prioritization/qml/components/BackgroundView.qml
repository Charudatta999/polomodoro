import QtQuick
import QtQuick.Effects
import Polomodoro

// Source-agnostic: binds to currentImageUrl whether that is a local file or a
// Spotify CDN URL. One image drives both the background AND the palette, so
// panels always sit in a colour family the accent belongs to.
Item {
    id: root

    Image {
        id: imgA
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: false
        source: BackgroundController.currentImageUrl
        opacity: 1
        Behavior on opacity { NumberAnimation { duration: Theme.dCrossfade; easing.type: Easing.InOutQuad } }
    }

    Image {
        id: imgB
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: false
        source: BackgroundController.previousImageUrl
        opacity: 0
    }

    MultiEffect {
        anchors.fill: parent
        source: imgA
        blurEnabled: true
        blur: 1.0
        blurMax: Theme.blurRadius
    }

    // Phase tint: ceiling of 0.34 alpha. The wallpaper must stay
    // recognisable — the accent was derived from it, and the two need to read
    // as the same object.
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 0.0; color: Qt.alpha(Theme.phaseColor(TimerController.phase), TimerController.phase === "longBreak" ? 0.34 : 0.22) }
            GradientStop { position: 1.0; color: Qt.alpha(Theme.phaseColor(TimerController.phase), 0.04) }
        }
        Behavior on opacity { NumberAnimation { duration: Theme.dAccent } }
    }

    // Fallback when no art has ever been seen.
    Rectangle {
        anchors.fill: parent
        visible: BackgroundController.currentImageUrl === ""
        gradient: Gradient {
            GradientStop { position: 0.0; color: Qt.darker(Theme.accent, 2.6) }
            GradientStop { position: 1.0; color: Theme.bgBase }
        }
    }
}
