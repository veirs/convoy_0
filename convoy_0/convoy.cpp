#include "convoy.h"
#include "ui_convoy.h"
#include <QMessageBox>

Convoy::Convoy(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Convoy)
{
    ui->setupUi(this);
    connect(ui->runModel,SIGNAL(clicked(bool)),this,SLOT(runModel()));
    connect(ui->clearGraph,SIGNAL(clicked(bool)),this,SLOT(rescaleGraph()));
    connect(ui->nBins,SIGNAL(valueChanged(int)),this,SLOT(setNbins(int)));
    connect(ui->graphY_0,SIGNAL(clicked(bool)),this,SLOT(setGraph_0(bool)));
    connect(ui->graphY_1,SIGNAL(clicked(bool)),this,SLOT(setGraph_1(bool)));

    connect(ui->xMax, SIGNAL(valueChanged(double)), this, SLOT(rescaleGraph()));
    connect(ui->xMin, SIGNAL(valueChanged(double)), this, SLOT(rescaleGraph()));
    connect(ui->xMid, SIGNAL(valueChanged(double)), this, SLOT(rescaleGraph()));
    connect(ui->yMax, SIGNAL(valueChanged(double)), this, SLOT(rescaleGraph()));
    connect(ui->yMid, SIGNAL(valueChanged(double)), this, SLOT(rescaleGraph()));
    connect(ui->yMin, SIGNAL(valueChanged(double)), this, SLOT(rescaleGraph()));

    connect(ui->N_ships_0, SIGNAL(valueChanged(int)), this, SLOT(updateCounts()));
    connect(ui->N_ships_1, SIGNAL(valueChanged(int)), this, SLOT(updateCounts()));
    connect(ui->N_ships_2, SIGNAL(valueChanged(int)), this, SLOT(updateCounts()));
    connect(ui->N_ships_3, SIGNAL(valueChanged(int)), this, SLOT(updateCounts()));

    connect(ui->saveRdata, SIGNAL(clicked(bool)),this,SLOT(saveRdata()));
    connect(ui->policy_2, SIGNAL(clicked(bool)),this,SLOT(chkConvoyPolicy()));
    connect(ui->policy_0, SIGNAL(clicked(bool)),this,SLOT(chkNSPolicy()));

    DEBUG = 0;
    theGraph = new valPlot(this,ui->graphicsView);
    m_graph = 1;  // default is cumulative probability for display
    setGraph_1(true);
    rescaleGraph();
    updateCounts();
}

