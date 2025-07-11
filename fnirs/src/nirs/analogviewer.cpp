#include "analogviewer.h"
#include <QTime>
#include <QVBoxLayout>
#include <QtDebug>
//#include "analogfs.h"

AnalogViewer::AnalogViewer(double _samplingRate, HardwareSettings* hardware_settings_ptr) : m_hardwareSettingsPtr(hardware_settings_ptr)
{

    // QFile file(":/icons/img/stylesheet.qss");
    // if(!file.open(QFile::ReadOnly))
    //     qDebug()<<"could not open stylesheet file";
    // QString styleSheet = QLatin1String(file.readAll());
    // this->setStyleSheet(styleSheet);

    // this->setWindowIcon(QIcon(":/icons/img/wave-icon.png"));
    samplingRate = _samplingRate;
    //double ai_fs = samplingRate;

    //Set the plots and checkboxes we want

    optionlayout=new QVBoxLayout;
    plotlayout=new QVBoxLayout;
    mainLayout = new QHBoxLayout;
    customPlot = new QCustomPlot[m_hardwareSettingsPtr->GetNAI()];
    isCustomPlotEnabled = new bool[m_hardwareSettingsPtr->GetNAI()];

    for (unsigned int i = 0; i < m_hardwareSettingsPtr->GetNAI(); i++) {
        m_theAIName.append(m_hardwareSettingsPtr->GetAI(i).name);
    }

    QGroupBox *horizontalGroupBoxCheckbox = new QGroupBox();
    QVBoxLayout *layoutCheckbox = new QVBoxLayout;
    checkBoxes = new QCheckBox[m_hardwareSettingsPtr->GetNAI()];
    for (int i = 0; i < m_hardwareSettingsPtr->GetNAI(); ++i) {
        checkBoxes[i].setText(tr("AI %1").arg(i + 1));
        checkBoxes[i].setStyleSheet("font-size:10px;");
        layoutCheckbox->addWidget(&checkBoxes[i]);
        //Set the plots and checkboxes we want
        isCustomPlotEnabled[i] = false;
        checkBoxes[i].setChecked(false);
    }
    QSpacerItem* spV = new QSpacerItem(40, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
    layoutCheckbox->addItem(spV);
    horizontalGroupBoxCheckbox->setLayout(layoutCheckbox);

    layoutCheckbox->setContentsMargins(2,2,2,2);
    layoutCheckbox->setSpacing(5);

    //Add a group of buttons for trigger settings
    QGroupBox *horizontalGroupBoxTrigger = new QGroupBox();
    QHBoxLayout *layoutTrigger = new QHBoxLayout;
    QLabel* lbT = new QLabel("Acquisition mode");
    layoutTrigger->addWidget(lbT);
    cbAcqMode = new QComboBox(); cbAcqMode->addItem("Continuous"); cbAcqMode->addItem("Triggered"); cbAcqMode->setCurrentIndex(0);
    cbAcqMode->setMinimumHeight(22);
    layoutTrigger->addWidget(cbAcqMode);
    QLabel* lbT2 = new QLabel("Source");
    layoutTrigger->addWidget(lbT2);
    cbSource = new QComboBox(); cbSource->setCurrentIndex(0);
    cbSource->setMinimumHeight(22);
    layoutTrigger->addWidget(cbSource);
    QLabel* lbT3 = new QLabel("Level (V)");
    layoutTrigger->addWidget(lbT3);
    leLevel = new QLineEdit("1");
    leLevel->setMaximumWidth(50);
    layoutTrigger->addWidget(leLevel);
    QLabel* lbT4 = new QLabel("Edge");
    layoutTrigger->addWidget(lbT4);
    cbEdge = new QComboBox(); cbEdge->addItem("Rising"); cbEdge->addItem("Falling"); cbEdge->addItem("Both"); cbEdge->setCurrentIndex(0);
    cbEdge->setMinimumHeight(22);
    layoutTrigger->addWidget(cbEdge);
    QSpacerItem* sp = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    m_labelInfo = new QLabel();
    m_labelInfo->setText("Click to pause");
    layoutTrigger->addItem(sp);

    layoutTrigger->setContentsMargins(2,2,2,2);

    horizontalGroupBoxTrigger->setTitle("Trigger options");
    horizontalGroupBoxTrigger->setLayout(layoutTrigger);

    connect(cbAcqMode,SIGNAL(activated(int)),this,SLOT(CheckTriggerSettings()));
    connect(cbSource,SIGNAL(activated(int)),this,SLOT(CheckTriggerSettings()));
    connect(leLevel,SIGNAL(editingFinished()),this,SLOT(CheckTriggerSettings()));
    connect(cbEdge,SIGNAL(activated(int)),this,SLOT(CheckTriggerSettings()));

    //Add options
    QGroupBox *horizontalGroupBoxOptions = new QGroupBox();
    QHBoxLayout *layoutOptions = new QHBoxLayout;
    QCheckBox* cbO = new QCheckBox();
    layoutOptions->addWidget(cbO);
    horizontalGroupBoxOptions->setTitle("Options");
    horizontalGroupBoxOptions->setLayout(layoutOptions);

    //connect(checkBoxes[0], SIGNAL(stateChanged(int)), this,  [this]{modifyPlot(1); });

    //removed status bar by christophe on 2021-08-16
    //connect(action1,  &QAction::triggered, this, [this]{ onStepIncreased(1); });
    /*p_status_bar_left = new QLabel();
    p_status_bar_left->setText("Use the mouse wheel to zoom.");
    p_status_bar_middle = new QLabel;
    p_status_bar_right = new QLabel;
    p_status_bar = new QStatusBar();
    p_status_bar_middle->setAlignment(Qt::AlignHCenter);
    p_status_bar_right->setAlignment(Qt::AlignRight);
    p_status_bar->addWidget(p_status_bar_left,1);
    p_status_bar->addWidget(p_status_bar_middle,1);
    p_status_bar->addWidget(p_status_bar_right,1);*/

    QSize minSize = QSize(100,40);
    for (int i = 0; i < m_hardwareSettingsPtr->GetNAI(); ++i) {
        plotlayout->addWidget(&customPlot[i]);
        customPlot[i].setMinimumSize(minSize);
        customPlot[i].setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
    }
    plotlayout->addWidget(horizontalGroupBoxTrigger);
    optionlayout->addWidget(horizontalGroupBoxCheckbox);
    optionlayout->addWidget(m_labelInfo);
    //optionlayout->addWidget(horizontalGroupBoxTrigger);

    mainLayout->addLayout(plotlayout);
    mainLayout->addLayout(optionlayout);
    //layout->addWidget(horizontalGroupBoxOptions);
    //layout->addWidget(p_status_bar);

    horizontalGroupBoxCheckbox->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::MinimumExpanding);
    horizontalGroupBoxTrigger->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Fixed);
    horizontalGroupBoxOptions->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);

    horizontalGroupBoxCheckbox->setMaximumWidth(100);
    //p_status_bar->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
    this->setLayout(mainLayout);
    this->setWindowTitle("Analog Inputs");
    setFocusPolicy(Qt::StrongFocus);
    //resize(350,800);

    // include this section to fully disable antialiasing for higher performance:

    /*customPlot->setNotAntialiasedElements(QCP::aeAll);
    QFont axisfont;
    axisfont.setStyleStrategy(QFont::NoAntialias);
    customPlot->xAxis->setTickLabelFont(axisfont);
    customPlot->yAxis->setTickLabelFont(axisfont);
    customPlot->legend->setFont(axisfont);*/


    for (int i = 0; i < m_hardwareSettingsPtr->GetNAI(); ++i) {
        QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
        timeTicker->setTimeFormat("%m:%s");
        customPlot[i].addGraph(); // blue line
        customPlot[i].graph(0)->setPen(QPen(Qt::red));
        customPlot[i].xAxis->setTicker(timeTicker);
        customPlot[i].axisRect()->setupFullAxesBox();
        customPlot[i].yAxis->setRange(-11, 11);
        //customPlot[i].mSetYAxisFixedWidth = true;
        customPlot[i].yAxis->setPadding(40);
        customPlot[i].axisRect()->setAutoMargins(QCP::msNone);
        customPlot[i].axisRect()->setMargins(QMargins(20,2,2,2));
    }

    for (int i = 0; i < m_hardwareSettingsPtr->GetNAI(); ++i) {
        connect(&customPlot[i], &QCustomPlot::mouseWheel,
            [=]( QWheelEvent *event ) { this->mouseWheel(event,i); } );
        connect(&customPlot[i], &QCustomPlot::mousePress,
                    [=]( QMouseEvent *event ) { this->mousePressed(event,i); } );
    }

    theTime = 0.0;
    theTimeRange = 5.0;

    dataTimer.setTimerType(Qt::PreciseTimer);
    connect(&dataTimer, SIGNAL(timeout()), this, SLOT(addData()));
    dataTimer.start(40); //Refresh at 25 Hz

    nChunksPerSecond = 20;
    chunkSize = m_hardwareSettingsPtr->GetNAI()*samplingRate/nChunksPerSecond; //Size of the data sent from the analog input class
    p_buffer_size = nChunksPerSecond * chunkSize;
    p_current_pos = 0;
    p_current_read_pos = 0;
    p_data_buffer = new double[p_buffer_size];

    int idxCh = 0;
    for (int i = 0; i < m_hardwareSettingsPtr->GetNAI(); i++) {
        channelsName.append(m_theAIName.at(i));
        customPlot[idxCh].graph(0)->setName(m_theAIName.at(i));
        checkBoxes[idxCh].setText(m_theAIName.at(i));

        customPlot[idxCh].legend->setVisible(true);
        QFont legendFont = font();  // start out with MainWindow's font..
        legendFont.setPointSize(9); // and make a bit smaller for legend
        customPlot[idxCh].legend->setFont(legendFont);
        customPlot[idxCh].legend->setIconSize(0,15);
        QPen pen;
        pen.setColor(QColor(255,255,255,150));
        customPlot[idxCh].legend->setBorderPen(pen);
        customPlot[idxCh].legend->setBrush(QColor(255, 255, 255, 150));
        customPlot[idxCh].axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignLeft);
        idxCh++;
    }

    for (int i = 0; i < m_hardwareSettingsPtr->GetNAI(); ++i) {
        if (!isCustomPlotEnabled[i]) {
            customPlot[i].hide();
        }
    }

    QSignalMapper* signalMapper = new QSignalMapper (this) ;
    for (int i = 0; i < m_hardwareSettingsPtr->GetNAI(); i++) {
        connect (&checkBoxes[i], SIGNAL(stateChanged(int)), signalMapper, SLOT(map())) ;
        signalMapper -> setMapping (&checkBoxes[i], i) ;
    }
    connect (signalMapper, SIGNAL(mappedInt(int)), this, SLOT(modifyPlot(int))) ;
    isFirstRun = true;

    profTest = QTime::currentTime();

    //Set default values
    mTrigger.deadTime = 2.0;
    mTrigger.displayTimeBeforeTrig = 0.2;
    mTrigger.displayTimeAfterTrig = 0.8 + 0.05; //Extra 50 ms to show the 1 sec tick

    mTrigger.rxRisingEdge = false;
    mTrigger.rxTriggerBelow = false;
    mTrigger.risingEdgePos = 0;
    mTrigger.risingEdgeChunkPos = 0;
    mTrigger.readPosTrigger = 0;

    for (int i = 0; i < channelsName.length(); i++) {
        cbSource->addItem(channelsName.at(i));
    }
    cbSource->setCurrentIndex(0);
    CheckTriggerSettings();
}

