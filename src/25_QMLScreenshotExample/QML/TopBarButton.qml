import QtQuick 2.12
import QtQuick.Controls 2.12
import QtGraphicalEffects 1.12

Button {
    id: btnTopBar

    // CUSTOM PROPERTIES
    property url btnIconSource: "../images/minimize_icon.svg"
    property color btnColorDefault: "#2f2f2f"
    property color btnColorMouseOver: "#23272E"
    property color btnColorClicked: "#00a1f1"

    QtObject{
        id: internal

        // MOUSE OVER AND CLICK CHANGE COLOR
        property var dynamicColor: btnTopBar.down ? btnColorClicked : btnTopBar.hovered ? btnColorMouseOver : btnColorDefault
    }

    width: 35
    height: 35

    background: Rectangle {
        id: bgBtn
        color: internal.dynamicColor

        Image {
            id: iconBtn
            source: btnIconSource
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            height: 16
            width: 16
            visible: false
            fillMode: Image.PreserveAspectFit
            antialiasing: false
        }

        ColorOverlay {
            anchors.fill: iconBtn
            source: iconBtn
            color: "#ffffff"
            antialiasing: false
        }
    }
}


