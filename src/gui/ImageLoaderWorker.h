#ifndef IMAGELOADERWORKER_H
#define IMAGELOADERWORKER_H

#include <QThread>
#include <QStringList>
#include <QMutex>
#include <opencv2/opencv.hpp>

namespace FingerprintEnhancer {

/**
 * @brief Worker para carregar múltiplas imagens em thread separada
 * 
 * Permite carregar várias imagens sem travar a interface, reportando
 * progresso e resultados através de signals
 */
class ImageLoaderWorker : public QThread {
    Q_OBJECT

public:
    struct ImageLoadResult {
        QString filePath;
        cv::Mat image;
        bool success;
        QString errorMessage;
    };

    explicit ImageLoaderWorker(const QStringList& filePaths, QObject *parent = nullptr);
    ~ImageLoaderWorker();

    void run() override;
    void cancel();

signals:
    void progressUpdated(int current, int total, const QString& currentFile);
    void imageLoaded(const QString& filePath, const cv::Mat& image);
    void loadingFailed(const QString& filePath, const QString& error);
    void allImagesLoaded(int successCount, int failCount);
    void finished();

private:
    QStringList filePaths;
    QMutex mutex;
    bool cancelled;
};

} // namespace FingerprintEnhancer

#endif // IMAGELOADERWORKER_H
