#include "mytranslator.h"

MyTranslator::MyTranslator(QObject *parent)  :
    QObject(parent)
{
}

void MyTranslator::read(QString text){
    store = text;

    inputData = text.
            replace(";"," ; ").replace("="," = ").replace("+"," + ").replace("-"," - ").
            replace("*"," * ").replace("/"," / ").replace("("," ( ").replace(")"," ) ").
            replace("["," [ ").replace("]"," ] ").replace("\t"," ").replace("\n"," ").
            replace(","," , ").replace("{"," { ").replace("}"," } ").split(QLatin1Char(' '), Qt::SkipEmptyParts);

    word = inputData.begin();

    ObjList.clear();
    Figures.clear();

    draw = false;

    while (word != inputData.end()){
        if (!operation()) return;
    }

    emit DrawChanged();
}

bool MyTranslator::getDraw() const{
    return draw;
}

void MyTranslator::throwError(QString text){
    int idx = 0;
    int i = word - inputData.begin();

    for (; i > 0; i--){
         idx += (*(word - i)).length();
         while (store[idx+1] == " " || store[idx+1] == "\n") idx++;
    }

    while (store[idx] == " " || store[idx] == "\n") idx++;

    emit getError(text, idx);
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


bool MyTranslator::is(QString tag, int offset){
    for(int i = 1; i <= offset; i++){
         if (word + i == inputData.end()) return false;
    }
    return (*(word + offset) == tag);
}

bool MyTranslator::next(uint step){
    for(int i = 1; i <= step; i++){
         if (word + i == inputData.end()) return false;
    }

    word+=step;
    return true;
}

bool MyTranslator::varName(){
    if (is("figure") || is("float") || is("vector") || is("point") || is("draw") || is("rotate")){
        throwError("нельзя называть обьекты служебными именами");
        return false;
    }


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
        throwError("после '"+*word+"' должно было быть имя переменной");
        return false;
    }
    if (!varName()) return false;

    if (ObjList.contains(*word)){
        throwError("переменная " + *word + " уже обьявлена");
        return false;
    }

    ObjList.insert(*word, {*(word-1), QVariant()});

    return true;
}

