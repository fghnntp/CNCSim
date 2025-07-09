//#include "gcode_loader.h"
//#include <QFile>
//#include <QTextStream>
//#include <QDebug>
//#include <dlfcn.h>

//GCodeLoader::GCodeLoader(QObject *parent) : QObject(parent) {
//}

//GCodeLoader::~GCodeLoader() {
//}

//bool GCodeLoader::initialize(const QString& iniFile) {
//    try {
//        // Load rs274ngc shared library
//        interpreter.reset(interp_from_shlib("librs274.so"));
//        if (!interpreter) {
//            lastError = "Failed to load rs274ngc library";
//            return false;
//        }
        
//        // Initialize interpreter
//        if (!iniFile.isEmpty()) {
//            if (interpreter->ini_load(iniFile.toLocal8Bit().data()) != 0) {
//                lastError = "Failed to load INI file";
//                return false;
//            }
//        }
        
//        if (interpreter->init() != 0) {
//            lastError = "Failed to initialize interpreter";
//            return false;
//        }
        
//        // Setup canon
//        if (!setupCanon()) {
//            return false;
//        }
        
//        return true;
//    } catch (const std::exception& e) {
//        lastError = QString("Exception during initialization: %1").arg(e.what());
//        return false;
//    }
//}

//bool GCodeLoader::setupCanon() {
//    canon = std::make_unique<PathCanon>();
    
//    // Set the canon in the interpreter
//    // Note: You may need to access interpreter internals or use a different approach
//    // depending on the exact rs274ngc implementation
    
//    return true;
//}

//bool GCodeLoader::loadFile(const QString& filename) {
//    if (!interpreter || !canon) {
//        lastError = "Interpreter not initialized";
//        return false;
//    }
    
//    canon->clearPath();
    
//    QFile file(filename);
//    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
//        lastError = QString("Cannot open file: %1").arg(filename);
//        return false;
//    }
    
//    QTextStream in(&file);
//    QStringList lines = in.readAll().split('\n');
    
//    try {
//        // Open file in interpreter
//        if (interpreter->open(filename.toLocal8Bit().data()) != 0) {
//            lastError = "Failed to open file in interpreter";
//            return false;
//        }
        
//        int totalLines = lines.size();
//        for (int i = 0; i < totalLines; ++i) {
//            QString line = lines[i].trimmed();
//            if (!line.isEmpty() && !line.startsWith(';')) {
//                processLine(line, i + 1);
//            }
            
//            if (i % 100 == 0) {
//                emit loadingProgress((i * 100) / totalLines);
//            }
//        }
        
//        interpreter->close();
//        emit loadingProgress(100);
//        emit pathUpdated();
//        emit loadingFinished(true);
        
//        return true;
//    } catch (const std::exception& e) {
//        lastError = QString("Exception during file loading: %1").arg(e.what());
//        emit loadingFinished(false);
//        return false;
//    }
//}

//void GCodeLoader::processLine(const QString& line, int lineNumber) {
//    // Execute the line through the interpreter
//    int result = interpreter->execute(line.toLocal8Bit().data(), lineNumber);
    
//    if (result != 0) {
//        char errorBuf[256];
//        interpreter->error_text(result, errorBuf, sizeof(errorBuf));
//        qWarning() << "G-code error at line" << lineNumber << ":" << errorBuf;
//    }
//}

//const std::vector<PathSegment>& GCodeLoader::getPathSegments() const {
//    static std::vector<PathSegment> empty;
//    return canon ? canon->getPathSegments() : empty;
//}

//void GCodeLoader::reset() {
//    if (canon) {
//        canon->clearPath();
//    }
//    if (interpreter) {
//        interpreter->reset();
//    }
//}
