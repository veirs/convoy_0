#ifndef CONVOY_H
#define CONVOY_H
#include <QFile>
#include <QTextStream>
#include <QDebug>

#include <QMainWindow>

#include "ship.h"
#include "valplot.h"

namespace Ui {
class Convoy;
}

class Convoy : public QMainWindow
{
    Q_OBJECT

public:
    explicit Convoy(QWidget *parent = 0);
    ~Convoy();

private:
    Ui::Convoy *ui;
    int DEBUG;
    QFile *m_inputDataFile;
    QList<Ship *> m_shipList;

    int m_shipCount;
    int m_N_minutes;
    VecDoub m_RL;
    VecInt m_histogram;
    VecDoub m_cumPercent;
    VecDoub m_percent;
    VecDoub m_xAxis;

    int m_graphCnt;


    /////////////////// global data structures
    ///   queue of passing ships
    QList<Ship *> m_passingShips;
    QList<Ship *> m_N_convoyList;
    QList<Ship *> m_S_convoyList;

    double m_dBabeam;
    double m_dBabeamSqr;
    int m_dBabeamCnt;
    int m_convoySize;
    int m_convoyCnt;

    valPlot * theGraph;
    void readShipPopulationData();

    void runShips();
    void selectNewShip(VecDoub chances);  // randomly select a ship weighted by chances [0] = Tanker  [1] = Bulker [2] = Container
    bool advanceShips();
    double calculateRL();  // use transmission to calc m_RL at each minute

    void doStats();
    int m_graph;
    void resetScales();


private slots:
    void runModel();
    void rescaleGraph();
    void updateCounts();
    void saveRdata();
    void setNbins(int nBins);
    void setGraph_0(bool flag);
    void setGraph_1(bool flag);
    void chkConvoyPolicy();
    void chkNSPolicy();
};

#endif // CONVOY_H
