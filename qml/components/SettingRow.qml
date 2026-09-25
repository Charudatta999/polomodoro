import QtQuick
import QtQuick.Layouts
import Polomodoro

RowLayout {
    id: root
    property string label: ""
    property string key: ""
    default property alias content: holder.data
    width: parent ? parent.width : implicitWidth
    spacing: Theme.md + 2

    ColumnLayout {
        Layout.fillWidth: false
        Layout.minimumWidth: 140
        spacing: 2
        Text { text: root.label; font.family: Theme.fontFamily; font.pixelSize: 13; font.weight: Font.Medium; color: Theme.textPrimary }
        Text { text: root.key; font.family: Theme.monoFamily; font.pixelSize: 11; color: Theme.textFaint }
    }
    Item { Layout.fillWidth: true }
    RowLayout {
        id: holder
        spacing: Theme.md
        Layout.alignment: Qt.AlignVCenter
        Layout.fillWidth: false
    }
}
