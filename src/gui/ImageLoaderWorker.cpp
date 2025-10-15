#include "ImageLoaderWorker.h"
#include <QFileInfo>
#include <QDebug>

namespace FingerprintEnhancer {

ImageLoaderWorker::ImageLoaderWorker(const QStringList& filePaths, QObject *parent)
    : QThread(parent), filePaths(filePaths), cancelled(false) {
}

ImageLoaderWorker::~ImageLoaderWorker() {
    cancel();
    wait();
}

void ImageLoaderWorker::run() {
    int total = filePaths.size();
    int successCount = 0;
    int failCount = 0;

    for (int i = 0; i < total; ++i) {
        // Verificar cancelamento
        {
            QMutexLocker locker(&mutex);
            if (cancelled) {
                break;
            }
        }

        QString filePath = filePaths[i];
        QFileInfo fileInfo(filePath);
        
        emit progressUpdated(i + 1, total, fileInfo.fileName());

        // Carregar imagem em cores originais (UNCHANGED mantém o formato original)
        // Se for colorida, permanece colorida. Se for grayscale, permanece grayscale
        cv::Mat image = cv::imread(filePath.toStdString(), cv::IMREAD_UNCHANGED);
        
        if (image.empty()) {
            QString error = QString("Falha ao carregar a imagem: formato não suportado ou arquivo corrompido");
            emit loadingFailed(filePath, error);
            failCount++;
        } else {
            // Converter para BGR se for grayscale (para consistência no projeto)
            // Mas manter cores originais se já for colorida
            if (image.channels() == 1) {
                cv::Mat colorImage;
                cv::cvtColor(image, colorImage, cv::COLOR_GRAY2BGR);
                emit imageLoaded(filePath, colorImage);
            } else {
                emit imageLoaded(filePath, image);
            }
            successCount++;
        }

        // Pequeno delay para não sobrecarregar a UI thread com signals
        msleep(10);
    }

    emit allImagesLoaded(successCount, failCount);
    emit finished();
}

void ImageLoaderWorker::cancel() {
    QMutexLocker locker(&mutex);
    cancelled = true;
}

} // namespace FingerprintEnhancer
