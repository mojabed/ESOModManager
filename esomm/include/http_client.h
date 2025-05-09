#pragma once

#include <QObject>
#include <QString>
#include <QSaveFile>
#include <QMutex>
#include <QQueue>
#include <QMap>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>
#include <QThreadPool>

constexpr int REQUEST_TIMEOUT_MS = 20000;
constexpr int MAX_RETRIES = 3;

// Singleton API handler
class HttpClient : public QObject {
    Q_OBJECT

public:
    explicit HttpClient(int maxConcurrentDownloads = 3, QObject* parent = nullptr);
    ~HttpClient();

    void addDownload(const QUrl& url, const QString& filePath);
    void setMaxConcurrentDownloads(int max);
    
    int activeDownloads() const;

signals:
    void downloadProgress(const QString& filePath, qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(const QString& filePath);
    void downloadFailed(const QString& filePath, const QString& errorString);
    void allDownloadsFinished();

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void checkDownloadQueue();

private:
    struct Download {
        QUrl url;
        QString filePath;
        int retries = 0;
    };

    QNetworkAccessManager* m_networkManager;
    QMap<QString, QString> m_headers;
    QQueue<Download> m_downloadQueue;
    QMap<QNetworkReply*, Download> m_activeDownloads;
    QList<QFutureWatcher<void>*> m_futureWatchers;
    QThreadPool* m_threadPool;

    int m_maxConcurrentDownloads = 3;
    mutable QMutex m_qMutex;

    void initHeaders();
    bool saveToDisk(const QString& filename, QIODevice* data);
    void processDownloadQueue();
    void start(const Download& download);
    void retryDownload(const Download& download);
    void handleDownloadResult(QNetworkReply* reply);

    QNetworkRequest createRequest(const QUrl& url) const;
};