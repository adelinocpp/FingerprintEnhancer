#ifndef FINGERPRINTLRCALCULATOR_H
#define FINGERPRINTLRCALCULATOR_H

#include <QVector>
#include <QMap>
#include <QString>
#include <QPointF>
#include <cmath>
#include "../core/ProjectModel.h"

namespace FingerprintEnhancer {

/**
 * Estrutura para armazenar features de forma (Shape Features)
 * Baseado em Neumann et al. (2015)
 */
struct ShapeFeatures {
    QVector<double> formFactors;      // Form factors dos triângulos
    QVector<double> aspectRatios;     // Aspect ratios dos triângulos
    QPointF centroid;                  // Centroide da configuração
    QVector<QPointF> orderedPositions; // Posições ordenadas anti-horário
};

/**
 * Estrutura para features de direção (Direction Features)
 */
struct DirectionFeatures {
    QVector<double> relativeAngles;   // Ângulos relativos ao eixo centroide-minúcia [0, 2π]
    QVector<double> absoluteAngles;   // Ângulos absolutos das minúcias
};

/**
 * Estrutura para features de tipo (Type Features)
 */
struct TypeFeatures {
    QVector<QString> types;           // Tipos classificados
    QVector<int> typeIndices;         // Índices para matriz de confusão
};

/**
 * Priors da população brasileira (Gomes et al. 2024)
 */
struct BrazilianPopulationData {
    // Frequências de tipos de minúcias (Tabela 8)
    QMap<QString, double> typeFrequencies;
    
    // Média de minúcias por configuração
    double avgMinutiaePerPrint = 82.28;
    
    // Médias por padrão
    QMap<QString, double> avgByPattern;
    
    // Matriz de confusão de examinadores (Neumann et al.)
    // [tipo_verdadeiro][tipo_observado]
    double confusionMatrix[3][3];  // RE, BI, UK
    
    BrazilianPopulationData();
};

/**
 * Opções de cálculo do LR
 */
enum class LRCalculationMode {
    SHAPE_ONLY,           // Apenas forma espacial
    SHAPE_DIRECTION,      // Forma + direção
    SHAPE_TYPE,           // Forma + tipo
    COMPLETE              // Todos os componentes
};

/**
 * Resultado do cálculo de LR
 */
struct LRResult {
    double lr_shape = 1.0;
    double lr_direction = 1.0;
    double lr_type = 1.0;
    double p_v_hd = 0.01;          // p(v=1|Hd) - raridade
    double lr_total = 1.0;
    
    // Logs para melhor interpretação
    double log10_lr_total = 0.0;
    
    // Contribuições individuais
    double shape_contribution = 1.0;
    double direction_contribution = 1.0;
    double type_contribution = 1.0;
    
    LRCalculationMode mode = LRCalculationMode::COMPLETE;
    int k_minutiae = 0;
    
    QString interpretation;  // Interpretação verbal
    
    LRResult() = default;
};

/**
 * Triângulo para cálculo de shape features
 */
class Triangle {
public:
    QPointF v1, v2, v3;  // Vértices
    
    Triangle(const QPointF& vertex1, const QPointF& vertex2, const QPointF& vertex3)
        : v1(vertex1), v2(vertex2), v3(vertex3) {}
    
    double area() const;
    double perimeter() const;
    double formFactor() const;           // área / perímetro
    double aspectRatio() const;          // diâmetro_circumcírculo / diâmetro_incírculo
    
private:
    double distance(const QPointF& p1, const QPointF& p2) const;
};

/**
 * Calculador de Likelihood Ratio para Impressões Digitais
 * Implementação baseada em Neumann et al. (2015) e Gomes et al. (2024)
 */
class FingerprintLRCalculator {
public:
    FingerprintLRCalculator();
    ~FingerprintLRCalculator() = default;
    
    // Habilitar log detalhado em arquivo
    void setDetailedLogging(bool enable, const QString& logFilePath = "");
    bool detailedLoggingEnabled() const { return m_detailedLogging; }
    
    // ==================== MÉTODOS PRINCIPAIS ====================
    
