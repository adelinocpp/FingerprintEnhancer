#ifndef AFISLIKELIHOODCALCULATOR_H
#define AFISLIKELIHOODCALCULATOR_H

#include <QVector>
#include <QPair>
#include "../core/ProjectModel.h"

namespace FingerprintEnhancer {
    class Minutia;
}

/**
 * @brief Configurações para cálculo de Likelihood Ratio
 */
struct AFISLikelihoodConfig {
    double positionTolerance;       // Tolerância de posição em pixels (padrão: ~120px = 3mm @ 40px/mm)
    double angleTolerance;          // Tolerância de ângulo em radianos (padrão: π = ignorar)
    double minMatchScore;           // Score mínimo para considerar match (padrão: 0.0 = aceitar todos)
    bool useTypeWeighting;          // Usar peso de tipo de minúcia (padrão: false)
    bool useQualityWeighting;       // Usar peso de qualidade (padrão: false)
    double scaleHint;               // Hint de escala esperada (padrão: 1.0 = mesma escala)
    
    AFISLikelihoodConfig()
        : positionTolerance(120.0),
          angleTolerance(3.14159265358979323846),  // M_PI
          minMatchScore(0.0),
          useTypeWeighting(false),
          useQualityWeighting(false),
          scaleHint(1.0) {}
};

/**
 * @brief Classe para cálculo de Likelihood Ratio (LR) em comparações de impressões digitais
 * 
 * Implementa algoritmos baseados em:
 * - Neumann et al. (2007) - "Quantifying the weight of fingerprint evidence"
 * - Neumann et al. (2012) - "Quantifying the weight of evidence from a forensic fingerprint comparison"
 * - Gomes et al. (2024) - Estatísticas de tipos de minúcias (dados brasileiros)
 * 
 * O Likelihood Ratio (LR) é definido como:
 * LR = P(E | H_p) / P(E | H_d)
 * 
 * Onde:
 * - E = evidência (conjunto de correspondências observadas)
 * - H_p = hipótese de mesma origem (prosecution hypothesis)
 * - H_d = hipótese de origens diferentes (defense hypothesis)
 * 
 * Interpretação do Log10(LR):
 * - LR > 10^6 (log > 6): Evidência extremamente forte de mesma origem
 * - LR > 10^4 (log > 4): Evidência muito forte de mesma origem
 * - LR > 10^2 (log > 2): Evidência forte de mesma origem
 * - LR > 10^1 (log > 1): Evidência moderada de mesma origem
 * - LR ~ 1 (log ~ 0): Evidência inconclusiva
 * - LR < 10^-1 (log < -1): Evidência moderada de origens diferentes
 * - LR < 10^-2 (log < -2): Evidência forte de origens diferentes
 * - etc.
 */
class AFISLikelihoodCalculator {
public:
    AFISLikelihoodCalculator();
    explicit AFISLikelihoodCalculator(const AFISLikelihoodConfig& config);
    
    // Configuração
    void setConfig(const AFISLikelihoodConfig& config) { this->config = config; }
    AFISLikelihoodConfig getConfig() const { return config; }
    
    // ==================== FUNÇÕES PRINCIPAIS ====================
    
    /**
     * @brief Estrutura para transformação geométrica (alinhamento)
     */
    struct GeometricTransform {
        double rotation;      // Rotação em radianos
        QPointF translation;  // Translação (dx, dy)
        double scale;         // Escala (padrão: 1.0)
        double confidence;    // Confiança na transformação (0.0-1.0)
        
        GeometricTransform() : rotation(0), translation(0, 0), scale(1.0), confidence(0.0) {}
    };
    
    /**
     * @brief Encontra correspondências entre dois conjuntos de minúcias
     * Usa algoritmo de alinhamento espacial considerando rotação/translação
     * @param minutiae1 Conjunto de minúcias 1
     * @param minutiae2 Conjunto de minúcias 2
     * @return Vetor de pares (índice em minutiae1, índice em minutiae2)
     */
    QVector<QPair<int, int>> findCorrespondences(
        const QVector<FingerprintEnhancer::Minutia>& minutiae1,
        const QVector<FingerprintEnhancer::Minutia>& minutiae2) const;
    
