import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Polomodoro

// Visual mock until DayTimelineModel lands — matches spec layout at 392px.
Rectangle {
    id: root
    color: Qt.rgba(0.078, 0.086, 0.106, Theme.panelOpacity)
    border.color: Theme.panelBorder
    border.width: 1
    radius: Theme.rPanel
    clip: true

    readonly property int hourHeight: 72

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.lg
        spacing: Theme.md

        RowLayout {
            Layout.fillWidth: true
            spacing: 5
            Repeater {
                model: ["12", "13", "14", "15", "16", "17", "18"]
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 44
                    radius: Theme.rRow
                    color: index === 4 ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15) : "transparent"
                    border.width: index === 4 ? 1 : 0
                    border.color: Theme.accent
                    Column {
                        anchors.centerIn: parent
                        spacing: 2
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: ["SAT","SUN","MON","TUE","WED","THU","FRI"][index]
                            font.family: Theme.monoFamily
                            font.pixelSize: Theme.fEyebrow
                            color: index === 4 ? Theme.accent : Theme.textFaint
                        }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: modelData
                            font.family: Theme.monoFamily
                            font.pixelSize: 12
                            font.weight: index === 4 ? Font.DemiBold : Font.Normal
                            color: index === 4 ? Theme.textPrimary : Theme.textDim
                        }
                    }
                }
            }
        }

        Text {
            Layout.fillWidth: true
            text: "Wednesday, 16 September"
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fBodyS
            font.weight: Font.Medium
            color: Theme.textPrimary
        }
        Text {
            Layout.fillWidth: true
            text: TaskController.activeCount + " ACTIVE · " + TaskController.combinedActiveLabel.toUpperCase() + " LOGGED"
            font.family: Theme.monoFamily
            font.pixelSize: Theme.fEyebrow
            color: Theme.textDim
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentHeight: timeline.height
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            Item {
                id: timeline
                width: parent.width
                height: 6 * root.hourHeight

                Repeater {
                    model: 6
                    Rectangle {
                        x: 0
                        y: index * root.hourHeight
                        width: parent.width
                        height: 1
                        color: Theme.line
                        Text {
                            x: 0
                            y: -10
                            text: (9 + index) + ":00"
                            font.family: Theme.monoFamily
                            font.pixelSize: Theme.fEyebrow
                            color: Theme.textFaint
                        }
                    }
                }

                Rectangle {
                    x: 52
                    y: root.hourHeight * 0.2
                    width: parent.width - 60
                    height: root.hourHeight * 0.7
                    radius: Theme.rTrough
                    color: "transparent"
                    border.color: Theme.textDim
                    border.width: 1
                    Text {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.margins: Theme.sm
                        text: "09:00 planned · Code Chef"
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                        color: Theme.textDim
                    }
                }

                Rectangle {
                    x: 52
                    y: root.hourHeight * 1.1
                    width: parent.width - 60
                    height: root.hourHeight * 0.65
                    radius: Theme.rTrough
                    color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.85)
                    Text {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.margins: Theme.sm
                        text: "Code Chef · 42m"
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                        color: Theme.onAccent
                    }
                }

                Rectangle {
                    x: 52
                    y: root.hourHeight * 2.3
                    width: parent.width - 60
                    height: root.hourHeight * 0.55
                    radius: Theme.rTrough
                    color: Theme.overflow
                    Text {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.margins: Theme.sm
                        text: "Read book · 48m · +3m over"
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                        color: Theme.bgBase
                    }
                }

                Rectangle {
                    x: 48
                    y: root.hourHeight * 4.5
                    width: parent.width - 52
                    height: 1
                    color: Theme.accent
                    Rectangle {
                        width: 8; height: 8; radius: 4
                        y: -3.5
                        color: Theme.accent
                    }
                    Text {
                        anchors.right: parent.right
                        y: -14
                        text: "NOW"
                        font.family: Theme.monoFamily
                        font.pixelSize: Theme.fEyebrow
                        color: Theme.accent
                    }
                }
            }
        }
    }
}
