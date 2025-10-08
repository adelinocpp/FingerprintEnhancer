#include "AFISMatcher.h"
#include "../core/MinutiaeExtractor.h"
#include "../core/ImageProcessor.h"
#include <QDir>
#include <QFileInfo>
#include <QtConcurrent>
#include <cmath>
#include <algorithm>

AFISMatcher::AFISMatcher() {
    config = AFISMatchConfig();
}

AFISMatcher::~AFISMatcher() {
}

void AFISMatcher::setConfig(const AFISMatchConfig& newConfig) {
    config = newConfig;
}

bool AFISMatcher::loadDatabase(const QString& dbPath) {
    QDir dir(dbPath);
    if (!dir.exists()) {
        return false;
    }

    databasePath = dbPath;
    clearDatabase();

    // Suportar múltiplos formatos de imagem
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jpeg" << "*.tiff" << "*.tif"
            << "*.bmp" << "*.PNG" << "*.JPG" << "*.JPEG";

    QFileInfoList files = dir.entryInfoList(filters, QDir::Files);

    for (const QFileInfo& fileInfo : files) {
        addCandidateImage(fileInfo.absoluteFilePath());
    }

    return !databaseMinutiae.isEmpty();
}

bool AFISMatcher::addCandidateImage(const QString& imagePath) {
    // Carregar imagem
    cv::Mat image = cv::imread(imagePath.toStdString(), cv::IMREAD_GRAYSCALE);
    if (image.empty()) {
        return false;
    }

    // Extrair minúcias
    QVector<MinutiaeData> minutiae = extractMinutiaeFromImage(image);

    if (minutiae.isEmpty()) {
        return false;
    }

    // Armazenar na base de dados
    QString candidateId = QFileInfo(imagePath).fileName();
    databaseMinutiae[candidateId] = minutiae;
    candidatePaths[candidateId] = imagePath;

    return true;
}

bool AFISMatcher::addCandidateMinutiae(const QString& candidateId,
                                       const QVector<MinutiaeData>& minutiae) {
    if (candidateId.isEmpty() || minutiae.isEmpty()) {
        return false;
    }

    databaseMinutiae[candidateId] = minutiae;
    return true;
}

void AFISMatcher::clearDatabase() {
    databaseMinutiae.clear();
    candidatePaths.clear();
}

QVector<AFISMatchResult> AFISMatcher::identifyFingerprint(
    const QVector<MinutiaeData>& queryMinutiae,
    int maxResults) {

    QVector<AFISMatchResult> results;

    if (queryMinutiae.isEmpty() || databaseMinutiae.isEmpty()) {
        return results;
    }

    // Comparar com cada candidato na base de dados
    int current = 0;
    int total = databaseMinutiae.size();

    for (auto it = databaseMinutiae.constBegin();
         it != databaseMinutiae.constEnd(); ++it) {

        AFISMatchResult result = verifyFingerprint(queryMinutiae, it.value());
        result.candidateId = it.key();
        result.candidatePath = candidatePaths.value(it.key(), "");

        // Filtrar por score mínimo
        if (result.similarityScore >= config.minSimilarityScore &&
            result.matchedMinutiae >= config.minMatchedMinutiae) {
            results.append(result);
        }

        current++;
        // emit matchingProgress(current, total);
    }

    // Ordenar por score (decrescente)
    std::sort(results.begin(), results.end());

    // Limitar número de resultados
    if (results.size() > maxResults) {
        results.resize(maxResults);
    }

    return results;
}

QVector<AFISMatchResult> AFISMatcher::identifyFingerprintFromImage(
    const cv::Mat& queryImage,
    int maxResults) {

    QVector<MinutiaeData> queryMinutiae = extractMinutiaeFromImage(queryImage);
    return identifyFingerprint(queryMinutiae, maxResults);
}

