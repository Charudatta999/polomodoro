import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#88000000"

    MouseArea {
        anchors.fill: parent
        onClicked: taskController.setMenuOpen(false)
    }

    Rectangle {
        width: Math.min(420, parent.width * 0.9)
        height: parent.height
        color: "#f012121f"
        border.color: "#33ffffff"
        radius: 12

        MouseArea {
            anchors.fill: parent
            onClicked: {} // block click-through to backdrop
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            z: 1
            spacing: 8

            RowLayout {
                Layout.fillWidth: true
                Label { text: "Tasks"; font.pixelSize: 20; color: "white"; Layout.fillWidth: true }
                ChromeButton { text: "Close"; onClicked: taskController.setMenuOpen(false) }
            }

            TabBar {
                id: tabs
                Layout.fillWidth: true
                TabButton { text: "Active"; onClicked: taskController.setBucketFilter("active") }
                TabButton { text: "Pending"; onClicked: taskController.setBucketFilter("pending") }
                TabButton { text: "Future"; onClicked: taskController.setBucketFilter("future") }
                TabButton { text: "All"; onClicked: taskController.setBucketFilter("all") }
            }

            Button {
                text: "+ New task"
                Layout.fillWidth: true
                onClicked: taskController.createTask("New task")
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                ListView {
                    model: taskController.model
                    spacing: 4
                    delegate: TaskRow { width: ListView.view.width }
                }
            }
        }
    }
}
