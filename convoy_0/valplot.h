#ifndef VALPLOT_H
#define VALPLOT_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>

#include "nr3.h"

class valPlot : public QWidget
{
    Q_OBJECT
public:
    explicit valPlot(QWidget *parent = 0, QGraphicsView *thisView = NULL);
    ~valPlot();
    void plotIt();
    double m_x_data_max;
signals:

public slots:

private:
    QGraphicsView * m_view;
    QGraphicsPixmapItem * m_pixmapItem;
    QGraphicsScene *m_scene;
    QImage *m_image;
    int m_width_px;
    int m_height_px;
    double m_x_data_min;
    double m_y_data_min;

    double m_y_data_max;
    int m_O_x_px;
    int m_O_y_px;
    double m_min_z;
    double m_max_z;

    int DEBUG;
    int m_xPlotPtr;
protected:

public:
    virtual void resizeEvent(QResizeEvent *);
    void initialize(int width_px, int height_px,
                    double x_data_min, double x_data_max,
                    double y_data_min, double y_data_max,
                    int O_x_px, int O_y_px,
                    double min_z, double max_z);
    void plotPoint(double x, double y, double z, double zmin, double zmax);
    void plotPoint_2(double x, double y, QColor clr);
    void plotSlice(VecDoub freqs, VecDoub yslice, double zMin, double zMax, double widthFrac);
};

#endif // VALPLOT_H