AFISMatchResult AFISMatcher::verifyFingerprint(
    const QVector<MinutiaeData>& queryMinutiae,
    const QVector<MinutiaeData>& candidateMinutiae) {

    AFISMatchResult result;
    result.totalQueryMinutiae = queryMinutiae.size();
    result.totalCandidateMinutiae = candidateMinutiae.size();

    if (queryMinutiae.isEmpty() || candidateMinutiae.isEmpty()) {
        return result;
    }

    // Encontrar correspondências
    QVector<QPair<int, int>> correspondences =
        findCorrespondences(queryMinutiae, candidateMinutiae);

    result.matchedMinutiae = correspondences.size();

    // Validar geometria se configurado
    if (config.performGeometricValidation) {
        if (!validateGeometry(correspondences, queryMinutiae, candidateMinutiae)) {
            result.similarityScore = 0.0;
            result.confidenceLevel = 0.0;
            return result;
        }
    }

    // Calcular score final
    result.similarityScore =
        computeFinalScore(correspondences, queryMinutiae, candidateMinutiae);

    // Calcular nível de confiança
    double matchRatio = static_cast<double>(result.matchedMinutiae) /
                       std::min(result.totalQueryMinutiae, result.totalCandidateMinutiae);
    result.confidenceLevel = matchRatio * result.similarityScore;

    return result;
}

double AFISMatcher::calculateSimilarityScore(
    const QVector<MinutiaeData>& minutiae1,
    const QVector<MinutiaeData>& minutiae2) {

    AFISMatchResult result = verifyFingerprint(minutiae1, minutiae2);
    return result.similarityScore;
}

QFuture<QVector<AFISMatchResult>> AFISMatcher::identifyFingerprintAsync(
    const QVector<MinutiaeData>& queryMinutiae,
    int maxResults) {

    return QtConcurrent::run([this, queryMinutiae, maxResults]() {
        return identifyFingerprint(queryMinutiae, maxResults);
    });
}

cv::Mat AFISMatcher::visualizeMatch(const AFISMatchResult& result,
                                    const QVector<MinutiaeData>& queryMinutiae) {
    // TODO: Implementar visualização detalhada do matching
    return cv::Mat();
}

double AFISMatcher::computeLocalSimilarity(
    const MinutiaeData& m1,
    const MinutiaeData& m2) const {

    // Calcular distância euclidiana
    float dx = m1.position.x - m2.position.x;
    float dy = m1.position.y - m2.position.y;
    double distance = std::sqrt(dx*dx + dy*dy);

    // Calcular diferença de ângulo
    double angleDiff = std::abs(m1.angle - m2.angle);
    if (angleDiff > M_PI) {
        angleDiff = 2*M_PI - angleDiff;
    }

    // Verificar se está dentro das tolerâncias
    if (distance > config.positionTolerance ||
        angleDiff > config.angleTolerance) {
        return 0.0;
    }

    // Calcular similaridade (normalizada)
    double positionSim = 1.0 - (distance / config.positionTolerance);
    double angleSim = 1.0 - (angleDiff / config.angleTolerance);

    // Combinar similaridades
    double similarity = (positionSim + angleSim) / 2.0;

    // Aplicar peso de qualidade se configurado
    if (config.useQualityWeighting) {
        double qualityWeight = (m1.quality + m2.quality) / 2.0;
        similarity *= qualityWeight;
    }

    return similarity;
}

QVector<QPair<int, int>> AFISMatcher::findCorrespondences(
    const QVector<MinutiaeData>& query,
    const QVector<MinutiaeData>& candidate) const {

    QVector<QPair<int, int>> correspondences;

    // Matriz de similaridade
    QVector<QVector<double>> simMatrix(query.size());
    for (int i = 0; i < query.size(); i++) {
        simMatrix[i].resize(candidate.size());
        for (int j = 0; j < candidate.size(); j++) {
            // Verificar se são do mesmo tipo
            if (query[i].type == candidate[j].type) {
                simMatrix[i][j] = computeLocalSimilarity(query[i], candidate[j]);
            } else {
                simMatrix[i][j] = 0.0;
            }
        }
    }

    // Greedy matching: encontrar correspondências com maior similaridade
    QSet<int> usedQuery;
    QSet<int> usedCandidate;

    while (true) {
        double maxSim = 0.0;
        int bestI = -1, bestJ = -1;

        // Encontrar melhor correspondência não usada
        for (int i = 0; i < query.size(); i++) {
            if (usedQuery.contains(i)) continue;

            for (int j = 0; j < candidate.size(); j++) {
                if (usedCandidate.contains(j)) continue;

                if (simMatrix[i][j] > maxSim) {
                    maxSim = simMatrix[i][j];
                    bestI = i;
                    bestJ = j;
                }
            }
        }

        // Se não encontrou mais correspondências válidas, parar
        if (maxSim == 0.0 || bestI == -1) {
            break;
        }

        // Adicionar correspondência
        correspondences.append(qMakePair(bestI, bestJ));
        usedQuery.insert(bestI);
        usedCandidate.insert(bestJ);
    }

    return correspondences;
}

