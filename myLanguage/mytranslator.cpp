#include "mytranslator.h"

MyTranslator::MyTranslator(QObject *parent) : QObject(parent)
{
}

bool MyTranslator::isError() const{
    return !state.isEmpty();
}

void MyTranslator::clearState(){
    state = "";
    emit StateChanged();
}

bool MyTranslator::getDraw() const{
    return draw;
}

QString MyTranslator::getState() const{
    return state;
}

void MyTranslator::throwError(QString text){
    state = text;
    emit StateChanged();
}

int MyTranslator::amountOfFigures() const{
    return Figures.size();
}

int MyTranslator::amountOfPointsOnFigure(int i) const{
    return Figures[i].size();
}

int MyTranslator::getX(int FigureID, int PointID) const{
    return Figures[FigureID][PointID].x();
}

int MyTranslator::getY(int FigureID, int PointID) const{
    return Figures[FigureID][PointID].y();
}

void MyTranslator::read(QString text)
{
    text = text.replace(";"," ; ").replace("="," = ").replace("+"," + ").replace("-"," - ").replace("*"," * ").replace("/"," / ").replace("("," ( ").replace(")"," ) ").replace("["," [ ").replace("]"," ] ").replace("\n"," ").replace("\t"," ").replace(","," , ").replace("{"," { ").replace("}"," } ");
    inputData = text.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    word = inputData.begin();

    ObjList.clear();
    Figures.clear();

    draw = false;

    while (word != inputData.end()){
        if (!operation()) return;
    }

    clearState();
    emit DrawChanged();
}

bool MyTranslator::is(QString tag, int offset){
    for(int i = 1; i <= offset; i++){
         if (word + offset == inputData.end()) return false;
    }
    return (*(word + offset) == tag);
}

bool MyTranslator::next(uint step){
    for(int i = 1; i <= step; i++){
         if (word + step == inputData.end()) return false;
    }

    word+=step;
    return true;
}

bool MyTranslator::varName(){
    if (!(*word)[0].isLetter()) {
        throwError("имя переменной должно начинаться с буквы");
        return false;
    }

    for (uint16_t i = 1; i < (*word).size(); i++){
        if (!(*word)[i].isLetterOrNumber()){
            throwError("в имени переменной могут быть только буквы и символы");
            return false;
        }
    }

    return true;
}

bool MyTranslator::initVariable(){
    if (!next()){
        throwError("тут должно было быть имя переменной");
        return false;
    }
    if (!varName()) return false;

    if (ObjList.contains(*word)){
        throwError("переменная " + *word + " уже обьявлена");
        return false;
    }

    ObjList.insert(*word, {*(word-1), QVariant(NULL)});
    return true;
}

