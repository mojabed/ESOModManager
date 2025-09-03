#include "logger.h"
#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

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

void Logger::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QString formattedMsg = qFormatLogMessage(type, context, msg);

    logStream << formattedMsg << Qt::endl;

    // stderr pipe for debugging
#ifdef QT_DEBUG
    QTextStream(stderr) << msg << Qt::endl;
#endif
}

void Logger::init() {  
   QString logDirPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/logs";  
   QDir logDir(logDirPath);  

   if (!logDir.exists()) {  
       fprintf(stderr, "Creating logs directory at: %s\n", logDirPath.toUtf8().constData());  
       logDir.mkpath(logDirPath);  
   }  

   qSetMessagePattern("%{time yyyy-MM-dd hh:mm:ss.zzz} [%{type}] (%{file}:%{line}) %{message}");
   QString filePath = logDirPath + "/esomm.log";  
   fprintf(stderr, "Log file path: %s\n", filePath.toUtf8().constData());  
   fflush(stderr);  

   // write to esomm.log  
   logFile.setFileName(filePath);  
   if (logFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Append)) {
       logStream.setDevice(&logFile);

       logStream << Qt::endl;

       qInstallMessageHandler(Logger::messageHandler);

       logStream << "<==========> ESOMM started at "
           << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")
           << " <==========>" << Qt::endl;
   } else {  
       fprintf(stderr, "Failed to open log file: %s (error: %s)\n",  
           filePath.toUtf8().constData(),  
           logFile.errorString().toUtf8().constData());  
       return;  
   }

   logStream << "Logger initialized, Qt version:" << QT_VERSION_STR << Qt::endl;
}