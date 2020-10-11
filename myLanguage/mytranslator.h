#ifndef MYTRANSLATOR_H
#define MYTRANSLATOR_H

#include <QObject>
#include <QPoint>
#include <QDebug>
#include <QTextDocumentFragment>
#include <math.h>
#include <limits>
#include <QHash>
#include <QColor>

const double EPS = 0.1;

inline bool between (float a,float b, float c) {
    return std::fmin(a,b) <= c + EPS && c <= std::fmax(a,b) + EPS;
}

enum State{
    pass_A = 0,
    pass_B = 1,
    end = 2
};

struct t_Variable{
    QString type;
    QVariant value;
};

struct Jump{
    int idx = -1;
    bool visited = false;
};

struct t_Figure{
    QList<QPointF> data;
    QMap<int,Jump> jumps;
    QHash<int,int> transition;

    QColor FillColor = QColor(255, 145, 66, 150);

    QColor StrokeColor = QColor(0,0,0,150);
    int StrokeWidth = 2;

    QColor DotColor = QColor(255,0,0,150);
    int DotRadius = 3;
};

struct RowData{
    uint32_t A_idx;
    uint32_t B_idx;
    bool visited = false;
};

Q_DECLARE_METATYPE(t_Figure)

inline uint qHash(const QPointF &p) {
    return qHash(QPair<qreal, qreal> (p.x(), p.y()));
}

class MyTranslator: public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool draw READ getDraw NOTIFY DrawChanged)
public:
    explicit MyTranslator(QObject *parent = nullptr);

    Q_INVOKABLE void read(QString text);

    Q_INVOKABLE int amountOfFigures() const;
    Q_INVOKABLE int amountOfPointsOnFigure(int i) const;

    Q_INVOKABLE int getX(int FigureID, int PointID) const;
    Q_INVOKABLE int getY(int FigureID, int PointID) const;

    Q_INVOKABLE QColor getFillColor(int FigureID) const;
    Q_INVOKABLE QColor getStrokeColor(int FigureID) const;
    Q_INVOKABLE QColor getDotColor(int FigureID) const;
    Q_INVOKABLE int getStrokeWidth(int FigureID) const;
    Q_INVOKABLE int getDotRadius(int FigureID) const;

    Q_INVOKABLE QVariantList getHidenEdges(int FigureID) const;

    void throwError(QString text, int s_pos = -1);

    Q_INVOKABLE bool getDraw() const;
private:
    bool draw;
    QString store;

    QStringList inputData;
    QStringList::iterator word;

    bool next(uint step = 1);
    bool is(QString tag, int offset = 0);
    bool getNum(t_Variable &result);
    bool getPoint(t_Variable &result);
    bool getFigure(t_Variable &result);

    bool operation();

    bool getVariable();
    bool varName();

    bool rightPart(t_Variable &result);
    bool block(t_Variable &result);
    bool part(t_Variable &result);

    QList<t_Figure> Figures;
    QHash<QString, t_Variable> ObjList;

    QString CutWord(QString &str);

    QPointF getCenter(t_Figure &figure);
    bool isFilledFigure(t_Figure &figure) const;
    bool isInside(QPointF point, t_Figure &figure, bool Ignore_borders = false);
    int Intersection(QPointF &a1,QPointF &a2, t_Figure &figure);
    QList<QPointF>  IntersectionList(t_Figure &A, t_Figure &B);
    int getIdxOfMinPerpendicular(QPointF &point, t_Figure &figure, float *dist = nullptr);
    int getIdxOfNearestEdge(QPointF &point, t_Figure &figure);
    QPointF normalizedVector(QPointF v);
    bool cross (QPointF &L11,QPointF &L12, QPointF &L21,QPointF &L22, QPointF *res = nullptr);
    bool simplify(t_Figure &figure);

    bool onSameLine(int i, QHash<int,int> &ranges);
    QPair<int,int> getRange(int i, QHash<int,int> &ranges);

    float Len(QPointF &A, QPointF &B);

    void getHidenEdges(t_Figure &f) const;
    void getFigureInfo(t_Figure &f) const;

    bool getPointOnFigure(t_Variable &result);
    bool getFloatOnPoint(t_Variable &result);
    bool getFloatOnColor(t_Variable &result);
signals:
    void getError(QString text, int pos);
    void DrawChanged();
};

#endif // MYTRANSLATOR_H
