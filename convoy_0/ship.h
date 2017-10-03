#ifndef SHIP_H
#define SHIP_H

#include <QObject>

class Ship : public QObject
{
    Q_OBJECT
public:
    explicit Ship(QObject *parent = nullptr);

    int shipID;
    int mmsi;
    double length;
    double SL_bb;
    double SL_call;
    double SL_click;
    double sog;
    double cog;
    QString dateTime;
    QString shipName;
    QString shipClass;
    double xLocation;
    double yLocation;

signals:

public slots:
};

#endif // SHIP_H
