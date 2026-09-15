import QtQuick
import QtQuick.Shapes

Item {
    property string timeText: "25:00"
    property bool running: false
    width: 200
    height: 200

    Shape {
        anchors.fill: parent
        ShapePath {
            strokeColor: "#333355"
            strokeWidth: 8
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            startX: width/2; startY: 10
            PathAngleArc {
                centerX: width/2; centerY: height/2
                radiusX: 90; radiusY: 90
                startAngle: 0; sweepAngle: 360
            }
        }
        ShapePath {
            strokeColor: running ? "#7c5cff" : "#555577"
            strokeWidth: 8
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            startX: width/2; startY: 10
            PathAngleArc {
                centerX: width/2; centerY: height/2
                radiusX: 90; radiusY: 90
                startAngle: -90; sweepAngle: 270
            }
        }
    }

    Text {
        anchors.centerIn: parent
        text: timeText
        font.pixelSize: 42
        font.family: "monospace"
        color: "white"
    }
}
