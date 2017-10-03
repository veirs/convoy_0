#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QColor>
#include <QDebug>
#include <QPainter>
#include <QPaintEvent>
#include <QRect>
#include <QFileDialog>
#include <QDir>

#include "valplot.h"
////////////////////////////////////////////GRAPHICS
QColor pallette(double data, double data_min, double data_max){

    int red=0; int green=0; int blue=0;
    double frac;
    if (data < (data_max + data_min)/2){
        frac = 2.0*(data-data_min)/(data_max - data_min);
        green = 255 * frac; blue=255*(1-frac);
    } else {
        frac = (2.0*data - (data_max + data_min))/(data_max - data_min);
        green = 255*(1-frac);
        red = 255*frac;
    }
//if (data < 1e-4){
//    qDebug()<<"????";
//}
//    qDebug()<<"pallette data"<<data<<"RGB"<<red<<green<<blue;
    ;
    if (red<0) red=0; if (red>255) red = 255;
    if (green<0) green=0; if (green>255) green = 255;
    if (blue<0) blue=0; if (blue>255) blue = 255;
    if (data<data_min){red=green=blue=0;}
    if (data>data_max){red=green=blue=255;}
    return QColor(red,green,blue);
}



valPlot::valPlot(QWidget *parent,QGraphicsView * thisView) :
    QWidget(parent),
    m_view(thisView)
{
    DEBUG = 0;
    m_pixmapItem = new QGraphicsPixmapItem();
}
valPlot::~valPlot(){
     ////////////////??????????????????????????????????????????????????????????????????
}
void valPlot::resizeEvent(QResizeEvent *evt){
    m_view->fitInView(m_pixmapItem);
    QWidget::resizeEvent(evt);
}



//initialize(int width_px, int height_px, double m_x_data_min, double m_x_data_max, double m_y_data_min, double m_y_data_max,
//  int O_x_px, int O_y_px, double min_z, double max_z);
void valPlot::initialize(int width_px, int height_px,
                         double x_data_min, double x_data_max,
                         double y_data_min, double y_data_max,
                         int O_x_px, int O_y_px,
                         double min_z, double max_z){
    m_width_px = width_px; m_height_px = height_px;
    m_x_data_min = x_data_min; m_x_data_max = x_data_max;
    m_y_data_min = y_data_min; m_y_data_max = y_data_max;
    m_O_x_px = O_x_px; m_O_y_px = O_y_px;
    m_min_z = min_z; m_max_z = max_z;
    m_xPlotPtr = 0;

    m_scene = new QGraphicsScene(0,0,m_width_px,m_height_px);
    m_scene->setBackgroundBrush(Qt::black);

    m_image = new QImage(m_width_px,m_height_px,QImage::Format_RGB32);
    m_image->fill(Qt::lightGray);
 ///  m_pixmapItem = new QGraphicsPixmapItem();
    QPixmap pixmap = QPixmap::fromImage(*m_image,0);

    m_pixmapItem->setPixmap(pixmap);
    m_scene->addItem(m_pixmapItem);
    m_view->setScene(m_scene);
}

