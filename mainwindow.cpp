#include "mainwindow.h"
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QCloseEvent>
#include <QSettings>
#include <QSignalMapper>
#include <QScreen>
#include "ToolTableDialog.h"
#include "cnc_config.h"
#include <cmath>
#include <QDateTime>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    toolManager = new ToolManager(TOOL_FILE_PATH);

    // 创建 MDI 区域
    mdiArea = new QMdiArea;
    mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setCentralWidget(mdiArea);
    
    // 连接 MDI 区域信号
    connect(mdiArea, &QMdiArea::subWindowActivated,
            this, &MainWindow::updateMenus);

    // 创建界面元素
    createActions();
    createMenus();
    createToolBars();
    createStatusBar();
    updateMenus();
    
    readSettings();
    
    setWindowTitle(tr("CNC G-Code Editor"));
    setUnifiedTitleAndToolBarOnMac(true);

    createBckThread();
}

MainWindow::~MainWindow()
{
}

void MainWindow::newFile()
{
    GCodeEdit *child = new GCodeEdit;
    QMdiSubWindow *subWindow = mdiArea->addSubWindow(child);
    
    child->setWindowTitle(tr("未命名%1").arg(mdiArea->subWindowList().size()));

    // Make the editor fill the subwindow
    child->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Make the subwindow fill the MDI area
    subWindow->setWindowState(Qt::WindowMaximized);


    child->show();
    
    // 连接子窗口信号
    connect(child, &GCodeEdit::textChanged, this, &MainWindow::updateMenus);
}

void MainWindow::openFile()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
        tr("打开G代码文件"), "",
        tr("G代码文件 (*.gcode *.nc *.cnc, *.tbl);;所有文件 (*)"));
    
    if (!fileName.isEmpty()) {
        QMdiSubWindow *existing = findMdiChild(fileName);
        if (existing) {
            mdiArea->setActiveSubWindow(existing);
            return;
        }
        
        GCodeEdit *child = new GCodeEdit;
        if (child->loadFromFile(fileName)) {
            statusBar()->showMessage(tr("文件已加载"), 2000);
            QMdiSubWindow *subWindow = mdiArea->addSubWindow(child);

            // Set window title
            QString title = QFileInfo(fileName).fileName();
            child->setWindowTitle(title);
            subWindow->setWindowTitle(title);

            // Make the editor fill the subwindow
            child->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

            // Make the subwindow fill the MDI area
            subWindow->setWindowState(Qt::WindowMaximized);

            // Alternatively, if you want to manually set the size:
            // subWindow->resize(mdiArea->size());

            child->show();

            connect(child, &GCodeEdit::textChanged, this, &MainWindow::updateMenus);
        } else {
            child->deleteLater();
            QMessageBox::warning(this, tr("错误"), tr("无法打开文件"));
        }
    }
}

void MainWindow::saveFile()
{
    GCodeEdit *child = activeGCodeEdit();
    if (!child)
        return;
    
    // 如果文件有有效路径，直接保存
    if (child->hasValidFilePath()) {
        if (child->saveToFile("")) { // 传空字符串，方法内部会使用当前路径
            statusBar()->showMessage(tr("文件已保存"), 2000);
        } else {
            QMessageBox::warning(this, tr("错误"), tr("保存文件失败"));
        }
    } else {
        // 如果没有路径，调用另存为
        saveAsFile();
    }

}

void MainWindow::saveAsFile()
{
    GCodeEdit *child = activeGCodeEdit();
    if (!child)
        return;
        
    const QString fileName = QFileDialog::getSaveFileName(this,
        tr("保存G代码文件"), child->getCurrentFilePath(),
        tr("G代码文件 (*.gcode *.nc *.cnc, *.tbl);;所有文件 (*)"));
    
    if (!fileName.isEmpty()) {
        if (child->saveToFile(fileName)) {
            statusBar()->showMessage(tr("文件已保存"), 2000);
            child->setWindowTitle(QFileInfo(fileName).fileName());
            
            // 更新 MDI 子窗口标题
            if (QMdiSubWindow *subWindow = mdiArea->activeSubWindow()) {
                subWindow->setWindowTitle(QFileInfo(fileName).fileName());
            }
        } else {
            QMessageBox::warning(this, tr("错误"), tr("保存文件失败"));
        }
    }

}

