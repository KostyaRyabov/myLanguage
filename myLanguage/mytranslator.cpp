#include "mytranslator.h"

MyTranslator::MyTranslator(QObject *parent)  :
    QObject(parent)
{
    DT["num"] = QStringList{"+","-","*","/"};
    DT["point"] = QStringList{"+","-","*","/","["};
    DT["vector"] = QStringList{"+","-","*","/","["};
    DT["figure"] = QStringList{"+","-","*","/","["};
}

void MyTranslator::read(QString text){
    store = text;

    inputData = text.replace(rx_comment," ").
            replace(";"," ; ").replace("="," = ").replace("+"," + ").replace("-"," - ").
            replace("*"," * ").replace("/"," / ").replace("("," ( ").replace(")"," ) ").
            replace("["," [ ").replace("]"," ] ").replace("\t"," ").replace("\n"," ").
            replace(","," , ").replace("{"," { ").replace("}"," } ").split(QLatin1Char(' '), Qt::SkipEmptyParts);

    word = inputData.begin();

    ObjList.clear();
    Figures.clear();
    end_characters.clear();

    end_characters.push({";"});

    draw = false;

    while (word != inputData.end()){
        if (!operation()) return;
    }

    end_characters.pop();

    emit DrawChanged();
}

QColor MyTranslator::getFillColor(int FigureID) const{
    return Figures[FigureID].FillColor;
}

QColor MyTranslator::getStrokeColor(int FigureID) const{
    return Figures[FigureID].StrokeColor;
}

QColor MyTranslator::getDotColor(int FigureID) const{
    return Figures[FigureID].DotColor;
}

int MyTranslator::getStrokeWidth(int FigureID) const{
    return Figures[FigureID].StrokeWidth;
}

int MyTranslator::getDotRadius(int FigureID) const{
    return Figures[FigureID].DotRadius;
}


bool MyTranslator::getDraw() const{
    return draw;
}

void MyTranslator::throwError(QString text, int s_pos){
    QList<QString>::iterator iter = inputData.begin();

    int    idx = 0,
            n = ((s_pos!=-1)?s_pos:(word - iter));

    QList<QChar> dic{' ','\n','\t'};

    while (dic.contains(store[idx])) idx++;
    if (idx == rx_comment.indexIn(store, idx, QRegExp::CaretMode::CaretAtOffset)) {
        idx += rx_comment.matchedLength()+1;
        while (dic.contains(store[idx])) idx++;
    }

    for (int i = 0; i < n; i++){
        idx += (*(iter + i)).length();

        while (dic.contains(store[idx])) idx++;
        if (idx == rx_comment.indexIn(store, idx, QRegExp::CaretMode::CaretAtOffset)) {
            idx += rx_comment.matchedLength();
            while (dic.contains(store[idx])) idx++;
        }
    }

    emit getError(text, idx);
}

int MyTranslator::amountOfFigures() const{
    return Figures.size();
}

int MyTranslator::amountOfPointsOnFigure(int i) const{
    return Figures[i].data.size();
}

int MyTranslator::getX(int FigureID, int PointID) const{
    return Figures[FigureID].data[PointID].x();
}

int MyTranslator::getY(int FigureID, int PointID) const{
    return  Figures[FigureID].data[PointID].y();
}

QVariantList MyTranslator::getHidenEdges(int FigureID) const{
    QVariantList list;
    auto &figure = Figures[FigureID];

    if (figure.jumps.size() > 1){
        for (auto &jump : figure.jumps){
            list << jump.idx+1;
        }
    }

    return list;
}

void MyTranslator::getHidenEdges(t_Figure &f) const{
    auto fig = f.data;
    int size = f.data.size()-1;

    Jump tmp;

    for (int i = 0; i < size; i++){
        for (int j = i+1; j < size; j++){
            if (i != j){
                if (fig[i] == fig[j+1]) {
                    if (fig[i+1] == fig[j]) {
                        tmp.idx = i;
                        f.jumps[i] = tmp;
                    }
                }
            }
        }
    }
}

bool MyTranslator::is(QString tag, int offset){
    for(int i = 1; i <= offset; i++){
         if (word + i == inputData.end()) return false;
    }
    return (*(word + offset) == tag);
}

bool MyTranslator::next(uint step){
    for(uint i = 1; i <= step; i++){
         if (word + i == inputData.end()) return false;
    }

    word+=step;
    return true;
}

bool MyTranslator::varName(){
    if (is("figure") || is("num") || is("vector") || is("point") || is("draw") || is("rotate") || is("var")){
        throwError("нельзя называть обьекты служебными именами");
        return false;
    }

    if (!(*word)[0].isLetter()) {
        throwError("имя переменной должно начинаться с буквы");
        return false;
    }

    for (uint16_t i = 1; i < (*word).size(); i++){
        if (!(*word)[i].isLetterOrNumber()){
            throwError("в имени переменной могут быть только буквы и цифры");
            return false;
        }
    }

    return true;
}