void valPlot::plotPoint(double x, double y, double z, double zmin, double zmax){
    int ix,iy;
    ix = m_O_x_px + (x-m_x_data_min)*(double)(m_width_px-m_O_x_px)/(double)(m_x_data_max-m_x_data_min);
    iy = m_O_y_px - (y-m_y_data_min)*((double)m_height_px)/(double)(m_y_data_max-m_y_data_min);
    if (ix<0 || iy < 0)
        return;
 //   m_image->setPixel(ix,iy,clr.rgb());
    QPixmap pixmap = QPixmap::fromImage(*m_image,0);
    QPainter p(&pixmap);
    p.setPen(Qt::white);
 //   p.drawRect(QRect(10,10,30,30));

//    for (int i=0;i<ix;i+=10)
//        for (int j=0;j<iy;j+=10){
//            p.fillRect(QRect(i,j,4,4),Qt::green);
//        }
//    ix=550; iy=150;
//    qDebug()<<ix<<iy;
    QColor clr = pallette(z,zmin,zmax);
    p.fillRect(QRect(ix,iy,4,3),clr);
    p.end();
    QImage img = pixmap.toImage();
    m_image = new QImage(img);
    m_pixmapItem->setPixmap(pixmap);
    return;
}
void valPlot::plotPoint_2(double x, double y, QColor clr){
    int iy;
    int widthBox = 8;
    double localT = fmod(x, m_x_data_max)/m_x_data_max;    // this modulus return fraction of time axis for this time slice
    int ix = m_O_x_px +  localT * m_width_px;
    iy = m_O_y_px - (y-m_y_data_min)*((double)m_height_px)/(double)(m_y_data_max-m_y_data_min);
 //   m_image->setPixel(ix,iy,clr.rgb());
    QPixmap pixmap = QPixmap::fromImage(*m_image,0);
    QPainter p(&pixmap);
    p.setPen(Qt::white);

    p.fillRect(QRect(ix,iy,widthBox,3),clr);
    p.end();
    QImage img = pixmap.toImage();
    m_image = new QImage(img);
    m_pixmapItem->setPixmap(pixmap);
    return;
}
void valPlot::plotSlice(VecDoub freqs, VecDoub yslice, double zMin, double zMax, double widthFrac){
    QPixmap pixmap = QPixmap::fromImage(*m_image,0);
    QPainter p(&pixmap);
    int iStart=1; int iStop;
    if (DEBUG == 1) qDebug()<<"plotSlice  start freq="<<freqs[iStart]*1000 <<"y min and max"<< m_y_data_min<<m_y_data_max;
    while (freqs[iStart]*1000 < m_y_data_min){
        if (DEBUG == 0)qDebug()<<"Too low freq"<<freqs[iStart] << m_y_data_min<<iStart;
        iStart++;
    }
    iStop = iStart;
    while (freqs[iStop]*1000 < m_y_data_max && iStop < yslice.size()-1){
        if (DEBUG == 1)qDebug()<<"freqs"<<freqs[iStop]*1000 << m_y_data_max<<iStop<<yslice.size();
        iStop++;
    }
    int widthBox = widthFrac*m_width_px + 1;
    double localT = fmod(yslice[0], m_x_data_max)/m_x_data_max;    // this modulus return fraction of time axis for this time slice

    int ix = m_O_x_px +  localT * m_width_px;
    if (DEBUG == 1) qDebug()<<"localT"<<localT<<ix<<"ySlice[0]"<<yslice[0];
    int htBox = m_height_px/(iStop - iStart) + 3;
   qDebug()<<"valPlot::localT"<<localT<<"yslice[0]"<<yslice[0]<<"widthBox"<<widthBox<<"m_O_x_px"<<m_O_x_px<<"ix"<<ix;

//DEBUG=1;
    if (DEBUG == 1) qDebug()<<"----------------valPlot:plotSlice at"<<m_O_x_px<<m_O_y_px<<yslice[0]<<"ix="<<ix<<htBox<<widthBox<<m_x_data_max;

    int iy;
    for (int i=iStart;i<iStop;i++){
        iy = m_O_y_px - i*htBox - htBox;
        iy = m_O_y_px - m_height_px*(freqs[i]-freqs[iStart])/(freqs[iStop]-freqs[iStart]);
        QColor clr = pallette(yslice[i],zMin,zMax);
        double fscale = 2.0*(double)(i-iStart)/(double)(iStop-iStart);
        int ht2 = htBox * fscale;
        p.fillRect(QRect(ix,iy,widthBox,ht2),clr);
        if (DEBUG == 0 && i<iStart+10)  qDebug()<<i<<"x y"<<ix<<iy<<"color="<<clr<<yslice[i]<<zMin<<zMax;
    }
//DEBUG=0;
    p.end();
    QImage img = pixmap.toImage();
    delete(m_image);
    m_image = new QImage(img);
    m_pixmapItem->setPixmap(pixmap);
}