void MainWindow::closeActiveSubWindow()
{
    mdiArea->closeActiveSubWindow();
}

void MainWindow::closeAllSubWindows()
{
    mdiArea->closeAllSubWindows();
}

void MainWindow::tileSubWindows()
{
    mdiArea->tileSubWindows();
}

void MainWindow::cascadeSubWindows()
{
    mdiArea->cascadeSubWindows();
}

void MainWindow::aboutQt()
{
    QApplication::aboutQt();
}

void MainWindow::updateMenus()
{
    bool hasMdiChild = (activeGCodeEdit() != nullptr);
    saveAct->setEnabled(hasMdiChild);
    saveAsAct->setEnabled(hasMdiChild);
    cutAct->setEnabled(hasMdiChild);
    copyAct->setEnabled(hasMdiChild);
    pasteAct->setEnabled(hasMdiChild);
    closeAct->setEnabled(hasMdiChild);
    closeAllAct->setEnabled(hasMdiChild);
    tileAct->setEnabled(hasMdiChild);
    cascadeAct->setEnabled(hasMdiChild);
    nextAct->setEnabled(hasMdiChild);
    previousAct->setEnabled(hasMdiChild);
    separatorAct->setVisible(hasMdiChild);
    
    updateWindowMenu();

    // Update search actions
    findAct->setEnabled(hasMdiChild);
    findNextAct->setEnabled(hasMdiChild);
    findPreviousAct->setEnabled(hasMdiChild);
    replaceAct->setEnabled(hasMdiChild);
    replaceAllAct->setEnabled(hasMdiChild);
}

void MainWindow::updateWindowMenu()
{
    windowMenu->clear();
    windowMenu->addAction(closeAct);
    windowMenu->addAction(closeAllAct);
    windowMenu->addSeparator();
    windowMenu->addAction(tileAct);
    windowMenu->addAction(cascadeAct);
    windowMenu->addSeparator();
    windowMenu->addAction(nextAct);
    windowMenu->addAction(previousAct);
    windowMenu->addAction(separatorAct);
    
    QList<QMdiSubWindow *> windows = mdiArea->subWindowList();
    separatorAct->setVisible(!windows.isEmpty());
    
    for (int i = 0; i < windows.size(); ++i) {
        QMdiSubWindow *mdiSubWindow = windows.at(i);
        GCodeEdit *child = qobject_cast<GCodeEdit *>(mdiSubWindow->widget());
        
        QString text;
        if (i < 9) {
            text = tr("&%1 %2").arg(i + 1).arg(child->windowTitle());
        } else {
            text = tr("%1 %2").arg(i + 1).arg(child->windowTitle());
        }
        
        QAction *action = windowMenu->addAction(text, mdiSubWindow, [this, mdiSubWindow]() {
            mdiArea->setActiveSubWindow(mdiSubWindow);
        });
        action->setCheckable(true);
        action->setChecked(child == activeGCodeEdit());
    }
}

void MainWindow::setActiveSubWindow(QWidget *window)
{
    if (!window)
        return;
    mdiArea->setActiveSubWindow(qobject_cast<QMdiSubWindow *>(window));
}

