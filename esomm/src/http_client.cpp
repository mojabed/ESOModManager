#include "http_client.h"
#include "logger.h"
#include "pathing.h"

#include <QMutexLocker>
#include <QTimer>

HttpClient::HttpClient(int maxConcurrentDownloads, QObject* parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)),
      m_maxConcurrentDownloads(maxConcurrentDownloads) {
    initHeaders();

    connect(m_networkManager, &QNetworkAccessManager::finished, this, &HttpClient::onResponseReceived);
}

HttpClient::~HttpClient() {
    QMutexLocker locker(&m_qMutex);
    m_downloadQueue.clear();
    if (m_networkManager) {
        m_networkManager->deleteLater();
    }
}

// Mimic the headers of a GET request
void HttpClient::initHeaders() {
    m_headers = {
        {"User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:137.0) Gecko/20100101 Firefox/137.0"},
        {"Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8"},
        {"Accept-Language", "en-US,en;q=0.5"},
        {"Accept-Encoding", "gzip, deflate, br, zstd"},
        {"Connection", "keep-alive"},
        {"Upgrade-Insecure-Requests", "1"},
        {"Sec-Fetch-Dest", "document"},
        {"Sec-Fetch-Mode", "navigate"},
        {"Sec-Fetch-Site", "none"},
        {"Sec-Fetch-User", "?1"}
    };
}

// Prepares the request object
QNetworkRequest HttpClient::createRequest(const QUrl& url) const {
    QNetworkRequest request(url);
    for (const auto& header : m_headers.keys()) {
        request.setRawHeader(header.toUtf8(), m_headers.value(header).toUtf8());
    }
    return request;
}

// Adds download jobs to queue for concurrent downloads
void HttpClient::addDownload(const QUrl& url, const QString& filePath) {
    QMutexLocker locker(&m_qMutex);
    m_downloadQueue.enqueue({ url, filePath });
    processNext();
}

void HttpClient::processNext() {
    QMutexLocker locker(&m_qMutex);

    while (!m_downloadQueue.isEmpty() && m_activeDownloads.size() < m_maxConcurrentDownloads) {
        Download download = m_downloadQueue.dequeue();
        start(download);
    }

    if (m_downloadQueue.isEmpty() && m_activeDownloads.isEmpty()) {
        emit allDownloadsFinished();
    }
}

void HttpClient::start(const Download& download) {
    QNetworkRequest request = createRequest(download.url);
    QNetworkReply* reply = m_networkManager->get(request);

    m_activeDownloads[reply] = download;

    QTimer* timeout = new QTimer(this);
    timeout->setSingleShot(true);
    connect(timeout, &QTimer::timeout, [reply, timeout]() {
        if (reply->isRunning()) {
            reply->abort();
            timeout->deleteLater();
            qCWarning(loggerCategory) << "Request timed out";
        }
        });
    timeout->start(REQUEST_TIMEOUT_MS);

    connect(reply, &QNetworkReply::downloadProgress, this, &HttpClient::onDownloadProgress);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { onResponseReceived(reply); });
}

// Handle lifetime of download
void HttpClient::onResponseReceived(QNetworkReply* reply) {
    if (!m_activeDownloads.contains(reply)) {
        reply->deleteLater();
        return;
    }
    
    Download download = m_activeDownloads.take(reply);
    const QUrl url = reply->request().url();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error()) {
        if (download.retries < MAX_RETRIES) {
            // handle retry
            reply->deleteLater();
            return;
        }
        emit downloadFailed(download.filePath, QString("Network error after %1 retries: %2")
            .arg(download.retries).arg(reply->errorString()));
    } else if (statusCode >= 400) {
        emit downloadFailed(download.filePath, QString("HTTP error %1: %2")
            .arg(statusCode).arg(reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString()));
    } else {
        // Save the file
        QString filePath = download.filePath;
        if (filePath.isEmpty()) {
            filePath = Pathing::getPaths()->getAddonsPath() + "/" + url.fileName();
        }

        if (saveToDisk(filePath, reply)) {
            emit downloadFinished(filePath);
        }
        else {
            emit downloadFailed(filePath, "Failed to save file");
        }
    }

    reply->deleteLater();
    processNext();
}

// Location will be given by caller
bool HttpClient::saveToDisk(const QString& filename, QIODevice* data) {
    QSaveFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(loggerCategory) << "Failed to open file for writing: " << filename;
        return false;
    }
    try {
        file.write(data->readAll());
    } catch (const std::exception& e) {
        // Log and emit error
        emit downloadFailed(filename, QString("Failed to write to file: %1 due to: %2").arg(filename).arg(e.what()));
        qCCritical(loggerCategory) << "Failed to write to file: " << filename << " - " << e.what();

        file.cancelWriting();
        return false;
    }
    return true;
}

void HttpClient::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (reply && m_activeDownloads.contains(reply)) {
        emit downloadProgress(m_activeDownloads[reply].filePath, bytesReceived, bytesTotal);
    }
}

void HttpClient::retryDownload(const Download& download) {
    QMutexLocker locker(&m_qMutex);
    Download retryDownload = download;

    retryDownload.retries++;
    m_downloadQueue.enqueue(retryDownload);
    QTimer::singleShot(1000 * retryDownload.retries, this, &HttpClient::processNext);
}

void HttpClient::setMaxConcurrentDownloads(int max) {
    QMutexLocker locker(&m_qMutex);
    m_maxConcurrentDownloads = qMax(1, max);
    processNext();
}

int HttpClient::activeDownloads() const {
    QMutexLocker locker(&m_qMutex);
    return m_activeDownloads.size();
}