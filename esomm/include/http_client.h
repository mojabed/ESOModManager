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
#include <QJsonObject>

constexpr int REQUEST_TIMEOUT_MS = 12000;
constexpr int MAX_RETRIES = 3;

class HttpClient : public QObject {
    Q_OBJECT

public:
    explicit HttpClient(int maxConcurrentDownloads = 3, QObject* parent = nullptr);
    ~HttpClient();

    void addDownload(const QUrl& url, const QString& filePath);
    void setMaxConcurrentDownloads(int max);
    
    int activeDownloads() const;

signals:
    void requestCompleted(const QJsonObject &response);
    void downloadProgress(const QString& filePath, qint64 bytesReceived, qint64 bytesTotal);
    void downloadFinished(const QString& filePath);
    void downloadFailed(const QString& filePath, const QString& errorString);
    void allDownloadsFinished();

private slots:
    void onResponseReceived(QNetworkReply* reply);
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);

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

    int m_maxConcurrentDownloads;
    mutable QMutex m_qMutex;

    void initHeaders();
    bool saveToDisk(const QString& filename, QIODevice* data);
    void processNext();
    void start(const Download& download);
    void retryDownload(const Download& download);

    QNetworkRequest createRequest(const QUrl& url) const;
};