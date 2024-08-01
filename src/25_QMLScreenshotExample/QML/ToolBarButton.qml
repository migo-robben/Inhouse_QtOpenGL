import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.12

RoundButton {
    id: btnToolBar

    // CUSTOM PROPERTIES
    property url btnIconSource: "../images/painter/pencil.png"
    property color btnColorDefault: "#2F2F2F"
    property color btnColorMouseOver: "#0D628C"
    property color btnColorClicked: "#00a1f1"
    property bool isSelected: false

    QtObject {
        id: internal
        property var dynamicColor: btnToolBar.down ? btnColorClicked : btnToolBar.hovered ? btnColorMouseOver : btnColorDefault
    }

    Layout.preferredWidth: 30
    Layout.preferredHeight: 30
    Layout.leftMargin: 5
    radius: 15
    padding: 5

    background: Rectangle {
        radius: btnToolBar.radius
        color: isSelected ? btnColorClicked : internal.dynamicColor
    }

    icon {
        source: btnIconSource
        color: "transparent"
    }
}
