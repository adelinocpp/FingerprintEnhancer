#ifndef PROCESSINGWORKER_H
#define PROCESSINGWORKER_H

#include <QObject>
#include <QThread>
#include <QMap>
#include <QString>
#include <functional>
#include <opencv2/opencv.hpp>

/**
 * @brief Worker para processar operações pesadas em thread separada
 *
 * Esta classe permite executar operações de processamento de imagem
 * em uma thread separada para não travar a interface gráfica.
 */
class ProcessingWorker : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Tipo de operação a ser executada
     */
    enum OperationType {
        SKELETONIZE,
        FFT_FILTER,
        EXTRACT_MINUTIAE,
        BINARIZE,
        GAUSSIAN_BLUR,
        SHARPEN,
        CLAHE,
        EQUALIZE_HISTOGRAM,
        CUSTOM
    };

    /**
     * @brief Função customizada para processamento
     */
    using ProcessingFunction = std::function<cv::Mat(const cv::Mat&, int&)>;

    explicit ProcessingWorker(QObject *parent = nullptr);
    ~ProcessingWorker();

    /**
     * @brief Configura a operação a ser executada
     */
    void setOperation(OperationType type);
    void setCustomOperation(ProcessingFunction func);
    void setInputImage(const cv::Mat &image);
    void setParameter(const QString &key, double value);

signals:
    void progressUpdated(int percentage);
    void operationCompleted(cv::Mat result);
    void operationFailed(QString errorMessage);
    void statusMessage(QString message);

public slots:
    void process();
    void cancel();

private:
    OperationType operationType;
    ProcessingFunction customFunction;
    cv::Mat inputImage;
    QMap<QString, double> parameters;
    bool cancelled;

    // Métodos de processamento específicos
    cv::Mat processSkeletonize(int &progress);
    cv::Mat processFFTFilter(int &progress);
    cv::Mat processExtractMinutiae(int &progress);
    cv::Mat processBinarize(int &progress);
    cv::Mat processGaussianBlur(int &progress);
    cv::Mat processSharpen(int &progress);
    cv::Mat processCLAHE(int &progress);
    cv::Mat processEqualizeHistogram(int &progress);
};

#endif // PROCESSINGWORKER_H
