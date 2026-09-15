import QtQuick
import QtQuick.Controls

Item {
    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0; color: "#2d1b4e" }
            GradientStop { position: 1; color: "#1a1a2e" }
        }
    }

    Image {
        anchors.fill: parent
        source: backgroundController.currentImageUrl.startsWith("gradient") ? "" : backgroundController.currentImageUrl
        fillMode: Image.PreserveAspectCrop
        opacity: 0.6
        asynchronous: true
    }

    Rectangle {
        anchors.fill: parent
        color: timerController.phase === "work" ? "#22ff8844"
               : timerController.phase === "shortBreak" || timerController.phase === "longBreak" ? "#2200d4aa"
               : "#00000000"
    }
}
