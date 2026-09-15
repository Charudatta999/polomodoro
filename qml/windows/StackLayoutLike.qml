import QtQuick

// Thin wrapper over StackLayout semantics without the layout attachment:
// all children stay alive, only one is visible.
Item {
    id: root
    property int current: 0
    onCurrentChanged: apply()
    Component.onCompleted: apply()
    function apply() {
        for (var i = 0; i < children.length; ++i) {
            children[i].visible = (i === current)
            children[i].anchors.fill = root
        }
    }
}
