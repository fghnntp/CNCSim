// LivePlotter.h
#ifndef LIVEPLOTTER_MOTION_H
#define LIVEPLOTTER_MOTION_H

#include "qmessagebox.h"
#include <QWidget>
#include <QVector>
#include <QVector3D>
#include <QTimer>
#include <QChart>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QString>

QT_CHARTS_USE_NAMESPACE

class LivePlotterMotion : public QWidget
{
    Q_OBJECT

public:
    enum AxisConfig {
        kX, kY, kZ,
        kA, kB, kC,
        kU, kV, kW,
        kACAll
    };
    const QVector<QString> kAxisName {"X", "Y", "Z", "A", "B", "C", "U", "V", "W"};
//    const QVector<QColor> kAxisColor {QColor(65, 105, 225)};
    enum AnalysiElement {
        kPos,
        kVel,
        kAcc,
        kJerk,
        kAEAll
    };
    const QVector<QString> kElementName {"Position", "Velocity", "Acceleration", "Jerk"};
    explicit LivePlotterMotion(QWidget *parent = nullptr);
    void setPath(const QVector<QVector<float>> &path);
    void addPoint(const QVector<float> &point);
    void clearData();

private slots:
    void updatePlots();

private:
    void setupUI();
    void updateChartData();

    // Data storage
    QVector<float> timeData;
    QVector<QVector<float>> pathData;
    QVector<QVector<float>> velocityData;
    QVector<QVector<float>> accelerationData;
    QVector<QVector<float>> jerkData;
    int valuedAxis;

    // UI elements
    QCheckBox *axisCheck[kACAll];

    // Chart elements
    QChartView *chartViews[kAEAll];
    QChart *charts[kAEAll];

    // Series for each axis and metric
    QLineSeries *series[kAEAll][kACAll];
    QValueAxis *axisX[kAEAll];
    QValueAxis *axisY[kAEAll];

    QPair<float, float> findMinMax(const QVector<QVector<float>> &data);
    void animateAxisRange(QChart *chart, float minX, float maxX, QPair<float, float> yRange);
    void applyStyles();
    QChartView *createChartView(const QString &title, const QString &subtitle, AnalysiElement elemt);
    void styleChart(QChart *chart, QLineSeries *series[], const QColor &color, const QString &yAxisTitle);
    template<typename T>
    void updateSeries(QLineSeries *series, const QVector<float> &times, const QVector<T> &values);
    void alignTimeAxis(QChart *chart, float minTime, float maxTime);
    void updateAxisVisibility();
    void calculateDerivatives(bool abs=false);

    float lastT[kACAll], lastP[kACAll], lastV[kACAll], lastA[kACAll], lastJ[kACAll];
};

#endif // LIVEPLOTTER_MOTION_H