AnalogViewer::~AnalogViewer()
{
    delete plotlayout;
    delete optionlayout;
    delete mainLayout;
    delete [] customPlot;
    delete [] isCustomPlotEnabled;
    delete [] p_data_buffer;
    delete [] checkBoxes;

   /* delete p_status_bar_left;
    delete p_status_bar_middle;
    delete p_status_bar_right;
    delete p_status_bar;*/
}

void AnalogViewer::closeEvent(QCloseEvent *bar)
{
    emit aboutToQuit();
    bar->accept();
}

void AnalogViewer::addData()
{
    //QTime prof = QTime::currentTime();
    //qDebug() << "Delay: " << prof.msecsTo(profTest);
    //profTest = prof;

    if (isFirstRun) {
        //uncheck all;
        for(int aiIdx=0; aiIdx<m_hardwareSettingsPtr->GetNAI(); aiIdx++)
            checkBoxes[aiIdx].setChecked(false);
        //check first 3.
        checkBoxes[0].setChecked(true);
        if ((int)m_hardwareSettingsPtr->GetNAI() > 1)
            checkBoxes[1].setChecked(true);
        if ((int)m_hardwareSettingsPtr->GetNAI() > 2)
            checkBoxes[2].setChecked(true);
        isFirstRun = false;
    }

    // Acquire a block of data

    //if (p_current_pos - p_current_read_pos > 10)
        //qDebug() << "Buffer is filling too fast: " << p_current_pos - p_current_read_pos;
    //qDebug() << "Buffer: " << p_current_pos - p_current_read_pos;

#if !defined(IS_ARDUINO)
    QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    timeTicker->setTimeFormat("%m:%s:%z");
    timeTicker->setTickCount(10);
    for (int i = 0; i < m_hardwareSettingsPtr->GetNAI(); ++i) {
        customPlot[i].xAxis->setTicker(timeTicker);
    }
    //In trigger mode, check if we need to display. Must be higher than the threshold
    if (mTrigger.isTriggerMode) {
        m_mutex.lock();
        //Check all values to find a rising edge above threshold
        double val;
        //qDebug() << "will check" << p_current_pos << mTrigger.readPosTrigger;
        while (p_current_pos > mTrigger.readPosTrigger) {
            for (int idx = 0; idx < (int) (chunkSize/m_hardwareSettingsPtr->GetNAI()); idx++) {
                if (!mTrigger.rxRisingEdge) { //Search only if rising edge is not found yet
                    val = (double) p_data_buffer[(mTrigger.readPosTrigger % nChunksPerSecond)*chunkSize + idx + mTrigger.sourceChannel*chunkSize/m_hardwareSettingsPtr->GetNAI()];
                    if (!mTrigger.rxTriggerBelow) { //Rising edge detection requires detecting below threshold
                        if (mTrigger.edgeType == 0 && val < mTrigger.level) {
                            mTrigger.rxTriggerBelow = true;
                        }
                        if (mTrigger.edgeType == 1 && val > mTrigger.level) {
                            mTrigger.rxTriggerBelow = true;
                        }
                        if (mTrigger.edgeType == 2) {
                            mTrigger.rxTriggerBelow = true;
                            if (val > mTrigger.level)
                                mTrigger.bothEdgeIsOver = true;
                            else
                                mTrigger.bothEdgeIsOver = false;
                        }
                    }
                    if (mTrigger.rxTriggerBelow && !mTrigger.rxRisingEdge) {
                        bool foundRisingEdge = false;
                        if (mTrigger.edgeType == 0 && val > mTrigger.level) {
                            foundRisingEdge = true;
                        }
                        if (mTrigger.edgeType == 1 && val < mTrigger.level) {
                            foundRisingEdge = true;
                        }
                        if (mTrigger.edgeType == 2) {
                            if (mTrigger.bothEdgeIsOver && val < mTrigger.level)
                                foundRisingEdge = true;
                            if (!mTrigger.bothEdgeIsOver && val > mTrigger.level)
                                foundRisingEdge = true;
                        }
                        if (foundRisingEdge) { //Found rising edge, only keep the last 200 ms
                            p_current_read_pos = mTrigger.readPosTrigger - (int)(mTrigger.displayTimeBeforeTrig*(double)nChunksPerSecond);
                            mTrigger.firstDisplayPos = p_current_read_pos;
                            mTrigger.risingEdgePos = mTrigger.readPosTrigger;
                            mTrigger.risingEdgeChunkPos = idx;
                            if (p_current_read_pos < 0) p_current_read_pos = 0;
                            mTrigger.rxRisingEdge = true;
                            //qDebug() << "Rising at " << mTrigger.risingEdgePos << idx << "go back to" << p_current_read_pos;
                            //Reset display
                            theTime = 0.0;
                            theTimeRange = 1.0;
                            for (int i = 0; i < m_hardwareSettingsPtr->GetNAI(); ++i) {
                                customPlot[i].graph(0)->data()->removeAfter(-1);
                            }
                        }
                    }
                }
            }
            mTrigger.readPosTrigger++;
        }

        m_mutex.unlock();
        //Continue with display only if rising edge is detected
        if (!mTrigger.rxRisingEdge)
            return;
        else {
            if (p_current_pos > mTrigger.risingEdgePos + (int)(mTrigger.deadTime*(double)nChunksPerSecond)) { //Wait dead time before acquiring a new rising edge
                mTrigger.rxRisingEdge = false;
                mTrigger.rxTriggerBelow = false;
                mTrigger.risingEdgePos = 0;
                mTrigger.risingEdgeChunkPos = 0;
                mTrigger.readPosTrigger = p_current_pos;
            }
        }
    }
#endif

    m_mutex.lock();
    //qDebug() << "will display" << p_current_pos << p_current_read_pos;
    while (p_current_pos > p_current_read_pos) {
        double val;
        double skipRatio;
#if !defined(IS_ARDUINO)
        skipRatio = 9;//Read only 1/9 of data;
        if (mTrigger.isTriggerMode) {
            skipRatio = 1;
        }
#else
        skipRatio = 1;
#endif
        double theTimeBackup = theTime;

        for (int i = 0; i < m_hardwareSettingsPtr->GetNAI(); i++) {
            theTime = theTimeBackup; // Reset the time array
            for (int idx = 0; idx < (int) (chunkSize/m_hardwareSettingsPtr->GetNAI()); idx += (int) skipRatio) {
                if (mTrigger.isTriggerMode && idx < mTrigger.risingEdgeChunkPos && mTrigger.firstDisplayPos == p_current_read_pos) {
                   //Skip display in trigger mode if before the display time
                } else if (mTrigger.isTriggerMode && idx > mTrigger.risingEdgeChunkPos && p_current_read_pos == mTrigger.risingEdgePos + (int)(mTrigger.displayTimeAfterTrig*(double)nChunksPerSecond)) {
                    //Skip display in trigger mode if after the display time
                } else if (mTrigger.isTriggerMode && p_current_read_pos > mTrigger.risingEdgePos + (int)(mTrigger.displayTimeAfterTrig*(double)nChunksPerSecond)) {
                    //Skip display in trigger mode if after the display time
                } else {
                    val = (double) p_data_buffer[(p_current_read_pos % nChunksPerSecond)*chunkSize + idx + i*chunkSize/m_hardwareSettingsPtr->GetNAI()];
                    customPlot[i].graph(0)->addData(theTime, (double)val);
                    theTime += 1.0/samplingRate*skipRatio;
                }
            }
        }
        p_current_read_pos++;   
    }
    m_mutex.unlock();
    //Prevent overflow on long acquisitions
    for (int i = 0; i < m_hardwareSettingsPtr->GetNAI(); ++i) {
        customPlot[i].graph(0)->data()->removeBefore(theTime-(maxTimeRange+5));
    }

    // rescale value (vertical) axis to fit the current data:
    for (int i = 0; i < m_hardwareSettingsPtr->GetNAI(); ++i) {
        customPlot[i].graph(0)->rescaleValueAxis(false, true);
    }

    //lastPointKey = key;

    // make key axis range scroll with the data:
    double extraTime = 0.0;
    if (mTrigger.isTriggerMode) extraTime = 0.05 + 0.01;
    for (int i = 0; i < m_hardwareSettingsPtr->GetNAI(); ++i) {
        customPlot[i].xAxis->setRange(theTime, theTimeRange+extraTime, Qt::AlignRight);
        if(!m_isPaused)
            customPlot[i].replot();
    }

    qApp->processEvents();

    //qDebug() << "Display duration:" << prof.elapsed();
}