    /**
     * Calcula LR completo entre dois fragmentos
     * @param fragment1 Fragmento 1 (marca latente ou referência)
     * @param fragment2 Fragmento 2 (impressão do suspeito)
     * @param mode Modo de cálculo (shape_only, shape+direction, etc.)
     * @param pattern Padrão geral opcional (whorl, left_loop, etc.)
     * @param rarity Raridade estimada p(v=1|Hd), padrão=0.001 (1 em 1000)
     * @return Resultado com LR e componentes
     */
    LRResult calculateLR(
        const Fragment* fragment1,
        const Fragment* fragment2,
        LRCalculationMode mode = LRCalculationMode::COMPLETE,
        const QString& pattern = QString(),
        double rarity = 0.001
    );
    
    /**
     * Análise de sensibilidade - calcula todos os modos
     */
    QMap<QString, LRResult> analyzeSensitivity(
        const Fragment* fragment1,
        const Fragment* fragment2,
        const QString& pattern = QString(),
        double rarity = 0.001
    );
    
    // ==================== EXTRAÇÃO DE FEATURES ====================
    
    ShapeFeatures extractShapeFeatures(const QVector<Minutia>& minutiae);
    DirectionFeatures extractDirectionFeatures(
        const QVector<Minutia>& minutiae, 
        const QPointF& centroid
    );
    TypeFeatures extractTypeFeatures(const QVector<Minutia>& minutiae);
    
    // ==================== CÁLCULO DOS COMPONENTES ====================
    
    /**
     * LR_shape: Baseado na configuração espacial (form factors)
     * Usa distribuições normais simplificadas para numerador e denominador
     */
    double computeLRShape(
        const ShapeFeatures& observed,
        const ShapeFeatures& reference
    );
    
    /**
     * LR_direction: Baseado nas direções das minúcias
     * Assume independência entre minúcias
     */
    double computeLRDirection(
        const DirectionFeatures& observed,
        const DirectionFeatures& reference
    );
    
    /**
     * LR_type: Baseado nos tipos das minúcias
     * Usa matriz de confusão e priors da população
     */
    double computeLRType(
        const TypeFeatures& observed,
        const TypeFeatures& reference
    );
    
    /**
     * Estima p(v=1|Hd) - raridade da configuração
     * Em implementação real, usaria AFIS search em banco de referência
     * Por enquanto, usa estimativa baseada em k e padrão
     */
    double estimatePVHd(
        int k_minutiae,
        const QString& pattern = QString()
    );
    
    // ==================== UTILITÁRIOS ====================
    
    /**
     * Calcula centroide de um conjunto de minúcias
     */
    static QPointF calculateCentroid(const QVector<Minutia>& minutiae);
    
    /**
     * Ordena minúcias no sentido anti-horário ao redor do centroide
     */
    static QVector<Minutia> orderCounterClockwise(
        const QVector<Minutia>& minutiae,
        const QPointF& centroid
    );
    
    /**
     * Mapeia tipo de minúcia detalhado para categoria simplificada (RE/BI/UK)
     */
    static int mapTypeToIndex(MinutiaeType type);
    
    /**
     * Interpretação verbal do LR (escala ENFSI)
     */
    static QString interpretLR(double log10_lr);
    
    /**
     * Normaliza ângulo para intervalo [0, 2π]
     */
    static double normalizeAngle(double angle);
    
    /**
     * Calcula diferença angular considerando periodicidade
     */
    static double angleDifference(double angle1, double angle2);
    
    // ==================== CONFIGURAÇÃO ====================
    
    void setUseBrazilianPriors(bool use) { useBrazilianPriors = use; }
    void setRarityFactor(double factor) { rarityFactor = factor; }
    void setDistortionStdDev(double stddev) { distortionStdDev = stddev; }
    
    // Obter dados da população
    const BrazilianPopulationData& getPopulationData() const { return populationData; }
    
private:
    // Dados da população brasileira
    BrazilianPopulationData populationData;
    
    // Configurações
    bool useBrazilianPriors;
    double rarityFactor;          // Fator multiplicativo para p(v=1|Hd)
    double distortionStdDev;      // Desvio padrão para modelo de distorção
    
    // Logging detalhado
    bool m_detailedLogging;
    QString m_logFilePath;
    void logToFile(const QString& message);
    
    // Funções auxiliares para cálculos estatísticos
    double gaussianPDF(double x, double mean, double stddev) const;
    double vonMisesPDF(double x, double mu, double kappa) const;
    double estimateStdDev(const QVector<double>& samples) const;
    double estimateMean(const QVector<double>& samples) const;
};

} // namespace FingerprintEnhancer

#endif // FINGERPRINTLRCALCULATOR_H
