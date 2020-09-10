import QtQuick 2.12
import QtQuick.Window 2.12

import Translator 1.0

Window {
    width: 1000
    height: 600
    visible: true
    title: qsTr("Vecto..")

    MyTranslator{
        id: translator
    }

    Canvas{
        id: drawingCanvas
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d");

            ctx.lineWidth = 5;
            ctx.strokeStyle = "black"
            ctx.beginPath()

            for (let f = 0; f < translator.amountOfFigures(); f++){
                ctx.moveTo(translator.getX(f,0), translator.getY(f,0))
                for (let p = 1; p < translator.amountOfPointsOnFigure(f); p++){
                    ctx.beginPath();
                    ctx.fillStyle = "red";
                    ctx.arc(translator.getX(f,p), translator.getY(f,p), 30, 360, 0, false);
                    ctx.lineTo(translator.getX(f,p), translator.getY(f,p));
                    ctx.fill();

                    ctx.lineTo(translator.getX(f,p), translator.getY(f,p))
                }
                ctx.stroke()
            }
        }

        // update - drawingCanvas.requestPaint()
    }

    Rectangle{
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
}