void AnalogViewer::put(double* data)
{
    //Analog input thread calls put every 50 ms with a chunk of data
    m_mutex.lock();
    memcpy(&p_data_buffer[(p_current_pos % nChunksPerSecond)*chunkSize],data,chunkSize*sizeof(double));
    p_current_pos+=1;
    m_mutex.unlock();

}
void AnalogViewer::mousePressed(QMouseEvent *event, int idxPlot)
{
    m_isPaused=!m_isPaused;
    if(m_isPaused)
        m_labelInfo->setText("Paused");
    else
        m_labelInfo->setText("");
}

void AnalogViewer::mouseWheel(QWheelEvent *event, int idxPlot)
{
    if (!isCustomPlotEnabled[idxPlot]) {
        return;
    }
    // Mouse range zooming interaction:
    double wheelSteps = -event->angleDelta().y()/120.0; // a single step delta is +/-120 usually

    if (mTrigger.isTriggerMode) { //Zoom on cursor for trigger mode
        double factor = qPow(0.85, -wheelSteps);;

        double extraTime = 0.05 + 0.01;
        for (int i = 0; i < m_hardwareSettingsPtr->GetNAI(); ++i) {
            customPlot[i].xAxis->scaleRange(factor, customPlot[idxPlot].xAxis->pixelToCoord(event->position().x()));
            QCPRange r = customPlot[i].xAxis->range();
            double lower = r.lower;
            double upper = r.upper;
            if (lower < 0) lower = 0;
            if (upper > theTimeRange + extraTime) upper = theTimeRange + extraTime;
            customPlot[i].xAxis->setRange(upper, upper-lower, Qt::AlignRight);
            if(!m_isPaused)
                customPlot[i].replot();
        }
        return;
    }

    //qDebug() << "Steps" << wheelSteps;
    if (wheelSteps > 0 && theTimeRange < maxTimeRange) { //Unzoom
        while (wheelSteps > 0 && theTimeRange < maxTimeRange) {
            if (theTimeRange < 5)
                theTimeRange += 1; //Unzoom when less than 5 seconds displayed
            else
                theTimeRange += 5;
            wheelSteps--;
        }
        wheelSteps = 0;
    }

    while (wheelSteps < 0) { //Zoom
        wheelSteps++;
        if (theTimeRange > 5) {
            theTimeRange -= 5;
        } else if (theTimeRange > 1) {
            theTimeRange -= 1;
        }
    }

    for (int i = 0; i < m_hardwareSettingsPtr->GetNAI(); ++i) {
        customPlot[i].xAxis->setRange(theTime, theTimeRange, Qt::AlignRight);
        //qDebug()<<theTimeRange;
        if(theTimeRange>10)
        {
            QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
            timeTicker->setTimeFormat("%m:%s");
            customPlot[i].xAxis->setTicker(timeTicker);
        }
        else
        {
            QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
            timeTicker->setTimeFormat("%m:%s:%z");
            customPlot[i].xAxis->setTicker(timeTicker);
        }
        if(!m_isPaused)
            customPlot[i].replot();
    }
}