bool MyTranslator::operation(){
    if (is("figure") || is("float") || is("vector") || is("point")){
        if (!initVariable()) return false;

        auto &obj = ObjList[*word];
        obj.type = *(word-1);

        if (is("=",1)){
            word++;
            if (next()){
                if (!rightPart(obj)) return false;
            }else{
                throwError("после знака '=' должно быть значение с типом '"+obj.type+"'");
                return false;
            }
        }else if (word+1 == inputData.end()){
            throwError("после '"+CutWord(*word)+"' должно быть ';' или '='");
            return false;
        }

    }else if (is("rotate")){
        if (!is("(",1)){
            throwError("после слова '"+*word+"' должна быть открывающая скобка '('");
            return false;
        }
        word++;

        if (!next()) {
            throwError("функция 'rotate' должна быть обьявлена с 1м параметром фигурой и со 2м - числом для ее повората по часовой стрелке в градусах");
            return false;
        }

        t_Variable obj, R;

        if (!getVariable()) return false;
        QString var = *word;
        obj = ObjList[var];

        if (obj.type != "figure"){
            throwError("переменная '" + *word + "' не является фигурой или точкой");
            return false;
        }

        if (!(is(",",1))) {
            throwError("после '"+*word+"' должна быть запятая");
            return false;
        }
        word++;

        if(!next()) {
            throwError("после запятой должно быть число\n(угол поворота фигуры по часовой стрелке)");
            return false;
        }

        R.type = "float";
        if (!rightPart(R)) return false;

        QPointF o;
        auto list = obj.value.value<QList<QPointF>>();

        for (auto &p : list){
            o.setX(o.x() + p.x());
            o.setY(o.y() + p.y());
        }

        o.setX(o.x() / list.size());
        o.setY(o.y() / list.size());

        float radians = R.value.toFloat() * 3.14159265 / 180;

        for (auto &p : list){
            p.setX((p.x() - o.x()) * cos(radians) - (p.y() - o.y()) * sin(radians) + o.x());
            p.setY((p.x() - o.x()) * sin(radians) + (p.y() - o.y()) * cos(radians) + o.y());
        }

        if (!(is(")",1))) {
            throwError("после '"+*word+"' должна быть закрывающая круглая скобка");
            return false;
        }

        obj.value.setValue(list);
        ObjList[var] = obj;
        word++;
    }else if (is("draw")){
        if (!is("(",1)){
            throwError("после слова '"+*word+"' должна быть открывающая скобка '('");
            return false;
        }
        word++;

        if (!next()) {
            throwError("аргументы функции 'draw' должны быть фигурой или точкой");
            return false;
        }

        t_Variable obj;

        if (getVariable()) obj = ObjList[*word];
        else if (!rightPart(obj)) return false;

        if (obj.type == "point"){
            Figures << QList<QPointF>({obj.value.toPointF()});
        }else if (obj.type == "figure"){
            Figures << obj.value.value<QList<QPointF>>();
        }else{
            throwError("переменная '" + *word + "' не является фигурой или точкой");
            return false;
        }

        if (!(is(",",1) || is(")",1))) {
            throwError("продолжите список или завершите так ');'");
            return false;
        }

        while (is(",",1)){
            word++;

            if (!next()) {
                throwError("введите фигуру или точку");
                return false;
            }

            if (getVariable()) obj = ObjList[*word];
            else if (!rightPart(obj)) return false;

            if (obj.type == "point"){
                Figures << QList<QPointF>({obj.value.toPointF()});
            }else if (obj.type == "figure"){
                Figures << obj.value.value<QList<QPointF>>();
            }else{
                throwError("переменная '" + *word + "' не является фигурой или точкой");
                return false;
            }

            if (!(is(",",1) || is(")",1))) {
                throwError("продолжите список или завершите так ');'");
                return false;
            }
        }

        word++;
        draw = true;
    }else if (getVariable()){
        if (is("=",1)){
            QString var = *(word);
            t_Variable result = ObjList[var];
            word++;
            if (next()){
                if (!rightPart(result)) return false;
                ObjList[var] = result;
            }else{
                throwError("после знака '=' должно быть присваемое значение типа '"+result.type+"'");
                return false;
            }
        }else{
            throwError("после слова '"+CutWord(*word)+"' должен быть знак '='");
            return false;
        }
    }else return false;

    if (!is(";",1)){
        throwError("после "+CutWord(*word)+" ожидалось ';'");
        return false;
    }

    word+=2;
    return true;
}

