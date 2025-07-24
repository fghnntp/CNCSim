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
#include "live_plotter_motion.h"
#include <QDockWidget>
#include "mill_task_interface.h"
#include <QLineEdit>
#include "CmdTextEdit.h"
#include "LogDisplayWidget.h"
#include "KeyInfoDisplayWidget.h"

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
    void setPathDock();
    void setMotionProfileDock();

    void createBckThread();
    IMillTaskInterface *millIf_;

    
    GCodeEdit *activeGCodeEdit();
    QMdiSubWindow *findMdiChild(const QString &fileName);
    
    QMdiArea *mdiArea;
    
    // 菜单
    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *windowMenu;
    QMenu *helpMenu;
    QMenu *toolMenu;
    QMenu *searchMenu;
    
    // 工具栏
    QToolBar *fileToolBar;
//    QToolBar *editToolBar;
    QToolBar *viewToolBar;
    QToolBar *searchToolBar;
    
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
    QAction *loadPlotFileAct;

    QAction *cncCmdAct;

    QAction *motionPlotAct;
    
    QActionGroup *windowActionGroup;

    ToolManager *toolManager = nullptr;

    QDockWidget *livePlotterDock = nullptr;
    LivePlotter *livePlotter = nullptr;

    QDockWidget *livePlotterMotionDock = nullptr;
    LivePlotterMotion *livePlotterMotion = nullptr;
private slots:
    void find();
    void findNext();
    void findPrevious();
    void replace();
    void replaceAll();

private:
    void createSearchActions();
    void createSearchToolBar();
    QLineEdit *searchLineEdit;
    QLineEdit *replaceLineEdit;
    QAction *findAct;
    QAction *findNextAct;
    QAction *findPreviousAct;
    QAction *replaceAct;
    QAction *replaceAllAct;
private:
    CommandTextEdit *cmdTextEdit = nullptr;
    QDockWidget *cmdTextDock = nullptr;
    LogDisplayWidget *logDisplayWidget = nullptr;
    QDockWidget *logDisplayDock = nullptr;

    QTimer *logTimer = nullptr;

    void setCmdLogDock();

    QDockWidget *keyInfoDisplayDock = nullptr;
    KeyInfoDisplayWidget *keyInfoDisplayWidget = nullptr;
    QTimer *infoTimer = nullptr;
    void setKeyInfoDock();
};

#endif // MAINWINDOW_H