Convoy::~Convoy()
{
    delete ui;
}
void Convoy::runModel(){
    readShipPopulationData();
    // one year's worth of minutes has, say, 9000 ships
    double minBetweenShips = (double)(365*24*60) / (double)ui->N_ships_0->value();
    int minuteIdx;
    minuteIdx = -60*5;  // start index at - 5 hours to let the passingID etc. lists initialize
    m_N_minutes =  60*24*365 * ui->nYears->value();
    m_passingShips.clear();
    m_N_convoyList.clear();
    m_S_convoyList.clear();
    runShips();
    doStats();
    
    
}
bool birthingNewShip(double minutesBetweenShips){
    bool birth = false;
    // use random number to see if a birth, probability = prob
    double ranNum = (double)qrand()/(double)RAND_MAX;
    int randIdx = ranNum * minutesBetweenShips;
    if (randIdx == 1)  // we birth a ship when the random index (0 - min between ships) hits any one selected index
        birth = true;
    return birth;
}
double rand_normal(double mean, double stddev)
{//Box muller method
    static double n2 = 0.0;
    static int n2_cached = 0;
    if (!n2_cached)
    {
        double x, y, r;
        do
        {
            x = 2.0*rand()/RAND_MAX - 1;
            y = 2.0*rand()/RAND_MAX - 1;
            r = x*x + y*y;
        }
        while (r == 0.0 || r > 1.0);
        {
            double d = sqrt(-2.0*log(r)/r);
            double n1 = x*d;
            n2 = y*d;
            double result = n1*stddev + mean;
            n2_cached = 1;
            return result;
        }
    }
    else
    {
        n2_cached = 0;
        return n2*stddev + mean;
    }
}
void Convoy::runShips(){

    m_RL.resize(m_N_minutes);// received dB from all ships in the passing queue
    int minuteIdx = 0;
    m_dBabeam = m_dBabeamSqr = m_dBabeamCnt = 0; m_convoySize = 0; m_convoyCnt = 0;
    VecDoub chances(3);
    int N_ships = ui->N_ships_0->value()+ui->N_ships_1->value()+ui->N_ships_2->value()+ui->N_ships_3->value();
    chances[0]=(double)ui->N_ships_1->value()/(double)N_ships;
    chances[1]=((double)ui->N_ships_1->value()+(double)ui->N_ships_2->value())/(double)N_ships;
    chances[2]=((double)ui->N_ships_1->value()+(double)ui->N_ships_2->value()+(double)ui->N_ships_3->value())/(double)N_ships;

    while (minuteIdx < m_N_minutes){
        if ((ui->N_ships_0->value() > 0 && birthingNewShip((double)(365*24*60)/(double)N_ships))){
           selectNewShip(chances);  // new ship will be randomly selected and pushed onto the m_passingShips list
        }
        /// if convoys are selected and if it is the right time of day, release convoy
        if (ui->policy_2->isChecked()){
            int minInDay = fmod(minuteIdx,24*60);
            if (minInDay == 6*60 && m_N_convoyList.length() > 0){ // 6 am - start one convoy  North Bound
                m_passingShips << m_N_convoyList;
                m_convoyCnt++; m_convoySize += m_N_convoyList.length();
                m_N_convoyList.clear();
            }
            if (minInDay == 1600*60 && m_S_convoyList.length() > 0){ // 6 am - start one convoy  North Bound
                m_passingShips << m_S_convoyList;
                m_convoyCnt++; m_convoySize += m_S_convoyList.length();
                m_S_convoyList.clear();
            }
        }
        m_RL[minuteIdx] = calculateRL();
        bool aBeam;
        aBeam = advanceShips();   // for each ship in m_passingID move the ship using its SOG -- removing and ships that 'leave' the region (+- max range m)
        if (aBeam && minuteIdx >=0){
            m_dBabeamCnt++;
            m_dBabeam += m_RL[minuteIdx];
            m_dBabeamSqr += SQR(m_RL[minuteIdx]);
        }
        minuteIdx++;
    }
}
void copyObject(Ship *newShip, Ship *oldShip){
    newShip->shipID = oldShip->shipID;
    newShip->mmsi = oldShip->mmsi;
    newShip->SL_bb = oldShip->SL_bb;
    newShip->SL_call = oldShip->SL_call;
    newShip->SL_click = oldShip->SL_click;
    newShip->sog = oldShip->sog;
    newShip->cog = oldShip->cog;
    newShip->dateTime = oldShip->dateTime;
    newShip->shipName = oldShip->shipName;
    newShip->shipClass = oldShip->shipClass;
    newShip->xLocation = oldShip->xLocation;
    newShip->yLocation = oldShip->yLocation;
    newShip->length = oldShip->length;


}
void Convoy::selectNewShip(VecDoub chances){  // randomly select a ship weighted by chances [0] = Tanker  [1] = Bulker [2] = Container
    double ranNum = (double)qrand()/(double)RAND_MAX;
    Ship *newShip = new Ship();   ///  ???? do I need a new Ship or is a new one created weh pulled off m_shipList????
    if (ranNum <= chances[0]){
        // select a Tanker
        bool done = false;
        while (!done){
            QString theClass;
            ranNum = (double)m_shipCount *(double)qrand()/(double)RAND_MAX;
            theClass = m_shipList[ranNum]->shipClass;
            if (theClass == "Tanker"){
                copyObject(newShip , m_shipList[ranNum]);
                done = true;
            }
        }
    }
    if (ranNum > chances[0] && ranNum <= chances[1]){
        // select a Bulk carrier
        bool done = false;
        while (!done){
            QString theClass;
            ranNum = (double)m_shipCount *(double)qrand()/(double)RAND_MAX;
            theClass = m_shipList[ranNum]->shipClass;
            if (theClass == "Bulk carrier"){
                copyObject(newShip , m_shipList[ranNum]);
                done = true;
            }
        }
    }
    if (ranNum > chances[1] && ranNum <= chances[2]){
        // select a Container
        bool done = false;
        while (!done){
            QString theClass;
            ranNum = (double)m_shipCount *(double)qrand()/(double)RAND_MAX;
            theClass = m_shipList[ranNum]->shipClass;
            if (theClass == "Container ship"){
                copyObject(newShip , m_shipList[ranNum]);
                done = true;
            }
        }
    }
    if (ranNum > chances[2]){
        // select from entire ship population
        ranNum = (double)m_shipCount *(double)qrand()/(double)RAND_MAX;
        copyObject(newShip , m_shipList[ranNum]);
    }
    ////////////   now position the new ship either to the North or the South  50:50
    ranNum = (double)qrand()/(double)RAND_MAX;
    bool dirN = true;
    if (ranNum <= 0.5){
        // north bound
        newShip->xLocation = ui->laneRange_0->value();  newShip->yLocation = -ui->maxRange->value(); newShip->cog = 0;
    } else {
        //south bound
        dirN = false;
        newShip->xLocation = ui->laneRange_1->value();  newShip->yLocation = ui->maxRange->value(); newShip->cog = 180;
    }
    if (ui->policy_1->isChecked()){  // this is speed limit policy
        if (newShip->sog > ui->speedLimit->value()){
            double deltaSpeed = newShip->sog - ui->speedLimit->value();
            newShip->sog = ui->speedLimit->value();
            newShip->SL_bb = newShip->SL_bb - deltaSpeed * ui->dBperKt->value();
        }
    }
    if (!ui->policy_2->isChecked())
        m_passingShips << newShip;
    else {  // here we have to hold for convoy the longer ships
        if (newShip->length >= ui->minShipLength->value()){
            if (dirN){
                int convoyCnt = m_N_convoyList.length(); // North bound so start from more and more southerly distances
                newShip->yLocation = newShip->yLocation - convoyCnt * ui->shipSeparation->value();

                m_N_convoyList << newShip;
            } else {
                int convoyCnt = m_S_convoyList.length();  // South bound so start at more and more north distances
                newShip->yLocation = newShip->yLocation + convoyCnt * ui->shipSeparation->value();
                m_S_convoyList << newShip;
            }
        } else
            m_passingShips << newShip;  // shorter ships just continue on there way
    }
}
bool Convoy::advanceShips(){
    bool aBeam = false;
    double knotsToMetersPerSec =0.514;  // multiply by to convert NM/hr to meters/sec
    int N = m_passingShips.length();
    for (int i = N-1; i>-1;i--){
        double dir = -1;  // -1 = going south
        if (m_passingShips[i]->cog == 0)
            dir = 1;  // going north
        if ((dir == 1 && m_passingShips[i]->yLocation < 0 && m_passingShips[i]->yLocation + knotsToMetersPerSec * m_passingShips[i]->sog*60.0 > 0) ||
                (dir == -1 && m_passingShips[i]->yLocation > 0 && m_passingShips[i]->yLocation - knotsToMetersPerSec * m_passingShips[i]->sog*60.0 < 0))
        {
            aBeam = true;
        }

        m_passingShips[i]->yLocation += dir * knotsToMetersPerSec * m_passingShips[i]->sog*60.0;  // advance ship by one minute of time at sog

        if ((dir == 1 && m_passingShips[i]->yLocation > ui->maxRange->value()) ||
                (dir == -1 && m_passingShips[i]->yLocation < -ui->maxRange->value())){
            m_passingShips.removeAt(i);
        }
    }
    return aBeam;
}
double Convoy::calculateRL(){
   // run over m_passingShips and calculate received level at this minute
   double RL = 0; double SL; double range; double totPwr = 0; double dBrl;
   for (int i=0;i<m_passingShips.length();i++){
        range = sqrt(SQR(m_passingShips[i]->xLocation)+SQR(m_passingShips[i]->yLocation));
        SL = m_passingShips[i]->SL_bb;

        if (range < ui->rangeCylindrical->value())
            RL = SL - ui->nearSpreading->value() * log10(range);
        else {
            RL = SL - ui->nearSpreading->value() * log10(ui->rangeCylindrical->value()) - 10.0*log10(range/ui->rangeCylindrical->value());
        }
        totPwr += pow(10.0,(RL/10.0));
   }
   double pwrBack = pow(10,rand_normal(ui->dbBackground->value(),ui->dbBackgroundSigma->value())/10.0);
   dBrl = 10*log10(pwrBack + totPwr);
   ////qDebug()<<"dBrl="<<dBrl<<pwrBack<<totPwr;
}
void Convoy::doStats(){
    if (m_convoyCnt > 0){
        m_convoySize /= m_convoyCnt;
    } else
        m_convoySize = 0;
    ui->convoySize->setValue(m_convoySize);
    m_dBabeam /= m_dBabeamCnt;  // calculate mean
    double stdev = sqrt(m_dBabeamSqr/m_dBabeamCnt - SQR(m_dBabeam));
    ui->dB_shipMean->setValue(m_dBabeam);
    ui->dB_shipStDev->setValue(stdev);
if (DEBUG == 1) qDebug()<<"in stats"<<m_dBabeam<<m_dBabeamCnt<<stdev;
    // build histogram of selected output variable
    double RL_min = 9e9;
    double RL_max = -9e9;

    int nBins = ui->nBins->value();
    int iBin;
    m_histogram.resize(nBins);
    m_cumPercent.resize(nBins);
    m_percent.resize(nBins);
    m_xAxis.resize(nBins);
    for (int i=0;i<nBins;i++){
        m_histogram[i] = m_cumPercent[i] = 0;
    }
    for (int i=0; i<m_RL.size();i++){
        RL_min = fmin(RL_min,m_RL[i]);
        RL_max = fmax(RL_max,m_RL[i]);
    }
    for (int i=0; i<m_RL.size();i++){
        iBin= (double)nBins * (m_RL[i] - RL_min)/(RL_max - RL_min);
        if (DEBUG == 1 && i<300)
            qDebug()<<iBin<<m_RL[i]<<RL_min<<RL_max;
         m_histogram[iBin]++;
    }


    for (int i=0;i<nBins;i++){
        m_percent[i] = 100.0*(double)m_histogram[i]/(double)m_N_minutes;
if (DEBUG == 1 && i<300)
            qDebug()<<"hist counts"<<i<<m_histogram[i]<<m_percent[i];
        if (i == 0){
            m_cumPercent[i] = m_percent[i];
        } else {
            m_cumPercent[i] = m_cumPercent[i-1] + m_percent[i];
        }
        m_xAxis[i] = RL_min + (RL_max - RL_min) * (double)i/(double)nBins;
        if (DEBUG == 1)
            qDebug()<<"Print xAxis, %, cumPercent,count"<<i<<m_xAxis[i]<<m_percent[i]<<m_cumPercent[i]<<m_histogram[i];

        if (m_graph == 0){
            theGraph->plotPoint(m_xAxis[i],m_percent[i],m_graphCnt*10,0,60);  //last three parameters select plot color
        }
        if (m_graph == 1)
            theGraph->plotPoint(m_xAxis[i],m_cumPercent[i],m_graphCnt*10,0,60);

    }
    m_graphCnt++;

}
void Convoy::readShipPopulationData(){
    m_inputDataFile = new QFile(ui->inputFile->text());
    if (!m_inputDataFile->open(QIODevice::ReadOnly | QIODevice::Text)){
        QMessageBox msgBox;
        msgBox.setText("Cannon open file"+ui->inputFile->text());
        msgBox.exec();
        return;
    }
    m_shipList.clear();
    QString header;
    header = m_inputDataFile->readLine();
    if (DEBUG == 1) qDebug()<<"Ship source level header line is:"<<header;
    QStringList items;
    QStringList items2;
    m_shipCount = 0;
    QString fileLine = m_inputDataFile->readLine();  // this first line is a header line

    while (!m_inputDataFile->atEnd() && m_shipCount<3000){

        fileLine = m_inputDataFile->readLine();
        items = fileLine.split(",");
        if (items[4].toDouble() > 90){  // ignore any really weird SL's
            Ship *thisShip = new Ship();
            thisShip->mmsi = items[1].toInt();
            items2 = items[2].split("\"");
            thisShip->shipName = items2[1];
            items2 = items[3].split("\"");
            if (items2.length() == 3)
                thisShip->shipClass = items2[1];
            else
                thisShip->shipClass = items[3];
            thisShip->SL_bb = items[7].toDouble();
thisShip->SL_call = thisShip->SL_click = 0;
            thisShip->sog = items[5].toDouble();
            thisShip->length = items[4].toDouble();
            thisShip->cog = -99;
            thisShip->xLocation = -99;
            thisShip->yLocation = -99;

            m_shipCount++;
            m_shipList << thisShip;
        }
    }
    if (DEBUG == 1) qDebug()<<"number of records read ="<<m_shipCount;
}
void Convoy::saveRdata(){
    QFile *outputFile = new QFile(ui->outputFileName->text());
    if (!outputFile->open(QIODevice::WriteOnly | QIODevice::Text))
        return;

    QTextStream out(outputFile);
    out << "dB\tCount\tPercent\tCumulativePercent\n";
    int nBins = m_histogram.size();
    for (int i=0;i<nBins;i++){
//        qDebug()<<m_dB[i]<<"\t"<<m_histogram[i]<<m_percent[i]<<"\t"<<m_cumPercent[i];
        out <<m_xAxis[i]<<"\t"<<m_histogram[i]<<"\t"<<m_percent[i]<<"\t"<<m_cumPercent[i]<<"\n";
    }

    outputFile->close();
}
void Convoy::setNbins(int nBins){
     m_histogram.resize(nBins);
     m_cumPercent.resize(nBins);
     m_percent.resize(nBins);
     m_xAxis.resize(nBins);
}
//////////////////////////////   graphics items
void Convoy::rescaleGraph(){
    ui->xMid->setValue((ui->xMin->value() + ui->xMax->value())/2.0);
    ui->yMid->setValue((ui->yMin->value() + ui->yMax->value())/2.0);

    theGraph->initialize(ui->graphicsView->width(), ui->graphicsView->height(),
                        ui->xMin->value(), ui->xMax->value(),
                        ui->yMin->value(), ui->yMax->value(),
                        0, ui->graphicsView->height(),             //origin at lower left
                        0,1);
    m_graphCnt = 0;
}
void Convoy::setGraph_0(bool flag){
    if (flag){
        m_graph = 0;
        m_graphCnt = 0;
        resetScales();
    }
}
void Convoy::setGraph_1(bool flag){
    if (flag){
        m_graph = 1;
        m_graphCnt = 0;
        resetScales();
    }
}
void Convoy::resetScales(){
    if (m_graph == 0){  //these are probabilites in PERCENT
        ui->yMax->setValue(10);
            }
    if (m_graph == 1){ // these are cumulative probability in PERCENT
        ui->yMax->setValue(100);

    }
    rescaleGraph();
}
void Convoy::updateCounts(){
    ui->shipsPerDay->setValue((ui->N_ships_0->value()+ui->N_ships_1->value()+ui->N_ships_2->value()+ui->N_ships_3->value())/365);
}
void Convoy::chkConvoyPolicy(){
    if (ui->policy_2->isChecked()){
        ui->policy_0->setChecked(false);
        ui->policy_1->setChecked(true);
    } else {
        ui->policy_0->setChecked(true);
    }
}
void Convoy::chkNSPolicy(){
    if (ui->policy_0->isChecked()){
        ui->policy_2->setChecked(false);
    }
}
