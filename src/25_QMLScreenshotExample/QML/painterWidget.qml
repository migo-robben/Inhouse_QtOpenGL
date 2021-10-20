import QtQuick 2.12
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.12

Rectangle {
    id: background
    width: 1000
    height: 580 - toolBarPadding

    color: "#0D002878"
    border.color: "#2796DF"
    border.width: 1
    z: -1

    // Propeties
    property int windowStatus: 0
    property int toolBarPadding: 40

    function resizeWidthHeight(w, h) {
        background.width = w
        background.height = h - toolBarPadding
    }

    function receiveCaptureScreenFromCpp(url) {
        screenShotImage.visible = true
        screenShotImage.source = url
        screenShotImageAnim.running = true
    }

    QtObject {
        id: internalFunction

        function maximizeRestore() {
            if (windowStatus == 0) {
                backend.maximizeWindow()
                windowStatus = 1
                btnMaximizeRestore.btnIconSource = "../images/restore_icon.svg"
            } else {
                backend.normalWindow()
                windowStatus = 0
                btnMaximizeRestore.btnIconSource = "../images/maximize_icon.svg"
            }
        }

        function restore() {
            windowStatus = 0
            btnMaximizeRestore.btnIconSource = "../images/maximize_icon.svg"
        }

        function callWidgetToCaptureScreen() {
            backend.captureScreenFromQML(titleBar.height, toolBarPadding)
        }
    }

    Rectangle {
        id: appContainer
        color: "#00000000" // transparent
        width: parent.width
        height: parent.height

        anchors {
            fill: background.fill
            margins: 1
        }

        // TitleBar Setting
        Rectangle {
            id: titleBar
            height: 35

            color: "#2f2f2f"

            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                rightMargin: 0
                leftMargin: 0
                topMargin: 0
            }

            Image {
                id: appLogoIcon
                anchors {
                    left: parent.left
                    top: parent.top
                    bottom: parent.bottom
                    leftMargin: 10
                }
                source: "../images/WSRNDLOGO.png"
                sourceSize.height: 26
                sourceSize.width: 26
                fillMode: Image.PreserveAspectFit
            }

            Label {
                id: label
                color: "#c3cbdd"
                text: qsTr("Murray Painter Renderer (Beta)")
                anchors.left: appLogoIcon.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.leftMargin: 5
                verticalAlignment: Text.AlignVCenter
                font.pointSize: 10
            }

            Row {
                id: rowBtns
                width: 105
                height: 35
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.topMargin: 0
                anchors.rightMargin: 0

                TopBarButton {
                    id: btnMinimize
                    onClicked: {
                        backend.minimizeWindow()
                        internalFunction.restore()
                    }
                }

                TopBarButton {
                    id: btnMaximizeRestore
                    btnIconSource: "../images/maximize_icon.svg"
                    onClicked: internalFunction.maximizeRestore()
                }

                TopBarButton {
                    id: btnClose
                    btnColorClicked: "#ff007f"
                    btnIconSource: "../images/close_icon.svg"
                    onClicked: backend.close()
                }
            }

            Rectangle {
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.bottom
                }
                
                width: parent.width
                height: 1
                color: background.border.color
            }

            MouseArea {
                id: moveWidgetArea
                height: 35
                width: titleBar.width - 3 * 35

                anchors {
                    top: parent.top
                    bottom: parent.bottom
                    left: parent.left
                }

                property int previousX
                property int previousY

                onPressed: {
                    previousX = mouseX
                    previousY = mouseY
                }

                onPositionChanged: {
                    var dx = mouseX - previousX
                    var dy = mouseY - previousY

                    backend.moveWindowPosition(dx, dy)
                }
            }
        }

        // Canvas Setting
        Rectangle {
            id: canvasArea
            color: "#00000000"
            width: parent.width
            height: background.height - titleBar.height

            anchors {
                top: titleBar.bottom
                left: parent.left
                right: parent.right
            }

            Canvas {
                id: paintCanvas
                width: parent.width
                height: parent.height

                // these properties allows to draw on canvas smoothly
                antialiasing: true
                // renderTarget: Canvas.FramebufferObject
                // renderStrategy: Canvas.Threaded

                property real lastX
                property real lastY
                property real arrowStartX
                property real arrowStartY
                property int lineWidth: 1
                property int toolStatus: 0
                property int canvasMargins: 20

                property var undoStack: []
                property var redoStack: []
                property var tempImageData
                property var tempImageDataArrow
                property int lastToolStatus
                property int preUndoStackLength: 0
                property bool mustGetImageData: false

                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                }

                function drawLine(ctx) {
                    ctx.lineWidth = paintCanvas.lineWidth
                    ctx.strokeStyle = colorSlider.selectedColor
                    ctx.lineJoin = 'round'
                    ctx.lineCap = 'round'
                    ctx.beginPath()
                    ctx.moveTo(lastX, lastY)
                    lastX = painterArea.mouseX
                    lastY = painterArea.mouseY
                    ctx.lineTo(lastX, lastY)
                    ctx.stroke()
                }

                function drawEraser(ctx) {
                    ctx.lineWidth = paintCanvas.lineWidth
                    ctx.globalCompositeOperation = 'destination-out'
                    ctx.beginPath()
                    ctx.moveTo(lastX, lastY)
                    lastX = painterArea.mouseX
                    lastY = painterArea.mouseY
                    ctx.lineTo(lastX, lastY)
                    ctx.stroke()
                    ctx.globalCompositeOperation = 'source-over'
                }

                function drawArrow(ctx) {
                    ctx.fillStyle = colorSlider.selectedColor
                    ctx.fill()

                    var offsetFactor = 22
                    var arrowScaleFactor = 2.0

                    var slopy = Math.atan2((lastY - arrowStartY), (lastX - arrowStartX))
                    var cosy = Math.cos(slopy)
                    var siny = Math.sin(slopy)

                    var OffsetPosX1 = -offsetFactor * cosy - (offsetFactor / arrowScaleFactor * siny)
                    var OffsetPosX2 = -offsetFactor * cosy + (offsetFactor / arrowScaleFactor * siny)
                    var OffsetPosY1 = -offsetFactor * siny + (offsetFactor / arrowScaleFactor * cosy)
                    var OffsetPosY2 = offsetFactor * siny + (offsetFactor / arrowScaleFactor * cosy)

                    offsetFactor = 18
                    arrowScaleFactor = 4

                    var OffsetPosX3 = -offsetFactor * cosy - (offsetFactor / arrowScaleFactor * siny)
                    var OffsetPosX4 = -offsetFactor * cosy + (offsetFactor / arrowScaleFactor * siny)
                    var OffsetPosY3 = -offsetFactor * siny + (offsetFactor / arrowScaleFactor * cosy)
                    var OffsetPosY4 = offsetFactor * siny + (offsetFactor / arrowScaleFactor * cosy)

                    ctx.beginPath()
                    ctx.moveTo(lastX, lastY)
                    ctx.lineTo(lastX + OffsetPosX1, lastY + OffsetPosY1)
                    ctx.lineTo(lastX + OffsetPosX3, lastY + OffsetPosY3)
                    ctx.lineTo(arrowStartX, arrowStartY)
                    ctx.lineTo(lastX + OffsetPosX4, lastY - OffsetPosY4)
                    ctx.lineTo(lastX + OffsetPosX2, lastY - OffsetPosY2)
                    ctx.closePath()
                }

                function drawUndoRedo(ctx) {
                    if (undoStack.length > 0) {
                        if (isImageLoaded(tempImageData)) {
                            ctx.clearRect(0, 0, paintCanvas.width, paintCanvas.height)
                            ctx.drawImage(tempImageData, 0, 0)
                            unloadImage(tempImageData)
                        }
                    } else {
                        ctx.clearRect(0, 0, paintCanvas.width, paintCanvas.height)
                    }
                }

                function beforeDrawUndoRedo() {
                    if (undoStack.length > 0) {
                        tempImageData = undoStack[undoStack.length - 1]
                        if (!paintCanvas.isImageLoaded(tempImageData))
                            paintCanvas.loadImage(tempImageData)
                    }
                }

                function simpleDraw(ctx) {
                    // Just for debug
                    // setup the stroke
                    ctx.strokeStyle = "red"

                    // create a path
                    ctx.beginPath()
                    ctx.moveTo(50,50)
                    ctx.lineTo(150,50)

                    // stroke path
                    ctx.stroke()
                }

                onPaint: {
                    var ctx = getContext('2d')
                    switch (toolStatus) {
                        case 0:
                            drawLine(ctx)
                            break
                        case 1:
                            drawEraser(ctx)
                            break
                        case 2: // draw arrow
                            lastX = painterArea.mouseX
                            lastY = painterArea.mouseY

                            if (mustGetImageData) {
                                tempImageDataArrow = ctx.getImageData(0, 0, paintCanvas.width, paintCanvas.height)
                                mustGetImageData = false
                            }

                            ctx.clearRect(0, 0, paintCanvas.width, paintCanvas.height)
                            if (undoStack.length > 0)
                                ctx.putImageData(tempImageDataArrow, 0, 0, 0, 0, paintCanvas.width, paintCanvas.height)
                            drawArrow(ctx)
                            break
                        case 3: // undo
                            beforeDrawUndoRedo()
                            drawUndoRedo(ctx)
                            break
                        case 4: // redo
                            beforeDrawUndoRedo()
                            drawUndoRedo(ctx)
                            break
                        case 5: // load image
                            break
                        case 6: // capture image
                            beforeCaptureImage()
                            drawCaptureImage(ctx)
                            break
                        default:
                            console.log("default")
                    }
                }

                onImageLoaded: {
                    // console.log("Finish Loaded Image.")
                    requestPaint()
                }

                onPainted: {
                    // console.log("On onPainted")

                    if (toolStatus === 3 || toolStatus === 4 || toolStatus === 5 || toolStatus === 6 || toolStatus === 7) {
                        toolStatus = lastToolStatus
                    }

                    // console.log("UndoStack lenght: ", undoStack.length, ", RedoStack lenght: ", redoStack.length, "Tool Status: ", toolStatus)
                    // console.log("Current tool status: ", toolStatus)
                }

                Component.onCompleted: {
                    // console.log("On onCompleted")
                }

                MouseArea {
                    id: painterArea
                    width: paintCanvas.width
                    height: paintCanvas.height
                    anchors.fill: parent.fill

                    onPressed: {
                        paintCanvas.preUndoStackLength = paintCanvas.undoStack.length
                        paintCanvas.lastX = mouseX
                        paintCanvas.lastY = mouseY

                        paintCanvas.arrowStartX = mouseX
                        paintCanvas.arrowStartY = mouseY
                    }

                    onPositionChanged: {
                        paintCanvas.requestPaint()
                    }

                    onWheel: {
                        paintCanvas.lineWidth = paintCanvas.lineWidth >= 1 ? (paintCanvas.lineWidth + wheel.angleDelta.y / 120) : 1
                    }

                    onReleased: {
                        // console.log("Mouse Release")

                        // push current history to undoStack
                        var imageDataURL = paintCanvas.toDataURL('image/png')
                        paintCanvas.undoStack.push(imageDataURL)

                        if (paintCanvas.redoStack.length !== 0) {
                            while(paintCanvas.redoStack.length > 0) {
                                paintCanvas.redoStack.pop()
                            }
                        }

                        // To prevent tailing, you can comment the following code to see the tailing effects
                        if (paintCanvas.toolStatus === 0) {
                            paintCanvas.toolStatus = 3
                            paintCanvas.requestPaint()
                        }

                        if (paintCanvas.toolStatus === 2) {
                            paintCanvas.toolStatus = 3
                            paintCanvas.mustGetImageData = true
                            paintCanvas.requestPaint()
                        }
                    }
                }
            }

            Image {
                id: screenShotImage
                width: parent.width
                height: parent.height
                property double scaleFractor: 1.0

                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                }

                visible: false

                
                transform: Scale {
                    origin.x: width / 2
                    origin.y: height / 2
                    xScale: screenShotImage.scaleFractor
                    yScale: screenShotImage.scaleFractor
                }
            }

            PropertyAnimation {
                id: screenShotImageAnim
                target: screenShotImage
                property: "scaleFractor"
                to: 0.0
                duration: 500
                easing.type: Easing.OutQuad

                onFinished: {
                    screenShotImage.visible = false
                    screenShotImage.scaleFractor = 1.0
                }
            }
        }

        // Bottom ToolBar Setting
        Rectangle {
            id: toolBar
            width: appContainer.width
            height: toolBarPadding
            color: "#00000000"

            anchors {
                left: parent.left
                right: parent.right
                top: parent.bottom
                topMargin: 0
                rightMargin: 0
                leftMargin: 0
            }

            RowLayout {
                id: rowToolBtns
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                Layout.fillWidth: true

                spacing: 0

                ToolBarButton {
                    id: pencilBtn

                    isSelected: true
                    onClicked: {
                        isSelected = true
                        eraserBtn.isSelected = false
                        arrowBtn.isSelected = false
                        paintCanvas.toolStatus = 0
                        paintCanvas.lastToolStatus = paintCanvas.toolStatus
                    }
                }

                ToolBarButton {
                    id: eraserBtn
                    btnIconSource: "../images/painter/eraser.png"

                    onClicked: {
                        isSelected = true
                        pencilBtn.isSelected = false
                        arrowBtn.isSelected = false
                        paintCanvas.toolStatus = 1
                    }
                }

                ToolBarButton {
                    id: arrowBtn
                    btnIconSource: "../images/painter/arrow.png"

                    onClicked: {
                        isSelected = true
                        pencilBtn.isSelected = false
                        eraserBtn.isSelected = false
                        paintCanvas.toolStatus = 2
                        paintCanvas.lastToolStatus = paintCanvas.toolStatus

                        if (paintCanvas.undoStack.length > 0) {
                            paintCanvas.mustGetImageData = true
                        }
                    }
                }

                ToolBarButton {
                    id: undoBtn
                    btnIconSource: "../images/painter/undo.png"

                    onClicked: {
                        if (paintCanvas.undoStack.length > 0) {
                            var lastImageData = paintCanvas.undoStack.pop()
                            paintCanvas.redoStack.push(lastImageData)
                            
                            // keep last tool status
                            paintCanvas.lastToolStatus = paintCanvas.toolStatus
                            paintCanvas.toolStatus = 3
                            paintCanvas.requestPaint()
                        }
                    }
                }

                ToolBarButton {
                    id: redoBtn
                    btnIconSource: "../images/painter/redo.png"

                    onClicked: {
                        if (paintCanvas.redoStack.length > 0) {
                            var lastImageData = paintCanvas.redoStack.pop()
                            paintCanvas.undoStack.push(lastImageData)

                            // keep last tool status
                            paintCanvas.lastToolStatus = paintCanvas.toolStatus
                            paintCanvas.toolStatus = 4
                            paintCanvas.requestPaint()
                        }
                    }
                }

                ToolBarButton {
                    id: saveImageBtn
                    btnIconSource: "../images/painter/saveImage.png"

                    onClicked: {
                        backend.batchSaveCaptureScreenImageToDisk()
                    }
                }

                ToolBarButton {
                    id: screenshotBtn
                    btnIconSource: "../images/painter/screenshot.png"

                    onClicked: {
                        internalFunction.callWidgetToCaptureScreen()
                    }
                }

                ToolBarButton {
                    id: settingBtn
                    btnIconSource: "../images/painter/setting.png"

                    ColorSlider {
                        id: colorSlider
                    }

                    PropertyAnimation {
                        id: animationColorSlider
                        target: colorSlider
                        property: "scaleFractor"
                        to: 1
                        duration: 500
                        easing.type: Easing.OutBack
                    }

                    onClicked: {
                        settingBtn.isSelected = !settingBtn.isSelected

                        animationColorSlider.to = settingBtn.isSelected ? 1 : 0
                        animationColorSlider.easing.type = settingBtn.isSelected ? Easing.OutBack : Easing.InBack
                        animationColorSlider.running = true
                    }
                }

                Rectangle {
                    id: resizeToolArea

                    color: "#00000000"
                    width: toolBar.width - 8 * 35
                    height: 30

                    function repaintThelastFrame() {
                        if (paintCanvas.undoStack.length > 0) {                                    
                            // keep last tool status
                            paintCanvas.toolStatus = 3
                            paintCanvas.requestPaint()
                        }
                    }

                    Rectangle {
                        id: resizeArea
                        color: "#2F2F2F"
                        width: 30
                        height: 30

                        anchors {
                            right: parent.right
                        }

                        radius: 5

                        Image {
                            anchors.centerIn: resizeArea
                            source: "../images/painter/scale.png"
                            sourceSize.height: 16
                            sourceSize.width: 16
                        }

                        MouseArea {
                            id: resizeWindowArea
                            width: parent.width
                            height: parent.height

                            property int previousX
                            property int previousY
                            property var lastToolStatus

                            onPressed: {
                                previousX = mouseX
                                previousY = mouseY

                                lastToolStatus = paintCanvas.toolStatus
                            }

                            onPositionChanged: {
                                var dx = mouseX - previousX
                                var dy = mouseY - previousY
                                backend.resizeWindow(dx, dy)
                            }

                            onReleased: {
                                paintCanvas.toolStatus = lastToolStatus
                                resizeToolArea.repaintThelastFrame()
                            }
                        }
                    }
                }
            }
        }
    }
}