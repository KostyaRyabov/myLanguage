import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12

import QtQuick.Shapes 1.12

import Translator 1.0

Window {
    id: window
    width: 1000
    minimumWidth: 900
    height: 500
    minimumHeight: 400
    visible: true
    title: qsTr("Vecto..")
    visibility: ApplicationWindow.Maximized

    Canvas{
        id: drawingCanvas
        anchors.left: codeArea.right
        width: window.width - codeArea.width
        height: window.height

        property variant list;

        onPaint: {
            var ctx = getContext("2d");

            ctx.reset()

            for (let f = 0; f < translator.amountOfFigures(); f++){
                ctx.lineWidth = translator.getStrokeWidth(f);
                ctx.strokeStyle = translator.getStrokeColor(f);

                ctx.beginPath();

                drawingCanvas.list = translator.getHidenEdges(f);

                if (translator.amountOfPointsOnFigure(f) > 0){

                    var pp = 1;

                    if (translator.amountOfPointsOnFigure(f) > 1){
                        ctx.moveTo(translator.getX(f,0), translator.getY(f,0))
                        for (; pp < translator.amountOfPointsOnFigure(f); pp++){
                            if (drawingCanvas.list.includes(pp)) {
                                ctx.moveTo(translator.getX(f,pp), translator.getY(f,pp))
                            }else{
                                ctx.lineTo(translator.getX(f,pp), translator.getY(f,pp))
                            }
                        }

                        pp--;

                        if (translator.getX(f,pp) === translator.getX(f,0) && translator.getY(f,pp) === translator.getY(f,0)){
                            ctx.fillStyle = translator.getFillColor(f);
                            ctx.fill();
                        }

                        ctx.stroke();
                    }

                    ctx.fillStyle = translator.getDotColor(f);

                    if (translator.getDotRadius(f) > 0){
                        for (pp = 0; pp < translator.amountOfPointsOnFigure(f); pp++){
                            ctx.beginPath();
                            ctx.arc(translator.getX(f,pp), translator.getY(f,pp), translator.getDotRadius(f), 360, 0, false);
                            ctx.fill();
                            ctx.stroke()
                        }
                    }
                }
            }
        }
    }

    Canvas{
        id: backCanvas
        width:drawingCanvas.width
        height: drawingCanvas.height

        visible: false
    }

    Rectangle{
        id:codeArea
        width: 600;
        height: parent.height
        anchors.left: parent.left

        color: "#BADBDE"

        Flickable {
            id:flic
            anchors.fill: parent
            boundsBehavior: Flickable.StopAtBounds

            visibleArea.onYPositionChanged: {
                errorRect.y = p.y + codeArea.y + textInput.y - flic.contentY
            }

            ScrollBar.vertical: ScrollBar {}

            FontMetrics {
                id: fontMetrics
                font: textInput.font
            }

            Rectangle {
                x: 0; y: textInput.cursorRectangle.y
                height: fontMetrics.height
                width: textInput.width
                color: "#8dc3c8"
                visible: textInput.activeFocus

                Behavior on y{ NumberAnimation { easing.type: Easing.OutElastic; easing.amplitude: 3.0; easing.period: 2.0; duration: 300 }}
            }

            TextArea.flickable: TextArea{
                id: textInput

                text: ""

                placeholderText: qsTr("Введите команду...")

                onTextChanged: translator.read(textInput.text)

                anchors.margins: 10
                wrapMode: TextEdit.Wrap
                font.pointSize: 12
                selectByMouse: true
            }
        }

        Rectangle{
            id:errorRect

            property rect p;
            property int newY: 0

            x: p.x + textInput.x
            y: p.y + textInput.y - flic.contentY
            width: codeArea.width - p.x - 10
            height: p.height

            onXChanged: {
                messageBox.x = errorRect.x-messageBox.width/2;

                if (messageBox.x < 10) messageBox.x = 10;
                else if (messageBox.x + messageBox.width > codeArea.width-10) messageBox.x = codeArea.width-messageBox.width-10;
            }

            onYChanged: {
                newY = errorRect.y+errorRect.height+10;

                if (newY < 20) {
                    arrow.opacity = 1;
                    arrow.scale = -1;

                    pointer.opacity = 0;
                    pointer.scale = 0;

                    newY = 20;
                } else if (newY+messageBox.height > codeArea.height-errorRect.height) {
                    if (codeArea.height <= errorRect.y+errorRect.height){
                        arrow.opacity = 1;
                        arrow.scale = 1;

                        pointer.opacity = 0;
                        pointer.scale = 0;

                        newY = codeArea.height-messageBox.height-errorRect.height;
                    }else{
                        pointer.opacity = 1;
                        pointer.scale = 1;

                        arrow.scale = 0;
                        arrow.opacity = 0;

                        newY = errorRect.y - messageBox.height-5;
                    }
                }else {
                    pointer.opacity = 1;
                    pointer.scale = -1;

                    arrow.scale = 0;
                    arrow.opacity = 0;
                }

                messageBox.y = newY;
            }

            gradient: Gradient {
                orientation: Gradient.Horizontal

                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.01; color: "red" }
                GradientStop { position: 0.8; color: "transparent" }
            }

            Behavior on width{ NumberAnimation { easing.type: Easing.OutElastic; duration: 300 } }
            Behavior on y{ NumberAnimation { easing.type: Easing.OutElastic; easing.amplitude: 3.0; easing.period: 2.0; duration: 300 }}
            Behavior on x{ NumberAnimation { easing.type: Easing.OutElastic; duration: 300 } }
            Behavior on opacity{ NumberAnimation { easing.type: Easing.OutElastic;  duration: 1000 } }
        }
    }

    Rectangle{
        id: messageBox
        width: errorOutput.width+10
        height: errorOutput.height+10

        color: "#f44848"

        radius: 3

        state: "passive"

        Shape {
            id: pointer

            width: 10
            height: 8

            scale: 0

            x: errorRect.x-messageBox.x;
            y: (messageBox.height-pointer.height)/2 + ((messageBox.height+pointer.height)/2)*pointer.scale;

            ShapePath {
                fillColor: "#f44848"
                strokeColor: "#f44848"

                startX: 0
                startY: 0

                PathLine { x: 0; y: 0 }
                PathLine { x: pointer.width/2; y: pointer.height }
                PathLine { x: pointer.width; y: 0 }
            }

            Behavior on scale{NumberAnimation { easing.type: Easing.InOutQuint; duration: 300 } }
            Behavior on opacity{ NumberAnimation { easing.type: Easing.InOutQuint; duration: 300 } }
        }

        Shape {
            id: arrow

            width: messageBox.width/10
            height: messageBox.height/4

            opacity: 0;
            scale: 0;

            x: (messageBox.width-arrow.width)/2;
            y: (messageBox.height-arrow.height)/2 + ((messageBox.height-arrow.height))*arrow.scale;

            ShapePath {
                fillColor: "#f44848"
                strokeColor: "#f44848"

                startX: 0
                startY: 0

                PathLine { x: 0; y: 0 }
                PathLine { x: arrow.width/2; y: arrow.height }
                PathLine { x: arrow.width; y: 0 }
            }

            Behavior on scale{ NumberAnimation { easing.type: Easing.InOutCubic; duration: 300 } }
            Behavior on opacity{ NumberAnimation { easing.type: Easing.InOutCubic; duration: 300 } }
        }

        MyTranslator{
            id: translator

            onGetError: {
                messageBox.state = "active"
                errorOutput.text = text

                errorRect.p = textInput.positionToRectangle(pos);

                errorRect.opacity = 0.3
            }

            onDrawChanged: {
                messageBox.state = "passive"
                drawingCanvas.requestPaint()
                errorRect.opacity = 0
            }
        }

        states: [
            State {
                name: "active"
                PropertyChanges { target: messageBox; opacity: 1; scale: 1;}
            },
            State {
                name: "passive"
                PropertyChanges { target: messageBox; opacity: 0; scale: 0.9;}
            }
        ]

        transitions: [
            Transition {
                to: "passive"
                PropertyAnimation { target: messageBox; property: "scale"; duration: 300; easing.type: Easing.InOutQuart}
                OpacityAnimator{ target: messageBox; duration: 250; easing.type: Easing.InOutCirc}
            },
            Transition {
                to: "active"
                PropertyAnimation { target: messageBox; property: "scale"; duration: 300; easing.type: Easing.InOutQuart}
                OpacityAnimator{ target: messageBox; duration: 250; easing.type: Easing.InOutQuad}
            }
        ]

        Text{
            id: errorOutput

            onWidthChanged: if (width > 400) errorOutput.width = 400;

            color: "black"

            wrapMode: Text.Wrap

            font.pointSize: 12
            anchors.centerIn: parent
        }
    }
}
