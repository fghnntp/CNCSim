// #include "mainwindow.h"
// #include "./ui_mainwindow.h"

// MainWindow::MainWindow(QWidget *parent)
//     : QMainWindow(parent)
//     , ui(new Ui::MainWindow)
// {
//     ui->setupUi(this);
// }

// MainWindow::~MainWindow()
// {
//     delete ui;
// }

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QCloseEvent>
#include <QSettings>
#include <QSignalMapper>
#include <QScreen>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
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
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::newFile()
{
    GCodeEdit *child = new GCodeEdit;
    QMdiSubWindow *subWindow = mdiArea->addSubWindow(child);
    
    child->setWindowTitle(tr("未命名%1").arg(mdiArea->subWindowList().size()));
    child->show();
    
    // 连接子窗口信号
    connect(child, &GCodeEdit::textChanged, this, &MainWindow::updateMenus);
}

void MainWindow::openFile()
{
    // const QString fileName = QFileDialog::getOpenFileName(this,
    //     tr("打开G代码文件"), "",
    //     tr("G代码文件 (*.gcode *.nc *.cnc);;所有文件 (*)"));
    
    // if (!fileName.isEmpty()) {
    //     QMdiSubWindow *existing = findMdiChild(fileName);
    //     if (existing) {
    //         mdiArea->setActiveSubWindow(existing);
    //         return;
    //     }
        
    //     GCodeEdit *child = new GCodeEdit;
    //     if (child->loadFromFile(fileName)) {
    //         statusBar()->showMessage(tr("文件已加载"), 2000);
    //         QMdiSubWindow *subWindow = mdiArea->addSubWindow(child);
    //         child->setWindowTitle(QFileInfo(fileName).fileName());
    //         child->show();
            
    //         connect(child, &GCodeEdit::textChanged, this, &MainWindow::updateMenus);
    //     } else {
    //         child->deleteLater();
    //         QMessageBox::warning(this, tr("错误"), tr("无法打开文件"));
    //     }
    // }
    const QString fileName = QFileDialog::getOpenFileName(this,
        tr("打开G代码文件"), "",
        tr("G代码文件 (*.gcode *.nc *.cnc);;所有文件 (*)"));
    
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
            child->setWindowTitle(QFileInfo(fileName).fileName());
            subWindow->setWindowTitle(QFileInfo(fileName).fileName());
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
    // if (activeGCodeEdit() && activeGCodeEdit()->saveToFile("")) {
    //     statusBar()->showMessage(tr("文件已保存"), 2000);
    // }
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
    // GCodeEdit *child = activeGCodeEdit();
    // if (!child)
    //     return;
        
    // const QString fileName = QFileDialog::getSaveFileName(this,
    //     tr("保存G代码文件"), "",
    //     tr("G代码文件 (*.gcode *.nc *.cnc);;所有文件 (*)"));
    
    // if (!fileName.isEmpty()) {
    //     if (child->saveToFile(fileName)) {
    //         statusBar()->showMessage(tr("文件已保存"), 2000);
    //         child->setWindowTitle(QFileInfo(fileName).fileName());
    //     }
    // }

     GCodeEdit *child = activeGCodeEdit();
    if (!child)
        return;
        
    const QString fileName = QFileDialog::getSaveFileName(this,
        tr("保存G代码文件"), child->getCurrentFilePath(),
        tr("G代码文件 (*.gcode *.nc *.cnc);;所有文件 (*)"));
    
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
    newAct = new QAction(QIcon(":/images/new.png"), tr("&新建"), this);
    newAct->setShortcuts(QKeySequence::New);
    newAct->setStatusTip(tr("创建新的G代码文件"));
    connect(newAct, &QAction::triggered, this, &MainWindow::newFile);
    
    openAct = new QAction(QIcon(":/images/open.png"), tr("&打开..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("打开现有G代码文件"));
    connect(openAct, &QAction::triggered, this, &MainWindow::openFile);
    
    saveAct = new QAction(QIcon(":/images/save.png"), tr("&保存"), this);
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
    
    cutAct = new QAction(QIcon(":/images/cut.png"), tr("剪切"), this);
    cutAct->setShortcuts(QKeySequence::Cut);
    cutAct->setStatusTip(tr("剪切当前选择的内容到剪贴板"));
    connect(cutAct, &QAction::triggered, this, [this]() {
        if (activeGCodeEdit()) activeGCodeEdit()->cut();
    });
    
    copyAct = new QAction(QIcon(":/images/copy.png"), tr("复制"), this);
    copyAct->setShortcuts(QKeySequence::Copy);
    copyAct->setStatusTip(tr("复制当前选择的内容到剪贴板"));
    connect(copyAct, &QAction::triggered, this, [this]() {
        if (activeGCodeEdit()) activeGCodeEdit()->copy();
    });
    
    pasteAct = new QAction(QIcon(":/images/paste.png"), tr("粘贴"), this);
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
    
    helpMenu = menuBar()->addMenu(tr("帮助"));
    helpMenu->addAction(aboutQtAct);
}

void MainWindow::createToolBars()
{
    // fileToolBar = addToolBar(tr("文件"));
    // fileToolBar->addAction(newAct);
    // fileToolBar->addAction(openAct);
    // fileToolBar->addAction(saveAct);
    
    // editToolBar = addToolBar(tr("编辑"));
    // editToolBar->addAction(cutAct);
    // savtToolBar->addAction(copyAct);
    // editToolBar->addAction(pasteAct);
    fileToolBar = addToolBar(tr("文件"));
    fileToolBar->addAction(newAct);
    fileToolBar->addAction(openAct);
    fileToolBar->addAction(saveAct);
    
    editToolBar = addToolBar(tr("编辑"));
    editToolBar->addAction(cutAct);
    editToolBar->addAction(copyAct);  // 修复：savtToolBar -> editToolBar
    editToolBar->addAction(pasteAct);
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