void AnalogViewer::modifyPlot(int idxPlot) {
    if(idxPlot<m_hardwareSettingsPtr->GetNAI())
    {
        //qDebug() << "Modif" << isCustomPlotEnabled[idxPlot];
        if (isCustomPlotEnabled[idxPlot]) {
            isCustomPlotEnabled[idxPlot] = false;
            //layout->removeWidget(customPlot);
            //delete customPlot;
            customPlot[idxPlot].hide();
        }
        else {
            isCustomPlotEnabled[idxPlot] = true;
            customPlot[idxPlot].show();
            //layout->addWidget(backupPlot);
        }

    }

    //QFont *p_font = new QFont("Roboto",1,QFont::Normal);


    //Loop all to check which one is last and make label visible
    int lastVisibleIdx=0;
    for (int i = 0; i < m_hardwareSettingsPtr->GetNAI(); ++i) {
        if(checkBoxes[i].isChecked())
            lastVisibleIdx=i;
        customPlot[i].xAxis->setTickLabelColor(Qt::transparent);
        customPlot[i].axisRect()->setMargins(QMargins(40,2,2,2));
        customPlot[i].contentsRect().setHeight(40);
        plotlayout->setStretch(i,10);
    }
    plotlayout->setStretch(lastVisibleIdx,12);
    customPlot[lastVisibleIdx].xAxis->setTickLabelColor(Qt::black);
    customPlot[lastVisibleIdx].axisRect()->setMargins(QMargins(40,2,2,18));


    //ResizeV2
    QRect currentPos = this->geometry();
    currentPos.moveBottom(this->screen()->availableGeometry().height());
    this->setGeometry(currentPos);

}

