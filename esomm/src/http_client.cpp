#include "http_client.h"
#include "logger.h"
#include "pathing.h"

#include <QMutexLocker>
#include <QTimer>
#include <QSslError>
#include <QEventLoop>
#include <QFileInfo>

// NEED TO REFACTOR FOR CLARITY

// HttpClient will run on a separate thread
HttpClient::HttpClient(int maxConcurrentDownloads, QObject* parent)
    : QObject(parent), m_maxConcurrentDownloads(maxConcurrentDownloads) {

    m_threadPool = new QThreadPool(this);
    m_threadPool->setMaxThreadCount(maxConcurrentDownloads);

    m_networkManager = new QNetworkAccessManager(this);
    m_networkManager->setTransferTimeout(REQUEST_TIMEOUT_MS);

    initHeaders();

    // Signal to handle SSL errors
    connect(m_networkManager, &QNetworkAccessManager::sslErrors,
        this, [this](QNetworkReply* reply, const QList<QSslError>& errors) {
            QString errorStr = "SSL Errors:";
            for (const QSslError& error : errors) {
                errorStr += " " + error.errorString();
            }
            qCWarning(loggerCategory) << errorStr << "for URL:" << reply->url().toString();
        });
}

HttpClient::~HttpClient() {
    {
        QMutexLocker locker(&m_qMutex);
        m_downloadQueue.clear();
    }

    for (QNetworkReply* reply : m_activeDownloads.keys()) {
        if (reply && reply->isRunning()) {
            reply->abort();
        }
    }

    for (auto watcher : m_futureWatchers) {
        if (watcher->isRunning()) {
            watcher->cancel();
        }
        delete watcher;
    }
    m_futureWatchers.clear();

    m_threadPool->waitForDone();

    if (m_networkManager) {
        m_networkManager->deleteLater();
    }
}

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

QNetworkRequest HttpClient::createRequest(const QUrl& url) const {
    QNetworkRequest request(url);

    for (const auto& header : m_headers.keys()) {
        if (header != "Accept-Encoding") {
            request.setRawHeader(header.toUtf8(), m_headers.value(header).toUtf8());
        }
    }

    request.setRawHeader("Accept-Encoding", "identity"); // Request uncompressed content
    return request;
}

void HttpClient::addDownload(const QUrl& url, const QString& filePath) {
    if (!url.isValid()) {
        qCWarning(loggerCategory) << "Invalid URL provided:" << url.toString();
        emit downloadFailed(filePath, "Invalid URL provided");
        return;
    }

    {
        QMutexLocker locker(&m_qMutex);
        m_downloadQueue.enqueue({ url, filePath });
    }
    QTimer::singleShot(0, this, &HttpClient::processDownloadQueue);
}

void HttpClient::checkDownloadQueue() {
    QMutexLocker locker(&m_qMutex);
    int availableSlots = m_maxConcurrentDownloads - m_activeDownloads.size();

    if (availableSlots > 0 && !m_downloadQueue.isEmpty()) {
        locker.unlock();
        processDownloadQueue();
    }
    else if (m_downloadQueue.isEmpty() && m_activeDownloads.isEmpty()) {
        locker.unlock();
        emit allDownloadsFinished();
    }
}

void HttpClient::processDownloadQueue() {
    QVector<Download> downloadsToProcess;

    {
        QMutexLocker locker(&m_qMutex);
        int availableSlots = m_maxConcurrentDownloads - m_activeDownloads.size();

        while (!m_downloadQueue.isEmpty() && availableSlots > 0) {
            downloadsToProcess.append(m_downloadQueue.dequeue());
            availableSlots--;
        }
    }

    for (const auto& download : downloadsToProcess) {
        QFutureWatcher<void>* watcher = new QFutureWatcher<void>(this);
        m_futureWatchers.append(watcher);

        QString filePath = download.filePath;
        connect(watcher, &QFutureWatcher<void>::progressValueChanged, this, [this, filePath](int progress) {
            emit downloadProgress(filePath, progress, 100);
            });

        connect(watcher, &QFutureWatcher<void>::finished, this, [this, watcher]() {
            m_futureWatchers.removeOne(watcher);
            watcher->deleteLater();
            QTimer::singleShot(0, this, &HttpClient::checkDownloadQueue);
        });

        // Begin concurrent job
        QFuture<void> future = QtConcurrent::run(m_threadPool, [this, download]() {
            start(download);
            });

        watcher->setFuture(future);
    }
}

void HttpClient::start(const Download& download) {
    QNetworkRequest request = createRequest(download.url);

    QNetworkReply* reply = nullptr;
    QMetaObject::invokeMethod(this, [this, &reply, request]() {
        reply = m_networkManager->get(request);
        }, Qt::BlockingQueuedConnection);

    if (!reply) {
        qCWarning(loggerCategory) << "Failed to create network reply for URL:" << download.url.toString();
        emit downloadFailed(download.filePath, "Failed to create network request");
        return;
    }

    {
        QMutexLocker locker(&m_qMutex);
        if (m_activeDownloads.contains(reply)) {
            qCWarning(loggerCategory) << "Download already in progress for URL:" << download.url.toString();
            reply->deleteLater();
            return;
        }
        m_activeDownloads[reply] = download;
    }

    connect(reply, &QNetworkReply::downloadProgress, this, &HttpClient::onDownloadProgress, Qt::QueuedConnection);

    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply](QNetworkReply::NetworkError error) {
        qCWarning(loggerCategory) << "Network error occurred:" << error << "-" << reply->errorString();
        }, Qt::QueuedConnection);

    QTimer* timer = nullptr;
    QMetaObject::invokeMethod(this, [&timer]() {
        timer = new QTimer();
        timer->setSingleShot(true);
        }, Qt::BlockingQueuedConnection);

    connect(timer, &QTimer::timeout, reply, &QNetworkReply::abort, Qt::QueuedConnection);
    connect(reply, &QNetworkReply::finished, timer, &QObject::deleteLater, Qt::QueuedConnection);

    QMetaObject::invokeMethod(timer, [timer]() {
        timer->start(REQUEST_TIMEOUT_MS);
        }, Qt::QueuedConnection);

    // Wait for download 
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit, Qt::QueuedConnection);
    loop.exec();

    handleDownloadResult(reply);
    QMetaObject::invokeMethod(this, [reply]() {
        reply->deleteLater();
        }, Qt::QueuedConnection);
}

