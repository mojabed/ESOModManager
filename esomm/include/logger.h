#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <QtWidgets/QApplication>
#include <QFile>
#include <QTextStream>
#include <QObject>
#include <QLoggingCategory>
#include <QMutex>

class Logger : public QObject {
    Q_OBJECT
    Q_DECLARE_LOGGING_CATEGORY(loggerCategory)
public:
    explicit Logger(QObject *parent = nullptr);
    ~Logger();

    static void init(const QString& filePath = "logs/esomm.log");
    static void messageHandler(QtMsgType type, 
        const QMessageLogContext& context, const QString& msg);

private:
    static QFile logFile;
    static QTextStream logStream;
};