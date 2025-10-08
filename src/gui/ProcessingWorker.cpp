#include "ProcessingWorker.h"
#include <opencv2/imgproc.hpp>
#include <QDebug>

ProcessingWorker::ProcessingWorker(QObject *parent)
    : QObject(parent), operationType(CUSTOM), cancelled(false) {
}

ProcessingWorker::~ProcessingWorker() {
}

void ProcessingWorker::setOperation(OperationType type) {
    operationType = type;
}

void ProcessingWorker::setCustomOperation(ProcessingFunction func) {
    customFunction = func;
    operationType = CUSTOM;
}

void ProcessingWorker::setInputImage(const cv::Mat &image) {
    inputImage = image.clone();
}

void ProcessingWorker::setParameter(const QString &key, double value) {
    parameters[key] = value;
}

void ProcessingWorker::cancel() {
    cancelled = true;
}

void ProcessingWorker::process() {
    if (inputImage.empty()) {
        emit operationFailed("Input image is empty");
        return;
    }

    cancelled = false;
    cv::Mat result;
    int progress = 0;

    try {
        emit statusMessage("Processing started...");
        emit progressUpdated(0);

        switch (operationType) {
            case SKELETONIZE:
                result = processSkeletonize(progress);
                break;
            case FFT_FILTER:
                result = processFFTFilter(progress);
                break;
            case EXTRACT_MINUTIAE:
                result = processExtractMinutiae(progress);
                break;
            case BINARIZE:
                result = processBinarize(progress);
                break;
            case GAUSSIAN_BLUR:
                result = processGaussianBlur(progress);
                break;
            case SHARPEN:
                result = processSharpen(progress);
                break;
            case CLAHE:
                result = processCLAHE(progress);
                break;
            case EQUALIZE_HISTOGRAM:
                result = processEqualizeHistogram(progress);
                break;
            case CUSTOM:
                if (customFunction) {
                    result = customFunction(inputImage, progress);
                } else {
                    emit operationFailed("No custom function defined");
                    return;
                }
                break;
        }

        if (!cancelled && !result.empty()) {
            emit progressUpdated(100);
            emit statusMessage("Processing completed successfully");
            emit operationCompleted(result);
        } else if (cancelled) {
            emit statusMessage("Processing cancelled");
        } else {
            emit operationFailed("Processing returned empty result");
        }

    } catch (const cv::Exception &e) {
        emit operationFailed(QString("OpenCV error: %1").arg(e.what()));
    } catch (const std::exception &e) {
        emit operationFailed(QString("Error: %1").arg(e.what()));
    }
}

cv::Mat ProcessingWorker::processSkeletonize(int &progress) {
    emit statusMessage("Skeletonizing image...");

    cv::Mat skeleton = cv::Mat::zeros(inputImage.size(), CV_8UC1);
    cv::Mat temp;
    cv::Mat eroded;
    cv::Mat element = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));

    cv::Mat img = inputImage.clone();
    int maxIterations = 100; // Limite de iterações
    int iteration = 0;

    bool done = false;
    while (!done && !cancelled && iteration < maxIterations) {
        cv::erode(img, eroded, element);
        cv::dilate(eroded, temp, element);
        cv::subtract(img, temp, temp);
        cv::bitwise_or(skeleton, temp, skeleton);
        eroded.copyTo(img);

        done = (cv::countNonZero(img) == 0);

        iteration++;
        progress = std::min(99, (iteration * 100) / maxIterations);
        emit progressUpdated(progress);
    }

    return skeleton;
}