    /**
     * @brief Encontra correspondências COM estimativa de transformação geométrica
     * @param minutiae1 Conjunto de minúcias 1
     * @param minutiae2 Conjunto de minúcias 2
     * @param transform [out] Transformação estimada
     * @return Vetor de pares (índice em minutiae1, índice em minutiae2)
     */
    QVector<QPair<int, int>> findCorrespondencesWithAlignment(
        const QVector<FingerprintEnhancer::Minutia>& minutiae1,
        const QVector<FingerprintEnhancer::Minutia>& minutiae2,
        GeometricTransform& transform) const;
    
    /**
     * @brief Calcula score de similaridade local entre duas minúcias
     * @param m1 Minúcia 1
     * @param m2 Minúcia 2
     * @return Score de 0.0 (muito diferente) a 1.0 (idênticas)
     */
    double computeLocalSimilarity(
        const FingerprintEnhancer::Minutia& m1,
        const FingerprintEnhancer::Minutia& m2) const;
    
    /**
     * @brief Calcula Likelihood Ratio baseado em correspondências
     * @param minutiae1 Conjunto de minúcias 1
     * @param minutiae2 Conjunto de minúcias 2
     * @param correspondences Correspondências encontradas
     * @return Likelihood Ratio (LR)
     * 
     * Implementação atual usa aproximação simples baseada em número de correspondências.
     * TODO: Implementar cálculo completo baseado em Neumann et al. (2007, 2012)
     */
    double calculateLikelihoodRatio(
        const QVector<FingerprintEnhancer::Minutia>& minutiae1,
        const QVector<FingerprintEnhancer::Minutia>& minutiae2,
        const QVector<QPair<int, int>>& correspondences) const;
    
    /**
     * @brief Calcula score de similaridade simples (0.0 a 1.0)
     * @param minutiae1 Conjunto de minúcias 1
     * @param minutiae2 Conjunto de minúcias 2
     * @param correspondences Correspondências encontradas
     * @return Score de 0.0 a 1.0
     */
    double calculateSimilarityScore(
        const QVector<FingerprintEnhancer::Minutia>& minutiae1,
        const QVector<FingerprintEnhancer::Minutia>& minutiae2,
        const QVector<QPair<int, int>>& correspondences) const;
    
    /**
     * @brief Retorna texto de interpretação do Log10(LR)
     * @param logLR Logaritmo base 10 do Likelihood Ratio
     * @return Texto descritivo da força da evidência
     */
    static QString getInterpretationText(double logLR);
    
private:
    AFISLikelihoodConfig config;
    
    // ==================== FUNÇÕES AUXILIARES ====================
    
    /**
     * @brief Calcula distância euclidiana entre duas posições
     */
    double calculateDistance(const QPoint& p1, const QPoint& p2) const;
    double calculateDistance(const QPointF& p1, const QPointF& p2) const;
    
    /**
     * @brief Calcula diferença angular considerando periodicidade
     */
    double calculateAngleDifference(double angle1, double angle2) const;
    
    /**
     * @brief Peso baseado no tipo de minúcia (tipos raros têm mais peso)
     * TODO: Integrar com estatísticas de Gomes et al. (2024)
     */
    double getTypeWeight(MinutiaeType type) const;
    
    // ==================== ALINHAMENTO GEOMÉTRICO ====================
    
    /**
     * @brief Estima transformação geométrica entre dois conjuntos de minúcias
     * usando RANSAC com pares de minúcias como hipóteses
     */
    GeometricTransform estimateTransform(
        const QVector<FingerprintEnhancer::Minutia>& minutiae1,
        const QVector<FingerprintEnhancer::Minutia>& minutiae2) const;
    
    /**
     * @brief Aplica transformação a um ponto
     */
    QPointF applyTransform(const QPoint& point, const GeometricTransform& transform) const;
    
    /**
     * @brief Aplica transformação a um ângulo
     */
    double applyRotationToAngle(double angle, const GeometricTransform& transform) const;
    
    /**
     * @brief Conta quantas minúcias fazem match após aplicar transformação
     */
    int countMatchesAfterTransform(
        const QVector<FingerprintEnhancer::Minutia>& minutiae1,
        const QVector<FingerprintEnhancer::Minutia>& minutiae2,
        const GeometricTransform& transform) const;
};

#endif // AFISLIKELIHOODCALCULATOR_H