bool MyTranslator::operation(){
    if (is("figure") || is("int") || is("vector") || is("point")){
        if (!initVariable()) return false;

        if (is("=",1)){
            word++;
            if (next()){
                if (!rightPart(ObjList.last())) return false;       // только ошибка типов и несоответствие символов
                qDebug() << "   result = " << ObjList.last().value;
            }else{
                throwError("тут надо выдать значение");
                return false;
            }
        }

    }else if (is("move")){
    }else if (is("rotate")){
    }else if (is("draw")){
        if (!is("(",1)){
            throwError("тут открывающаяся скобка");
            return false;
        }
        word++;

        if (!next()) {
            throwError("введите название фигуры");
            return false;
        }

        if (!getVariable()) return false;
        Figures << ObjList[*word].value.value<QList<QPoint>>();

        if (!(is(",",1) || is(")",1))) {
            throwError("продолжите список или завершите так ');'");
            return false;
        }

        while (is(",",1)){
            word++;

            if (!next()) {
                throwError("введите название фигуры");
                return false;
            }

            if (!getVariable()) return false;
            Figures << ObjList[*word].value.value<QList<QPoint>>();

            if (!(is(",",1) || is(")",1))) {
                throwError("продолжите список или завершите так ');'");
                return false;
            }
        }

        if (!next()) return false;
        draw = true;
    }else if (getVariable()){
        if (is("=",1)){
            word++;
            if (next()){
                if (!rightPart(ObjList[*(word-2)])) return false;       // только ошибка типов и несоответствие символов
                qDebug() << "   result = " << ObjList.last().value;
            }else{
                throwError("тут надо выдать значение");
                return false;
            }
        }else if (is("+",1) && is("=",2)){
            word+=2;
            if (next()){
                t_Variable &result = ObjList[*(word-3)];
                t_Variable tmp;
                if (!block(tmp)) return false;

                if (result.type == "int"){
                    if (tmp.type == "int"){
                        qDebug() << result.value.toInt() << " + " << tmp.value.toInt();
                        result.value.setValue(result.value.toInt()+tmp.value.toInt());
                        qDebug() << "         = " << result.value.toInt();
                    }
                }
                qDebug() << "   result = " << ObjList.last().value;
            }else{
                throwError("тут надо выдать значение");
                return false;
            }
        }else if (is("-",1) && is("=",2)){
            word+=2;
            if (next()){
                t_Variable &result = ObjList[*(word-3)];
                t_Variable tmp;
                if (!block(tmp)) return false;

                if (result.type == "int"){
                    if (tmp.type == "int"){
                        qDebug() << result.value.toInt() << " - " << tmp.value.toInt();
                        result.value.setValue(result.value.toInt()-tmp.value.toInt());
                        qDebug() << "         = " << result.value.toInt();
                    }
                }
                qDebug() << "   result = " << ObjList.last().value;
            }else{
                throwError("тут надо выдать значение");
                return false;
            }
        }else if (is("*",1) && is("=",2)){
            word+=2;
            if (next()){
                t_Variable &result = ObjList[*(word-3)];
                t_Variable tmp;
                if (!part(tmp)) return false;

                if (result.type == "int"){
                    if (tmp.type == "int"){
                        qDebug() << result.value.toInt() << " * " << tmp.value.toInt();
                        result.value.setValue(result.value.toInt()*tmp.value.toInt());
                        qDebug() << "         = " << result.value.toInt();
                    }
                }
                qDebug() << "   result = " << ObjList.last().value;
            }else{
                throwError("тут надо выдать значение");
                return false;
            }
        }else if (is("/",1) && is("=",2)){
            word+=2;
            if (next()){
                t_Variable &result = ObjList[*(word-3)];
                t_Variable tmp;
                if (!block(tmp)) return false;

                if (result.type == "int"){
                    if (tmp.type == "int"){
                        if (tmp.value.toInt() == 0) {
                            throwError("нельзя делить на 0");
                            return false;
                        }

                        qDebug() << result.value.toInt() << " / " << tmp.value.toInt();
                        result.value.setValue(result.value.toInt() / tmp.value.toInt());
                        qDebug() << "         = " << result.value.toInt();
                    }
                }
                qDebug() << "   result = " << ObjList.last().value;
            }else{
                throwError("тут надо выдать значение");
                return false;
            }
        }
    }else{
        throwError("переменная " + *word + " не обьявлена\nожидалось то то-то");
        return false;
    }

    if (!is(";",1)){
        throwError("ожидалось ';'");
        return false;
    }

    word+=2;
    return true;
}

bool MyTranslator::rightPart(t_Variable &result){
    bool minus = false;

    if (is("-")){
        minus = true;
        if (!next()) return false;
    }

    if (!block(result)) return false;

    if (minus) {
        result.value.setValue(-result.value.toInt());
    }

    t_Variable tmp;

    while(is("+",1) || is("-",1)){
        if (is("+",1)){
            if (!next(2)) return false;
            if (!block(tmp)) return false;

            if (result.type == "int"){
                if (tmp.type == "int"){
                    result.value.setValue(result.value.toInt()+tmp.value.toInt());
                }
            }else if (result.type == "point"){
                if (tmp.type == "point"){

                }
            }
        }else if (is("-",1)){
            if (!next(2)) return false;
            if (!block(tmp)) return false;

            if (result.type == "int"){
                if (tmp.type == "int"){
                    qDebug() << result.value.toInt() << " - " << tmp.value.toInt();
                    result.value.setValue(result.value.toInt()-tmp.value.toInt());
                    qDebug() << "         = " << result.value.toInt();
                }
            }
        }
    }

    return true;
}