bool AFISMatcher::validateGeometry(
    const QVector<QPair<int, int>>& correspondences,
    const QVector<MinutiaeData>& query,
    const QVector<MinutiaeData>& candidate) const {

    // Requer pelo menos 3 correspondências para validação geométrica
    if (correspondences.size() < 3) {
        return correspondences.size() >= config.minMatchedMinutiae;
    }

    // Validação básica: verificar consistência de distâncias
    // TODO: Implementar validação geométrica mais robusta (RANSAC, etc)

    return true;
}

double AFISMatcher::computeFinalScore(
    const QVector<QPair<int, int>>& correspondences,
    const QVector<MinutiaeData>& query,
    const QVector<MinutiaeData>& candidate) const {

    if (correspondences.isEmpty()) {
        return 0.0;
    }

    // Calcular score baseado em múltiplos fatores

    // 1. Razão de minúcias correspondentes
    int totalMinutiae = std::min(query.size(), candidate.size());
    double matchRatio = static_cast<double>(correspondences.size()) / totalMinutiae;

    // 2. Qualidade média das correspondências
    double totalQuality = 0.0;
    for (const auto& corr : correspondences) {
        double sim = computeLocalSimilarity(query[corr.first], candidate[corr.second]);
        totalQuality += sim;
    }
    double avgQuality = totalQuality / correspondences.size();

    // 3. Combinar fatores
    double score = (matchRatio * 0.6) + (avgQuality * 0.4);

    // Normalizar para [0, 1]
    return std::max(0.0, std::min(1.0, score));
}

QVector<MinutiaeData> AFISMatcher::extractMinutiaeFromImage(const cv::Mat& image) {
    QVector<MinutiaeData> minutiae;

    if (image.empty()) {
        return minutiae;
    }

    // Processar imagem diretamente com OpenCV
    cv::Mat processed;
    if (image.channels() == 3) {
        cv::cvtColor(image, processed, cv::COLOR_BGR2GRAY);
    } else {
        processed = image.clone();
    }

    // Aplicar enhancement básico
    cv::equalizeHist(processed, processed);

    // Binarizar
    cv::threshold(processed, processed, 128, 255, cv::THRESH_BINARY);

    // Esqueletizar (morfologia)
    cv::Mat skeleton = cv::Mat::zeros(processed.size(), CV_8UC1);
    cv::Mat element = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));
    cv::Mat temp;
    cv::Mat eroded;

    bool done = false;
    while (!done) {
        cv::erode(processed, eroded, element);
        cv::dilate(eroded, temp, element);
        cv::subtract(processed, temp, temp);
        cv::bitwise_or(skeleton, temp, skeleton);
        eroded.copyTo(processed);
        done = (cv::countNonZero(processed) == 0);
    }

    // Extrair minúcias
    MinutiaeExtractor extractor;
    std::vector<Minutia> extractedMinutiae = extractor.extractMinutiae(skeleton);

    // Converter para MinutiaeData
    for (const auto& m : extractedMinutiae) {
        MinutiaeData data;
        data.position = m.position;
        data.angle = m.angle;
        data.quality = m.quality;
        data.id = m.id;

        // Converter tipo antigo para novo enum
        if (m.type == 0) {
            data.type = MinutiaeType::RIDGE_ENDING_B;
        } else if (m.type == 1) {
            data.type = MinutiaeType::BIFURCATION;
        } else {
            data.type = MinutiaeType::OTHER;
        }

        minutiae.append(data);
    }

    return minutiae;
}

void AFISMatcher::normalizeMinutiae(QVector<MinutiaeData>& minutiae) {
    if (minutiae.isEmpty()) {
        return;
    }

    // TODO: Implementar normalização (rotação, translação, escala)
    // Por enquanto, sem normalização
}
