#include "logger.h"
#include <QDateTime>
#include <QDir>

static QMutex mutex;
QFile Logger::logFile;
QTextStream Logger::logStream;
Q_LOGGING_CATEGORY(loggerCategory, "esomm.core")

Logger::Logger(QObject* parent) : QObject(parent) {}

Logger::~Logger() {
    if (logFile.isOpen()) {
        logStream.flush();
        logFile.close();
    }
}

void Logger::init(const QString& filePath) {
    // create logs directory
    QDir logDir = QFileInfo(filePath).absoluteDir();
    if (!logDir.exists()) {
        logDir.mkpath(logDir.absolutePath());
    }

    // write to esomm.log
    logFile.setFileName(filePath);
    if (logFile.open(QIODevice::WriteOnly)) {
        logStream.setDevice(&logFile);
        qInstallMessageHandler(Logger::messageHandler);
        qSetMessagePattern("%{time yyyy-MM-dd hh:mm:ss} [%{type}] (%{file}:%{line}): %{message} %{if-critical}%{backtrace}%{endif}");
    }
    else {
        qWarning() << "Failed to open log file:" << filePath;
    }
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QMutexLocker locker(&mutex);
    logStream << qFormatLogMessage(type, context, msg);

    // stderr pipe for debugging
    #ifdef QT_DEBUG
    QTextStream(stderr) << msg << Qt::endl;
    #endif
}