bool MyTranslator::block(t_Variable &result){
    if (!part(result)) return false;

    t_Variable tmp;

    while(is("*",1) || is("/",1)){
        if (is("*",1)){
            if (!next(2)) return false;
            if (!part(tmp)) return false;

            if (result.type == "int"){
                if (tmp.type == "int"){
                    result.value.setValue(result.value.toInt()*tmp.value.toInt());
                }
            }

        }else if (is("/",1)){
            if (!next(2)) return false;
            if (!part(tmp)) return false;

            if (result.type == "int"){
                if (tmp.type == "int"){
                    if (tmp.value.toInt() == 0) {
                        throwError("нельзя делить на 0");
                        return false;
                    }
                    result.value.setValue(result.value.toInt() / tmp.value.toInt());
                }
            }
        }
    }

    return true;
}

bool MyTranslator::getNum(t_Variable &result){
    bool ok;
    result.value.setValue<int>((*word).toInt(&ok, 10));

    return ok;
}

bool MyTranslator::getPoint(t_Variable &result){
    if (!is("(")){
        throwError("тут ваша точка");
        return false;
    }

    if (!next()){
        throwError("тут должно быть целое число");
        return false;
    }

    t_Variable tmp;
    if (!rightPart(tmp)) return false;

    if (tmp.type != "int"){
        throwError("тут должно быть целое число");
        return false;
    }

    if (!is(",",1)) {
        throwError("тут должен быть символ ','");
        return false;
    }
    word++;

    if (!next()){
        throwError("тут должно быть целое число");
        return false;
    }

     QPoint tmp_point;
     tmp_point.setX(tmp.value.toInt());

     if (!rightPart(tmp)) return false;

     if (tmp.type != "int"){
         throwError("тут должно быть целое число");
         return false;
     }

     if (!is(")",1)){
         throwError("после " + *word + "должна быть закрывающая скобка");
         return false;
     }
     word++;

     // оформляем точку
     tmp_point.setY(tmp.value.toInt());
     result.value.setValue(tmp_point);

     return true;
}

bool MyTranslator::getFigure(t_Variable &result){
    if (!is("{")){
        throwError("начало фигуры");
        return false;
    }

    if (!next()) return false;

    t_Variable tmp;

    if (!getPoint(tmp)) return false;

    QList<QPoint> tmp_fig;
    tmp_fig << tmp.value.toPoint();

    // добавление еще точек
    while (is(",",1)){
        word++;
        if (!next()){
            throwError("тут ваша точа");
            return false;
        }
        if (!getPoint(tmp)) return false;
        tmp_fig << tmp.value.toPoint();
    }

    result.value.setValue(tmp_fig);

    if (!is("}",1)){
        throwError("продолжите список точек или завершите сиволом '}'");
        return false;
    }
    word++;
}

bool MyTranslator::getVector(t_Variable &result){
    if (!is("[")) {
        throwError("начало вектора");
        return false;
    }

    if (!next()){
        throwError("тут должно быть целое число");
        return false;
    }

    t_Variable tmp;
    if (!rightPart(tmp)) return false;

    if (tmp.type != "int"){
        throwError("тут должно быть целое число");
        return false;
    }

    if (!is(",",1)) {
        throwError("тут должен быть символ ','");
        return false;
    }
    word++;

    if (!next()){
        throwError("тут должно быть целое число");
        return false;
    }

     QPoint tmp_point;
     tmp_point.setX(tmp.value.toInt());

     if (!rightPart(tmp)) return false;

     if (tmp.type != "int"){
         throwError("тут должно быть целое число");
         return false;
     }

     if (!is("]",1)){
         throwError("после " + *word + "должна быть закрывающая скобка");
         return false;
     }
     word++;

     // оформляем точку
     tmp_point.setY(tmp.value.toInt());
     result.value.setValue(tmp_point);

     return true;
}

bool MyTranslator::part(t_Variable &result){
    if (is("(")){
        if (!getPoint(result)) return false;
        result.type = "point";
    }else if (is("{")){
        if (!getFigure(result)) return false;
        result.type = "figure";
    }else if (is("[")){          // vector
        if (!getVector(result)) return false;
        result.type = "vector";
    }else if (getNum(result)){
        result.type = "int";
    }else if (getVariable()){
        result = ObjList[*word];
    }else {
        throwError("где сука ответ?");
        return false;
    }

    return true;
}

bool MyTranslator::getVariable(){
    if (!varName()) return false;

    if (!ObjList.contains(*word)){
        throwError("переменная " + *word + " не существует");
        return false;
    }

    return true;
}
