import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15


Item {
    id: rootTextEditorItem
    property int fixedHeight: 23
    property int top_margin: 5
    property int left_margin: 2
    property int right_margin: 0
    property int frame: 25
    property string currentText: frame.toString()
    property color background_Color: "#2B2B2B"

    Layout.preferredWidth: 88
    Layout.preferredHeight: fixedHeight

    Layout.topMargin: top_margin
    Layout.leftMargin: left_margin
    Layout.rightMargin: right_margin

    Layout.alignment: Qt.AlignTop

    Rectangle {
        id: textEditorBackground
        anchors.fill: parent
        color: background_Color
        radius: 4

        border.width: 2
        border.color: "#00000000"
        clip: true

        TextInput {
            id: textEditorInput
            
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            anchors.leftMargin: 4
            anchors.rightMargin: 4

            color: "#D7D7D7"
            text: rootTextEditorItem.currentText
            verticalAlignment: TextInput.AlignVCenter

            selectByMouse: true
            selectionColor: "#5285A6"

            onEditingFinished: {
                // currentText = text
                frame = parseInt(text)
                focus = false
            }

            onActiveFocusChanged: {
                if (activeFocus) {
                    selectAll()
                    textEditorBackground.border.color = "#5285A6"
                } else {
                    textEditorBackground.border.color = "#00000000"
                }
            }
        }
    }
}
