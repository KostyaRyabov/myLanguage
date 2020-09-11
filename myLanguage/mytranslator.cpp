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

QString MyTranslator::getState() const{
    return state;
}

void MyTranslator::throwErrow(QString text){
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
    text = text.replace(";"," ; ").replace("="," = ").replace("+"," + ").replace("-"," - ").replace("*"," * ").replace("/"," / ").replace("("," ( ").replace(")"," ) ").replace("["," [ ").replace("]"," ] ").replace("\n"," ").replace("\t"," ").replace(","," , ");
    inputData = text.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    word = inputData.begin();

    ObjList.clear();

    while (word != inputData.end()){
        if (!operation()) return;
    }

    clearState();
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
        throwErrow("имя переменной должно начинаться с буквы");
        return false;
    }

    for (uint16_t i = 1; i < (*word).size(); i++){
        if (!(*word)[i].isLetterOrNumber()){
            throwErrow("в имени переменной могут быть только буквы и символы");
            return false;
        }
    }

    return true;
}

bool MyTranslator::initVariable(){
    if (!next()){
        throwErrow("тут должно было быть имя переменной");
        return false;
    }
    if (!varName()) return false;

    if (ObjList.contains(*word)){
        throwErrow("переменная " + *word + " уже обьявлена");
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
                throwErrow("тут надо выдать значение");
                return false;
            }
        }

    }else if (is("move")){
    }else if (is("rotate")){
    }else if (is("draw")){
    }else if (getVariable()){
        if (is("=",1)){
            word++;
            if (next()){
                if (!rightPart(ObjList[*(word-2)])) return false;       // только ошибка типов и несоответствие символов
                qDebug() << "   result = " << ObjList.last().value;
            }else{
                throwErrow("тут надо выдать значение");
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
                throwErrow("тут надо выдать значение");
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
                throwErrow("тут надо выдать значение");
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
                throwErrow("тут надо выдать значение");
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
                            throwErrow("нельзя делить на 0");
                            return false;
                        }

                        qDebug() << result.value.toInt() << " / " << tmp.value.toInt();
                        result.value.setValue(result.value.toInt() / tmp.value.toInt());
                        qDebug() << "         = " << result.value.toInt();
                    }
                }
                qDebug() << "   result = " << ObjList.last().value;
            }else{
                throwErrow("тут надо выдать значение");
                return false;
            }
        }
    }else{
        throwErrow("переменная " + *word + " не обьявлена\nожидалось то то-то");
        return false;
    }

    if (!is(";",1)){
        throwErrow("ожидалось ';'");
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

    if (minus) result.value.setValue(-result.value.toInt());

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
                        throwErrow("нельзя делить на 0");
                        return false;
                    }
                    result.value.setValue(result.value.toInt() / tmp.value.toInt());
                }
            }
        }
    }

    return true;
}

bool MyTranslator::isNum(t_Variable &result){
    bool ok;
    result.value.setValue<int>((*word).toInt(&ok, 10));

    return ok;
}

bool MyTranslator::part(t_Variable &result){
   if (is("(")){
        if (!next()) return false;
        if (!rightPart(result)) return false;

        if (is(",",1)){
            if (result.type == "int"){
                word++;
                t_Variable tmp;
                if (!next()) return false;
                if (!rightPart(tmp)) return false;
                if (tmp.type == "int"){
                    if (is(")",1)){
                        if (!next()) return false;

                        // оформляем точку
                        result.type = "point";
                        result.value.setValue(QPoint(result.value.toInt(),tmp.value.toInt()));

                        // если еще запятая - это фигура
                    }else{
                        throwErrow("после " + *word + "должна быть закрывающая скобка");
                        return false;
                    }
                }

                // добавление еще точек
                if (is(",",1)){

                }
            }
        }else if (is(")",1)){
            if (!next()) return false;
        }else{
            throwErrow("после " + *word + "должна быть закрывающая скобка");
            return false;
        }
    }else if (isNum(result)){
        result.type = "int";
        qDebug() << "           isNum(" << result.value.toString() << ")";
    }else{
        if (getVariable()){
            result = ObjList[*word];
        }else return false;
    }

    return true;
}

bool MyTranslator::getVariable(){
    if (!varName()) return false;

    if (!ObjList.contains(*word)){
        throwErrow("переменная " + *word + " не существует");
        return false;
    }

    return true;
}
