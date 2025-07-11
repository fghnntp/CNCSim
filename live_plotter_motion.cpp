#include "live_plotter_motion.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <math.h>
#include <QGraphicsLayout>
#include <QCheckBox>
#include <QDebug>

LivePlotterMotion::LivePlotterMotion(QWidget *parent) : QWidget(parent)
{
    setupUI();
    applyStyles();
    updateAxisVisibility(); // Set initial visibility
}

void LivePlotterMotion::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(2, 2, 2, 2);  // Minimal outer margins
    mainLayout->setSpacing(5);  // Reduced spacing between charts

    // Add axis checkboxes
    QHBoxLayout *controlLayout = new QHBoxLayout();
    controlLayout->addWidget(new QLabel("Show Axes:"));

    for (int i = 0; i < kACAll; i++) {
        axisCheck[i] = new QCheckBox(kAxisName[i]);
        if (i < kA) {
            axisCheck[i]->setChecked(true);
        }
        else {
            axisCheck[i]->setChecked(false);
        }
        controlLayout->addWidget(axisCheck[i]);
        connect(axisCheck[i], &QCheckBox::stateChanged, this, &LivePlotterMotion::updateAxisVisibility);
    }

    controlLayout->addStretch();
    mainLayout->addLayout(controlLayout);

    // Create chart views with minimal internal margins
    chartViews[kPos] = createChartView("Position", "Position over Time", kPos);
    chartViews[kVel] = createChartView("Velocity", "Velocity over Time", kVel);
    chartViews[kAcc] = createChartView("Acceleration", "Acceleration over Time", kAcc);
    chartViews[kJerk] = createChartView("Jerk", "Jerk over Time", kJerk);

    // Add stretch factors to distribute space evenly
    mainLayout->addWidget(chartViews[kPos], 1);  // Stretch factor 1
    mainLayout->addWidget(chartViews[kVel], 1);
    mainLayout->addWidget(chartViews[kAcc], 1);
    mainLayout->addWidget(chartViews[kJerk], 1);

    // Configure each chart view to minimize margins
    for (auto chartView : chartViews) {
        chartView->setContentsMargins(0, 0, 0, 0);
        chartView->chart()->setMargins(QMargins(5, 5, 5, 5));  // Tight inner margins
        chartView->chart()->layout()->setContentsMargins(0, 0, 0, 0);

        // Set size policy to expand both horizontally and vertically
        chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    }
}

QChartView* LivePlotterMotion::createChartView(const QString &title, const QString &subtitle, AnalysiElement elemt)
{
    QChartView *chartView = new QChartView(this);
    chartView->setRenderHint(QPainter::Antialiasing);

    charts[elemt] = new QChart();
    charts[elemt]->setTitle(subtitle);
    charts[elemt]->legend()->hide();

    // Configure chart margins and background
    charts[elemt]->setBackgroundVisible(false);  // Transparent background
    charts[elemt]->setMargins(QMargins(5, 5, 5, 5));  // Tight margins

    // Create series for each axis and metric
    for (int axis = 0; axis < kACAll; axis++) {
        series[elemt][axis] = new QLineSeries();
        series[elemt][axis]->setName(kAxisName[axis] + kElementName[elemt]);
        charts[elemt]->addSeries(series[elemt][axis]);
    }

    // Create axes
    axisX[elemt] = new QValueAxis();
    axisX[elemt]->setTitleText("Time (s)");
    axisX[elemt]->setLabelFormat("%.1f");

    axisY[elemt] = new QValueAxis();
    axisY[elemt]->setTitleText("Values");
    axisY[elemt]->setLabelFormat("%.1f");

    charts[elemt]->addAxis(axisX[elemt], Qt::AlignBottom);
    charts[elemt]->addAxis(axisY[elemt], Qt::AlignLeft);

    // Attach axes to all series
    for (auto series : charts[elemt]->series()) {
        QLineSeries *lineSeries = qobject_cast<QLineSeries*>(series);
        if (lineSeries) {
            lineSeries->attachAxis(axisX[elemt]);
            lineSeries->attachAxis(axisY[elemt]);
        }
    }

    chartView->setChart(charts[elemt]);

    return chartView;
}

