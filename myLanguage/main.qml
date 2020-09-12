import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12

import Translator 1.0

Window {
    id: window
    width: 1000
    height: 600
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
            anchors.fill: parent
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}

            TextArea.flickable: TextArea{
                anchors.margins: 10
                id: textInput
                wrapMode: TextEdit.Wrap

                font.pointSize: 12
                selectByMouse: true

                onTextChanged: translator.read(textInput.text)
            }
        }
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

            onStateChanged:{
                if (translator.isError()){
                    errorOutput.text = translator.getState()
                    messageBox.state = "active"
                }else{
                    messageBox.state = "passive"
                }
            }

            onDrawChanged: {
                drawingCanvas.requestPaint()
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
