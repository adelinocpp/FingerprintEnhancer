#ifndef AFISMATCHER_H
#define AFISMATCHER_H

#include <QString>
#include <QVector>
#include <QFuture>
#include <opencv2/opencv.hpp>
#include "../core/MinutiaeTypes.h"

/**
 * @brief Resultado de comparação AFIS
 */
struct AFISMatchResult {
    QString candidatePath;          // Caminho da imagem candidata
    double similarityScore;         // Score de similaridade (0.0 a 1.0)
    int matchedMinutiae;            // Número de minúcias correspondentes
    int totalQueryMinutiae;         // Total de minúcias na query
    int totalCandidateMinutiae;     // Total de minúcias no candidato
    cv::Mat visualMatch;            // Imagem visual do matching (opcional)
    QString candidateId;            // ID ou nome do candidato
    double confidenceLevel;         // Nível de confiança (0.0 a 1.0)

    AFISMatchResult()
        : similarityScore(0.0), matchedMinutiae(0),
          totalQueryMinutiae(0), totalCandidateMinutiae(0),
          confidenceLevel(0.0) {}

    bool operator<(const AFISMatchResult& other) const {
        return similarityScore > other.similarityScore;  // Ordem decrescente
    }
};

/**
 * @brief Configurações do matcher AFIS
 */
struct AFISMatchConfig {
    double positionTolerance;       // Tolerância de posição (pixels)
    double angleTolerance;          // Tolerância de ângulo (radianos)
    int minMatchedMinutiae;         // Mínimo de minúcias para match válido
    double minSimilarityScore;      // Score mínimo para considerar match
    bool useQualityWeighting;       // Usar peso de qualidade das minúcias
    bool performGeometricValidation;// Validar geometria do matching
    int maxCandidates;              // Máximo de candidatos a retornar

    AFISMatchConfig()
        : positionTolerance(15.0),
          angleTolerance(0.3),
          minMatchedMinutiae(12),
          minSimilarityScore(0.3),
          useQualityWeighting(true),
          performGeometricValidation(true),
          maxCandidates(10) {}
};

/**
 * @brief Sistema AFIS (Automated Fingerprint Identification System)
 *
 * Implementa comparação automatizada de impressões digitais
 * utilizando algoritmos baseados em minúcias e openAFIS
 */
class AFISMatcher {
public:
    AFISMatcher();
    ~AFISMatcher();

    // Configuração
    void setConfig(const AFISMatchConfig& config);
    AFISMatchConfig getConfig() const { return config; }

    // Carregar base de dados de impressões digitais
    bool loadDatabase(const QString& databasePath);
    bool addCandidateImage(const QString& imagePath);
    bool addCandidateMinutiae(const QString& candidateId,
                             const QVector<MinutiaeData>& minutiae);
    void clearDatabase();

    // Operações de matching
    QVector<AFISMatchResult> identifyFingerprint(
        const QVector<MinutiaeData>& queryMinutiae,
        int maxResults = 10);

    QVector<AFISMatchResult> identifyFingerprintFromImage(
        const cv::Mat& queryImage,
        int maxResults = 10);

    // Comparação 1:1
    AFISMatchResult verifyFingerprint(
        const QVector<MinutiaeData>& queryMinutiae,
        const QVector<MinutiaeData>& candidateMinutiae);

    double calculateSimilarityScore(
        const QVector<MinutiaeData>& minutiae1,
        const QVector<MinutiaeData>& minutiae2);

    // Operações assíncronas
    QFuture<QVector<AFISMatchResult>> identifyFingerprintAsync(
        const QVector<MinutiaeData>& queryMinutiae,
        int maxResults = 10);

    // Estatísticas
    int getDatabaseSize() const { return databaseMinutiae.size(); }
    QString getDatabasePath() const { return databasePath; }

    // Visualização
    cv::Mat visualizeMatch(const AFISMatchResult& result,
                          const QVector<MinutiaeData>& queryMinutiae);

signals:
    void matchingProgress(int current, int total);
    void matchingCompleted(QVector<AFISMatchResult> results);
    void matchingError(QString errorMessage);

private:
    AFISMatchConfig config;
    QString databasePath;

    // Base de dados de candidatos
    QMap<QString, QVector<MinutiaeData>> databaseMinutiae;
    QMap<QString, QString> candidatePaths;

    // Métodos de matching internos
    double computeLocalSimilarity(
        const MinutiaeData& m1,
        const MinutiaeData& m2) const;

    QVector<QPair<int, int>> findCorrespondences(
        const QVector<MinutiaeData>& query,
        const QVector<MinutiaeData>& candidate) const;

    bool validateGeometry(
        const QVector<QPair<int, int>>& correspondences,
        const QVector<MinutiaeData>& query,
        const QVector<MinutiaeData>& candidate) const;

    double computeFinalScore(
        const QVector<QPair<int, int>>& correspondences,
        const QVector<MinutiaeData>& query,
        const QVector<MinutiaeData>& candidate) const;

    // Métodos auxiliares
    QVector<MinutiaeData> extractMinutiaeFromImage(const cv::Mat& image);
    void normalizeMinutiae(QVector<MinutiaeData>& minutiae);
};

#endif // AFISMATCHER_H