void LivePlotterMotion::updateAxisVisibility()
{
    for(int elemt = 0; elemt < kAEAll; elemt++) {
        for (int axis = 0; axis < kACAll; axis++) {
            series[elemt][axis]->setVisible(axisCheck[axis]->isChecked());
            series[elemt][axis]->setVisible(axisCheck[axis]->isChecked());
            series[elemt][axis]->setVisible(axisCheck[axis]->isChecked());
            series[elemt][axis]->setVisible(axisCheck[axis]->isChecked());
        }
    }

    updateChartData();
}

void LivePlotterMotion::applyStyles()
{
    // Set a unified theme for all charts
    QFont titleFont("Arial", 10, QFont::Bold);
    QFont labelFont("Arial", 8);

    // Color palette
    QColor positionColor(65, 105, 225);   // Royal Blue
    QColor velocityColor(220, 20, 60);    // Crimson
    QColor accelerationColor(34, 139, 34); // Forest Green
    QColor jerkColor(255, 140, 0);        // Dark Orange

    // Apply styles to each chart
    for (int i = 0; i < kACAll; i++) {
        styleChart(chartViews[kPos]->chart(), series[kPos], positionColor, "Position (m)");
        styleChart(chartViews[kVel]->chart(), series[kVel], velocityColor, "Velocity (mm/min)");
        styleChart(chartViews[kAcc]->chart(), series[kAcc], accelerationColor, "Acceleration (m/s²)");
        styleChart(chartViews[kJerk]->chart(), series[kJerk], jerkColor, "Jerk (m/s³)");
    }
}

void LivePlotterMotion::styleChart(QChart *chart, QLineSeries *series[], const QColor &color, const QString &yAxisTitle)
{
    // Series style
    QPen pen(color);
    pen.setWidth(2);

    // Axis configuration
    QValueAxis *axisX = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).first());
    QValueAxis *axisY = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).first());

    axisX->setTitleText("Time (s)");
    axisY->setTitleText(yAxisTitle);

    // Reduce axis label space
    axisX->setLabelFormat("%.1f");
    axisY->setLabelFormat("%.1f");

    // Grid lines
    axisX->setGridLineVisible(true);
    axisY->setGridLineVisible(true);
    axisX->setGridLinePen(QPen(QColor(200, 200, 200), 1, Qt::DotLine));
    axisY->setGridLinePen(QPen(QColor(200, 200, 200), 1, Qt::DotLine));
}

void LivePlotterMotion::setPath(const QVector<QVector<float>> &path)
{
    if (path.size() < 2 || path.size() > 10) {
        // we should at list get one time and one axis value
        return;
    }
    // update interal vars
    valuedAxis = path.size() - 1;
    timeData = path[0];
    pathData.clear();
    for (int i = 0; i < valuedAxis; i++) {
        pathData.append(path[i + 1]);
    }

    calculateDerivatives(true);
    updateChartData();
}

void LivePlotterMotion::addPoint(const QVector<float> &point)
{
    calculateDerivatives(true);
    updateChartData();
}

void LivePlotterMotion::calculateDerivatives(bool abs)
{
    if (abs) {
        velocityData.clear();
        accelerationData.clear();
        jerkData.clear();

        for (int axis = 0; axis < valuedAxis; axis++) {
            QVector<float> velVec;
            for (int t = 1; t < timeData.size(); t++) {
                float dt = timeData[t] - timeData[t - 1];
                float vel = (pathData[axis][t] - pathData[axis][t - 1]) / dt;
                velVec.append(vel);
            }
            velocityData.append(velVec);

        }

        for (int axis = 0; axis < valuedAxis; axis++) {
            QVector<float> accVec;
            for (int t = 1; t < velocityData[axis].size(); t++) {
                float dt = timeData[t + 1] - timeData[t];
                float acc = (velocityData[axis][t] - velocityData[axis][t - 1]) / dt;
                accVec.append(acc);
            }
            accelerationData.append(accVec);
        }

        bool flag = false;
        for (int axis = 0; axis < valuedAxis; axis++) {
            QVector<float> jerkVec;
            for (int t = 1; t < accelerationData[axis].size(); t++) {
                float dt = timeData[t + 2] - timeData[t + 1];
                float jerk = (accelerationData[axis][t] - accelerationData[axis][t - 1]) / dt;
                jerkVec.append(jerk);

            }
            jerkData.append(jerkVec);
        }
    }
    else {

    }

}