bool MyTranslator::rightPart(t_Variable &result){
    bool minus = false;
    QString outputType = result.type;

    if (is("-")){
        minus = true;
        if (!next()) return false;
    }

    if (!block(result)) return false;

    if (minus) {
        if (result.type == "float"){
            result.value.setValue(-result.value.toFloat());
        }else if (result.type == "point" || result.type == "vector"){
            QPointF tmp_point = result.value.toPointF();
            result.value.setValue(QPointF(-tmp_point.x(),-tmp_point.y()));
        }else if (result.type == "figure"){
            QList<QPointF> tmp_fig = result.value.value<QList<QPointF>>();
            for (auto &p : tmp_fig){
                p.setX(-p.x());
                p.setY(-p.y());
            }
            result.value.setValue(tmp_fig);
        }

    }

    t_Variable tmp;

    while(is("+",1) || is("-",1)){
        if (is("+",1)){
            if (!next(2)) return false;
            if (!block(tmp)) return false;

            if (result.type == "float"){
                if (tmp.type == "float"){
                    result.value.setValue(result.value.toFloat()+tmp.value.toFloat());
                }else{
                    throwError("нельзя преобразовать '" + tmp.type + "' в '"+ result.type +"'");
                    return false;
                }
            }else if (result.type == "point"){
                if (tmp.type == "point"){
                    result.value.setValue(QList<QPointF>() = {result.value.toPointF(), tmp.value.toPointF()});
                    result.type = "figure";
                }else if (tmp.type == "vector"){
                    QPointF tmp_vec = tmp.value.toPointF();
                    result.value.setValue(QPointF(result.value.toPointF().x() + tmp_vec.x(), result.value.toPointF().y() + tmp_vec.y()));
                }else if (tmp.type == "figure"){
                    QList<QPointF> tmp_fig = tmp.value.value<QList<QPointF>>();
                    tmp_fig << result.value.toPointF();
                    result.value.setValue(tmp_fig);
                    result.type = "figure";
                }else{
                    throwError("нельзя преобразовать '" + tmp.type + "' в '"+ result.type +"'");
                    return false;
                }
            }else if (result.type == "vector"){
                if (tmp.type == "point"){
                    QPointF tmp_vec = result.value.toPointF();
                    result.value.setValue(QPointF(tmp.value.toPointF().x() + tmp_vec.x(), tmp.value.toPointF().y() + tmp_vec.y()));
                    result.type = "point";
                }else if (tmp.type == "vector"){
                    QPointF tmp_vec = tmp.value.toPointF();
                    QPointF res_vec = result.value.toPointF();
                    result.value.setValue(QPointF(res_vec.x() + tmp_vec.x(), res_vec.y() + tmp_vec.y()));
                }else if (tmp.type == "figure"){
                    QList<QPointF> tmp_fig = tmp.value.value<QList<QPointF>>();
                    QPointF tmp_vec = result.value.toPointF();
                    for (auto &p : tmp_fig){
                        p.setX(p.x() + tmp_vec.x());
                        p.setY(p.y() + tmp_vec.y());
                    }
                    result.value.setValue(tmp_fig);
                    result.type = "figure";
                }else{
                    throwError("нельзя преобразовать '" + tmp.type + "' в '"+ result.type +"'");
                    return false;
                }
            }else if (result.type == "figure"){
                if (tmp.type == "vector"){
                    QList<QPointF> res_fig = result.value.value<QList<QPointF>>();
                    QPointF tmp_vec = tmp.value.toPointF();
                    for (auto &p : res_fig){
                        p.setX(p.x() + tmp_vec.x());
                        p.setY(p.y() + tmp_vec.y());
                    }
                    result.value.setValue(res_fig);
                }else if (tmp.type == "point"){
                    QList<QPointF> res_fig = result.value.value<QList<QPointF>>();
                    res_fig << tmp.value.toPointF();
                    result.value.setValue(res_fig);
                }else if (tmp.type == "figure"){
                    QList<QPointF> res_fig = result.value.value<QList<QPointF>>();
                    QList<QPointF> tmp_fig = tmp.value.value<QList<QPointF>>();
                    res_fig << tmp_fig;
                    result.value.setValue(res_fig);
                }else{
                    throwError("нельзя преобразовать '" + tmp.type + "' в '"+ result.type +"'");
                    return false;
                }
            }
        }else if (is("-",1)){
            if (!next(2)) return false;
            if (!block(tmp)) return false;

            if (result.type == "float"){
                if (tmp.type == "float"){
                    result.value.setValue(result.value.toFloat()-tmp.value.toFloat());
                }else{
                    throwError("нельзя преобразовать '" + tmp.type + "' в '"+ result.type +"'");
                    return false;
                }
            }else if (result.type == "point"){
                if (tmp.type == "point"){
                    throwError("точки нельзя вычитать");
                    return false;
                }else if (tmp.type == "vector"){
                    QPointF tmp_vec = tmp.value.toPointF();
                    QPointF res_point = result.value.toPointF();
                    result.value.setValue(QPointF(res_point.x() - tmp_vec.x(), res_point.y() - tmp_vec.y()));
                }else if (tmp.type == "figure"){
                    throwError("нельзя вычитать из точки фигуру");
                    return false;
                }else{
                    throwError("нельзя преобразовать '" + tmp.type + "' в '"+ result.type +"'");
                    return false;
                }
            }else if (result.type == "vector"){
                if (tmp.type == "point"){
                    QPointF res_vec = result.value.toPointF();
                    QPointF tmp_point = tmp.value.toPointF();
                    result.value.setValue(QPointF(res_vec.x() - tmp_point.x(), res_vec.y() - tmp_point.y()));
                    result.type = "point";
                }else if (tmp.type == "vector"){
                    QPointF res_vec = result.value.toPointF();
                    QPointF tmp_vec = tmp.value.toPointF();
                    result.value.setValue(QPointF(res_vec.x() - tmp_vec.x(), res_vec.y() - tmp_vec.y()));
                }else if (tmp.type == "figure"){
                    QList<QPointF> tmp_fig = tmp.value.value<QList<QPointF>>();
                    QPointF res_vec = result.value.toPointF();
                    for (auto &p : tmp_fig){
                        p.setX(res_vec.x() - p.x());
                        p.setY(res_vec.y() - p.y());
                    }
                    result.value.setValue(tmp_fig);
                    result.type = "figure";
                }else{
                    throwError("нельзя преобразовать '" + tmp.type + "' в '"+ result.type +"'");
                    return false;
                }
            }else if (result.type == "figure"){
                if (tmp.type == "vector"){
                    QList<QPointF> res_fig = result.value.value<QList<QPointF>>();
                    QPointF tmp_vec = tmp.value.toPointF();
                    for (auto &p : res_fig){
                        p.setX(p.x() - tmp_vec.x());
                        p.setY(p.y() - tmp_vec.y());
                    }
                    result.value.setValue(res_fig);
                }else if (tmp.type == "point"){
                    QList<QPointF> res_fig = result.value.value<QList<QPointF>>();
                    QPointF tmp_point = tmp.value.toPointF();

                    res_fig.removeOne(tmp_point);
                    result.value.setValue(res_fig);
                }else if (tmp.type == "figure"){
                    QList<QPointF> res_fig = result.value.value<QList<QPointF>>();
                    QList<QPointF> tmp_fig = tmp.value.value<QList<QPointF>>();

                    for (auto &p : tmp_fig){
                        if (res_fig.contains(p)){
                            res_fig.removeOne(p);
                        }
                    }

                    result.value.setValue(res_fig);
                }else{
                    throwError("нельзя вычитать '" + tmp.type + "' из '"+ result.type +"'");
                    return false;
                }
            }
        }
    }

    if (!outputType.isEmpty()){
        if (result.type != outputType){
            throwError("нельзя преобразовать '" + result.type + "' в '"+ outputType +"'");
            return false;
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

            if (result.type == "float"){
                if (tmp.type == "float"){
                    result.value.setValue(result.value.toFloat()*tmp.value.toFloat());
                }else if (tmp.type == "vector"){
                    int num = result.value.toFloat();
                    result.value.setValue(QPointF(tmp.value.toPointF().x()*num, tmp.value.toPointF().y()*num));
                    result.type = "vector";
                }else if (tmp.type == "point"){
                    int num = result.value.toFloat();
                    result.value.setValue(QPointF(tmp.value.toPointF().x()*num, tmp.value.toPointF().y()*num));
                    result.type = "point";
                }else if (tmp.type == "figure"){
                    QList<QPointF> tmp_fig = tmp.value.value<QList<QPointF>>();
                    int num = result.value.toFloat();
                    for (auto &p : tmp_fig){
                        p.setX(num * p.x());
                        p.setY(num * p.y());
                    }
                    result.value.setValue(tmp_fig);
                    result.type = "figure";
                }
            }else if (result.type == "vector"){
                if (tmp.type == "float"){
                    int num = tmp.value.toFloat();
                    QPointF res_vec = result.value.toPointF();
                    result.value.setValue(QPointF(res_vec.x()*num, res_vec.y()*num));
                }else if (tmp.type == "vector"){
                    QPointF tmp_vec = tmp.value.toPointF();
                    QPointF res_vec = result.value.toPointF();
                    result.value.setValue(QPointF(res_vec.x() * tmp_vec.x(), res_vec.y() * tmp_vec.y()));
                }else if (tmp.type == "point"){
                    QPointF tmp_vec = tmp.value.toPointF();
                    QPointF res_vec = result.value.toPointF();
                    result.value.setValue(QPointF(res_vec.x() * tmp_vec.x(), res_vec.y() * tmp_vec.y()));
                    result.type = "point";
                }else if (tmp.type == "figure"){
                    QList<QPointF> tmp_fig = tmp.value.value<QList<QPointF>>();
                    QPointF res_vec = result.value.toPointF();
                    for (auto &p : tmp_fig){
                        p.setX(res_vec.x() * p.x());
                        p.setY(res_vec.y() * p.y());
                    }
                    result.value.setValue(tmp_fig);
                    result.type = "figure";
                }
            }else if (result.type == "point"){
                if (tmp.type == "float"){
                    int num = tmp.value.toFloat();
                    QPointF res_point = result.value.value<QPointF>();
                    result.value.setValue(QPointF(res_point.x()*num, res_point.y()*num));
                }else if (tmp.type == "vector"){
                    QPointF tmp_vec = tmp.value.toPointF();
                    QPointF res_vec = result.value.toPointF();
                    result.value.setValue(QPointF(res_vec.x() * tmp_vec.x(), res_vec.y() * tmp_vec.y()));
                }else{
                    throwError("нельзя перемножать '"+ result.type +"' с '" + tmp.type + "'");
                    return false;
                }
            }else if (result.type == "figure"){
                if (tmp.type == "float"){
                    QList<QPointF> res_fig = result.value.value<QList<QPointF>>();
                    int num = tmp.value.toFloat();
                    for (auto &p : res_fig){
                        p.setX(p.x() * num);
                        p.setY(p.y() * num);
                    }
                    result.value.setValue(res_fig);
                }else if (tmp.type == "vector"){
                    QList<QPointF> res_fig = tmp.value.value<QList<QPointF>>();
                    QPointF tmp_vec = tmp.value.toPointF();
                    for (auto &p : res_fig){
                        p.setX(p.x() * tmp_vec.x());
                        p.setY(p.y() * tmp_vec.y());
                    }
                    result.value.setValue(res_fig);
                }else{
                    throwError("нельзя перемножать '"+ result.type +"' с '" + tmp.type + "'");
                    return false;
                }
            }
        }else if (is("/",1)){
            if (!next(2)) return false;
            if (!part(tmp)) return false;

            if (result.type == "float"){
                if (tmp.type == "float"){
                    int num = tmp.value.toFloat();
                    if (num == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }
                    result.value.setValue(result.value.toFloat()/num);
                }else if (tmp.type == "vector"){
                    int num = result.value.toFloat();
                    QPointF tmp_vec = tmp.value.toPointF();

                    if (tmp_vec.x() == 0 || tmp_vec.y() == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    result.value.setValue(QPointF(num / tmp_vec.x(), num / tmp_vec.y()));
                    result.type = "vector";
                }else if (tmp.type == "point"){
                    int num = result.value.toFloat();
                    QPointF tmp_point = tmp.value.toPointF();

                    if (tmp_point.x() == 0 || tmp_point.y() == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    result.value.setValue(QPointF(num / tmp_point.x(), num / tmp_point.y()));
                    result.type = "point";
                }else if (tmp.type == "figure"){
                    QList<QPointF> tmp_fig = tmp.value.value<QList<QPointF>>();
                    int num = result.value.toFloat();
                    for (auto &p : tmp_fig){
                        if (p.x() == 0 || p.y() == 0){
                            throwError("нельзя делить на 0");
                            return false;
                        }

                        p.setX(num / p.x());
                        p.setY(num / p.y());
                    }
                    result.value.setValue(tmp_fig);
                    result.type = "figure";
                }
            }else if (result.type == "vector"){
                if (tmp.type == "float"){
                    int num = tmp.value.toFloat();

                    if (num == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    QPointF res_vec = result.value.toPointF();
                    result.value.setValue(QPointF(res_vec.x()/num, res_vec.y()/num));
                }else if (tmp.type == "vector"){
                    QPointF tmp_vec = tmp.value.toPointF();

                    if (tmp_vec.x() == 0 || tmp_vec.y() == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    QPointF res_vec = result.value.toPointF();
                    result.value.setValue(QPointF(res_vec.x() / tmp_vec.x(), res_vec.y() / tmp_vec.y()));
                }else if (tmp.type == "point"){
                    QPointF tmp_vec = tmp.value.toPointF();

                    if (tmp_vec.x() == 0 || tmp_vec.y() == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    QPointF res_vec = result.value.toPointF();
                    result.value.setValue(QPointF(res_vec.x() / tmp_vec.x(), res_vec.y() / tmp_vec.y()));
                    result.type = "point";
                }else if (tmp.type == "figure"){
                    QList<QPointF> tmp_fig = tmp.value.value<QList<QPointF>>();
                    QPointF res_vec = result.value.toPointF();
                    for (auto &p : tmp_fig){
                        if (p.x() == 0 || p.y() == 0){
                            throwError("нельзя делить на 0");
                            return false;
                        }
                        p.setX(res_vec.x() / p.x());
                        p.setY(res_vec.y() / p.y());
                    }
                    result.value.setValue(tmp_fig);
                    result.type = "figure";
                }
            }else if (result.type == "point"){
                if (tmp.type == "float"){
                    int num = tmp.value.toFloat();

                    if (num == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    QPointF res_point = result.value.toPointF();
                    result.value.setValue(QPointF(res_point.x()/num, res_point.y()/num));
                }else if (tmp.type == "vector"){
                    QPointF tmp_vec = tmp.value.toPointF();
                    QPointF res_vec = result.value.toPointF();

                    if (tmp_vec.x() == 0 || tmp_vec.y() == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    result.value.setValue(QPointF(res_vec.x() / tmp_vec.x(), res_vec.y() / tmp_vec.y()));
                }else{
                    throwError("нельзя перемножать '"+ result.type +"' с '" + tmp.type + "'");
                    return false;
                }
            }else if (result.type == "figure"){
                if (tmp.type == "float"){
                    QList<QPointF> res_fig = result.value.value<QList<QPointF>>();
                    int num = tmp.value.toFloat();

                    if (num == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    for (auto &p : res_fig){
                        p.setX(p.x() / num);
                        p.setY(p.y() / num);
                    }
                    result.value.setValue(res_fig);
                }else if (tmp.type == "vector"){
                    QList<QPointF> res_fig = tmp.value.value<QList<QPointF>>();
                    QPointF tmp_vec = tmp.value.toPointF();

                    if (tmp_vec.x() == 0 || tmp_vec.y() == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    for (auto &p : res_fig){
                        p.setX(p.x() / tmp_vec.x());
                        p.setY(p.y() / tmp_vec.y());
                    }
                    result.value.setValue(res_fig);
                }else{
                    throwError("нельзя перемножать '"+ result.type +"' с '" + tmp.type + "'");
                    return false;
                }
            }
        }
    }

    return true;
}

bool MyTranslator::getNum(t_Variable &result){
    bool ok;
    result.value.setValue<float>((*word).toFloat(&ok));
    return ok;
}

bool MyTranslator::getFigure(t_Variable &result){
    if (!is("{")){
        throwError("фигура задается внутри фигурных скобок");
        return false;
    }

    if (is("}",1)){
        word++;
        result.type = "figure";
        result.value.setValue(QList<QPointF>());
        return true;
    }

    if (!next()) {
        throwError("после '"+*word+"' должны быть перечислены обьекты типа 'point'");
        return false;
    }

    t_Variable tmp;

    if (varName()){
        tmp = ObjList[*word];

        if (tmp.type != "point"){
            throwError("нельзя преобразовать '"+tmp.type+"' в 'point'");
            return false;
        }
    }else{
        tmp.type = "point";
        if (!rightPart(tmp)) return false;
    }

    QList<QPointF> tmp_fig;
    tmp_fig << tmp.value.toPointF();

    while (is(",",1)){
        word++;
        if (!next()){
            throwError("после запятой должна быть обьект типа 'point'");
            return false;
        }
        if (varName()){
            tmp = ObjList[*word];

            if (tmp.type != "point"){
                throwError("нельзя преобразовать '"+tmp.type+"' в 'point'");
                return false;
            }
        }else{
            tmp.type = "point";
            if (!rightPart(tmp)) return false;
        }

        tmp_fig << tmp.value.toPointF();
    }

    if (!is("}",1)){
        throwError("продолжите список точек или завершите фигурной скобкой");
        return false;
    }
    word++;

    result.value.setValue(tmp_fig);
    result.type = "figure";

    return true;
}

QString MyTranslator::CutWord(QString &str){
    if (str.size() > 10) return (str.left(10) + "...");
    else return str;
}

bool MyTranslator::getVector(t_Variable &result){
    if (!is("[")) {
        throwError("вектор задается внутри квадратных скобок");
        return false;
    }

    if (!next()){
        throwError("после '"+*word+"' должно быть число");
        return false;
    }

    t_Variable tmp;

    tmp.type = "float";
    if (!rightPart(tmp)) return false;

    if (!is(",",1)) {
        throwError("после '"+*word+"' должна быть запятая");
        return false;
    }
    word++;

    if (!next()){
        throwError("после '"+*word+"' должно быть число");
        return false;
    }

     QPointF tmp_point;
     tmp_point.setX(tmp.value.toFloat());

     if (!rightPart(tmp)) return false;

     if (!is("]",1)){
         throwError("после " + *word + "должна быть закрывающая квадратная скобка");
         return false;
     }
     word++;

     // оформляем точку
     tmp_point.setY(tmp.value.toFloat());
     result.value.setValue(tmp_point);
     result.type = "vector";

     return true;
}

bool MyTranslator::part(t_Variable &result){
    if (is("(")){
        if (!next()){
            throwError("после '(' должно быть число");
            return false;
        }

        t_Variable tmp;

        if (result.type == "point"){
            tmp.type = "float";
        }

        if (!rightPart(tmp)) return false;

        if (!next()) {
            if (result.type == "point") throwError("после '"+*word+"' должна быть запятая");
            else throwError("после '" + *word + "' должна быть круглая скобка");
            return false;
        }

        if (is(",")) {
            if (!next()){
                throwError("после запятой должно быть число");
                return false;
            }

             QPointF tmp_point;
             tmp_point.setX(tmp.value.toFloat());

             if (!rightPart(tmp)) return false;

             tmp_point.setY(tmp.value.toFloat());
             result.value.setValue(tmp_point);
             result.type = "point";

             if (!next()) {
                 throwError("после '" + *word + "' должна быть круглая скобка");
                 return false;
             }
        }else{
            result.value.setValue(tmp);
            result.type = tmp.type;
        }

        if (!is(")")){
            throwError("после '" + *(word-1) + "' должна быть круглая скобка");
            return false;
        }
    }else if (is(";")){
        throwError("после знака '=' должно быть новое значение типа '"+result.type+"'");
        return false;
    }else if (is("{")){
        if (!getFigure(result)) return false;
    }else if (is("[")){
        if (!getVector(result)) return false;
    }else if (getNum(result)){
        result.type = "float";
    }else if (getVariable()){
        result = ObjList[*word];
    }else return false;

    return true;
}

bool MyTranslator::getVariable(){
    if (!varName()) return false;

    if (!ObjList.contains(*word)){
        throwError("переменной '" + CutWord(*word) + "' не существует");
        return false;
    }

    return true;
}
