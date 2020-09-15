import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12

import Translator 1.0

Window {
    id: window
    width: 1000
    height: 500
    visible: true
    title: qsTr("Vecto..")

    Canvas{
        id: drawingCanvas
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset()

            ctx.lineWidth = 2;
            ctx.strokeStyle = "black"

            for (let f = 0; f < translator.amountOfFigures(); f++){
                if (translator.amountOfPointsOnFigure(f) > 0){

                    var p = 1;

                    if (translator.amountOfPointsOnFigure(f) > 1){
                        ctx.moveTo(translator.getX(f,0), translator.getY(f,0))
                        for (; p < translator.amountOfPointsOnFigure(f); p++){
                            ctx.lineTo(translator.getX(f,p), translator.getY(f,p))
                        }

                        p--;

                        if (translator.getX(f,p) === translator.getX(f,0) && translator.getY(f,p) === translator.getY(f,0)){
                            ctx.fillStyle = "rgba(255, 136, 26, 0.4)";
                            ctx.fill();
                        }

                        ctx.stroke()
                    }

                    ctx.fillStyle = "rgba(255, 26, 26, 0.4)";

                    for (p = 0; p < translator.amountOfPointsOnFigure(f); p++){
                        ctx.beginPath();
                        ctx.arc(translator.getX(f,p), translator.getY(f,p), 4, 360, 0, false);
                        ctx.fill();
                        ctx.stroke()
                    }
                }
            }
        }
    }

    Rectangle{
        id:codeArea
        width: 300;
        height: parent.height
        anchors.right: parent.right

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

                 Behavior on y{
                    NumberAnimation { easing.type: Easing.OutElastic; easing.amplitude: 3.0; easing.period: 2.0; duration: 300 }
                }
            }

            TextArea.flickable: TextArea{
                id: textInput

                placeholderText: qsTr("Введите команду...")

                onTextChanged: {
                    translator.read(textInput.text)
                }

                anchors.margins: 10
                wrapMode: TextEdit.Wrap
                font.pointSize: 12
                selectByMouse: true
            }
        }
    }

    Rectangle{
        id:errorRect

        property rect p: [0,0,0,0]

        x: p.x + codeArea.x + textInput.x
        y: p.y + codeArea.y + textInput.y - flic.contentY
        width: codeArea.x - p.x - 10
        height: p.height

        gradient: Gradient {
            orientation: Gradient.Horizontal

            GradientStop { position: 0.0; color: "transparent" }
            GradientStop { position: 0.01; color: "red" }
            GradientStop { position: 0.8; color: "transparent" }
        }

        Behavior on width{ NumberAnimation { easing.type: Easing.OutElastic; easing.amplitude: 3.0; easing.period: 2.0; duration: 300 } }
        Behavior on x{ NumberAnimation { easing.type: Easing.OutElastic; easing.amplitude: 3.0; easing.period: 2.0; duration: 300 } }
        //Behavior on y{ NumberAnimation { easing.type: Easing.OutElastic; easing.amplitude: 3.0; easing.period: 2.0; duration: 300 } }
        Behavior on opacity{ NumberAnimation { easing.type: Easing.OutElastic; easing.amplitude: 3.0; easing.period: 2.0; duration: 1000 } }
    }

    Rectangle{
        id: messageBox
        width: 400
        height: 50
        color: "red"

        anchors.left: parent.left
        anchors.leftMargin:  25
        radius: 5

        state: "passive"

        MyTranslator{
            id: translator


            onGetError: {
                messageBox.state = "active"
                errorOutput.text = text

                errorRect.p = textInput.positionToRectangle(pos)
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
                PropertyChanges { target: messageBox; opacity: 0.6; y: window.height - messageBox.height + 5}
            },
            State {
                name: "passive"
                PropertyChanges { target: messageBox; opacity: 0; y: window.height + messageBox.height + 5}
            }
        ]

        transitions: [
            Transition {
                from: "active"
                to: "passive"
                OpacityAnimator{ target: messageBox; duration: 1000; easing.type: Easing.InOutCirc}
                PropertyAnimation { target: messageBox; property: "y"; duration: 1000; easing.type: Easing.InOutQuart}
            },
            Transition {
                from: "passive"
                to: "active"
                OpacityAnimator{ target: messageBox; duration: 250; easing.type: Easing.InOutQuad}
                PropertyAnimation { target: messageBox; property: "y"; duration: 250; easing.type: Easing.InOutQuad}
            }
        ]

        Text{
            id: errorOutput
            color: "black"

            wrapMode: Text.Wrap

            font.pointSize: 12
            anchors.fill: parent
            anchors.margins: 5
        }
    }
}
