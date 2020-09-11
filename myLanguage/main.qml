import QtQuick 2.12
import QtQuick.Window 2.12

import Translator 1.0

Window {
    id: window
    width: 800
    height: 400
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
            ctx.beginPath()

            for (let f = 0; f < translator.amountOfFigures(); f++){
                ctx.moveTo(translator.getX(f,0), translator.getY(f,0))
                console.log("       f")
                for (let p = 1; p < translator.amountOfPointsOnFigure(f); p++){
                    /*
                    ctx.beginPath();
                    ctx.fillStyle = "red";
                    ctx.arc(translator.getX(f,p), translator.getY(f,p), 30, 360, 0, false);
                    ctx.lineTo(translator.getX(f,p), translator.getY(f,p));
                    ctx.fill();
*/
                    ctx.lineTo(translator.getX(f,p), translator.getY(f,p))
                    console.log("       p")
                }

                ctx.stroke()
            }
        }

        // update - drawingCanvas.requestPaint()
    }

    Rectangle{
        id:codeArea
        width: 300;
        height: parent.height
        anchors.right: parent.right

        color: "#BADBDE"

        TextEdit{
            id: textInput
            anchors.fill: parent
            textMargin: 10
            font.pointSize: 12

            onTextChanged: translator.read(textInput.text)
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
                if (translator.amountOfFigures() > 0){
                    drawingCanvas.requestPaint()
                }
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

            font.pointSize: 12
            anchors.fill: parent
            anchors.margins: 5
        }
    }
}