void LivePlotterMotion::clearData()
{
    pathData.clear();
    timeData.clear();
    velocityData.clear();
    accelerationData.clear();
    jerkData.clear();
    updateChartData();
}

void LivePlotterMotion::updatePlots()
{
    updateChartData();
}

void LivePlotterMotion::updateChartData()
{
    if (timeData.size() < 3)
        return;
    // First calculate the global time range
    float globalMinTime = 0;
    float globalMaxTime = timeData.isEmpty() ? 1 : timeData.last();

    // Position chart (uses full time range)
    for (int axis = 0; axis < valuedAxis; axis++) {
        updateSeries(series[kPos][axis], timeData, pathData[axis]);
    }
    alignTimeAxis(chartViews[kPos]->chart(), globalMinTime, globalMaxTime);


    // Velocity chart (offset by 1 time unit)
    if (timeData.size() > 1) {
        for (int axis = 0; axis < valuedAxis; axis++) {
            updateSeries(series[kVel][axis], timeData.mid(1), velocityData[axis]);
        }
        alignTimeAxis(chartViews[kVel]->chart(), globalMinTime, globalMaxTime);
    }

    // Acceleration chart (offset by 2 time units)
    if (timeData.size() > 2) {
        for (int axis = 0; axis < valuedAxis; axis++) {
            updateSeries(series[kAcc][axis], timeData.mid(2), accelerationData[axis]);
        }
        alignTimeAxis(chartViews[kAcc]->chart(), globalMinTime, globalMaxTime);
    }

    // Jerk chart (offset by 3 time units)
    if (timeData.size() > 3) {
        for (int axis = 0; axis < valuedAxis; axis++) {
            updateSeries(series[kJerk][axis], timeData.mid(3), jerkData[axis]);
        }
        alignTimeAxis(chartViews[kJerk]->chart(), globalMinTime, globalMaxTime);
    }


    // Adjust axes ranges with animation
    if (!timeData.isEmpty()) {
        animateAxisRange(chartViews[kPos]->chart(), timeData.first(), timeData.last(),
                        findMinMax(pathData));

        animateAxisRange(chartViews[kVel]->chart(), timeData[1], timeData.last(),
                        findMinMax(velocityData));

        animateAxisRange(chartViews[kAcc]->chart(), timeData[2], timeData.last(),
                        findMinMax(accelerationData));

        animateAxisRange(chartViews[kJerk]->chart(), timeData[3], timeData.last(),
                        findMinMax(jerkData));
    }
}

void LivePlotterMotion::alignTimeAxis(QChart* chart, float minTime, float maxTime)
{
    QValueAxis* axisX = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).first());
    if (axisX) {
        axisX->setRange(minTime, maxTime);
        axisX->setTickCount(5); // Consistent number of ticks
        axisX->applyNiceNumbers(); // Improve tick spacing
    }
}

template<typename T>
void LivePlotterMotion::updateSeries(QLineSeries *series, const QVector<float> &times, const QVector<T> &values)
{
    series->clear();
    for (int i = 0; i < values.size(); ++i) {
        series->append(times[i], values[i]);
    }
}

QPair<float, float> LivePlotterMotion::findMinMax(const QVector<QVector<float>> &data)
{
    if (data.isEmpty()) return {0, 0};

    float minVal = data.first().first();
    float maxVal = minVal;

    int i = 0;

    for (const auto &element : data) {
        for (const auto &axis : element) {
            if (axisCheck[i]->isChecked()) {
                minVal = qMin(minVal, axis);
                maxVal = qMax(maxVal, axis);
            }
        }
        i++;
    }

    // Add 10% padding
    float range = maxVal - minVal;
    if (range < 1e-6) range = 1.0f; // Prevent division by zero

    return {minVal - 0.1f * range, maxVal + 0.1f * range};
}

void LivePlotterMotion::animateAxisRange(QChart *chart, float minX, float maxX, QPair<float, float> yRange)
{
    QValueAxis *axisX = qobject_cast<QValueAxis*>(chart->axes(Qt::Horizontal).first());
    QValueAxis *axisY = qobject_cast<QValueAxis*>(chart->axes(Qt::Vertical).first());

    // Smooth transition for axis ranges
    axisX->setRange(minX, maxX);
    axisY->setRange(yRange.first, yRange.second);
}