cv::Mat ProcessingWorker::processFFTFilter(int &progress) {
    emit statusMessage("Applying FFT filter...");
    progress = 20;
    emit progressUpdated(progress);

    // Implementação básica - pode ser expandida com máscara
    cv::Mat padded;
    int m = cv::getOptimalDFTSize(inputImage.rows);
    int n = cv::getOptimalDFTSize(inputImage.cols);
    cv::copyMakeBorder(inputImage, padded, 0, m - inputImage.rows, 0, n - inputImage.cols,
                       cv::BORDER_CONSTANT, cv::Scalar::all(0));

    progress = 40;
    emit progressUpdated(progress);

    cv::Mat planes[] = {cv::Mat_<float>(padded), cv::Mat::zeros(padded.size(), CV_32F)};
    cv::Mat complexImage;
    cv::merge(planes, 2, complexImage);

    progress = 60;
    emit progressUpdated(progress);

    cv::dft(complexImage, complexImage);

    progress = 80;
    emit progressUpdated(progress);

    // Aplicar filtro passa-baixa simples (pode ser customizado)
    // TODO: Adicionar suporte a máscara customizada

    cv::Mat result;
    cv::dft(complexImage, result, cv::DFT_INVERSE | cv::DFT_REAL_OUTPUT);
    cv::normalize(result, result, 0, 255, cv::NORM_MINMAX, CV_8UC1);

    // Remover padding
    result = result(cv::Rect(0, 0, inputImage.cols, inputImage.rows));

    return result;
}

cv::Mat ProcessingWorker::processExtractMinutiae(int &progress) {
    emit statusMessage("Extracting minutiae...");

    // Este método retorna a imagem original com minúcias desenhadas
    // A extração real seria feita pelo MinutiaeExtractor
    progress = 50;
    emit progressUpdated(progress);

    // Placeholder - a extração real deve ser feita na MainWindow
    // usando o MinutiaeExtractor
    cv::Mat result = inputImage.clone();

    progress = 100;
    emit progressUpdated(progress);

    return result;
}

cv::Mat ProcessingWorker::processBinarize(int &progress) {
    emit statusMessage("Binarizing image...");
    progress = 30;
    emit progressUpdated(progress);

    cv::Mat result;
    double threshold = parameters.value("threshold", -1.0);

    if (threshold < 0) {
        // Usar Otsu
        cv::threshold(inputImage, result, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    } else {
        cv::threshold(inputImage, result, threshold, 255, cv::THRESH_BINARY);
    }

    progress = 100;
    emit progressUpdated(progress);

    return result;
}

cv::Mat ProcessingWorker::processGaussianBlur(int &progress) {
    emit statusMessage("Applying Gaussian blur...");
    progress = 40;
    emit progressUpdated(progress);

    double sigma = parameters.value("sigma", 1.0);
    cv::Mat result;
    cv::GaussianBlur(inputImage, result, cv::Size(0, 0), sigma, sigma);

    progress = 100;
    emit progressUpdated(progress);

    return result;
}

cv::Mat ProcessingWorker::processSharpen(int &progress) {
    emit statusMessage("Sharpening image...");
    progress = 40;
    emit progressUpdated(progress);

    double strength = parameters.value("strength", 1.0);

    cv::Mat kernel = (cv::Mat_<float>(3, 3) <<
        0, -1 * strength, 0,
        -1 * strength, 1 + 4 * strength, -1 * strength,
        0, -1 * strength, 0);

    cv::Mat result;
    cv::filter2D(inputImage, result, -1, kernel);

    progress = 100;
    emit progressUpdated(progress);

    return result;
}

cv::Mat ProcessingWorker::processCLAHE(int &progress) {
    emit statusMessage("Applying CLAHE...");
    progress = 30;
    emit progressUpdated(progress);

    double clipLimit = parameters.value("clipLimit", 2.0);
    int tileSize = parameters.value("tileSize", 8);

    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
    clahe->setClipLimit(clipLimit);
    clahe->setTilesGridSize(cv::Size(tileSize, tileSize));

    progress = 60;
    emit progressUpdated(progress);

    cv::Mat result;
    clahe->apply(inputImage, result);

    progress = 100;
    emit progressUpdated(progress);

    return result;
}

cv::Mat ProcessingWorker::processEqualizeHistogram(int &progress) {
    emit statusMessage("Equalizing histogram...");
    progress = 40;
    emit progressUpdated(progress);

    cv::Mat result;
    cv::equalizeHist(inputImage, result);

    progress = 100;
    emit progressUpdated(progress);

    return result;
}
