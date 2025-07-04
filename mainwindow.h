#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QAction>
#include <QActionGroup>
#include "GCodeEdit.h"
#include "tool_manager.h"
#include "live_plotter.h"
#include <QDockWidget>
#include "mill_task_interface.h"
#include "rtapp_interface.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void  closeEvent(QCloseEvent *event);

private slots:
    void newFile();
    void openFile();
    void saveFile();
    void saveAsFile();
    void closeActiveSubWindow();
    void closeAllSubWindows();
    void tileSubWindows();
    void cascadeSubWindows();
    void aboutQt();
    
    void updateMenus();
    void updateWindowMenu();
    void setActiveSubWindow(QWidget *window);

private:
    void createActions();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void readSettings();
    void writeSettings();
    void setDock();

    void createBckThread();
    IMillTaskInterface *millIf_;
    IRtAppInterface *rtAppIf_;

    
    GCodeEdit *activeGCodeEdit();
    QMdiSubWindow *findMdiChild(const QString &fileName);
    
    Ui::MainWindow *ui;
    QMdiArea *mdiArea;
    
    // 菜单
    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *windowMenu;
    QMenu *helpMenu;
    QMenu *toolMenu;
    
    // 工具栏
    QToolBar *fileToolBar;
    QToolBar *editToolBar;
    
    // 动作
    QAction *newAct;
    QAction *openAct;
    QAction *saveAct;
    QAction *saveAsAct;
    QAction *exitAct;
    QAction *cutAct;
    QAction *copyAct;
    QAction *pasteAct;
    QAction *closeAct;
    QAction *closeAllAct;
    QAction *tileAct;
    QAction *cascadeAct;
    QAction *nextAct;
    QAction *previousAct;
    QAction *separatorAct;
    QAction *aboutQtAct;

    QAction *toolTableAct;
    QAction *pathPlotAct;
    
    QActionGroup *windowActionGroup;

    ToolManager *toolManager;

    QTimer *simTimer;
    QDockWidget *livePlotterDock;
    LivePlotter *livePlotter;
};

#endif // MAINWINDOW_H