void AnalogViewer::getDefault(void** theDefaults)
{
    free(*theDefaults); //Free previous malloc
    *theDefaults = malloc(sizeof(*theDefaults) * (m_hardwareSettingsPtr->GetNAI()*sizeof(bool) + 1*sizeof(bool) + 2*sizeof(int) + 1*sizeof(double)));
    int pos = 0; //pos in the void buffer;

    bool b;
    for (int i = 0; i < m_hardwareSettingsPtr->GetNAI(); i++) {
        b = checkBoxes[i].isChecked();
        memcpy(((bool*)*theDefaults+pos), &b, sizeof(bool));
        pos += sizeof(bool);
    }

    bool isTriggerMode = (cbAcqMode->currentIndex() == 1);
    int sourceChannel = cbSource->currentIndex();
    double level = leLevel->text().toDouble();
    int edgeType = cbEdge->currentIndex();

    memcpy(((bool*)*theDefaults+pos), &isTriggerMode, sizeof(bool));
    pos += sizeof(bool);
    memcpy(((int*)*theDefaults+pos), &sourceChannel, sizeof(int));
    pos += sizeof(int);
    memcpy(((double*)*theDefaults+pos), &level, sizeof(double));
    pos += sizeof(double);
    memcpy(((int*)*theDefaults+pos), &edgeType, sizeof(int));
    pos += sizeof(int);
    QRect windowPosition = this->geometry();
    memcpy(((int*)*theDefaults+pos), &windowPosition, sizeof(QRect));
    pos += sizeof(QRect);
}

