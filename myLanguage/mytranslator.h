#ifndef MYTRANSLATOR_H
#define MYTRANSLATOR_H

#include <QObject>
#include <QPoint>
#include <QDebug>
#include <QTextDocumentFragment>
#include <math.h>
#include <limits>

struct t_Variable{
    QString type;
    QVariant value;
};

Q_DECLARE_METATYPE(t_Variable)

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

    void throwError(QString text);

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
    bool getVector(t_Variable &result);

    bool operation();

    bool initVariable();
    bool getVariable();
    bool varName();

    bool rightPart(t_Variable &result);
    bool block(t_Variable &result);
    bool part(t_Variable &result);

    QList<QList<QPointF>> Figures;
    QHash<QString, t_Variable> ObjList;

    QString CutWord(QString &str);

    QPointF getCenter(QList<QPointF> &figure);
    bool isFilledFigure(QList<QPointF> &figure) const;
    bool isInside(QPointF &point, QList<QPointF> &figure);
    int Intersection(QPointF &a1,QPointF &a2, QList<QPointF> &figure, QPointF *p = nullptr);
    int getIdxOfMinPerpendicular(QPointF &point, QList<QPointF> &figure);
    int getIdxOfNearestEdge(QPointF &point, QList<QPointF> &figure);
signals:
    void getError(QString text, int pos);
    void DrawChanged();
};

#endif // MYTRANSLATOR_H
