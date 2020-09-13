#include "mytranslator.h"

MyTranslator::MyTranslator(QObject *parent)  :
    QSyntaxHighlighter(parent),
    m_TextDocument(nullptr)
{
}

void MyTranslator::read(QString text){
    if (store == text) return;
    store = text;

    inputData = text.
            replace(";"," ; ").replace("="," = ").replace("+"," + ").replace("-"," - ").
            replace("*"," * ").replace("/"," / ").replace("("," ( ").replace(")"," ) ").
            replace("["," [ ").replace("]"," ] ").replace("\t"," ").
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

void MyTranslator::highlightBlock( const QString &text )
{
    Q_UNUSED(text)
    emit highlightBlock(QVariant(m_TextDocument->textDocument()->toRawText()));
}

QQuickTextDocument* MyTranslator::textDocument() const
{
    QTextDocument* doc = m_TextDocument->textDocument();
    qDebug() << doc->toRawText();

    return m_TextDocument;
}

void MyTranslator::setTextDocument( QQuickTextDocument* textDocument )
{
    if (textDocument == m_TextDocument)
    {
        return;
    }

    m_TextDocument = textDocument;

    QTextDocument* doc = m_TextDocument->textDocument();
    setDocument(doc);

    emit textDocumentChanged();
}

void MyTranslator::setFormat( int start, int count, const QVariant& format )
{

}



bool MyTranslator::getDraw() const{
    return draw;
}

void MyTranslator::throwError(QString text){
    int idx = 0;
    int i = word - inputData.begin();

    QString raw = m_TextDocument->textDocument()->toRawText();

    for (; i > 0; i--){
         idx += (*(word - i)).length();
         while (raw[idx+1] == " ") idx++;
    }

    //emit getError(text, raw.left(idx) + "<font color=\"red\">" + raw.mid(idx) + "</font>");
    emit getError(text, raw.left(idx) + "<b>" + raw.mid(idx) + "</b>");
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
                auto &obj = ObjList.last();
                obj.type = *(word-3);
                if (!rightPart(obj)) return false;
            }else{
                throwError("тут надо выдать значение");
                return false;
            }
        }

    }else if (is("rotate")){
        if (!is("(",1)){
            throwError("тут открывающаяся скобка");
            return false;
        }
        word++;

        if (!next()) {
            throwError("введите название фигуры");
            return false;
        }

        t_Variable obj;

        if (getVariable()){
            obj = ObjList[*word];

            if (obj.type != "int") {
                throwError("'" + *word + "' не является фигурой");
                return false;
            }
        }else{
            obj.type = "int";
            if (!rightPart(obj)) return false;
        }

        if (!is("(",1)){
            throwError("тут закрывающаяся скобка");
            return false;
        }
        word++;
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

        t_Variable obj;

        if (getVariable()){
            obj = ObjList[*word];

            if (obj.type != "figure") {
                throwError("'" + *word + "' не является фигурой");
                return false;
            }
        }else{
            obj.type = "figure";
            if (!rightPart(obj)) return false;
        }

        Figures << obj.value.value<QList<QPoint>>();

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

            if (getVariable()){
                obj = ObjList[*word];

                if (obj.type != "figure") {
                    throwError("'" + *word + "' не является фигурой");
                    return false;
                }
            }else{
                obj.type = "figure";
                if (!rightPart(obj)) return false;
            }

            Figures << obj.value.value<QList<QPoint>>();

            if (!(is(",",1) || is(")",1))) {
                throwError("продолжите список или завершите так ');'");
                return false;
            }
        }

        word++;
        draw = true;
    }else if (getVariable()){
        if (is("=",1)){
            word++;
            if (next()){
                QString var = *(word-2);
                t_Variable result = ObjList[var];
                if (!rightPart(result)) return false;
                ObjList[var] = result;
            }else{
                throwError("тут надо выдать значение");
                return false;
            }
        }else{
            throwError("тут нужно '='");
            return false;
        }
    }else{
        throwError("переменная " + CutWord(*word) + " не обьявлена\nожидалось то то-то");
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
    QString outputType = result.type;

    if (is("-")){
        minus = true;
        if (!next()) return false;
    }

    if (!block(result)) return false;

    if (result.type == "figure")

    if (minus) {
        if (result.type == "int"){
            result.value.setValue(-result.value.toInt());
        }else if (result.type == "point" || result.type == "vector"){
            QPoint tmp_point = result.value.toPoint();
            result.value.setValue(QPoint(-tmp_point.x(),-tmp_point.y()));
        }else if (result.type == "figure"){
            QList<QPoint> tmp_fig = result.value.value<QList<QPoint>>();
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

            if (result.type == "int"){
                if (tmp.type == "int"){
                    result.value.setValue(result.value.toInt()+tmp.value.toInt());
                }else{
                    throwError("нельзя преобразовать '" + tmp.type + "' в '"+ result.type +"'");
                    return false;
                }
            }else if (result.type == "point"){
                if (tmp.type == "point"){
                    result.value.setValue(QList<QPoint>() = {result.value.toPoint(), tmp.value.toPoint()});
                    result.type = "figure";
                }else if (tmp.type == "vector"){
                    QPoint tmp_vec = tmp.value.toPoint();
                    result.value.setValue(QPoint(result.value.toPoint().x() + tmp_vec.x(), result.value.toPoint().y() + tmp_vec.y()));
                }else if (tmp.type == "figure"){
                    QList<QPoint> tmp_fig = tmp.value.value<QList<QPoint>>();
                    tmp_fig << result.value.toPoint();
                    result.value.setValue(tmp_fig);
                    result.type = "figure";
                }else{
                    throwError("нельзя преобразовать '" + tmp.type + "' в '"+ result.type +"'");
                    return false;
                }
            }else if (result.type == "vector"){
                if (tmp.type == "point"){
                    QPoint tmp_vec = result.value.toPoint();
                    result.value.setValue(QPoint(tmp.value.toPoint().x() + tmp_vec.x(), tmp.value.toPoint().y() + tmp_vec.y()));
                    result.type = "point";
                }else if (tmp.type == "vector"){
                    QPoint tmp_vec = result.value.toPoint();
                    result.value.setValue(QPoint(tmp.value.toPoint().x() + tmp_vec.x(), tmp.value.toPoint().y() + tmp_vec.y()));
                    result.type = "vector";
                }else if (tmp.type == "figure"){
                    QList<QPoint> tmp_fig = tmp.value.value<QList<QPoint>>();
                    QPoint tmp_vec = result.value.toPoint();
                    for (auto &p : tmp_fig){
                        p.setX(p.x() + tmp_vec.x());
                        p.setY(p.y() + tmp_vec.y());
                    }
                    result.value.setValue(tmp_fig);
                }else{
                    throwError("нельзя преобразовать '" + tmp.type + "' в '"+ result.type +"'");
                    return false;
                }
            }else if (result.type == "figure"){
                if (tmp.type == "vector"){
                    QList<QPoint> res_fig = result.value.value<QList<QPoint>>();
                    QPoint tmp_vec = tmp.value.toPoint();
                    for (auto &p : res_fig){
                        p.setX(p.x() + tmp_vec.x());
                        p.setY(p.y() + tmp_vec.y());
                    }
                    result.value.setValue(res_fig);
                }else if (tmp.type == "point"){
                    QList<QPoint> res_fig = result.value.value<QList<QPoint>>();
                    res_fig << tmp.value.toPoint();
                    result.value.setValue(res_fig);
                }else if (tmp.type == "figure"){
                    QList<QPoint> res_fig = result.value.value<QList<QPoint>>();
                    QList<QPoint> tmp_fig = tmp.value.value<QList<QPoint>>();
                    res_fig << tmp_fig;
                    result.value.setValue(res_fig);
                }else{
                    throwError("нельзя сгладывать '" + result.type + "' с '"+ tmp.type +"'");
                    return false;
                }
            }
        }else if (is("-",1)){
            if (!next(2)) return false;
            if (!block(tmp)) return false;

            if (result.type == "int"){
                if (tmp.type == "int"){
                    result.value.setValue(result.value.toInt()-tmp.value.toInt());
                }else{
                    throwError("нельзя преобразовать '" + tmp.type + "' в '"+ result.type +"'");
                    return false;
                }
            }else if (result.type == "point"){
                if (tmp.type == "point"){
                    throwError("нельзя вычитать точки");
                    return false;
                }else if (tmp.type == "vector"){
                    QPoint tmp_vec = tmp.value.toPoint();
                    result.value.setValue(QPoint(result.value.toPoint().x() - tmp_vec.x(), result.value.toPoint().y() - tmp_vec.y()));
                    result.type = "point";
                }else if (tmp.type == "figure"){
                    throwError("нельзя вычитать из точки фигуру");
                    return false;
                }else{
                    throwError("нельзя преобразовать '" + tmp.type + "' в '"+ result.type +"'");
                    return false;
                }
            }else if (result.type == "vector"){
                if (tmp.type == "point"){
                    QPoint tmp_vec = result.value.toPoint();
                    result.value.setValue(QPoint(tmp_vec.x() - tmp.value.toPoint().x(), tmp_vec.y() - tmp.value.toPoint().y()));
                    result.type = "point";
                }else if (tmp.type == "vector"){
                    QPoint tmp_vec = result.value.toPoint();
                    result.value.setValue(QPoint(tmp.value.toPoint().x() - tmp_vec.x(), tmp.value.toPoint().y() - tmp_vec.y()));
                    result.type = "vector";
                }else if (tmp.type == "figure"){
                    QList<QPoint> tmp_fig = tmp.value.value<QList<QPoint>>();
                    QPoint tmp_vec = result.value.toPoint();
                    for (auto &p : tmp_fig){
                        p.setX(tmp_vec.x() - p.x());
                        p.setY(tmp_vec.y() - p.y());
                    }
                    result.value.setValue(tmp_fig);
                }else{
                    throwError("нельзя преобразовать '" + tmp.type + "' в '"+ result.type +"'");
                    return false;
                }
            }else if (result.type == "figure"){
                if (tmp.type == "vector"){
                    QList<QPoint> res_fig = result.value.value<QList<QPoint>>();
                    QPoint tmp_vec = tmp.value.toPoint();
                    for (auto &p : res_fig){
                        p.setX(p.x() - tmp_vec.x());
                        p.setY(p.y() - tmp_vec.y());
                    }
                    result.value.setValue(res_fig);
                }else if (tmp.type == "point"){
                    QList<QPoint> res_fig = result.value.value<QList<QPoint>>();
                    QPoint tmp_point = tmp.value.toPoint();

                    if (res_fig.removeOne(tmp_point)){
                        qDebug() << "remove point";
                    }

                    result.value.setValue(res_fig);
                }else if (tmp.type == "figure"){
                    QList<QPoint> res_fig = result.value.value<QList<QPoint>>();
                    QList<QPoint> tmp_fig = tmp.value.value<QList<QPoint>>();

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

            if (result.type == "int"){
                if (tmp.type == "int"){
                    result.value.setValue(result.value.toInt()*tmp.value.toInt());
                }else if (tmp.type == "vector"){
                    int num = result.value.toInt();
                    result.value.setValue(QPoint(tmp.value.toPoint().x()*num, tmp.value.toPoint().y()*num));
                }else if (tmp.type == "point"){
                    int num = result.value.toInt();
                    result.value.setValue(QPoint(tmp.value.toPoint().x()*num, tmp.value.toPoint().y()*num));
                }else if (tmp.type == "figure"){
                    QList<QPoint> tmp_fig = tmp.value.value<QList<QPoint>>();
                    int num = result.value.toInt();
                    for (auto &p : tmp_fig){
                        p.setX(num * p.x());
                        p.setY(num * p.y());
                    }
                    result.value.setValue(tmp_fig);
                }
            }else if (result.type == "vector"){
                if (tmp.type == "int"){
                    int num = tmp.value.toInt();
                    result.value.setValue(QPoint(result.value.toPoint().x()*num, result.value.toPoint().y()*num));
                }else if (tmp.type == "vector"){
                    QPoint tmp_vec = tmp.value.toPoint();
                    QPoint res_vec = result.value.toPoint();
                    result.value.setValue(QPoint(res_vec.x() * tmp_vec.x(), res_vec.y() * tmp_vec.y()));
                }else if (tmp.type == "point"){
                    QPoint tmp_vec = tmp.value.toPoint();
                    QPoint res_vec = result.value.toPoint();
                    result.value.setValue(QPoint(res_vec.x() * tmp_vec.x(), res_vec.y() * tmp_vec.y()));
                    result.type = "point";
                }else if (tmp.type == "figure"){
                    QList<QPoint> tmp_fig = tmp.value.value<QList<QPoint>>();
                    QPoint res_vec = result.value.toPoint();
                    for (auto &p : tmp_fig){
                        p.setX(res_vec.x() * p.x());
                        p.setY(res_vec.y() * p.y());
                    }
                    result.value.setValue(tmp_fig);
                }
            }else if (result.type == "point"){
                if (tmp.type == "int"){
                    int num = tmp.value.toInt();
                    result.value.setValue(QPoint(result.value.toPoint().x()*num, result.value.toPoint().y()*num));
                }else if (tmp.type == "vector"){
                    QPoint tmp_vec = tmp.value.toPoint();
                    QPoint res_vec = result.value.toPoint();
                    result.value.setValue(QPoint(res_vec.x() * tmp_vec.x(), res_vec.y() * tmp_vec.y()));
                }else{
                    throwError("нельзя перемножать '"+ result.type +"' с '" + tmp.type + "'");
                    return false;
                }
            }else if (result.type == "figure"){
                if (tmp.type == "int"){
                    QList<QPoint> res_fig = result.value.value<QList<QPoint>>();
                    int num = tmp.value.toInt();
                    for (auto &p : res_fig){
                        p.setX(p.x() * num);
                        p.setY(p.y() * num);
                    }
                    result.value.setValue(res_fig);
                }else if (tmp.type == "vector"){
                    QList<QPoint> res_fig = tmp.value.value<QList<QPoint>>();
                    QPoint tmp_vec = tmp.value.toPoint();
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


            if (result.type == "int"){
                if (tmp.type == "int"){
                    int num = tmp.value.toInt();
                    if (num == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }
                    result.value.setValue(result.value.toInt()/num);
                }else if (tmp.type == "vector"){
                    int num = result.value.toInt();
                    QPoint tmp_vec = tmp.value.toPoint();

                    if (tmp_vec.x() == 0 || tmp_vec.y() == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    result.value.setValue(QPoint(num / tmp_vec.x(), num / tmp_vec.y()));
                }else if (tmp.type == "point"){
                    int num = result.value.toInt();
                    QPoint tmp_point = tmp.value.toPoint();

                    if (tmp_point.x() == 0 || tmp_point.y() == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    result.value.setValue(QPoint(num / tmp_point.x(), num / tmp_point.y()));
                }else if (tmp.type == "figure"){
                    QList<QPoint> tmp_fig = tmp.value.value<QList<QPoint>>();
                    int num = result.value.toInt();
                    for (auto &p : tmp_fig){
                        if (p.x() == 0 || p.y() == 0){
                            throwError("нельзя делить на 0");
                            return false;
                        }

                        p.setX(num / p.x());
                        p.setY(num / p.y());
                    }
                    result.value.setValue(tmp_fig);
                }
            }else if (result.type == "vector"){
                if (tmp.type == "int"){
                    int num = tmp.value.toInt();

                    if (num == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    result.value.setValue(QPoint(result.value.toPoint().x()/num, result.value.toPoint().y()/num));
                }else if (tmp.type == "vector"){
                    QPoint tmp_vec = tmp.value.toPoint();

                    if (tmp_vec.x() == 0 || tmp_vec.y() == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    QPoint res_vec = result.value.toPoint();
                    result.value.setValue(QPoint(res_vec.x() / tmp_vec.x(), res_vec.y() / tmp_vec.y()));
                }else if (tmp.type == "point"){
                    QPoint tmp_vec = tmp.value.toPoint();

                    if (tmp_vec.x() == 0 || tmp_vec.y() == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    QPoint res_vec = result.value.toPoint();
                    result.value.setValue(QPoint(res_vec.x() / tmp_vec.x(), res_vec.y() / tmp_vec.y()));
                    result.type = "point";
                }else if (tmp.type == "figure"){
                    QList<QPoint> tmp_fig = tmp.value.value<QList<QPoint>>();
                    QPoint res_vec = result.value.toPoint();
                    for (auto &p : tmp_fig){
                        if (p.x() == 0 || p.y() == 0){
                            throwError("нельзя делить на 0");
                            return false;
                        }
                        p.setX(res_vec.x() / p.x());
                        p.setY(res_vec.y() / p.y());
                    }
                    result.value.setValue(tmp_fig);
                }
            }else if (result.type == "point"){
                if (tmp.type == "int"){
                    int num = tmp.value.toInt();

                    if (num == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    result.value.setValue(QPoint(result.value.toPoint().x()/num, result.value.toPoint().y()/num));
                }else if (tmp.type == "vector"){
                    QPoint tmp_vec = tmp.value.toPoint();
                    QPoint res_vec = result.value.toPoint();

                    if (tmp_vec.x() == 0 || tmp_vec.y() == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    result.value.setValue(QPoint(res_vec.x() / tmp_vec.x(), res_vec.y() / tmp_vec.y()));
                }else{
                    throwError("нельзя перемножать '"+ result.type +"' с '" + tmp.type + "'");
                    return false;
                }
            }else if (result.type == "figure"){
                if (tmp.type == "int"){
                    QList<QPoint> res_fig = result.value.value<QList<QPoint>>();
                    int num = tmp.value.toInt();

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
                    QList<QPoint> res_fig = tmp.value.value<QList<QPoint>>();
                    QPoint tmp_vec = tmp.value.toPoint();

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

     tmp.type.clear();          // чтобы на следующем шаге не проверялся выходной тип
     if (!rightPart(tmp)) return false;

     if (tmp.type != "int"){
         throwError("тут должно быть целое число");
         return false;
     }

     if (!is(")",1)){
         throwError("после '" + *word + "' должна быть закрывающая скобка");
         return false;
     }
     word++;

     // оформляем точку
     tmp_point.setY(tmp.value.toInt());
     result.value.setValue(tmp_point);
     result.type = "point";

     return true;
}

bool MyTranslator::getFigure(t_Variable &result){
    if (!is("{")){
        throwError("начало фигуры");
        return false;
    }

    if (!next()) return false;

    t_Variable tmp;

    if (varName()){
        tmp = ObjList[*word];

        if (tmp.type != "point"){
            throwError("это не точа");
            return false;
        }
    }else{
        tmp.type = "point";
        if (!rightPart(tmp)) return false;
    }

    QList<QPoint> tmp_fig;
    tmp_fig << tmp.value.toPoint();

    // добавление еще точек
    while (is(",",1)){
        word++;
        if (!next()){
            throwError("тут ваша точа");
            return false;
        }
        if (varName()){
            tmp = ObjList[*word];

            if (tmp.type != "point"){
                throwError("это не точа");
                return false;
            }
        }else{
            tmp.type = "point";
            if (!rightPart(tmp)) return false;
        }

        tmp_fig << tmp.value.toPoint();
    }

    result.value.setValue(tmp_fig);

    if (!is("}",1)){
        throwError("продолжите список точек или завершите сиволом '}'");
        return false;
    }
    word++;

    return true;
}

QString MyTranslator::CutWord(QString &str){
    if (str.size() > 10) return (str.left(10) + "...");
    else return str;
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
        if (is(",",2) && is(")",4)){
            if (!getPoint(result)) return false;
        }else{
            if (!next()){
                throwError("введите выражение в скобках");
                return false;
            }

            result.type.clear();
            if (!rightPart(result)) return false;
            if (!is(")",1)){
                throwError("закройте скобу");
                return false;
            }
            word++;
        }
    }else if (is("{")){
        if (!getFigure(result)) return false;
        result.type = "figure";
    }else if (is("[")){
        if (!getVector(result)) return false;
        result.type = "vector";
    }else if (getNum(result)){
        result.type = "int";
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
