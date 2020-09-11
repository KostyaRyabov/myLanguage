#ifndef MYTRANSLATOR_H
#define MYTRANSLATOR_H

#include <QObject>
#include <QPoint>
#include <QDebug>

struct t_Variable{
    QString type;
    QVariant value;
};

class MyTranslator: public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString state READ getState NOTIFY StateChanged)
public:
    explicit MyTranslator(QObject *parent = nullptr);

    //QHash<int, QByteArray> roleNames() const;

    Q_INVOKABLE int amountOfFigures() const;
    Q_INVOKABLE int amountOfPointsOnFigure(int i) const;

    Q_INVOKABLE int getX(int FigureID, int PointID) const;
    Q_INVOKABLE int getY(int FigureID, int PointID) const;

    Q_INVOKABLE void read(QString text);

    Q_INVOKABLE bool isError() const;
    Q_INVOKABLE QString getState() const;
    void throwErrow(QString text);
    void clearState();
private:
    QString state;

    QStringList inputData;     // split input text into parts
    QStringList::iterator word;

    bool next(uint step = 1);
    bool is(QString tag, int offset = 0);
    bool isNum(t_Variable &result);

    bool operation();

    bool initVariable();
    bool getVariable();
    bool varName();

    bool rightPart(t_Variable &result);
    bool block(t_Variable &result);
    bool part(t_Variable &result);

    QList<QList<QPoint>> Figures;
    QMap<QString, t_Variable> ObjList;

signals:
    Q_INVOKABLE void StateChanged();
};

#endif // MYTRANSLATOR_H