void MainWindow::createActions()
{
    // Get application style for standard icons

    newAct = new QAction(QIcon::fromTheme("document-new"), tr("&新建"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("创建新的G代码文件"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newFile);
    
    openAct = new QAction(QIcon::fromTheme("document-open"), tr("&打开..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("打开现有G代码文件"));
    connect(openAct, &QAction::triggered, this, &MainWindow::openFile);
    
    saveAct = new QAction(QIcon::fromTheme("document-save"), tr("&保存"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("保存文档到磁盘"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::saveFile);
    
    saveAsAct = new QAction(tr("另存为..."), this);
    saveAsAct->setShortcuts(QKeySequence::SaveAs);
    saveAsAct->setStatusTip(tr("用新名称保存文档"));
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::saveAsFile);
    
    exitAct = new QAction(tr("退出"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("退出应用程序"));
    connect(exitAct, &QAction::triggered, qApp, &QApplication::closeAllWindows);
    
    cutAct = new QAction(QIcon::fromTheme("edit-cut"), tr("剪切"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(tr("剪切当前选择的内容到剪贴板"));
    connect(cutAct, &QAction::triggered, this, [this]() {
        if (activeGCodeEdit()) activeGCodeEdit()->cut();
    });
    
    copyAct = new QAction(QIcon::fromTheme("edit-copy"), tr("复制"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("复制当前选择的内容到剪贴板"));
    connect(copyAct, &QAction::triggered, this, [this]() {
        if (activeGCodeEdit()) activeGCodeEdit()->copy();
    });
    
    pasteAct = new QAction(QIcon::fromTheme("edit-paste"), tr("粘贴"), this);
    pasteAct->setShortcuts(QKeySequence::Paste);
    pasteAct->setStatusTip(tr("粘贴剪贴板的内容"));
    connect(pasteAct, &QAction::triggered, this, [this]() {
        if (activeGCodeEdit()) activeGCodeEdit()->paste();
    });
    
    closeAct = new QAction(tr("关闭"), this);
    closeAct->setStatusTip(tr("关闭活动窗口"));
    connect(closeAct, &QAction::triggered, mdiArea, &QMdiArea::closeActiveSubWindow);
    
    closeAllAct = new QAction(tr("关闭全部"), this);
    closeAllAct->setStatusTip(tr("关闭所有窗口"));
    connect(closeAllAct, &QAction::triggered, mdiArea, &QMdiArea::closeAllSubWindows);
    
    tileAct = new QAction(tr("平铺"), this);
    tileAct->setStatusTip(tr("平铺窗口"));
    connect(tileAct, &QAction::triggered, mdiArea, &QMdiArea::tileSubWindows);
    
    cascadeAct = new QAction(tr("层叠"), this);
    cascadeAct->setStatusTip(tr("层叠窗口"));
    connect(cascadeAct, &QAction::triggered, mdiArea, &QMdiArea::cascadeSubWindows);
    
    nextAct = new QAction(tr("下一个"), this);
    nextAct->setShortcuts(QKeySequence::NextChild);
    nextAct->setStatusTip(tr("将焦点移动到下一个窗口"));
    connect(nextAct, &QAction::triggered, mdiArea, &QMdiArea::activateNextSubWindow);
    
    previousAct = new QAction(tr("上一个"), this);
    previousAct->setShortcuts(QKeySequence::PreviousChild);
    previousAct->setStatusTip(tr("将焦点移动到上一个窗口"));
    connect(previousAct, &QAction::triggered, mdiArea, &QMdiArea::activatePreviousSubWindow);
    
    separatorAct = new QAction(this);
    separatorAct->setSeparator(true);
    
    aboutQtAct = new QAction(tr("关于 Qt"), this);
    aboutQtAct->setStatusTip(tr("显示Qt库的About对话框"));
    connect(aboutQtAct, &QAction::triggered, this, &MainWindow::aboutQt);

    toolTableAct = new QAction(tr("tool"), this);
    // toolTableAct->setShortcuts();
    toolTableAct->setStatusTip(tr("管理刀具"));
    connect(toolTableAct, &QAction::triggered, this, [this]() {
        ToolTableDialog *dlg = new ToolTableDialog(toolManager, this); // Create on heap
        dlg->setAttribute(Qt::WA_DeleteOnClose); // Auto-delete when closed
        dlg->show(); // Non-modal display
    });

    pathPlotAct = new QAction(tr("path"), this);
    // pathPlotAct->setShortcuts();
    pathPlotAct->setStatusTip(tr("tool path"));
    connect(pathPlotAct, &QAction::triggered, this, [this]() {
        setPathDock();

    });

    loadPlotFileAct = new QAction(tr("load"), this);
    loadPlotFileAct->setStatusTip(tr("load the opened file in 3d-plot"));
    connect(loadPlotFileAct, &QAction::triggered, this, [this]() {
        GCodeEdit *child = activeGCodeEdit();
        if (!child)
            return;

        if (!livePlotterDock)
            return;
        std::vector<IMillTaskInterface::ToolPath> toolPath;

        QString qFileName = child->getActiveFilePath();
        std::string filename(qFileName.toStdString());
        std::string err;

        if (millIf_->loadfile(filename.c_str(), toolPath, err) == 0) {
            QVector<QVector3D> initialPath;
            auto firstPoint = toolPath.back();
            for (auto &point : toolPath) {
                initialPath.append(QVector3D(
                    point.x - firstPoint.x,
                    point.y - firstPoint.y,
                    point.z - firstPoint.z
                ));

            }
            livePlotter->setPath(initialPath);
            statusBar()->showMessage(tr("load path finished"), 2000);
        }
        else {
            statusBar()->showMessage(tr(err.c_str()), 10000);
        }
    });

    cncCmdAct = new QAction(tr("cmdline"), this);
    cncCmdAct->setStatusTip(tr("open cmdline for comnunication with cnc"));
    connect(cncCmdAct, &QAction::triggered, this, [this]() {
        setCmdLogDock();
    });

    motionPlotAct = new QAction(tr("motion"), this);
    motionPlotAct->setStatusTip(tr("open motion profile ploter"));
    connect(motionPlotAct, &QAction::triggered, this, [this]() {
        setMotionProfileDock();
    });

    // Add search actions
    createSearchActions();

}

void MainWindow::createSearchActions()
{
    // Find action
    findAct = new QAction(QIcon::fromTheme("edit-find"), tr("&Find..."), this);
    findAct->setShortcuts(QKeySequence::Find);
    findAct->setStatusTip(tr("Find text in current document"));
    connect(findAct, &QAction::triggered, this, &MainWindow::find);

    // Find next action
    findNextAct = new QAction(tr("Find &Next"), this);
    findNextAct->setShortcut(QKeySequence::FindNext);
    findNextAct->setStatusTip(tr("Find next occurrence"));
    connect(findNextAct, &QAction::triggered, this, &MainWindow::findNext);

    // Find previous action
    findPreviousAct = new QAction(tr("Find &Previous"), this);
    findPreviousAct->setShortcut(QKeySequence::FindPrevious);
    findPreviousAct->setStatusTip(tr("Find previous occurrence"));
    connect(findPreviousAct, &QAction::triggered, this, &MainWindow::findPrevious);

    // Replace action
    replaceAct = new QAction(tr("&Replace..."), this);
    replaceAct->setShortcut(QKeySequence::Replace);
    replaceAct->setStatusTip(tr("Replace text in current document"));
    connect(replaceAct, &QAction::triggered, this, &MainWindow::replace);

    // Replace all action
    replaceAllAct = new QAction(tr("Replace &All"), this);
    replaceAllAct->setStatusTip(tr("Replace all occurrences"));
    connect(replaceAllAct, &QAction::triggered, this, &MainWindow::replaceAll);
}



void MainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("文件"));
    fileMenu->addAction(newAct);
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveAsAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);
    
    editMenu = menuBar()->addMenu(tr("编辑"));
    editMenu->addAction(cutAct);
    editMenu->addAction(copyAct);
    editMenu->addAction(pasteAct);
    
    windowMenu = menuBar()->addMenu(tr("窗口"));
    updateWindowMenu();
    connect(windowMenu, &QMenu::aboutToShow, this, &MainWindow::updateWindowMenu);

    toolMenu = menuBar()->addMenu(tr("工具"));
    toolMenu->addAction(toolTableAct);
    toolMenu->addAction(pathPlotAct);
    toolMenu->addAction(loadPlotFileAct);
    toolMenu->addAction(cncCmdAct);
    toolMenu->addAction(motionPlotAct);
    
    // Add search menu
    searchMenu = menuBar()->addMenu(tr("&Search"));
    searchMenu->addAction(findAct);
    searchMenu->addAction(findNextAct);
    searchMenu->addAction(findPreviousAct);
    searchMenu->addSeparator();
    searchMenu->addAction(replaceAct);
    searchMenu->addAction(replaceAllAct);

    helpMenu = menuBar()->addMenu(tr("帮助"));
    helpMenu->addAction(aboutQtAct);
}

void MainWindow::createSearchToolBar()
{
    searchToolBar = addToolBar(tr("Search"));
    searchToolBar->setObjectName("SearchToolBar");

    // Search line edit
    searchLineEdit = new QLineEdit;
    searchLineEdit->setPlaceholderText(tr("Search..."));
    searchLineEdit->setClearButtonEnabled(true);
    searchLineEdit->setMaximumWidth(200);
    connect(searchLineEdit, &QLineEdit::returnPressed, this, &MainWindow::find);

    // Replace line edit
    replaceLineEdit = new QLineEdit;
    replaceLineEdit->setPlaceholderText(tr("Replace with..."));
    replaceLineEdit->setClearButtonEnabled(true);
    replaceLineEdit->setMaximumWidth(200);

    // Add widgets to toolbar
    searchToolBar->addWidget(searchLineEdit);
    searchToolBar->addAction(findAct);
    searchToolBar->addAction(findNextAct);
    searchToolBar->addAction(findPreviousAct);
    searchToolBar->addSeparator();
    searchToolBar->addWidget(replaceLineEdit);
    searchToolBar->addAction(replaceAct);
    searchToolBar->addAction(replaceAllAct);
}


void MainWindow::find()
{
    GCodeEdit *editor = activeGCodeEdit();
    if (!editor) return;

    QString searchText = searchLineEdit->text();
    if (searchText.isEmpty()) return;

    bool found = editor->find(searchText);
    if (!found) {
        QMessageBox::information(this, tr("Find"),
                                tr("No matches found for '%1'").arg(searchText));
    }
}

void MainWindow::findNext()
{
    GCodeEdit *editor = activeGCodeEdit();
    if (!editor) return;

    QString searchText = searchLineEdit->text();
    if (searchText.isEmpty()) return;

    bool found = editor->findNext();
    if (!found) {
        QMessageBox::information(this, tr("Find"),
                                tr("No more matches found for '%1'").arg(searchText));
    }
}

void MainWindow::findPrevious()
{
    GCodeEdit *editor = activeGCodeEdit();
    if (!editor) return;

    QString searchText = searchLineEdit->text();
    if (searchText.isEmpty()) return;

    bool found = editor->findPrevious();
    if (!found) {
        QMessageBox::information(this, tr("Find"),
                                tr("No previous matches found for '%1'").arg(searchText));
    }
}

void MainWindow::replace()
{
    GCodeEdit *editor = activeGCodeEdit();
    if (!editor) return;

    QString searchText = searchLineEdit->text();
    QString replaceText = replaceLineEdit->text();

    if (searchText.isEmpty()) return;

    editor->replace(searchText, replaceText);
}

void MainWindow::replaceAll()
{
    GCodeEdit *editor = activeGCodeEdit();
    if (!editor) return;

    QString searchText = searchLineEdit->text();
    QString replaceText = replaceLineEdit->text();

    if (searchText.isEmpty()) return;

    editor->replaceAll(searchText, replaceText);
}


void MainWindow::createToolBars()
{
    fileToolBar = addToolBar(tr("文件"));
    fileToolBar->addAction(newAct);
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(saveAct);
    
//    editToolBar = addToolBar(tr("编辑"));
//    editToolBar->addAction(cutAct);
//    editToolBar->addAction(copyAct);  // 修复：savtToolBar -> editToolBar
//    editToolBar->addAction(pasteAct);

    // Add search toolbar
    createSearchToolBar();

    viewToolBar = addToolBar(tr("View"));
    viewToolBar->addAction(toolTableAct);
    viewToolBar->addAction(pathPlotAct);
    viewToolBar->addAction(loadPlotFileAct);
    viewToolBar->addAction(cncCmdAct);
    viewToolBar->addAction(motionPlotAct);
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("就绪"));
}

void MainWindow::readSettings()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    const QByteArray geometry = settings.value("geometry", QByteArray()).toByteArray();
    if (geometry.isEmpty()) {
        const QRect availableGeometry = screen()->availableGeometry();
        resize(availableGeometry.width() / 3, availableGeometry.height() / 2);
        move((availableGeometry.width() - width()) / 2,
             (availableGeometry.height() - height()) / 2);
    } else {
        restoreGeometry(geometry);
    }
}

void MainWindow::writeSettings()
{
    QSettings settings(QCoreApplication::organizationName(), QCoreApplication::applicationName());
    settings.setValue("geometry", saveGeometry());
}

void MainWindow::setPathDock()
{
    if (!livePlotterDock) {
        livePlotterDock = new QDockWidget(tr("3D Path Viewer"), this);
        livePlotter = new LivePlotter(livePlotterDock);
        livePlotterDock->setWidget(livePlotter);
        addDockWidget(Qt::LeftDockWidgetArea, livePlotterDock);
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect screenGeometry = screen->availableGeometry();
        int oneThirdWidth = screenGeometry.width() / 3;

        // Resize the dock widget
        livePlotterDock->setMinimumWidth(oneThirdWidth);

        // Start with some sample data
        QVector<QVector3D> initialPath;
        for (float t = 0; t < 6.28f; t += 0.1f) {
            float r = 0.5f + 0.1f * t;
            initialPath.append(QVector3D(
                r * cos(t),
                r * sin(t),
                t/10.0f
            ));
        }
        livePlotter->setPath(initialPath);
    }
    else {
        if (livePlotterDock->isHidden()) {
            //Reset the dock for the clear postion when reshow
            removeDockWidget(livePlotterDock);
            addDockWidget(Qt::LeftDockWidgetArea, livePlotterDock);
            if (livePlotterDock->isFloating())
                livePlotterDock->setFloating(false);
            livePlotterDock->show();
        }
    }


}

void MainWindow::setMotionProfileDock()
{
    if (!livePlotterMotionDock) {
        livePlotterMotionDock = new QDockWidget(tr("Motion Analysis Viewer"), this);
        livePlotterMotion = new LivePlotterMotion(livePlotterMotionDock);
        livePlotterMotionDock->setWidget(livePlotterMotion);
        addDockWidget(Qt::LeftDockWidgetArea, livePlotterMotionDock);

        // Calculate 1/3 of screen width
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect screenGeometry = screen->availableGeometry();
        int oneThirdWidth = screenGeometry.width() / 3;

        // Resize the dock widget
        livePlotterMotionDock->setMinimumWidth(oneThirdWidth);

        // Start with some sample data
        QVector<QVector<float>> initialPath;
        QVector<float> tVec;
        QVector<float> xVec;
        QVector<float> yVec;
        QVector<float> zVec;
        for (float t = 0; t < 6.28f; t += 0.1f) {
            float r = 0.5f + 0.1f * t;
            tVec.append(t);
            xVec.append(r * cos(t));
            yVec.append(r * sin(t));
            zVec.append(t / 10.0f);
        }
        initialPath.append(tVec);
        initialPath.append(xVec);
        initialPath.append(yVec);
        initialPath.append(zVec);

        livePlotterMotion->setPath(initialPath);
    }
    else {
        if (livePlotterMotionDock->isHidden()) {
            //Reset the dock for the clear postion when reshow
            removeDockWidget(livePlotterMotionDock);
            addDockWidget(Qt::LeftDockWidgetArea, livePlotterMotionDock);
            if (livePlotterMotionDock->isFloating())
                livePlotterMotionDock->setFloating(false);
            livePlotterMotionDock->show();
        }
    }
}

void MainWindow::setCmdLogDock()
{
    if (!cmdTextDock) {
        cmdTextDock = new QDockWidget(tr("CmdLine"), this);
        cmdTextEdit = new CommandTextEdit(cmdTextDock);

        connect(cmdTextEdit, &CommandTextEdit::commandEntered, this, [this](const QString &cmd) {
            std::string cmdStdStr = cmd.toStdString();
            millIf_->setCmd(cmdStdStr);
        });

        cmdTextDock->setWidget(cmdTextEdit);
        addDockWidget(Qt::RightDockWidgetArea, cmdTextDock);
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect screenGeometry = screen->availableGeometry();
        int oneThirdWidth = screenGeometry.width() / 3;

        // Resize the dock widget
        cmdTextDock->setMinimumWidth(oneThirdWidth);
    }
    else {
        if (cmdTextDock->isHidden()) {
            //Reset the dock for the clear postion when reshow
            removeDockWidget(cmdTextDock);
            addDockWidget(Qt::RightDockWidgetArea, cmdTextDock);
            if (cmdTextDock->isFloating())
                cmdTextDock->setFloating(false);
            cmdTextDock->show();
        }
    }

    if (!logDisplayDock) {
        logTimer = new QTimer(this);
        logDisplayDock = new QDockWidget(tr("LogDisplay"), this);
        logDisplayWidget = new LogDisplayWidget(logDisplayDock);
        logDisplayDock->setWidget(logDisplayWidget);
        addDockWidget(Qt::RightDockWidgetArea, logDisplayDock);
        QScreen *screen = QGuiApplication::primaryScreen();
        QRect screenGeometry = screen->availableGeometry();
        int oneThirdWidth = screenGeometry.width() / 3;

        logTimer->start(100);
        connect (logTimer, &QTimer::timeout, this, [this] {
            std::string log;
            QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ");
            int level = 0;
            while (millIf_->getlog(log, level) == 0) {
                if (level == 0)
                    logDisplayWidget->appendLog(timeStr + QString::fromStdString(log));
                else if (level == 1)
                    logDisplayWidget->appendWarning(timeStr + QString::fromStdString(log));
                else if (level == 2)
                    logDisplayWidget->appendError(timeStr + QString::fromStdString(log));
                else if (level == 3)
                    cmdTextEdit->appendOutput(QString::fromStdString(log));
            }
        });

        // Resize the dock widget
        logDisplayDock->setMinimumWidth(oneThirdWidth);
    }
    else {
        if (logDisplayDock->isHidden()) {
            //Reset the dock for the clear postion when reshow
            removeDockWidget(logDisplayDock);
            addDockWidget(Qt::RightDockWidgetArea, logDisplayDock);
            if (logDisplayDock->isFloating())
                logDisplayDock->setFloating(false);
            logDisplayDock->show();
        }
    }
}

void MainWindow::createBckThread()
{
    millIf_ = IMillTaskInterface::create(EMC_INI_FILE);
    millIf_->initialize();
}

GCodeEdit *MainWindow::activeGCodeEdit()
{
    if (QMdiSubWindow *activeSubWindow = mdiArea->activeSubWindow())
        return qobject_cast<GCodeEdit *>(activeSubWindow->widget());
    return nullptr;
}

QMdiSubWindow *MainWindow::findMdiChild(const QString &fileName)
{
    QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();
    
    const QList<QMdiSubWindow *> subWindows = mdiArea->subWindowList();
    for (QMdiSubWindow *window : subWindows) {
        GCodeEdit *gCodeEdit = qobject_cast<GCodeEdit *>(window->widget());
        if (gCodeEdit && gCodeEdit->windowFilePath() == canonicalFilePath)
            return window;
    }
    return nullptr;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    mdiArea->closeAllSubWindows();
    if (mdiArea->currentSubWindow()) {
        event->ignore();
    } else {
        writeSettings();
        event->accept();
    }
}