bool MyTranslator::operation(){
    if (is("figure") || is("num") || is("vector") || is("point") || is("var") || is("color")){
        if (!next()){
            throwError("после '"+*word+"' должно было быть имя переменной");
            return false;
        }
        if (!varName()) return false;

        QString initVar = *word;

        if (ObjList.contains(initVar)){
            throwError("переменная '" + CutWord(initVar) + "' уже обьявлена");
            return false;
        }

        t_Variable obj;
        obj.type = *(word-1);

        if (is("=",1)){
            word++;
            if (next()){
                if (!rightPart(obj)) return false;
            }else{
                if (obj.type == "var") throwError("после знака '=' должно быть объявлено новое значение");
                else throwError("после знака '=' должно быть объявлено новое значение с типом '"+obj.type+"'");
                return false;
            }
        }else if (!is(";",1)){
            throwError("после '"+CutWord(*word)+"' должно быть ';' или '='");
            return false;
        }else if (obj.type == "var"){
            throwError("динамическая переменная '"+CutWord(initVar)+"' не может быть неинициализированной");
            return false;
        }

        ObjList.insert(initVar, obj);
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

        R.type = "num";
        if (!rightPart(R, {")"})) return false;

        auto fig = obj.value.value<t_Figure>();

        QPointF o = getCenter(fig);

        float radians = R.value.toFloat() * 3.14159265 / 180;

        for (auto &_p : fig.data){
            auto p = _p;

            _p.setX((p.x() - o.x()) * cos(radians) - (p.y() - o.y()) * sin(radians) + o.x());
            _p.setY((p.x() - o.x()) * sin(radians) + (p.y() - o.y()) * cos(radians) + o.y());
        }

        if (!(is(")",1))) {
            throwError("после '"+*word+"' должна быть закрывающая круглая скобка");
            return false;
        }

        obj.value.setValue(fig);
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

        if (is(")")){
            throwError("функция 'draw' не может быть без аргументов. Введите в скобках список точек или фигур", word-inputData.begin()-1);
            return false;
        }

        int s_pos = word - inputData.begin();

        t_Variable obj;
        if (!rightPart(obj, {",",")"})) return false;

        if (obj.type == "point"){
            t_Figure tmp;
            tmp.data = QList<QPointF>({obj.value.toPointF()});

            Figures << tmp;
        }else if (obj.type == "figure"){
            Figures << obj.value.value<t_Figure>();
        }else{
            throwError("нельзя преобразовать '" + obj.type + "' в 'figure'",s_pos);
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

            obj.type.clear();
            if (!rightPart(obj)) return false;

            if (obj.type == "point"){
                t_Figure tmp;
                tmp.data = QList<QPointF>({obj.value.toPointF()});

                Figures << tmp;
            }else if (obj.type == "figure"){
                Figures << obj.value.value<t_Figure>();
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
        QString var = *(word);
        QStringList property;
        t_Variable result = ObjList[var];
        t_Figure figure;

        t_Variable tmp;

        if (result.type == "figure"){
            figure = result.value.value<t_Figure>();

            if (!is("=",1)){
                if (is("FillColor",1) || is("StrokeColor",1) || is("DotColor",1)){
                    word++;
                    property << *word;

                    tmp.type = "color";

                    if (is("Red",1) || is("Green",1) || is("Blue",1) || is("Alpha",1)){
                        word++;
                        property << *word;

                        tmp.type = "num";

                    }else if ((word+1) != inputData.end()){
                        if (!is("=",1) && !is(";",1)){
                            word++;
                            throwError("у обьекта 'color' нет такого параметра '"+CutWord(*word)+"'.\n\nВозможные варианты:\nRed, Green, Blue, Alpha");
                            return false;
                        }
                    }else{
                        throwError("после слова '"+CutWord(*word)+"' ожидался знак '=' или один из параметров цвета: Red, Green, Blue, Alpha");
                        return false;
                    }
                }else if (is("StrokeWidth",1) || is("DotRadius",1)){
                    word++;
                    property << *word;

                    tmp.type = "num";
                }else if ((word+1) != inputData.end()){
                    throwError("у фигур нет такого параметра '"+CutWord(*(word+1))+"'.\n\nВозможные варианты:\nFillColor, StrokeColor, DotColor, StrokeWidth, DotRadius");
                    return false;
                }else{
                    throwError("после слова '"+CutWord(*word)+"' ожидался знак '=' или один из параметров фигуры:\nFillColor, StrokeColor, DotColor, StrokeWidth, DotRadius");
                    return false;
                }
            }
        }

        if (result.type == "color"){
            if (is("Red",1) || is("Green",1) || is("Blue",1) || is("Alpha",1)){
                word++;
                property << *word;

                tmp.type = "num";
            }else if ((word+1) != inputData.end()){
                if (!is("=",1) && !is(";",1)){
                    word++;
                    throwError("у обьекта 'color' нет такого параметра '"+CutWord(*word)+"'.\n\nВозможные варианты:\nRed, Green, Blue, Alpha");
                    return false;
                }
            }
        }

        if (is("=",1)){
            word++;

            if (next()){
                if (property.size() == 2){
                    QColor *color;
                    if (property.first() == "FillColor") color = &figure.FillColor;
                    else if (property.first() == "StrokeColor") color = &figure.StrokeColor;
                    else if (property.first() == "DotColor") color = &figure.DotColor;

                    tmp.type = "num";
                    if (!rightPart(tmp)) return false;

                    if (property.last()  == "Red") color->setRedF(tmp.value.toFloat());
                    else if (property.last()  == "Green") color->setGreenF(tmp.value.toFloat());
                    else if (property.last()  == "Blue") color->setBlueF(tmp.value.toFloat());
                    else if (property.last()  == "Alpha") color->setAlphaF(tmp.value.toFloat());

                    result.value.setValue(figure);
                }else if (property.size() == 1){
                    QColor color;
                    if (result.type == "color") color = result.value.value<QColor>();

                    if (property.first() == "FillColor")                   tmp.value.setValue(figure.FillColor);
                    else if (property.first() == "StrokeColor")     tmp.value.setValue(figure.StrokeColor);
                    else if (property.first() == "DotColor")          tmp.value.setValue(figure.DotColor);
                    else if (property.first() == "DotRadius")        tmp.value.setValue(figure.DotRadius);
                    else if (property.first() == "StrokeWidth")    tmp.value.setValue(figure.StrokeWidth);

                    if (!rightPart(tmp)) return false;

                    if (property.first() == "FillColor")                  figure.FillColor = tmp.value.value<QColor>();
                    else if (property.first() == "StrokeColor")    figure.StrokeColor = tmp.value.value<QColor>();
                    else if (property.first() == "DotColor")         figure.DotColor = tmp.value.value<QColor>();
                    else if (property.first() == "DotRadius")       figure.DotRadius = tmp.value.toFloat();
                    else if (property.first() == "StrokeWidth")   figure.StrokeWidth = tmp.value.toFloat();
                    else if (property.last()  == "Red")                 color.setRedF(tmp.value.toFloat());
                    else if (property.last()  == "Green")              color.setGreenF(tmp.value.toFloat());
                    else if (property.last()  == "Blue")                color.setBlueF(tmp.value.toFloat());
                    else if (property.last()  == "Alpha")              color.setAlphaF(tmp.value.toFloat());

                    if (result.type == "color") result.value.setValue(color);
                    else result.value.setValue(figure);
                }else{
                    if (!rightPart(result)) return false;
                }

                ObjList[var] = result;
            }else{
                throwError("после знака '=' должно быть присваемое значение типа '"+tmp.type+"'");
                return false;
            }
        }else{
            if (result.type == "color" && property.isEmpty()) throwError("после слова '"+CutWord(*word)+"' ожидался знак '=' или один из параметров цвета: Red, Green, Blue, Alpha");
            else throwError("после слова '"+CutWord(*word)+"' должен быть знак '='");

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

QPair<int,int> MyTranslator::getRange(int i, QHash<int,int> &ranges){
    if (i < ranges[i]) return {i,ranges[i]};
    else return {ranges[i],i};
}

bool MyTranslator::onSameLine(int i, QHash<int,int> &ranges){
    auto keys = ranges.keys();

    for (auto &k : keys){
        if (k <= i-1 && i+1 <= ranges[k]) return true;
    }

    return false;
}

bool MyTranslator::simplify(t_Figure &figure){
    if (figure.data.size() > 0){

        bool changed = false;

        for (auto it = figure.data.begin()+1; it+1 != figure.data.end();){
            if (figure.jumps.contains(it - figure.data.begin())) {
                it++;
                continue;
            }

            if (abs(it->x()-(it+1)->x()) <= 1.1 && abs(it->y()-(it+1)->y()) <= 1.1){
                it = figure.data.erase(it);
                changed = true;

                getFigureInfo(figure);
            }else if (onSameLine(it - figure.data.begin(), figure.transition) && abs(((it-1)->x()-(it+1)->x())*((it)->y()-(it+1)->y())-((it-1)->y()-(it+1)->y())*((it)->x()-(it+1)->x())) <= EPS){
                if (figure.jumps.contains(it - figure.data.begin())) {
                    it++;
                    continue;
                }

                it = figure.data.erase(it);
                changed = true;

                getFigureInfo(figure);
            }else it++;
        }

        return changed;
    }

    return false;
}

QPointF MyTranslator::getCenter(t_Figure &figure){
    QPointF o;
    int size = figure.data.size()-1,count = 0;

    for (int i = 0; i < size; i++){
        if (figure.jumps.contains(i-1)) continue;

        o += figure.data[i];
        count++;
    }

    o /= count;

    return o;
}

bool MyTranslator::isFilledFigure(t_Figure &figure) const{
    return (figure.data.first() == figure.data.back());
}

QPointF MyTranslator::normalizedVector(QPointF v){
    return  v/(sqrt(pow(v.x(),2) + pow(v.y(),2)));
}

bool MyTranslator::isInside(QPointF point, t_Figure &figure, bool Ignore_borders){
    auto &fig = figure.data;
    int size = fig.size()-1, count=0;
    float ax1,ax2,ay1,ay2,
            by1 = point.y(),
            bx1 = point.x();

    QPointF vec = point+QPointF{2000,2000}, cp;

    for (int i = 0; i < size; i++){
        if (figure.jumps.contains(i) && figure.jumps.size()>1) continue;

        if (fig[i] == point) return !Ignore_borders;

        ax1 = fig[i].x();
        ay1 = fig[i].y();
        ax2 = fig[i+1].x();
        ay2 = fig[i+1].y();

        if (std::abs((by1 - ay2) *  (ax1 - ax2)  - (bx1 - ax2) * (ay1 - ay2)) <= EPS) {
            if (between(ax1,ax2,bx1) && between(ay1,ay2,by1)) {
                return !Ignore_borders;
            }
        }

        if (cross(point,vec,fig[i],fig[i+1],&cp)){
            if (fig[i] == cp) continue;
            count++;
        }
    }

    return (count%2==1);
}

int MyTranslator::getIdxOfMinPerpendicular(QPointF &point, t_Figure &figure, float *dist){
    auto fig = figure.data;
    int idx = 0, size = fig.size()-1;
    float x1,x2,y1,y2,rx,ry,L,PR,cf,cur_dist,
            min_dist =std::numeric_limits<float>::max(),
            x = point.x(),
            y = point.y();

    for (int i = 0; i < size; i++){
        if (figure.jumps.contains(i) && figure.jumps.size() > 1) continue;

        x1 = fig[i].x();
        y1 = fig[i].y();
        x2 = fig[(i+1)].x();
        y2 = fig[(i+1)].y();

        L=(x1-x2)*(x1-x2)+(y1-y2)*(y1-y2);
        PR=(x-x1)*(x2-x1)+(y-y1)*(y2-y1);
        cf=PR/L;

        if (0 <= cf && cf <= 1){
            rx=x1+cf*(x2-x1);
            ry=y1+cf*(y2-y1);

            cur_dist = sqrt(pow(rx-x,2)+pow(ry-y,2));

            if (cur_dist < min_dist){
                min_dist = cur_dist;
                idx = i+1;
            }
        }
    }

    if (dist) if (idx >= 0) *dist = min_dist;

    return idx;
}

QList<QPointF> MyTranslator::IntersectionList(t_Figure &A, t_Figure &B){
    auto Af = A.data, Bf = B.data;

    if (Af.isEmpty() || Bf.isEmpty()) return {};

    uint32_t size_A = Af.size()-1,
            size_B = Bf.size()-1;

    QList<QPointF> list;
    QPointF p,f;

    for (uint32_t i = 0; i < size_A; i++){
        if (A.jumps.contains(i) && A.jumps.size() > 1) continue;

        for (uint32_t j = 0; j < size_B; j++){
            if (B.jumps.contains(j) && B.jumps.size() > 1) continue;

            if (cross(Af[i],Af[i+1],Bf[j],Bf[j+1],&p)){
                if (!list.contains(p)) list << p;
            }

            if (std::abs((Af[i].x()-Af[i+1].x())*(Bf[j].y()-Bf[j+1].y())-(Af[i].y()-Af[i+1].y())*(Bf[j].x()-Bf[j+1].x())) <= EPS){

                f = (Af[i]+Af[i+1]+Bf[j]+Bf[j+1])/4;

                if (std::abs((f.x()-Af[i+1].x())*(Af[i].y()-Af[i+1].y())-(f.y()-Af[i+1].y())*(Af[i].x()-Af[i+1].x())) <= EPS){
                    if (between(Af[i].x(),Af[i+1].x(),f.x()) && between(Af[i].y(),Af[i+1].y(),f.y())){
                        if (!list.contains(f)) list << f;
                    }
                }
            }
        }
    }

    return list;
}

int MyTranslator::Intersection(QPointF &A,QPointF &B,  t_Figure &figure){
    uint32_t size = figure.data.size()-1;

    QList<QPointF> list;
    QPointF p;

    for (uint32_t j = 0; j < size; j++){
        if (figure.jumps.contains(j)) continue;
        if (cross(A,B,figure.data[j],figure.data[j+1],&p)) if (!list.contains(p)) list << p;
    }

    return -1;
}

int MyTranslator::getIdxOfNearestEdge(QPointF &point, t_Figure &figure){
    int size = figure.data.size()-1, idx = -1;
    float min_dist = std::numeric_limits<float>::max(), cur_dist;
    bool iff = figure.data.size()>1;

    if (iff){
        if (isInside(point,figure)) return -1;
    }

    for (int i = 0; i < size+((int)!iff); i++){
        if (figure.jumps.contains(i)) continue;

        if (Intersection(figure.data[i],point,figure) != -1) continue;
        if (Intersection(figure.data[(i+1)%size],point,figure) != -1) continue;

        cur_dist = sqrt(pow(point.x()-figure.data[i].x(),2)+pow(point.y()-figure.data[i].y(),2)) + sqrt(pow(point.x()-figure.data[(i+1)%size].x(),2)+pow(point.y()-figure.data[(i+1)%size].y(),2));

        if (cur_dist <= min_dist){
            min_dist = cur_dist;
            idx = i+1;
        }
    }

    if (!iff){
        if (idx == 1) idx = 0;
        else if (idx == -1) idx = size;
    }

    return idx;
}

void MyTranslator::getFigureInfo(t_Figure &f) const{
    auto fig = f.data;
    Jump tmp;

    int size = fig.size()-1;

    f.jumps.clear();
    f.transition.clear();

    if (f.data.size() > 3){
        for (int i = 0; i < size; i++){
            for (int j = i+1; j < size; j++){
                if (i != j){
                    if (fig[i] == fig[j+1]) {
                        if (fig[i+1] == fig[j]) {
                            tmp.idx = i;
                            f.jumps[i] = tmp;

                            tmp.idx = j;
                            f.jumps[j] = tmp;
                        }
                    }
                }
            }
        }
    }

    if (f.jumps.isEmpty()){
        Jump jump;
        jump.idx = 0;

        f.jumps[0] = jump;

        f.transition[0] = size;
        f.transition[size] = 0;
    }else{
        auto jumps = f.jumps;

        jumps[-1];

        auto keys = jumps.keys();
        int k_size = keys.size()-1;

        for (int i = 0; i < k_size; i++){
            f.transition[keys[i+1]] = keys[i]+1;
            f.transition[keys[i]+1] = keys[i+1];
        }
    }
}

float MyTranslator::Len(QPointF &A, QPointF &B){
    auto d = A-B;
    return (sqrt(pow(d.x(),2)+pow(d.y(),2)));
}

bool MyTranslator::cross(QPointF &L11,QPointF &L12, QPointF &L21,QPointF &L22, QPointF *res){
    float a1,b1,c1,a2,b2,c2,x1,x2,x3,x4,y1,y2,y3,y4,div;

    x1 = L11.x();
    y1 = L11.y();
    x2 = L12.x();
    y2 = L12.y();

    a1 = y2-y1;
    b1 = x1-x2;
    c1 = y1*x2-x1*y2;

    x3 = L21.x();
    y3 = L21.y();
    x4 = L22.x();
    y4 = L22.y();

    a2 = y4-y3;
    b2 = x3-x4;
    c2 = y3*x4-x3*y4;

    div = (a1 * b2 - a2 * b1);

    if (std::abs(div) >= EPS){
        float x = (b1 * c2 - b2 * c1) / div;
        float y = (a2 * c1 - a1 * c2) / div;

        if (between(x1,x2,x) && between(y1,y2,y) && between(x3,x4,x) && between(y3,y4,y)){
            if (res != nullptr) {
                *res = {x,y};

                if (Len(*res,L11) <= EPS) *res = L11;
                else if (Len(*res,L12) <= EPS) *res = L12;
                else if (Len(*res,L21) <= EPS) *res = L21;
                else if (Len(*res,L22) <= EPS) *res = L22;
            }
            return true;
        }
    }

    return false;
}

bool MyTranslator::rightPart(t_Variable &result, QStringList expected_end_characters){
    if (!expected_end_characters.isEmpty())
        end_characters.push(expected_end_characters);

    bool minus = false;
    QString outputType = result.type;

    int s_pos = word-inputData.begin(), s_pos2;

    if (is("-")){
        minus = true;
        if (!next()) return false;
    }else if (is("+")){
        word++;
    }

    if (!block(result)) return false;

    if (minus) {
        if (result.type == "num"){
            result.value.setValue(-result.value.toFloat());
        }else if (result.type == "point" || result.type == "vector"){
            QPointF tmp_point = result.value.toPointF();
            result.value.setValue(QPointF(-tmp_point.x(),-tmp_point.y()));
        }else if (result.type == "figure"){
            t_Figure tmp_fig = result.value.value<t_Figure>();
            for (auto &p : tmp_fig.data){
                p=-p;
            }
            result.value.setValue(tmp_fig);
        }

    }

    t_Variable tmp;

    while(is("+",1) || is("-",1)){
        s_pos2 = word-inputData.begin()+2;

        if (is("+",1)){
            if (!next(2)) return false;

            if (!block(tmp)) return false;

            if (result.type == "num"){
                if (tmp.type == "num"){
                    result.value.setValue(result.value.toFloat()+tmp.value.toFloat());
                }else{
                    throwError("нельзя складывать '" + result.type + "' с '"+ tmp.type +"'\nожидаемые типы объектов:\nnum",s_pos2);
                    return false;
                }
            }else if (result.type == "point"){
                if (tmp.type == "point"){
                    t_Figure figure;
                    figure.data = {result.value.toPointF(), tmp.value.toPointF(),result.value.toPointF()};

                    result.value.setValue(figure);
                    result.type = "figure";
                }else if (tmp.type == "vector"){
                    QPointF tmp_vec = tmp.value.toPointF();
                    result.value.setValue(result.value.toPointF() + tmp_vec);
                }else if (tmp.type == "figure"){
                    t_Figure figure = tmp.value.value<t_Figure>();
                    QPointF point = result.value.toPointF();
                    int idx = -1;

                    idx = getIdxOfNearestEdge(point, figure);

                    if (idx >= 0) figure.data.insert(idx,point);

                    result.value.setValue(figure);
                    result.type = "figure";
                }else{
                    throwError("нельзя складывать '" + result.type + "' с '"+ tmp.type +"'\nожидаемые типы объектов:\npoint, vector, figure",s_pos2);
                    return false;
                }
            }else if (result.type == "vector"){
                if (tmp.type == "point"){
                    QPointF tmp_vec = result.value.toPointF();
                    result.value.setValue(tmp.value.toPointF() + tmp_vec);
                    result.type = "point";
                }else if (tmp.type == "vector"){
                    QPointF tmp_vec = tmp.value.toPointF();
                    QPointF res_vec = result.value.toPointF();
                    result.value.setValue(res_vec + tmp_vec);
                }else if (tmp.type == "figure"){
                    t_Figure tmp_fig = tmp.value.value<t_Figure>();
                    QPointF tmp_vec = result.value.toPointF();
                    for (auto &p : tmp_fig.data){
                        p += tmp_vec;
                    }
                    result.value.setValue(tmp_fig);
                    result.type = "figure";
                }else{
                    throwError("нельзя складывать '" + result.type + "' с '"+ tmp.type +"'\nожидаемые типы объектов:\npoint, vector, figure",s_pos2);
                    return false;
                }
            }else if (result.type == "figure"){
                if (tmp.type == "vector"){
                    t_Figure res_fig = result.value.value<t_Figure>();
                    QPointF tmp_vec = tmp.value.toPointF();
                    for (auto &p : res_fig.data){
                        p+=tmp_vec;
                    }
                    result.value.setValue(res_fig);
                }else if (tmp.type == "point"){
                    t_Figure figure = result.value.value<t_Figure>();
                    QPointF point = tmp.value.toPointF();
                    int idx = -1;

                    idx = getIdxOfNearestEdge(point, figure);

                    if (idx >= 0) figure.data.insert(idx,point);

                    result.value.setValue(figure);
                }else if (tmp.type == "figure"){
                    t_Figure res,
                            B = tmp.value.value<t_Figure>(),
                            A = result.value.value<t_Figure>();

                    QList<QPointF> &Af = A.data, &Bf = B.data, &Rf = res.data;

                    QList<QPointF> list = IntersectionList(A,B);
                    QHash<QPointF, RowData> table;

                    uint32_t A_idx = 0, B_idx;

                    getFigureInfo(A);
                    getFigureInfo(B);

                    if (simplify(A)) getFigureInfo(A);
                    if (simplify(B)) getFigureInfo(B);

                    for (auto &p : list){
                        A_idx = getIdxOfMinPerpendicular(p, A);
                        if (Af[A_idx] != p) Af.insert(A_idx,p);
                        getFigureInfo(A);

                        B_idx = getIdxOfMinPerpendicular(p, B);
                        if (Bf[B_idx] != p) Bf.insert(B_idx ,p);
                        getFigureInfo(B);
                    }

                    int    A_size = Af.size()-1,
                            B_size = Bf.size()-1;

                    for (int i = 0;i < A_size;i++) if (list.contains(Af[i])) table[Af[i]].A_idx = i;
                    for (int i = 0;i < B_size;i++) if (list.contains(Bf[i])) table[Bf[i]].B_idx = i;

                    RowData pc;

                    int startPos;

                    {
                        QPointF v;

                        int8_t stepA,stepB;
                        int i,j,pv;

                        uint8_t state = 0;

                        Jump jump;
                        jump.idx = 0;
                        jump.visited = false;

                        if (A.jumps.isEmpty()) A.jumps[0] = jump;
                        if (B.jumps.isEmpty()) B.jumps[0] = jump;

                        getFigureInfo(A);
                        getFigureInfo(B);

                        do{
                            do{
                                startPos = -1;

                                for (auto &jump : A.jumps){
                                    if (!jump.visited){
                                        jump.visited = true;
                                        auto range = getRange(jump.idx,A.transition);

                                        for (i = range.first; i < range.second; i++){
                                            if (!isInside(Af[i],B) && !table.contains(Af[i])){
                                                startPos = i;
                                                break;
                                            }
                                        }

                                        if (startPos == -1){
                                            for (i = range.first; i < range.second; i++){
                                                if (!isInside(Af[i],B,true)) break;
                                            }

                                            if (i != range.second){
                                                if (state == State::pass_A) Rf << Af.mid(range.first, (range.second-range.first+1));
                                            }

                                            continue;
                                        }else{
                                            for (i = range.first; i < range.second; i++){
                                                if (table.contains(Af[i])){
                                                    if (table[Af[i]].visited){
                                                        startPos = -1;
                                                        break;
                                                    }
                                                }
                                            }
                                            if (i != range.second) continue;
                                        }

                                        break;
                                    }
                                }

                                if (startPos != -1){
                                    i=startPos;

                                    Rf<<Af[i];

                                    pv = i;
                                    if (A.transition.contains(pv)){
                                        if ((pv - A.transition[pv]) > 0){
                                            pv = A.transition[pv];
                                        }
                                    }
                                    pv++;

                                    v = normalizedVector(Af[pv]-Af[i]);
                                    stepA = (isInside(Af[i]+v,B,true)?-1:1);

                                    if (A.transition.contains(i)) {
                                        if (A.jumps.contains(i)) A.jumps[i].visited = true;
                                        if ((i - A.transition[i])*stepA > 0){
                                            i = A.transition[i];
                                            if (A.jumps.contains(i)) A.jumps[i].visited = true;
                                        }
                                    }

                                    i+=stepA;

                                    if (A.transition.contains(i)) {
                                        if (A.jumps.contains(i)) A.jumps[i].visited = true;
                                        i = A.transition[i];
                                        if (A.jumps.contains(i)) A.jumps[i].visited = true;
                                    }

                                    Rf << Af[i];

                                    for (;;) {
                                        if (list.contains(Af[i])){
                                            if (table[Af[i]].visited){
                                                break;
                                            }else{
                                                table[Af[i]].visited = true;

                                                pc = table[Af[i]];
                                                j = pc.B_idx;

                                                pv = j;
                                                if (B.transition.contains(pv)){
                                                    if (pc.B_idx > B.transition[pc.B_idx]){
                                                        pv = B.transition[pv];
                                                    }
                                                }
                                                pv++;

                                                v = normalizedVector(Bf[pv]-Bf[j]);
                                                stepB = (isInside(Bf[j]+v,A,true)?-1:1);

                                                if (B.transition.contains(pc.B_idx)) {
                                                    if (B.jumps.contains(j)) B.jumps[j].visited = true;
                                                    if ((pc.B_idx - B.transition[pc.B_idx])*stepB > 0){
                                                        j = B.transition[pc.B_idx];
                                                        if (B.jumps.contains(j)) B.jumps[j].visited = true;
                                                    }
                                                }

                                                j+=stepB;

                                                if (B.transition.contains(j)) {
                                                    j = B.transition[j];
                                                    if (B.jumps.contains(j)) B.jumps[j].visited = true;
                                                }

                                                Rf << Bf[j];

                                                for (;;) {
                                                    if (list.contains(Bf[j])){
                                                        table[Bf[j]].visited = true;
                                                        i = table[Bf[j]].A_idx;
                                                        break;
                                                    }

                                                    j+=stepB;

                                                    Rf << Bf[j];

                                                    if (B.transition.contains(j)){
                                                        if (B.jumps.contains(j)) B.jumps[j].visited = true;
                                                        j = B.transition[j];
                                                        if (B.jumps.contains(j)) B.jumps[j].visited = true;
                                                    }
                                                }
                                            }

                                            if (i == startPos) break;

                                            if (A.transition.contains(i)){
                                                if (A.jumps.contains(i)) A.jumps[i].visited = true;
                                                i = A.transition[i];
                                                if (A.jumps.contains(i)) A.jumps[i].visited = true;
                                                //if (i == startPos) break;
                                            }
                                        }

                                        i+=stepA;

                                        Rf << Af[i];

                                        if (i == startPos) break;

                                        if (A.transition.contains(i)){
                                            if (A.jumps.contains(i)) A.jumps[i].visited = true;
                                            i = A.transition[i];
                                            if (A.jumps.contains(i)) A.jumps[i].visited = true;
                                            if (i == startPos) break;
                                        }
                                    }

                                    Rf << Rf[0];
                                }

                                if (!Rf.isEmpty() && Rf.back() != Rf.first()) Rf << Rf[0];
                            }while(startPos != -1);

                            state++;

                            if (state == State::pass_B){
                                std::swap(A,B);
                            }
                        }while(state != State::end);
                    }

                    getFigureInfo(res);
                    if (simplify(res)) getFigureInfo(res);


                    result.value.setValue(res);
                }else{
                    throwError("нельзя складывать '" + result.type + "' с '"+ tmp.type +"'\nожидаемые типы объектов:\npoint, vector, figure",s_pos2);
                    return false;
                }
            }
        }else if (is("-",1)){
            if (!next(2)) return false;
            if (!block(tmp)) return false;

            if (result.type == "num"){
                if (tmp.type == "num"){
                    result.value.setValue(result.value.toFloat()-tmp.value.toFloat());
                }else{
                    throwError("нельзя вычитать из '" + result.type + "' '"+ tmp.type +"'\nожидаемые типы объектов:\nnum",s_pos2);
                    return false;
                }
            }else if (result.type == "point"){
                if (tmp.type == "vector"){
                    QPointF tmp_vec = tmp.value.toPointF();
                    QPointF res_point = result.value.toPointF();
                    result.value.setValue(res_point - tmp_vec);
                }else{
                    throwError("нельзя вычитать из '" + result.type + "' '"+ tmp.type +"'\nожидаемые типы объектов:\nvector",s_pos2);
                    return false;
                }
            }else if (result.type == "vector"){
                if (tmp.type == "point"){
                    QPointF res_vec = result.value.toPointF();
                    QPointF tmp_point = tmp.value.toPointF();
                    result.value.setValue(res_vec - tmp_point);
                    result.type = "point";
                }else if (tmp.type == "vector"){
                    QPointF res_vec = result.value.toPointF();
                    QPointF tmp_vec = tmp.value.toPointF();
                    result.value.setValue(res_vec - tmp_vec);
                }else if (tmp.type == "figure"){
                    t_Figure tmp_fig = tmp.value.value<t_Figure>();
                    QPointF res_vec = result.value.toPointF();
                    for (auto &p : tmp_fig.data){
                        p = res_vec - p;
                    }
                    result.value.setValue(tmp_fig);
                    result.type = "figure";
                }else{
                    throwError("нельзя вычитать из '" + result.type + "' '"+ tmp.type +"'\nожидаемые типы объектов:\nvector, point, figure",s_pos2);
                    return false;
                }
            }else if (result.type == "figure"){
                if (tmp.type == "vector"){
                    t_Figure res_fig = result.value.value<t_Figure>();
                    QPointF tmp_vec = tmp.value.toPointF();
                    for (auto &p : res_fig.data){
                        p -= tmp_vec;
                    }
                    result.value.setValue(res_fig);
                }else if (tmp.type == "point"){
                    t_Figure figure = result.value.value<t_Figure>();
                    QPointF point = tmp.value.toPointF();

                    if (isFilledFigure(figure)){
                        if (isInside(point,figure)){
                            int idx = getIdxOfMinPerpendicular(point,figure);

                            figure.data.insert(idx,point);
                            result.value.setValue(figure);
                        }
                    }
                }else if (tmp.type == "figure"){
                    t_Figure res,
                            B = tmp.value.value<t_Figure>(),
                            A = result.value.value<t_Figure>();

                    try{
                        QList<QPointF> &Af = A.data, &Bf = B.data, &Rf = res.data;

                        if (!Af.isEmpty()){
                        QList<QPointF> list = IntersectionList(A,B);
                        QHash<QPointF, RowData> table;

                        uint32_t A_idx = 0, B_idx;

                        for (auto &p : list){
                            A_idx = getIdxOfMinPerpendicular(p, A);
                            if (Af[A_idx] != p) Af.insert(A_idx,p);
                            getFigureInfo(A);

                            B_idx = getIdxOfMinPerpendicular(p, B);
                            if (Bf[B_idx] != p) Bf.insert(B_idx ,p);
                            getFigureInfo(B);
                        }

                        int    A_size = Af.size()-1,
                                B_size = Bf.size()-1;

                        for (int i = 0;i < A_size;i++) if (list.contains(Af[i])) table[Af[i]].A_idx = i;
                        for (int i = 0;i < B_size;i++) if (list.contains(Bf[i])) table[Bf[i]].B_idx = i;

                        RowData pc;

                        int startPos;

                        {
                            QPointF v;

                            int8_t stepA,stepB;
                            int i,j,pv;

                            Jump jump;
                            jump.idx = 0;
                            jump.visited = false;

                            if (A.jumps.isEmpty()) A.jumps[0] = jump;
                            if (B.jumps.isEmpty()) B.jumps[0] = jump;

                            getFigureInfo(A);
                            getFigureInfo(B);

                            do{
                                startPos = -1;

                                for (auto &cross_point : table){
                                    if (!cross_point.visited){
                                        startPos = cross_point.A_idx;

                                        for (auto &jump : A.jumps){
                                            auto range = getRange(jump.idx,A.transition);

                                            if (range.first < startPos && startPos < range.second){
                                                jump.visited = true;

                                                for (i = range.first; i < range.second; i++){
                                                    if (!isInside(Af[i],B)) break;
                                                }

                                                if (i == range.second) startPos = -1;

                                                break;
                                            }
                                        }
                                        break;
                                    }
                                }

                                if (startPos == -1){
                                    for (auto &jump : A.jumps){
                                        if (!jump.visited){
                                            jump.visited = true;
                                            auto range = getRange(jump.idx,A.transition);

                                            for (i = range.first; i < range.second; i++){
                                                if (!isInside(Af[i],B) && !table.contains(Af[i])){
                                                    startPos = i;
                                                    break;
                                                }
                                            }

                                            break;
                                        }
                                    }
                                }

                                if (startPos != -1){
                                    i=startPos;

                                    Rf<<Af[i];

                                    pv = i;
                                    if (A.transition.contains(pv)){
                                        if ((pv - A.transition[pv]) > 0){
                                            pv = A.transition[pv];
                                        }
                                    }
                                    pv++;

                                    v = normalizedVector(Af[pv]-Af[i]);
                                    stepA = isInside(Af[i]+v,B,true)?-1:1;

                                    if (A.transition.contains(i)) {
                                        if (A.jumps.contains(i)) A.jumps[i].visited = true;
                                        if ((i - A.transition[i])*stepA > 0){
                                            i = A.transition[i];
                                            if (A.jumps.contains(i)) A.jumps[i].visited = true;
                                        }
                                    }

                                    i+=stepA;

                                    if (A.transition.contains(i)) {
                                        if (A.jumps.contains(i)) A.jumps[i].visited = true;
                                        i = A.transition[i];
                                        if (A.jumps.contains(i)) A.jumps[i].visited = true;
                                    }

                                    Rf << Af[i];

                                    for (;;) {
                                        if (list.contains(Af[i])){
                                            if (table[Af[i]].visited){
                                                break;
                                            }else{
                                                table[Af[i]].visited = true;

                                                pc = table[Af[i]];
                                                j = pc.B_idx;

                                                pv = j;
                                                if (B.transition.contains(pv)){
                                                    if (pc.B_idx > B.transition[pc.B_idx]){
                                                        pv = B.transition[pv];
                                                    }
                                                }
                                                pv++;

                                                v = normalizedVector(Bf[pv]-Bf[j]);
                                                stepB = isInside(Bf[j]+v,A,true)?1:-1;

                                                if (B.transition.contains(pc.B_idx)) {
                                                    if (B.jumps.contains(j)) B.jumps[j].visited = true;
                                                    if ((pc.B_idx - B.transition[pc.B_idx])*stepB > 0){
                                                        j = B.transition[pc.B_idx];
                                                        if (B.jumps.contains(j)) B.jumps[j].visited = true;
                                                    }
                                                }

                                                j+=stepB;

                                                if (B.transition.contains(j)) {
                                                    j = B.transition[j];
                                                    if (B.jumps.contains(j)) B.jumps[j].visited = true;
                                                }

                                                Rf << Bf[j];

                                                for (;;) {
                                                    if (list.contains(Bf[j])){
                                                        table[Bf[j]].visited = true;
                                                        i = table[Bf[j]].A_idx;
                                                        break;
                                                    }

                                                    j+=stepB;

                                                    if (B.jumps.contains(j)) B.jumps[j].visited = true;

                                                    Rf << Bf[j];

                                                    if (B.transition.contains(j)){
                                                        j = B.transition[j];
                                                        if (B.jumps.contains(j)) B.jumps[j].visited = true;
                                                    }
                                                }
                                            }

                                            if (i == startPos) break;

                                            if (A.transition.contains(i)){
                                                if (A.jumps.contains(i)) A.jumps[i].visited = true;
                                                i = A.transition[i];
                                                if (A.jumps.contains(i)) A.jumps[i].visited = true;
                                                if (i == startPos) break;
                                            }
                                        }

                                        i+=stepA;

                                        if (i == startPos) break;

                                        Rf << Af[i];

                                        if (A.transition.contains(i)){
                                            if (A.jumps.contains(i)) A.jumps[i].visited = true;
                                            i = A.transition[i];
                                            if (A.jumps.contains(i)) A.jumps[i].visited = true;
                                            if (i == startPos) break;
                                        }
                                    }

                                    if (!Rf.isEmpty())
                                        Rf << Rf[0];
                                }

                                if (!Rf.isEmpty() && Rf.back() != Rf.first()) Rf << Rf.first();
                            }while(startPos != -1);
                        }

                        getFigureInfo(res);
                        if (simplify(res)) getFigureInfo(res);
                    }
                    }  catch (QException &e) {
                        qDebug() << e.what();
                    }

                    result.value.setValue(res);
                }else{
                    throwError("нельзя вычитать '" + tmp.type + "' из '"+ result.type +"'",s_pos2);
                    return false;
                }
            }
        }
    }

    if (!outputType.isEmpty()){
        if (result.type == "point" && outputType == "figure"){
            result.type = "figure";
            result.value.setValue(QList<QPointF>({result.value.toPointF()}));
        }

        if (result.type != outputType && outputType != "var"){
            throwError("нельзя преобразовать '" + result.type + "' в '"+ outputType +"'",s_pos);
            return false;
        }
    }

    if (!expected_end_characters.isEmpty())
        end_characters.pop();
    return true;
}

bool MyTranslator::block(t_Variable &result, QStringList expected_end_characters){
    if (!expected_end_characters.isEmpty())
        end_characters.push(expected_end_characters);

    if (!part(result)) return false;

    t_Variable tmp;

    int s_pos;

    while(is("*",1) || is("/",1)){
        s_pos = word-inputData.begin()+2;

        if (is("*",1)){
            if (!next(2)) return false;
            if (!part(tmp)) return false;

            if (result.type == "num"){
                if (tmp.type == "num"){
                    result.value.setValue(result.value.toFloat()*tmp.value.toFloat());
                }else if (tmp.type == "vector"){
                    float num = result.value.toFloat();
                    result.value.setValue(tmp.value.toPointF()*num);
                    result.type = "vector";
                }else if (tmp.type == "point"){
                    float num = result.value.toFloat();
                    result.value.setValue(tmp.value.toPointF()*num);
                    result.type = "point";
                }else if (tmp.type == "figure"){
                    t_Figure tmp_fig = tmp.value.value<t_Figure>();
                    float num = result.value.toFloat();
                    auto o = getCenter(tmp_fig);

                    for (auto &p : tmp_fig.data){
                        p = num * (p-o)+o;
                    }
                    result.value.setValue(tmp_fig);
                    result.type = "figure";
                }
            }else if (result.type == "vector"){
                if (tmp.type == "num"){
                    float num = tmp.value.toFloat();
                    QPointF res_vec = result.value.toPointF();
                    result.value.setValue(res_vec*num);
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
                    t_Figure tmp_fig = tmp.value.value<t_Figure>();
                    QPointF res_vec = result.value.toPointF();
                    auto o = getCenter(tmp_fig);

                    for (auto &p : tmp_fig.data){
                        p.setX(res_vec.x() * (p.x()-o.x())+o.x());
                        p.setY(res_vec.y() * (p.y()-o.y())+o.y());
                    }

                    result.value.setValue(tmp_fig);
                    result.type = "figure";
                }
            }else if (result.type == "point"){
                if (tmp.type == "num"){
                    float num = tmp.value.toFloat();
                    QPointF res_point = result.value.value<QPointF>();
                    result.value.setValue(res_point*num);
                }else if (tmp.type == "vector"){
                    QPointF tmp_vec = tmp.value.toPointF();
                    QPointF res_vec = result.value.toPointF();
                    result.value.setValue(QPointF(res_vec.x() * tmp_vec.x(), res_vec.y() * tmp_vec.y()));
                }else{
                    throwError("нельзя перемножать '"+ result.type +"' с '" + tmp.type + "'\nожидаемые типы объектов:\nvector, num",s_pos);
                    return false;
                }
            }else if (result.type == "figure"){
                if (tmp.type == "num"){
                    t_Figure res_fig = result.value.value<t_Figure>();
                    float num = tmp.value.toFloat();
                    auto o = getCenter(res_fig);

                    for (auto &p : res_fig.data){
                        p = num * (p-o)+o;
                    }
                    result.value.setValue(res_fig);
                }else if (tmp.type == "vector"){
                    t_Figure res_fig = result.value.value<t_Figure>();
                    QPointF tmp_vec = tmp.value.toPointF();
                    auto o = getCenter(res_fig);

                    for (auto &p : res_fig.data){
                        p.setX(tmp_vec.x() * (p.x()-o.x())+o.x());
                        p.setY(tmp_vec.y() * (p.y()-o.y())+o.y());
                    }

                    result.value.setValue(res_fig);
                }else{
                    throwError("нельзя перемножать '"+ result.type +"' с '" + tmp.type + "'\nожидаемые типы объектов:\nvector, num",s_pos);
                    return false;
                }
            }
        }else if (is("/",1)){
            if (!next(2)) return false;
            if (!part(tmp)) return false;

            if (result.type == "num"){
                if (tmp.type == "num"){
                    float num = tmp.value.toFloat();
                    if (num == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }
                    result.value.setValue(result.value.toFloat()/num);
                }else if (tmp.type == "vector"){
                    float num = result.value.toFloat();
                    QPointF tmp_vec = tmp.value.toPointF();

                    if (tmp_vec.x() == 0 || tmp_vec.y() == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    result.value.setValue(QPointF(num / tmp_vec.x(), num / tmp_vec.y()));
                    result.type = "vector";
                }else if (tmp.type == "point"){
                    float num = result.value.toFloat();
                    QPointF tmp_point = tmp.value.toPointF();

                    if (tmp_point.x() == 0 || tmp_point.y() == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    result.value.setValue(QPointF(num / tmp_point.x(), num / tmp_point.y()));
                    result.type = "point";
                }else{
                    throwError("нельзя делить '"+ result.type +"' на '" + tmp.type + "'\nожидаемые типы объектов:\nvector, num, point",s_pos);
                    return false;
                }
            }else if (result.type == "vector"){
                if (tmp.type == "num"){
                    float num = tmp.value.toFloat();

                    if (num == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    QPointF res_vec = result.value.toPointF();
                    result.value.setValue(res_vec/num);
                }else if (tmp.type == "vector"){
                    QPointF tmp_vec = tmp.value.toPointF();

                    if (tmp_vec.x() == 0 || tmp_vec.y() == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    QPointF res_vec = result.value.toPointF();
                    result.value.setValue(QPointF(res_vec.x() / tmp_vec.x(), res_vec.y() / tmp_vec.y()));
                }else{
                    throwError("нельзя делить '"+ result.type +"' на '" + tmp.type + "'\nожидаемые типы объектов:\nvector, num",s_pos);
                    return false;
                }
            }else if (result.type == "point"){
                if (tmp.type == "num"){
                    float num = tmp.value.toFloat();

                    if (num == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    QPointF res_point = result.value.toPointF();
                    result.value.setValue(res_point/num);
                }else if (tmp.type == "vector"){
                    QPointF tmp_vec = tmp.value.toPointF();
                    QPointF res_vec = result.value.toPointF();

                    if (tmp_vec.x() == 0 || tmp_vec.y() == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    result.value.setValue(QPointF(res_vec.x() / tmp_vec.x(), res_vec.y() / tmp_vec.y()));
                }else{
                    throwError("нельзя делить '"+ result.type +"' на '" + tmp.type + "'\nожидаемые типы объектов:\nvector, num",s_pos);
                    return false;
                }
            }else if (result.type == "figure"){
                if (tmp.type == "num"){
                    t_Figure res_fig = result.value.value<t_Figure>();
                    float num = tmp.value.toFloat();

                    if (num == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    auto o = getCenter(res_fig);
                    for (auto &p : res_fig.data){
                        p = (p-o)/num+o;
                    }
                    result.value.setValue(res_fig);
                }else if (tmp.type == "vector"){
                    t_Figure res_fig = result.value.value<t_Figure>();
                    QPointF tmp_vec = tmp.value.toPointF();

                    if (tmp_vec.x() == 0 || tmp_vec.y() == 0){
                        throwError("нельзя делить на 0");
                        return false;
                    }

                    auto o = getCenter(res_fig);
                    for (auto &p : res_fig.data){
                        p.setX((p.x()-o.x())/tmp_vec.x()+o.x());
                        p.setY((p.y()-o.y())/tmp_vec.y()+o.y());
                    }
                    result.value.setValue(res_fig);
                }else{
                    throwError("нельзя делить '"+ result.type +"' на '" + tmp.type + "'\nожидаемые типы объектов:\nvector, num",s_pos);
                    return false;
                }
            }
        }
    }

    if (!expected_end_characters.isEmpty())
        end_characters.pop();

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

    tmp.type = "point";
    if (!rightPart(tmp)) return false;

    t_Figure tmp_fig;
    tmp_fig.data << tmp.value.toPointF();

    while (is(",",1)){
        word++;
        if (!next()){
            throwError("после запятой должен быть обьект типа 'point'");
            return false;
        }

        tmp.type = "point";
        if (!rightPart(tmp, {",","}"})) return false;

        tmp_fig.data << tmp.value.toPointF();
    }

    if (!is("}",1)){
        throwError("продолжите список точек или завершите фигурной скобкой");
        return false;
    }
    word++;

    if (!isFilledFigure(tmp_fig) && tmp_fig.data.size() > 1) tmp_fig.data << tmp_fig.data.first();

    result.value.setValue(tmp_fig);
    result.type = "figure";

    return true;
}

QString MyTranslator::CutWord(QString &str){
    if (str.size() > 10) return (str.left(10) + "...");
    else return str;
}

bool MyTranslator::part(t_Variable &result, QStringList expected_end_characters){
    if (!expected_end_characters.isEmpty())
        end_characters.push(expected_end_characters);

    if (is("(")){
        int s_pos = word - inputData.begin();

        if (!next()){
            if (result.type == "var") throwError("после открывающейся круглой скобоки ожидалось выражение",s_pos);
            else throwError("после открывающейся круглой скобоки ожидалось выражение, результат которого должен соответствовать типу '"+result.type+"'",s_pos);
            return false;
        }

        if (is(")")){
            if (result.type == "var") throwError("внутри круглых скобок ожидалось выражение",s_pos);
            else throwError("внутри круглых скобок ожидалось выражение, результат которого должен соответствовать типу '"+result.type+"'",s_pos);
            return false;
        }

        t_Variable tmp;


        if (!rightPart(tmp, {",",")"})) return false;

        if (result.type == "point"){
            if (tmp.type == "num" && !is(",",1)){
                throwError("после '" + *word +"' ожидалась запятая");
                return false;
            }
        }

        if (!next()) {
            if (result.type == "point") throwError("после '"+CutWord(*word)+"' должна быть запятая");
            else throwError("после '" + CutWord(*word) + "' должна быть закрывающаяся круглая скобка");
            return false;
        }

        if (is(",")) {
            if (!next()){
                throwError("после запятой должно быть число");
                return false;
            }

             QPointF tmp_point;
             tmp_point.setX(tmp.value.toFloat());

             if (!rightPart(tmp, {")"})) return false;

             tmp_point.setY(tmp.value.toFloat());
             result.value.setValue(tmp_point);
             result.type = "point";

             if (!next()) {
                 throwError("после '" + CutWord(*word) + "' должна быть закрывающаяся круглая скобка");
                 return false;
             }
        }else{
            result.value.setValue(tmp.value);
            result.type = tmp.type;
        }

        if (!is(")")){
            throwError("после '" + *(word-1) + "' должна быть закрывающаяся круглая скобка");
            return false;
        }

        if (result.type == "point") return getFloatOnPoint(result);
    }else if (is("{")){
        if (!getFigure(result)) return false;

        return getPointOnFigure(result);
    }else if (is("[")){
        if (!next()){
            throwError("после '"+*word+"' должно быть число");
            return false;
        }

        t_Variable tmp;

        tmp.type = "num";
        if (!rightPart(tmp, {","})) return false;

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

        QStringList echars = {};

        if (result.type == "vector") echars = QStringList({"]"});
        else if (result.type == "color") echars = QStringList({","});
        else echars = QStringList({",","]"});

        if (!rightPart(tmp, echars)) return false;
        tmp_point.setY(tmp.value.toFloat());

        QColor color;

        if (result.type == "vector"){
            if (!is("]",1)){
                throwError("после '" + *word + "' должна быть закрывающая квадратная скобка");
                return false;
            }
        }

        if (is(",",1)) {
            result.type = "color";
            word++;

            if (!next()){
                throwError("после '"+*word+"' должно быть число");
                return false;
            }

            if (!rightPart(tmp, {",","]"})) return false;

            color.setRedF(tmp_point.x());
            color.setGreenF(tmp_point.y());
            color.setBlueF(tmp.value.toFloat());


            if (is(",",1)) {
                word++;

                if (!next()){
                    throwError("после '"+*word+"' должно быть число");
                    return false;
                }

                if (!rightPart(tmp, {"]"})) return false;

                color.setAlphaF(tmp.value.toFloat());
            }else if (is("]",1)){
                color.setAlphaF(0.8);
            }else{
                throwError("после '"+*word+"' должна быть запятая или закрывающая квадратная скобка");
                return false;
            }
        }else if (result.type == "color"){
            throwError("после '"+*word+"' должна быть запятая");
            return false;
        }

        if (!is("]",1)){
            throwError("после " + *word + "должна быть закрывающая квадратная скобка");
            return false;
        }
        word++;

        if (result.type == "color"){
            result.value.setValue(color);

            return getFloatOnColor(result);
        }else{
            result.value.setValue(tmp_point);
            result.type = "vector";

            return getFloatOnPoint(result);
        }
    }else if (is("-")){
        if (getNum(result))
            result.type = "num";
    }else if (getNum(result)){
        result.type = "num";
    }else if (getVariable()){
        result = ObjList[*word];

        if (result.type == "figure"){
            return getPointOnFigure(result);
        }else if (result.type == "point" || result.type == "vector"){
            return getFloatOnPoint(result);
        }
    }else return false;

    if (is("(",1) || is("[",1) || is("{",1)){
        word++;
        throwError("перед '" + *word + "' ожидалось:\n- '+'\n- '-'\n- '*'\n- '/'\n- ';'");
        return false;
    }else if (!(is("(",1) || is("[",1) || is("{",1) || is(")",1) || is("]",1) || is("}",1) || is("-",1) || is("+",1) || is("*",1) || is("/",1) || is(";",1)  || is(",",1))){
        if (next()){
            throwError("перед '" + *word + "' ожидалось:\n- '+'\n- '-'\n- '*'\n- '/'"+"\n- '" + end_characters.top().join("'\n- '")+"'");
            return false;
        }
    }else if (result.type != "color"){
        auto list = DT[result.type];
        if ((((word+1) == inputData.end())?(true):(!(list.contains(*(word+1)) || end_characters.top().contains(*(word+1)))))){
            word++;

            throwError(((word != inputData.end())?("вместо '"+ *(word) + "' "):"")  + "ожидалось:"
                        + (list.empty()?"":("\n- '"+list.join("'\n- '")+"'"))
                        + "\n- '" + end_characters.top().join("'\n- '")+"'");
            return false;
        }
    }

    if (!expected_end_characters.isEmpty())
        end_characters.pop();

    return true;
}

bool MyTranslator::getFloatOnColor(t_Variable &result){
    if (is("Red",1)){
        word++;
        result.type = "num";
        result.value.setValue(result.value.value<QColor>().redF());
    }else if (is("Green",1)){
        word++;
        result.type = "num";
        result.value.setValue(result.value.value<QColor>().greenF());
    }else if (is("Blue",1)){
        word++;
        result.type = "num";
        result.value.setValue(result.value.value<QColor>().blueF());
    }else if (is("Alpha",1)){
        word++;
        result.type = "num";
        result.value.setValue(result.value.value<QColor>().alphaF());
    }

    return true;
}

bool MyTranslator::getFloatOnPoint(t_Variable &result){
    if (is("[",1)){
        word++;

        if (!next()){
            throwError("введите индекс ('num')");
            return false;
        }

        t_Variable tmp;
        tmp.type = "num";
        if (!rightPart(tmp, {"]"})) {
            throwError("внутри квадратных скобок должен быть обьект типа 'num'");
            return false;
        }

        if (!next()){
            throwError("ожидалась закрыващая квадратная скобка");
            return false;
        }

        if (!is("]")){
            throwError("вместо "+CutWord(*word)+" ожидалась закрыващая квадратная скобка");
            return false;
        }

        int id = tmp.value.toInt();

        if (id % 2 == 0){
            result.value.setValue(result.value.toPointF().x());
        }else{
            result.value.setValue(result.value.toPointF().y());
        }

        result.type = "num";
    }

    return true;
}

bool MyTranslator::getPointOnFigure(t_Variable &result){
    if (is("[",1)){
        word++;

        if (!next()){
            throwError("введите индекс ('num')");
            return false;
        }

        t_Variable tmp;
        tmp.type = "num";
        if (!rightPart(tmp, {"]"})) {
            throwError("внутри квадратных скобок должен быть обьект типа 'num'");
            return false;
        }

        if (!next()){
            throwError("ожидалась закрыващая квадратная скобка");
            return false;
        }

        if (!is("]")){
            throwError("вместо "+CutWord(*word)+" ожидалась закрыващая квадратная скобка");
            return false;
        }

        result.type = "point";
        auto list = result.value.value<t_Figure>().data;

        int id = tmp.value.toInt();
        int size = list.size()-1;

        if (size < 0){
            result.value.setValue(QPointF());
            return true;
        }

        if (id<0) id = size+id%size;
        else id %= size;

        result.value.setValue(list.at(id%(list.size()-1)));
    }

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
