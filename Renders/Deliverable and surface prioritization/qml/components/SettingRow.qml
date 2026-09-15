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
        spacing: 2
        Text { text: root.label; font.family: Theme.fontFamily; font.pixelSize: 12.5; font.weight: Font.Medium; color: Theme.textPrimary }
        Text { text: root.key; font.family: Theme.monoFamily; font.pixelSize: 10.5; color: Theme.textFaint }
    }
    Item { Layout.fillWidth: true }
    Item {
        id: holder
        implicitWidth: childrenRect.width
        implicitHeight: childrenRect.height
        Layout.alignment: Qt.AlignVCenter
    }
}
