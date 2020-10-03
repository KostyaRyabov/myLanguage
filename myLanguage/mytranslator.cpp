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

    for (auto &jump : figure.jumps){
        list << jump.idx+1;
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

        t_Variable obj;
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

void MyTranslator::simpify(t_Figure &figure){
    qDebug() << "simplify : " << figure.jumps.keys();

    if (figure.data.size() > 0){
        for (auto it = figure.data.begin()+1; it+1 != figure.data.end();){
            if (figure.jumps.contains(it - figure.data.begin())) {
                it++;
                continue;
            }

            if (std::abs(it->x()-(it+1)->x()) <= 3 && std::abs(it->y()-(it+1)->y()) <= 3){
                it = figure.data.erase(it);
            }else if (std::abs(((it-1)->x()-(it+1)->x())*((it)->y()-(it+1)->y())-((it-1)->y()-(it+1)->y())*((it)->x()-(it+1)->x())) <= EPS){
                if (figure.jumps.contains((it+1) - figure.data.begin())) {
                    it++;
                    continue;
                }

                it = figure.data.erase(it);
            }else it++;
        }
    }
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
    return (figure.data.size() > 1 && figure.data.first() == figure.data.back());
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
        if (figure.jumps.contains(i)) continue;

        if (fig[i] == point) return true;

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
        if (figure.jumps.contains(i)) continue;

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

    uint32_t size_A = Af.size()-1,
            size_B = Bf.size()-1;

    QList<QPointF> list;
    QPointF p,f;

    for (uint32_t i = 0; i < size_A; i++){
        if (A.jumps.contains(i)) continue;

        for (uint32_t j = 0; j < size_B; j++){
            if (B.jumps.contains(j)) continue;

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
    bool iff = isFilledFigure(figure);

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

    qDebug() << "jumps : ";

    for (int i = 0; i < size; i++){
        for (int j = i+1; j < size; j++){
            if (i != j){
                if (fig[i] == fig[j+1]) {
                    if (fig[i+1] == fig[j]) {
                        tmp.idx = i;
                        f.jumps[i] = tmp;

                        tmp.idx = j;
                        f.jumps[j] = tmp;

                        qDebug() << "\t" << i << j;
                    }
                }
            }
        }
    }

    //qDebug() << "   trans : ";

    if (f.jumps.isEmpty()){
        f.transition[0] = size;
        f.transition[size] = 0;

        //qDebug() << "\t" << -1 <<size-1;
        //qDebug() << "\t" << size << 0;
    }else{
        auto jumps = f.jumps;

        jumps[-1];

        auto keys = jumps.keys();
        int k_size = keys.size()-1;

        //qDebug() << keys;

        for (int i = 0; i < k_size; i++){
            f.transition[keys[i+1]] = keys[i]+1;
            f.transition[keys[i]+1] = keys[i+1];

            qDebug() << keys[i+1] << keys[i]+1;
            qDebug() << keys[i]+1 << keys[i+1];
            qDebug() << "\t";
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
            t_Figure tmp_fig = result.value.value<t_Figure>();
            for (auto &p : tmp_fig.data){
                p=-p;
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
                    t_Figure figure;
                    figure.data = {result.value.toPointF(), tmp.value.toPointF()};

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
                    throwError("нельзя преобразовать '" + tmp.type + "' в '"+ result.type +"'");
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
                    throwError("нельзя преобразовать '" + tmp.type + "' в '"+ result.type +"'");
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
                            A = tmp.value.value<t_Figure>(),
                            B = result.value.value<t_Figure>();

                    QList<QPointF> &Af = A.data, &Bf = B.data, &Rf = res.data;

                    if (Af.size() < Bf.size()) std::swap(A,B);

                    QList<QPointF> list = IntersectionList(A,B);
                    QHash<QPointF, RowData> table;

                    uint32_t A_idx = 0, B_idx;

                    qDebug() << "insert points...";

                    for (auto &p : list){
                        A_idx = getIdxOfMinPerpendicular(p, A);

                        qDebug() << "\tA:" << A_idx << p;

                        if (Af[A_idx] != p)
                            Af.insert(A_idx,p);

                        B_idx = getIdxOfMinPerpendicular(p, B);

                        qDebug() << "\tB:" << B_idx << p;

                        if (Bf[B_idx] != p)
                            Bf.insert(B_idx ,p);

                        getFigureInfo(A);
                        getFigureInfo(B);
                    }

                    int    A_size = Af.size()-1,
                            B_size = Bf.size()-1;

                    int k =  B_size;
                    for (; k >= 0; k--){
                        if (!isInside(Bf[k],A,true)) break;
                    }

                    if (k == -1){
                        qDebug() <<  "(4) фигура В полностью внутри А";
                        res = A;
                    }else if (list.size() > 0){
                        qDebug() << "-----";

                        for (int i = 0;i < A_size;i++) if (list.contains(Af[i])) table[Af[i]].A_idx = i;
                        for (int i = 0;i < B_size;i++) if (list.contains(Bf[i])) table[Bf[i]].B_idx = i;

                        RowData pc;

                        int startPos = -1;

                        for (int i = 0; i < A_size; i++) {
                            if (A.jumps.contains(i)) continue;

                            if (!isInside(Af[i],B) && !table.contains(Af[i])) {
                                startPos = i;
                                break;
                            }
                        }

                        if (startPos == -1){
                            qDebug() <<  "(1) фигура А полностью внутри В";
                            res = B;
                        }else{
                            QPointF v;

                            int8_t stepA,stepB;
                            int i,j;

                            qDebug() << " пересекают";

                            do{
                                Rf<<Af[startPos];

                                if (A.jumps.contains(startPos)) A.jumps[startPos].visited = true;

                                v = normalizedVector(Af[startPos+1]-Af[startPos]);

                                stepA = isInside(Af[startPos]+v,B,true)?-1:1;

                                i = startPos+stepA;

                                if (A.transition.contains(startPos)) {
                                    if ((startPos - A.transition[startPos])*stepA > 0){
                                        i = A.transition[startPos]+stepA;
                                        if (A.jumps.contains(i)) A.jumps[i].visited = true;
                                    }
                                }

                                for (;;) {
                                    Rf << Af[i];

                                    if (list.contains(Af[i])){
                                        if (table[Af[i]].visited){
                                            break;
                                        }else{
                                            table[Af[i]].visited = true;

                                            pc = table[Af[i]];

                                            v = normalizedVector(Bf[pc.B_idx + 1]-Af[i]);

                                            stepB = isInside(Af[i]+v,A,true)?-1:1;

                                            j = pc.B_idx+stepB;

                                            if (B.transition.contains(pc.B_idx)) {
                                                if ((pc.B_idx - B.transition[pc.B_idx])*stepB > 0){
                                                    j = B.transition[pc.B_idx]+stepB;
                                                    if (B.jumps.contains(j)) B.jumps[j].visited = true;
                                                }
                                            }

                                            if (B.transition.contains(j)) {
                                                j = B.transition[j];
                                                if (B.jumps.contains(j)) B.jumps[j].visited = true;
                                            }

                                            for (;;) {
                                                Rf << Bf[j];

                                                if (list.contains(Bf[j])){
                                                    table[Bf[j]].visited = true;
                                                    i = table[Bf[j]].A_idx;
                                                    break;
                                                }

                                                j+=stepB;

                                                if (B.jumps.contains(j)) B.jumps[j].visited = true;

                                                if (B.transition.contains(j)){
                                                    j = B.transition[j];
                                                    if (B.jumps.contains(j)) B.jumps[j].visited = true;
                                                }
                                            }
                                        }

                                        if (i == startPos) break;
                                    }

                                    i+=stepA;

                                    if (i == startPos) break;

                                    if (A.jumps.contains(i)) A.jumps[i].visited = true;

                                    if (A.transition.contains(i)){
                                        i = A.transition[i];
                                        if (A.jumps.contains(i)) A.jumps[i].visited = true;
                                        if (i == startPos) break;
                                    }
                                }

                                Rf << Af[i];

                                startPos = -1;
                                for (auto &jump : A.jumps){
                                    if (!jump.visited){
                                        startPos = jump.idx;
                                        break;
                                    }
                                }

                            }while(startPos != -1);

                            Rf << Rf[0];
                        }
                    }else{
                        int i = Af.size()-1;

                        for (; i >= 0; i--){
                            if (!isInside(Af[i],B)) break;
                        }

                        if (i == Af.size()-1){
                            qDebug() <<  "(2) фигуры А и В не имеют пересечений";

                            Rf << Af << Bf;
                            Rf << Rf[0];
                        }else{
                            qDebug() <<  "(3) A внутри B";
                            Rf << Bf;
                        }
                    }

                    getFigureInfo(res);

                    simpify (res);
                    qDebug() << "res = " << Rf;

                    result.value.setValue(res);
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
                    result.value.setValue(res_point - tmp_vec);
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
                    throwError("нельзя преобразовать '" + tmp.type + "' в '"+ result.type +"'");
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
                    // coming soon...
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
                if (tmp.type == "float"){
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
                if (tmp.type == "float"){
                    float num = tmp.value.toFloat();
                    QPointF res_point = result.value.value<QPointF>();
                    result.value.setValue(res_point*num);
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
                    throwError("нельзя перемножать '"+ result.type +"' с '" + tmp.type + "'");
                    return false;
                }
            }
        }else if (is("/",1)){
            if (!next(2)) return false;
            if (!part(tmp)) return false;

            if (result.type == "float"){
                if (tmp.type == "float"){
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
                    throwError("нельзя делить '"+ result.type +"' на '" + tmp.type + "'");
                    return false;
                }
            }else if (result.type == "vector"){
                if (tmp.type == "float"){
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
                    throwError("нельзя делить '"+ result.type +"' на '" + tmp.type + "'");
                    return false;
                }
            }else if (result.type == "point"){
                if (tmp.type == "float"){
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
                    throwError("нельзя перемножать '"+ result.type +"' с '" + tmp.type + "'");
                    return false;
                }
            }else if (result.type == "figure"){
                if (tmp.type == "float"){
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
                    t_Figure res_fig = tmp.value.value<t_Figure>();
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

    t_Figure tmp_fig;
    tmp_fig.data << tmp.value.toPointF();

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

        tmp_fig.data << tmp.value.toPointF();
    }

    if (!is("}",1)){
        throwError("продолжите список точек или завершите фигурной скобкой");
        return false;
    }
    word++;

    getFigureInfo(tmp_fig);
    qDebug() << "get info";
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
            throwError("после '(' должно быть выражение");
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
            result.value.setValue(tmp.value);
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
