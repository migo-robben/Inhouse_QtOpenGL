import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

Item {
    id: huePicker
    width: 300
    height: 12

    anchors.leftMargin: 10
    anchors.left: parent.right
    anchors.verticalCenter: parent.verticalCenter

    property color selectedColor: "#ff0000"
    property double scaleFractor: 0

    transform: Scale { origin.x: 0; origin.y: 0; xScale: scaleFractor}

    Rectangle {
        id: colorBar
        anchors.fill: parent
        radius: 2

        gradient: Gradient {
            orientation: Gradient.Horizontal
            GradientStop { position: 1.0;  color: "#FF0000" }
            GradientStop { position: 0.85; color: "#FFFF00" }
            GradientStop { position: 0.76; color: "#00FF00" }
            GradientStop { position: 0.5;  color: "#00FFFF" }
            GradientStop { position: 0.33; color: "#0000FF" }
            GradientStop { position: 0.16; color: "#FF00FF" }
            GradientStop { position: 0.0;  color: "#FF0000" }
        }
    }

    Item {
        id: sliderHandle
        width: 300
        height: 14

        property int cursorHeight: 4

        anchors.verticalCenter: parent.verticalCenter

        Item {
            id: pickerCursor

            // width: parent.width
            height: parent.height

            anchors.verticalCenter: parent.verticalCenter

            Rectangle {
                x: -3
                y: -sliderHandle.cursorHeight * 0.5
                width: 6
                height: parent.height + sliderHandle.cursorHeight
                border.color: "black"
                border.width: 1
                color: "transparent"

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 2
                    border.color: "white"
                    border.width: 1
                    color: "transparent"
                }
            }
        }

        MouseArea {
            id: pickerCursorMouseArea

            width: parent.width
            height: parent.height

            function handleMouse(mouse) {
                if (mouse.buttons & Qt.LeftButton) {
                    pickerCursor.x = Math.max(0, Math.min(width, mouse.x))
                }
            }

            function hslaColor(h, s, b, a) {
                var lightness = (2 - s) * b
                var satHSL = s * b / ((lightness <= 1) ? lightness : 2 - lightness)
                lightness /= 2

                huePicker.selectedColor = Qt.hsla(h, satHSL, lightness, a)
            }

            onPositionChanged: {
                handleMouse(mouse)
                // hslaColor(1 - (pickerCursor.x / 300), 1, 1, 1)
            }

            onPressed: {
                handleMouse(mouse)
            }

            onReleased: {
                hslaColor(1 - (pickerCursor.x / 300), 1, 1, 1)
            }
        }
    }
}