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

            ctx.lineWidth = 15;
            ctx.strokeStyle = "red"
            ctx.beginPath()

            for (let f = 0; f < translator.amountOfFigures(); f++){
                for (let p = 0; p < translator.amountOfPointsOnFigure(f); p++){
                }
            }

            ctx.moveTo(drawingCanvas.width / 2, 0)
            ctx.lineTo((drawingCanvas.width / 2) + 40, 40)

            ctx.stroke()
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
