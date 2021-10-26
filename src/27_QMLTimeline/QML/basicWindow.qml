import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtGraphicalEffects 1.15


ApplicationWindow {
    id: root
    visible: true
    title: qsTr("Basic Window")

    flags: Qt.Window | Qt.FramelessWindowHint
    
    width: 1284
    height: 124
    minimumWidth: 1280
    minimumHeight: 120

    color: "#00000000"

    property int controlAreaWidth: 300

    QtObject {
        id: internal

        function computeHowToDrawTimeLine() {
            var animationStartFrameValue = animationStartFrame.frame
            var playbackStartFrameValue = playbackStartFrame.frame
            var playbackEndFrameValue = playbackEndFrame.frame
            var animationEndFrameValue = animationEndFrame.frame
            var numberFrameRect = playbackEndFrameValue - playbackStartFrameValue + 1 // 如果每一帧都画，要画的帧数

            var D = (timelineRectRow.width / numberFrameRect) - 2 // 2为帧的Rect的像素（2pix），此值为帧与帧之间的距离（像素为单位）
            var step = 1

            while (D < 40.0) {
                // 迭代寻找满足条件的step和D
                step += 1
                D = timelineRectRow.width / (numberFrameRect / step) - 2
            }
            D = Math.floor(D)

            timelineBackground.numberFrameRect = Math.ceil(numberFrameRect / step) // 最后要画多少帧
            timelineBackground.step = step // 步辐值
            timelineBackground.frameRectDistance = D
        }

        function computeDrawCurrentFrame() {
            var startX = 0
            var frameWidth = 0

            var currentFrameValue = parseInt(currentFrameTextEditor.currentText)
            var playbackStartFrameValue = playbackStartFrame.frame
            var playbackEndFrameValue = playbackEndFrame.frame

            if (currentFrameValue >= playbackStartFrameValue && currentFrameValue <= playbackEndFrameValue) {
                var currentScope = Math.floor((currentFrameValue - playbackStartFrameValue) / timelineBackground.step) + 1 // 当前所处的区间
                var pixelPerFrameRect = ((timelineBackground.frameRectDistance + 2) / timelineBackground.step)   // 当前帧Rect占用的基本像素
                
                var remainPixel = (timelineBackground.frameRectDistance + 2) - (pixelPerFrameRect * timelineBackground.step) // 剩余的像素
                var numberOfFrameWithinScope = timelineBackground.step

                var scopeStartPixel = (currentScope - 1 ) * (2 + timelineBackground.frameRectDistance) // 该区间的起始像素值
                var scopeStartFrame = playbackStartFrameValue + (currentScope - 1) * timelineBackground.step // 当前区间的第一帧

                // Result
                frameWidth = currentFrameValue - scopeStartFrame + 1 > remainPixel ?  pixelPerFrameRect : pixelPerFrameRect + 1
                frameWidth = Math.floor(frameWidth)
                if (frameWidth === 0) {
                    frameWidth += 1
                } // frameWidth为最后表示当前帧的rect的宽度（像素为单位）

                var frameIndex = currentFrameValue - scopeStartFrame + 1

                if (frameIndex <= remainPixel + 1) {
                    startX = scopeStartPixel + (frameIndex - 1) * (pixelPerFrameRect + 1)
                } else {
                    startX = scopeStartPixel + remainPixel * (pixelPerFrameRect + 1) + (frameIndex - remainPixel - 1) * pixelPerFrameRect
                }
            }

            frameRect.rectWidth = frameWidth
            frameRect.startX = Math.floor(startX) // 该rect要画的起始位置
        }

        function computeSelectedFrame(mx) {
            // 计算鼠标点击处为第几帧
            var clickedAt = mx

            var step = timelineBackground.step
            var D = timelineBackground.frameRectDistance
            var playbackStartFrameValue = playbackStartFrame.frame
            var playbackEndFrameValue = playbackEndFrame.frame

            var clickedScope = Math.floor(clickedAt / (2 + D)) + 1
            var scopeStartPixel = (clickedScope - 1) * (2 + D)
            var scopeEndPixel = clickedScope * (2 + D)

            var index = Math.floor((clickedAt - scopeStartPixel) / (scopeEndPixel - scopeStartPixel) * step)
            var selectedFrame = playbackStartFrameValue + (clickedScope - 1) * step + index

            if (selectedFrame < playbackStartFrameValue) {
                selectedFrame = playbackStartFrameValue
            } else if (selectedFrame > playbackEndFrameValue) {
                selectedFrame = playbackEndFrameValue
            }

            currentFrameTextEditor.frame = selectedFrame
        }

        function computeForwardDuration() {
            var fDruation = (playbackEndFrame.frame - currentFrameTextEditor.frame) / fps.frame * 1000
            fDruation = fDruation < 0 ? 0 : fDruation
            return fDruation
        }

        function computeBackwardDuration() {
            var bDration = (parseInt(currentFrameTextEditor.currentText) - playbackStartFrame.frame) / parseInt(fps.currentText) * 1000
            bDration = bDration < 0 ? 0 : bDration

            return bDration
        }
    }

    Rectangle {
        id: background

        width: 1280
        height: 120

        anchors {
            centerIn: parent
        }
        color: "#444444"
        radius: 5

        RowLayout {
            id: timelineLayout
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            property bool backwardStatus: true
            property bool forwardStatus: true

            Rectangle {
                id: timelineBackground

                Layout.fillWidth: true
                Layout.minimumHeight: 80

                Layout.leftMargin: 5
                Layout.topMargin: 5

                color: "#2B2B2B"

                property int numberFrameRect: 0
                property int step: 0
                property int frameRectDistance: 0

                Rectangle {
                    id: timelineRectRow

                    property int fixedMargin: 5
                    property int spacing: timelineBackground.frameRectDistance
                    property int topPadding: 14
                    property int bottomPadding: 2

                    height: timelineBackground.height - (fixedMargin - 3) * 2

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right

                    anchors.leftMargin: fixedMargin
                    anchors.rightMargin: fixedMargin
                    anchors.topMargin: fixedMargin - 3
                    anchors.bottomMargin: fixedMargin - 3

                    color: "#00000000"

                    Repeater {
                        id: repeaterFrame
                        model: timelineBackground.numberFrameRect
                        
                        anchors.fill: parent

                        Item {
                            anchors.top: parent.top
                            anchors.left: parent.left

                            anchors.leftMargin: index * (timelineBackground.frameRectDistance + 2)

                            Column {
                                anchors.fill: parent
                                spacing: 5

                                Label {
                                    text: playbackStartFrame.frame + index * timelineBackground.step
                                    color: "#7C7C7C"
                                    font.pointSize: 8

                                }

                                Rectangle {
                                    height: timelineRectRow.height - 18
                                    width: 2
                                    color: "#7C7C7C"
                                }
                            }
                        }
                    }

                    Rectangle {
                        id: frameRect
                        anchors.top: parent.top

                        property int startX
                        property int rectWidth
                        
                        x : startX
                        width: rectWidth
                        height: timelineRectRow.height
                        color: "#4D7C7C7C"

                        Label {
                            id: displayCurrentFrame
                            anchors.bottom: parent.bottom
                            anchors.bottomMargin: 5
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: currentFrameTextEditor.frame
                            color: frameRect.rectWidth > 0 ? "#C4C4C4" : "#00000000"
                        }
                    }

                    MouseArea {
                        id: selectedFrameArea
                        anchors.fill: parent

                        onPressed: {
                            internal.computeSelectedFrame(mouseX)
                        }

                        onPositionChanged: {
                            internal.computeSelectedFrame(mouseX)
                        }
                    }
                }
            }

            CustomTextField {
                id: currentFrameTextEditor
                top_margin: 13

                property bool status: true
                property bool textEnabled: true

                background_Color: status ? "#2B2B2B" : "#404040"

                enabled: textEnabled

                onCurrentTextChanged: {
                    internal.computeDrawCurrentFrame()
                }
            }

            PropertyAnimation {
                id: playForward
                target: currentFrameTextEditor
                property: "frame"
                to: playbackEndFrame.frame
                duration: internal.computeForwardDuration()
                easing.type: Easing.Linear

                onFinished: {
                    currentFrameTextEditor.frame = playbackStartFrame.frame
                    playForward.restart()
                }
            }

            PropertyAnimation {
                id: playBackward
                target: currentFrameTextEditor
                property: "frame"
                to: playbackStartFrame.frame
                duration: internal.computeBackwardDuration()
                easing.type: Easing.Linear

                onFinished: {
                    currentFrameTextEditor.frame = playbackEndFrame.frame
                    playBackward.restart()
                }
            }

            ControlButton {
                id: playGoToStart

                onClicked: {
                    var playbackStartFrameValue = playbackStartFrame.frame
                    currentFrameTextEditor.frame = playbackStartFrameValue
                    
                    if (playForward.running) {
                        playForward.stop()
                        playForward.start()
                    }

                    if (playBackward.running) {
                        playBackward.stop()
                        playBackward.start()
                    }
                }
            }

            ControlButton {
                id: playLastFrame
                btnIconSource: "icons/play_last_frame.png"
                sidePadding: 3
                left_margin: -1
            }

            ControlButton {
                id: playBackwards

                btnIconSource: timelineLayout.backwardStatus ? "icons/play_backwards.png" : "icons/stop.png"
                pressed_Color: timelineLayout.backwardStatus ? "#ABABAB" : "#FF0000"
                
                sidePadding: 3
                left_margin: -4

                onClicked: {
                    playBackward.stop()
                    playForward.stop()

                    timelineLayout.backwardStatus = !timelineLayout.backwardStatus
                    timelineLayout.forwardStatus = true
                    currentFrameTextEditor.status = timelineLayout.backwardStatus
                    currentFrameTextEditor.textEnabled = timelineLayout.backwardStatus
                    fps.status = timelineLayout.backwardStatus
                    fps.textEnabled = timelineLayout.backwardStatus

                    var currentFrameValue = parseInt(currentFrameTextEditor.currentText)
                    var playbackStartFrameValue = playbackStartFrame.frame
                    var playbackEndFrameValue = playbackEndFrame.frame

                    if (!timelineLayout.backwardStatus) {
                        if (currentFrameValue < playbackStartFrameValue || currentFrameValue > playbackEndFrameValue) {
                            currentFrameValue = playbackEndFrameValue
                            currentFrameTextEditor.frame = currentFrameValue
                        }
                    }

                    if (!timelineLayout.backwardStatus)
                        playBackward.start()
                }
            }

            ControlButton {
                id: playForwards

                btnIconSource: timelineLayout.forwardStatus ? "icons/play_forwards.png" : "icons/stop.png"
                pressed_Color: timelineLayout.forwardStatus ? "#ABABAB" : "#FF0000"

                sidePadding: 3
                left_margin: -3

                onClicked: {
                    playForward.stop()
                    playBackward.stop()

                    timelineLayout.forwardStatus = !timelineLayout.forwardStatus
                    timelineLayout.backwardStatus = true
                    currentFrameTextEditor.status = timelineLayout.forwardStatus
                    currentFrameTextEditor.textEnabled = timelineLayout.forwardStatus
                    fps.status = timelineLayout.forwardStatus
                    fps.textEnabled = timelineLayout.forwardStatus

                    var currentFrameValue = parseInt(currentFrameTextEditor.currentText)
                    var playbackStartFrameValue = playbackStartFrame.frame
                    var playbackEndFrameValue = playbackEndFrame.frame

                    if (!timelineLayout.forwardStatus) {
                        if (currentFrameValue < playbackStartFrameValue || currentFrameValue > playbackEndFrameValue) {
                            currentFrameValue = playbackStartFrameValue
                            currentFrameTextEditor.frame = currentFrameValue
                        }
                    }

                    if (!timelineLayout.forwardStatus)
                        playForward.start()
                }
            }

            ControlButton {
                id: playNextFrame
                btnIconSource: "icons/play_next_frame.png"
                sidePadding: 3
                left_margin: -4
            }

            ControlButton {
                id: playGoToEnd
                btnIconSource: "icons/play_go_to_end.png"
                left_margin: -2
                right_margin: 2

                onClicked: {
                    var playbackEndFrameValue = playbackEndFrame.frame
                    currentFrameTextEditor.frame = playbackEndFrameValue
                    
                    if (playForward.running) {
                        playForward.stop()
                        playForward.start()
                    }

                    if (playBackward.running) {
                        playBackward.stop()
                        playBackward.start()
                    }
                }
            }
        }

        Rectangle {
            id: spliter
            anchors.top: timelineLayout.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 2
            height: 3

            color: "#373737"
        }

        RowLayout {
            id: rangeSliderLayout
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: timelineLayout.bottom

            CustomTextField {
                id: animationStartFrame
                fixedHeight: 25
                top_margin: 7
                left_margin: 5

                frame: 1

                onFrameChanged: {
                    var animationStartFrameValue = animationStartFrame.frame
                    var playbackStartFrameValue = playbackStartFrame.frame
                    var playbackEndFrameValue = playbackEndFrame.frame
                    var animationEndFrameValue = animationEndFrame.frame

                    if (animationStartFrameValue >= playbackEndFrameValue) {
                        animationStartFrameValue = playbackEndFrameValue
                        playbackStartFrameValue = playbackEndFrameValue
                    } else if (animationStartFrameValue >= playbackStartFrameValue) {
                        playbackStartFrameValue = animationStartFrameValue
                    }

                    // console.log("Animation Start Frame Text Changed: \n\t",
                    //     animationStartFrameValue,
                    //     playbackStartFrameValue,
                    //     playbackEndFrameValue,
                    //     animationEndFrameValue)

                    // Set values
                    timeRangeSlider.from = animationStartFrameValue
                    timeRangeSlider.to = animationEndFrameValue
                    timeRangeSlider.setValues(playbackStartFrameValue, playbackEndFrameValue)

                    animationStartFrame.frame = animationStartFrameValue
                    playbackStartFrame.frame = playbackStartFrameValue
                    playbackEndFrame.frame = playbackEndFrameValue
                    animationEndFrame.frame = animationEndFrameValue     

                    // console.log(timeRangeSlider.from, timeRangeSlider.first.value, timeRangeSlider.second.value, timeRangeSlider.to)

                    // Compute how to draw timeline
                    internal.computeHowToDrawTimeLine()
                    internal.computeDrawCurrentFrame()
                }
            }

            CustomTextField {
                id: playbackStartFrame
                fixedHeight: 25
                top_margin: 7
                left_margin: 5

                frame: 10

                onFrameChanged: {
                    var animationStartFrameValue = animationStartFrame.frame
                    var playbackStartFrameValue = playbackStartFrame.frame
                    var playbackEndFrameValue = playbackEndFrame.frame
                    var animationEndFrameValue = animationEndFrame.frame

                    playbackStartFrameValue = playbackStartFrameValue > playbackEndFrameValue ? playbackEndFrameValue : playbackStartFrameValue
                    playbackStartFrameValue = playbackStartFrameValue < animationStartFrameValue ? animationStartFrameValue : playbackStartFrameValue

                    // console.log("Play Back Start Frame Text Changed: \n\t",
                    //     animationStartFrameValue,
                    //     playbackStartFrameValue,
                    //     playbackEndFrameValue,
                    //     animationEndFrameValue)

                    // Set values
                    timeRangeSlider.from = animationStartFrameValue
                    timeRangeSlider.to = animationEndFrameValue
                    timeRangeSlider.setValues(playbackStartFrameValue, playbackEndFrameValue)

                    animationStartFrame.frame = animationStartFrameValue
                    playbackStartFrame.frame = playbackStartFrameValue
                    playbackEndFrame.frame = playbackEndFrameValue
                    animationEndFrame.frame = animationEndFrameValue

                    // console.log(timeRangeSlider.from, timeRangeSlider.first.value, timeRangeSlider.second.value, timeRangeSlider.to)

                    // Compute how to draw timeline
                    internal.computeHowToDrawTimeLine()
                    internal.computeDrawCurrentFrame()
                }
            }

            Rectangle {
                id: rangeSlider

                Layout.fillWidth: true
                Layout.preferredHeight: 26

                Layout.rightMargin: 5
                Layout.leftMargin: 5
                Layout.topMargin: 7

                color: "#373737"

                RangeSlider {
                    id: timeRangeSlider
                    anchors.fill: parent

                    property int handleSize: 16

                    from: animationStartFrame.frame
                    to: animationEndFrame.frame

                    first {
                        value: playbackStartFrame.frame

                        handle: Rectangle {
                            id: leftHandle
                            x: timeRangeSlider.first.position * (rangeSlider.width - sliderBackground.sideMargin * 2 - timeRangeSlider.handleSize) + sliderBackground.sideMargin
                            width: timeRangeSlider.handleSize
                            height: timeRangeSlider.handleSize
                            radius: 3
                            anchors.verticalCenter: parent.verticalCenter

                            color: "#919191"
                        }

                        onMoved: {
                            var animationStartFrameValue = animationStartFrame.frame
                            var playbackStartFrameValue = playbackStartFrame.frame
                            var playbackEndFrameValue = playbackEndFrame.frame
                            var animationEndFrameValue = animationEndFrame.frame

                            // console.log("Left Handle On Moved: \n\t",
                            //     animationStartFrameValue,
                            //     playbackStartFrameValue,
                            //     playbackEndFrameValue,
                            //     animationEndFrameValue)

                            timeRangeSlider.setValues(Math.round(timeRangeSlider.first.value), Math.round(timeRangeSlider.second.value))
                            playbackStartFrame.frame = Math.round(timeRangeSlider.first.value)
                            // playbackStartFrame.currentText = Math.round(timeRangeSlider.first.value).toString()

                            // console.log(timeRangeSlider.from, timeRangeSlider.first.value, timeRangeSlider.second.value, timeRangeSlider.to)

                            // Compute how to draw timeline
                            internal.computeHowToDrawTimeLine()
                            internal.computeDrawCurrentFrame()
                        }
                    }

                    second {
                        value: playbackEndFrame.frame

                        handle: Rectangle {
                            id: rightHandle
                            x: timeRangeSlider.second.position * (rangeSlider.width - sliderBackground.sideMargin * 2 - timeRangeSlider.handleSize) + sliderBackground.sideMargin
                            width: timeRangeSlider.handleSize
                            height: timeRangeSlider.handleSize
                            radius: 3
                            anchors.verticalCenter: parent.verticalCenter

                            color: "#919191"
                        }

                        onMoved: {
                            var animationStartFrameValue = animationStartFrame.frame
                            var playbackStartFrameValue = playbackStartFrame.frame
                            var playbackEndFrameValue = playbackEndFrame.frame
                            var animationEndFrameValue = animationEndFrame.frame

                            // console.log("Right Handle On Moved: \n\t",
                            //     animationStartFrameValue,
                            //     playbackStartFrameValue,
                            //     playbackEndFrameValue,
                            //     animationEndFrameValue)

                            timeRangeSlider.setValues(Math.round(timeRangeSlider.first.value), Math.round(timeRangeSlider.second.value))
                            playbackEndFrame.frame = Math.round(timeRangeSlider.second.value)
                            // playbackEndFrame.currentText = Math.round(timeRangeSlider.second.value).toString()

                            // console.log(timeRangeSlider.from, timeRangeSlider.first.value, timeRangeSlider.second.value, timeRangeSlider.to)

                            // Compute how to draw timeline
                            internal.computeHowToDrawTimeLine()
                            internal.computeDrawCurrentFrame()
                        }
                    }

                    background: Rectangle {
                        id: sliderBackground
                        
                        property int sideMargin: 3
                        color: "#5D5D5D"
                        radius: 3

                        anchors.verticalCenter: parent.verticalCenter
                        
                        x: leftHandle.x - sideMargin
                        width: rightHandle.x - leftHandle.x + leftHandle.width + sideMargin * 2
                        height: rangeSlider.height - 4

                        MouseArea {
                            id: mainControlArea

                            anchors.fill: parent
                            anchors.leftMargin: 22
                            anchors.rightMargin: 22

                            // color: "#66FF0000"

                            property double pressedMouseX
                            property double pixelPerValueStep
                            property int differenceRightLeft

                            onPressed: {
                                pressedMouseX = mouseX
                                
                                var animationStartFrameValue = animationStartFrame.frame
                                var playbackStartFrameValue = playbackStartFrame.frame
                                var playbackEndFrameValue = playbackEndFrame.frame
                                var animationEndFrameValue = animationEndFrame.frame

                                pixelPerValueStep = 0

                                if (playbackStartFrameValue !== animationStartFrameValue) {
                                    pixelPerValueStep = (leftHandle.x - sliderBackground.sideMargin) / (playbackStartFrameValue - animationStartFrameValue)
                                } else if (playbackEndFrameValue !== animationEndFrameValue) {
                                    pixelPerValueStep = (timeRangeSlider.width - (rightHandle.x + timeRangeSlider.handleSize + sliderBackground.sideMargin)) / (animationEndFrameValue - playbackEndFrameValue)
                                }

                                differenceRightLeft = playbackEndFrameValue - playbackStartFrameValue
                            }

                            onPositionChanged: {
                                var mouseOffset = mouseX - pressedMouseX

                                var animationStartFrameValue = animationStartFrame.frame
                                var playbackStartFrameValue = playbackStartFrame.frame
                                var playbackEndFrameValue = playbackEndFrame.frame
                                var animationEndFrameValue = animationEndFrame.frame

                                if (pixelPerValueStep !== 0 ) {
                                    // Compute offset
                                    var offset = Math.round(mouseOffset / pixelPerValueStep)
                                    
                                    if (offset < 0) {
                                        playbackStartFrameValue += offset
                                        playbackStartFrameValue = playbackStartFrameValue < animationStartFrameValue ? animationStartFrameValue : playbackStartFrameValue
                                        playbackEndFrameValue = playbackStartFrameValue + differenceRightLeft
                                    } else if (offset >= 0) {
                                        playbackEndFrameValue += offset
                                        playbackEndFrameValue = playbackEndFrameValue > animationEndFrameValue ? animationEndFrameValue : playbackEndFrameValue
                                        playbackStartFrameValue = playbackEndFrameValue - differenceRightLeft
                                    }
                                }

                                // Set values
                                timeRangeSlider.from = animationStartFrameValue
                                timeRangeSlider.to = animationEndFrameValue
                                timeRangeSlider.setValues(playbackStartFrameValue, playbackEndFrameValue)

                                // animationStartFrame.currentText = animationStartFrameValue.toString()
                                // playbackStartFrame.currentText = playbackStartFrameValue.toString()
                                // playbackEndFrame.currentText = playbackEndFrameValue.toString()
                                // animationEndFrame.currentText = animationEndFrameValue.toString()

                                animationStartFrame.frame = animationStartFrameValue
                                playbackStartFrame.frame = playbackStartFrameValue
                                playbackEndFrame.frame = playbackEndFrameValue
                                animationEndFrame.frame = animationEndFrameValue
                            }
                        }
                    }
                }
            }

            CustomTextField {
                id: playbackEndFrame
                fixedHeight: 25
                top_margin: 7
                left_margin: 0

                frame: 155

                onFrameChanged: {
                    var animationStartFrameValue = animationStartFrame.frame
                    var playbackStartFrameValue = playbackStartFrame.frame
                    var playbackEndFrameValue = playbackEndFrame.frame
                    var animationEndFrameValue = animationEndFrame.frame

                    playbackEndFrameValue = playbackEndFrameValue > animationEndFrameValue ? animationEndFrameValue : playbackEndFrameValue
                    playbackEndFrameValue = playbackEndFrameValue < playbackStartFrameValue ? playbackStartFrameValue : playbackEndFrameValue

                    // console.log("Play Back End Frame Text Changed: \n\t",
                    //     animationStartFrameValue,
                    //     playbackStartFrameValue,
                    //     playbackEndFrameValue,
                    //     animationEndFrameValue)

                    // Set values
                    timeRangeSlider.from = animationStartFrameValue
                    timeRangeSlider.to = animationEndFrameValue
                    timeRangeSlider.setValues(playbackStartFrameValue, playbackEndFrameValue)

                    animationStartFrame.frame = animationStartFrameValue
                    playbackStartFrame.frame = playbackStartFrameValue
                    playbackEndFrame.frame = playbackEndFrameValue
                    animationEndFrame.frame = animationEndFrameValue

                    // console.log(timeRangeSlider.from, timeRangeSlider.first.value, timeRangeSlider.second.value, timeRangeSlider.to)

                    // Compute how to draw timeline
                    internal.computeHowToDrawTimeLine()
                    internal.computeDrawCurrentFrame()
                }
            }

            CustomTextField {
                id: animationEndFrame
                fixedHeight: 25
                top_margin: 7
                left_margin: 5

                frame: 200

                onFrameChanged: {
                    var animationStartFrameValue = animationStartFrame.frame
                    var playbackStartFrameValue = playbackStartFrame.frame
                    var playbackEndFrameValue = playbackEndFrame.frame
                    var animationEndFrameValue = animationEndFrame.frame

                    if (animationEndFrameValue <= playbackStartFrameValue) {
                        animationEndFrameValue = playbackStartFrameValue
                        playbackEndFrameValue = playbackStartFrameValue
                    } else if (animationEndFrameValue <= playbackEndFrameValue) {
                        playbackEndFrameValue = animationEndFrameValue
                    }

                    // console.log("Animation End Frame Text Changed: \n\t",
                    //     animationStartFrameValue,
                    //     playbackStartFrameValue,
                    //     playbackEndFrameValue,
                    //     animationEndFrameValue)

                    // Set values
                    timeRangeSlider.from = animationStartFrameValue
                    timeRangeSlider.to = animationEndFrameValue
                    timeRangeSlider.setValues(playbackStartFrameValue, playbackEndFrameValue)

                    animationStartFrame.frame = animationStartFrameValue
                    playbackStartFrame.frame = playbackStartFrameValue
                    playbackEndFrame.frame = playbackEndFrameValue
                    animationEndFrame.frame = animationEndFrameValue

                    // console.log(timeRangeSlider.from, timeRangeSlider.first.value, timeRangeSlider.second.value, timeRangeSlider.to)

                    // Compute how to draw timeline
                    internal.computeHowToDrawTimeLine()
                    internal.computeDrawCurrentFrame()
                }
            }

            Label {
                anchors.verticalCenter: animationEndFrame.verticalCenter
                text: "FPS"
                color: "#C4C4C4"
            }

            CustomTextField {
                id: fps
                fixedHeight: 25
                top_margin: 7
                left_margin: 5
                right_margin: 5
                frame: 24

                property bool status: true
                property bool textEnabled: true

                background_Color: status ? "#2B2B2B" : "#404040"
                enabled: textEnabled

                onFrameChanged: {
                    internal.computeForwardDuration()
                    internal.computeBackwardDuration()
                }
            }

            Component.onCompleted: {
                console.log("RangeSlider completed.")
                internal.computeHowToDrawTimeLine()
                internal.computeDrawCurrentFrame()
            }
        }
    }

    Shortcut {
        sequence: "Esc"
        onActivated: root.close()
    }
}