void AnalogViewer::setDefault(void** theDefaults)
{

    isFirstRun = false;
    int pos = 0; //pos in the void buffer;

    bool b;
    for (int i = 0; i < m_hardwareSettingsPtr->GetNAI(); ++i) {
        memcpy(&b, ((bool*)*theDefaults+pos), sizeof(bool));
        pos += sizeof(bool);
        checkBoxes[i].setChecked(b);
    }

    bool isTriggerMode;
    int sourceChannel;
    double level;
    int edgeType;

    memcpy(&isTriggerMode, ((bool*)*theDefaults+pos), sizeof(bool));
    pos += sizeof(bool);
    memcpy(&sourceChannel,((int*)*theDefaults+pos), sizeof(int));
    pos += sizeof(int);
    memcpy(&level, ((double*)*theDefaults+pos), sizeof(double));
    pos += sizeof(double);
    memcpy(&edgeType, ((int*)*theDefaults+pos), sizeof(int));
    pos += sizeof(int);
    QRect windowPosition;
    memcpy(&windowPosition, ((int*)*theDefaults+pos), sizeof(QRect));
    pos += sizeof(QRect);
    this->setGeometry(windowPosition);

    if (isTriggerMode)
        cbAcqMode->setCurrentIndex(1);
    else
        cbAcqMode->setCurrentIndex(0);
    cbSource->setCurrentIndex(sourceChannel);
    leLevel->setText(QString::number(level));
    cbEdge->setCurrentIndex(edgeType);

    CheckTriggerSettings();
}


void AnalogViewer::CheckTriggerSettings()
{
    m_isPaused=false;
    mTrigger.lastTriggerMode = mTrigger.isTriggerMode;
    mTrigger.isTriggerMode = (cbAcqMode->currentIndex() == 1);
    mTrigger.sourceChannel = cbSource->currentIndex();
    mTrigger.level = leLevel->text().toDouble();
    mTrigger.edgeType = cbEdge->currentIndex();

    cbSource->setEnabled(mTrigger.isTriggerMode);
    leLevel->setEnabled(mTrigger.isTriggerMode);
    cbEdge->setEnabled(mTrigger.isTriggerMode);

    if (mTrigger.lastTriggerMode && !mTrigger.isTriggerMode) { //Range is 5 seconds when coming back from trigger mode
        theTimeRange = 5.0;
    }
}
