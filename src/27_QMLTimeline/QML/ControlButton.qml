import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15


Button {
    id: customControlButton

    property int buttonSize: 24
    property url btnIconSource: "icons/play_go_to_start.png"
    property int sidePadding: 0
    property int left_margin: 0
    property int right_margin: 0
    property color pressed_Color: "#ABABAB"
    property color mouseOver_Color: "#525252"

    QtObject {
        id: internal

        property var dynamicColor: customControlButton.hovered ? mouseOver_Color : "#00000000"
    }

    Layout.preferredWidth: buttonSize
    Layout.preferredHeight: buttonSize

    Layout.topMargin: 12 // res = 24 - buttonSize / 2
    Layout.leftMargin: left_margin
    Layout.rightMargin: right_margin

    Layout.alignment: Qt.AlignTop

    padding: sidePadding

    background: Rectangle {
        color: internal.dynamicColor

        Image {
            id: iconBtn
            anchors.centerIn: parent
            source: btnIconSource
            height: 18
            width: 18
            fillMode: Image.PreserveAspectFit
        }

        ColorOverlay {
            anchors.fill: iconBtn
            source: iconBtn
            color: pressed_Color
        }
    }
}