void HttpClient::handleDownloadResult(QNetworkReply* reply) {
    QMutexLocker locker(&m_qMutex);

    if (!m_activeDownloads.contains(reply)) return;

    Download download = m_activeDownloads.take(reply);
    locker.unlock();

    const QUrl url = reply->request().url();
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    QString filePath = download.filePath;
    if (filePath.isEmpty()) {
        filePath = Pathing::getPaths()->getAddonsPath() + "/" + url.fileName();
    }

    // Helper function to emit signals in the main thread
    auto emitSignal = [this](const QString& filePath, bool success, const QString& errorMsg = QString()) {
        QMetaObject::invokeMethod(this, [this, filePath, success, errorMsg]() {
            if (success) {
                emit downloadFinished(filePath);
            } else {
                emit downloadFailed(filePath, errorMsg);
            }
        }, Qt::QueuedConnection);
    };

    if (reply->error()) {
        if (download.retries < MAX_RETRIES) {
            qCInfo(loggerCategory) << "Retrying download for" << url.toString()
                << "Attempt" << (download.retries + 1) << "of" << MAX_RETRIES;
            retryDownload(download);
            return;
        }

        // Download failed
        QString errorMsg = QString("Network error after %1 retries: %2")
            .arg(download.retries).arg(reply->errorString());
        emitSignal(filePath, false, errorMsg);

    } else if (statusCode >= 400) { // HTTP error
        QString errorMsg = QString("HTTP error %1: %2")
            .arg(statusCode).arg(reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString());
        emitSignal(filePath, false, errorMsg);

    } else { // Success
        if (saveToDisk(filePath, reply)) {
            emitSignal(filePath, true);
        } else {
            emitSignal(filePath, false, "Failed to save file");
        }
    }
}

// Location will be given by caller
bool HttpClient::saveToDisk(const QString& filename, QIODevice* data) {
    QSaveFile file(filename);

    if (!file.open(QIODevice::WriteOnly)) {
        qCWarning(loggerCategory) << "Failed to open file for writing: " << filename;
        return false;
    }

    try {
        QNetworkReply* reply = qobject_cast<QNetworkReply*>(data);
        QByteArray content;

        if (reply) {
            //// Check if the response is compressed
            //QByteArray encoding = reply->rawHeader("Content-Encoding");
            //QByteArray contentType = reply->rawHeader("Content-Type");
            //qCDebug(loggerCategory) << "Content-Encoding:" << encoding;
            //qCDebug(loggerCategory) << "Content-Type:" << contentType;
            content = reply->readAll();
        }

        if (content.isEmpty() && data->size() > 0) {
            qCWarning(loggerCategory) << "Empty content received for non-empty data source";
            return false;
        }

        qint64 bytesWritten = file.write(content);
        if (bytesWritten != content.size()) {
            qCWarning(loggerCategory) << "Failed to write all data:" << bytesWritten << "of" << content.size();
            file.cancelWriting();
            return false;
        }

        if (!file.commit()) {
            qCWarning(loggerCategory) << "Failed to commit file:" << filename;
            return false;
        }
    } catch (const std::exception& e) {
        //emit downloadFailed(filename, QString("Failed to write to file: %1 due to: %2").arg(filename).arg(e.what()));
        emit downloadFailed(filename, QString("Failed to write file: %1").arg(e.what()));
        qCCritical(loggerCategory) << "Failed to write to file: " << filename << " - " << e.what();

        file.cancelWriting();
        return false;
    } catch (...) {
        emit downloadFailed(filename, "Unknown error while writing file");
        qCCritical(loggerCategory) << "Unknown exception while writing file:" << filename;

        file.cancelWriting();
        return false;
    }
    return true;
}

void HttpClient::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    QString filePath;

    {
        QMutexLocker locker(&m_qMutex);
        if (reply && m_activeDownloads.contains(reply)) {
            //emit downloadProgress(m_activeDownloads[reply].filePath, bytesReceived, bytesTotal);
            filePath = m_activeDownloads[reply].filePath;
        }
        else {
            qCWarning(loggerCategory) << "Download progress received for unknown reply";
            return;
        }
    }

    emit downloadProgress(filePath, bytesReceived, bytesTotal);
}

void HttpClient::retryDownload(const Download& download) {
    Download retryDownload = download;

    retryDownload.retries++;

    QTimer::singleShot(1000 * retryDownload.retries, this, [this, retryDownload]() {
        QMutexLocker locker(&m_qMutex);
        m_downloadQueue.enqueue(retryDownload);
        locker.unlock();
        checkDownloadQueue();
    });
}

void HttpClient::setMaxConcurrentDownloads(int max) {
    {
        QMutexLocker locker(&m_qMutex);
        m_maxConcurrentDownloads = qMax(1, max);

        m_threadPool->setMaxThreadCount(m_maxConcurrentDownloads);
    }
    checkDownloadQueue();
}

int HttpClient::activeDownloads() const {
    QMutexLocker locker(&m_qMutex);
    return m_activeDownloads.size();
}