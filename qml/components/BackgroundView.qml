import QtQuick
import QtQuick.Effects
import Polomodoro

Item {
    id: root

    readonly property string imageUrl: {
        const url = BackgroundController.currentImageUrl
        if (!url || url.startsWith("gradient://"))
            return "qrc:/wallpapers/default.png"
        return url
    }

    readonly property bool hasImage: imageUrl.length > 0

    // Dark floor under blur so bright covers never blow out the UI.
    Rectangle {
        anchors.fill: parent
        color: Theme.bgBase
    }

    Image {
        id: imgA
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: false
        source: root.imageUrl
        opacity: crossfade.frontIsA ? 1 : 0
        onStatusChanged: {
            if (status === Image.Error)
                console.warn("polomodoro.background: imgA error", source, errorString)
            else if (status === Image.Ready)
                console.info("polomodoro.background: imgA ready", source, implicitWidth, "x", implicitHeight)
        }
        Behavior on opacity { NumberAnimation { duration: Theme.dCrossfade; easing.type: Easing.InOutQuad } }
    }

    Image {
        id: imgB
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: false
        opacity: crossfade.frontIsA ? 0 : 1
        onStatusChanged: {
            if (status === Image.Error)
                console.warn("polomodoro.background: imgB error", source, errorString)
            else if (status === Image.Ready)
                console.info("polomodoro.background: imgB ready", source, implicitWidth, "x", implicitHeight)
        }
        Behavior on opacity { NumberAnimation { duration: Theme.dCrossfade; easing.type: Easing.InOutQuad } }
    }

    QtObject {
        id: crossfade
        property bool frontIsA: true
        property string lastUrl: root.imageUrl
    }

    Connections {
        target: BackgroundController
        function onBackgroundChanged() {
            const url = root.imageUrl
            if (url === crossfade.lastUrl)
                return
            if (crossfade.frontIsA) {
                imgB.source = url
                crossfade.frontIsA = false
            } else {
                imgA.source = url
                crossfade.frontIsA = true
            }
            crossfade.lastUrl = url
        }
    }

    readonly property QtObject frontImage: crossfade.frontIsA ? imgA : imgB
    readonly property bool imageReady: frontImage.status === Image.Ready

    MultiEffect {
        anchors.fill: parent
        visible: root.imageReady
        source: root.frontImage
        blurEnabled: true
        blur: 1.0
        blurMax: Theme.blurRadius
    }

    // Last resort when no image has loaded — never the default (R3).
    Rectangle {
        anchors.fill: parent
        visible: !root.imageReady
        gradient: Gradient {
            GradientStop { position: 0; color: Theme.bgBase }
            GradientStop { position: 1; color: Theme.surface }
        }
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop {
                position: 0.0
                color: Qt.alpha(Theme.phaseColor(TimerController.phase),
                                TimerController.phase === "longBreak" ? 0.34 : 0.22)
            }
            GradientStop {
                position: 1.0
                color: Qt.alpha(Theme.phaseColor(TimerController.phase), 0.04)
            }
        }
        Behavior on opacity { ColorAnimation { duration: Theme.dAccent } }
    }
}
