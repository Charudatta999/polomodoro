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
        Behavior on opacity { NumberAnimation { duration: Theme.dCrossfade; easing.type: Easing.InOutQuad } }
    }

    Image {
        id: imgB
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
        asynchronous: true
        cache: false
        opacity: crossfade.frontIsA ? 0 : 1
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

    MultiEffect {
        anchors.fill: parent
        source: crossfade.frontIsA ? imgA : imgB
        blurEnabled: true
        blur: 1.0
        blurMax: Theme.blurRadius
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
