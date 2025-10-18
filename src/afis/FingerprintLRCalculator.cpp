#include "FingerprintLRCalculator.h"
#include <QtMath>
#include <QDebug>
#include <algorithm>

namespace FingerprintEnhancer {

// ==================== BrazilianPopulationData ====================

BrazilianPopulationData::BrazilianPopulationData() {
    // Frequências de tipos baseadas em Gomes et al. (2024) - Tabela 8
    typeFrequencies["ridge_ending_B"] = 0.44005;
    typeFrequencies["ridge_ending_C"] = 0.19308;
    typeFrequencies["bifurcation"] = 0.19308;
    typeFrequencies["convergence"] = 0.15064;
    typeFrequencies["fragment_big"] = 0.02392;
    typeFrequencies["fragment_small"] = 0.01477;
    typeFrequencies["other"] = 0.00533;  // Não classificadas
    
    // Médias por padrão (Tabela 3)
    avgByPattern["arch"] = 74.50;
    avgByPattern["left_loop"] = 79.68;
    avgByPattern["right_loop"] = 81.06;
    avgByPattern["whorl"] = 87.87;
    
    // Matriz de confusão de examinadores (Neumann et al.)
    // Linha = tipo verdadeiro, Coluna = tipo observado
    // [0][*] = Ridge Ending verdadeiro
    confusionMatrix[0][0] = 0.76;  // RE → RE
    confusionMatrix[0][1] = 0.16;  // RE → BI
    confusionMatrix[0][2] = 0.08;  // RE → UK
    
    // [1][*] = Bifurcation verdadeiro
    confusionMatrix[1][0] = 0.16;  // BI → RE
    confusionMatrix[1][1] = 0.75;  // BI → BI
    confusionMatrix[1][2] = 0.09;  // BI → UK
    
    // [2][*] = Unknown verdadeiro
    confusionMatrix[2][0] = 0.00;  // UK → RE
    confusionMatrix[2][1] = 0.00;  // UK → BI
    confusionMatrix[2][2] = 1.00;  // UK → UK
}

// ==================== Triangle ====================

double Triangle::distance(const QPointF& p1, const QPointF& p2) const {
    double dx = p2.x() - p1.x();
    double dy = p2.y() - p1.y();
    return std::sqrt(dx * dx + dy * dy);
}

double Triangle::area() const {
    // Fórmula do determinante para área do triângulo
    return 0.5 * std::abs(
        v1.x() * (v2.y() - v3.y()) +
        v2.x() * (v3.y() - v1.y()) +
        v3.x() * (v1.y() - v2.y())
    );
}

double Triangle::perimeter() const {
    return distance(v1, v2) + distance(v2, v3) + distance(v3, v1);
}

double Triangle::formFactor() const {
    double p = perimeter();
    if (p < 1e-6) return 0.0;
    return area() / p;
}

double Triangle::aspectRatio() const {
    // Aspect Ratio = diâmetro_circumcírculo / diâmetro_incírculo
    double a = distance(v1, v2);
    double b = distance(v2, v3);
    double c = distance(v3, v1);
    double A = area();
    
    if (A < 1e-6) return 0.0;
    
    // Raio do círculo circunscrito: R = (abc) / (4A)
    double R = (a * b * c) / (4.0 * A);
    
    // Raio do círculo inscrito: r = A / s (onde s = semiperímetro)
    double s = (a + b + c) / 2.0;
    double r = A / s;
    
    if (r < 1e-6) return 100.0;  // Limite superior
    
    return (2.0 * R) / (2.0 * r);  // Diâmetros
}

// ==================== FingerprintLRCalculator ====================

FingerprintLRCalculator::FingerprintLRCalculator()
    : useBrazilianPriors(true)
    , rarityFactor(1.0)
    , distortionStdDev(2.0)  // pixels - modelo de distorção simplificado
{
    qDebug() << "[LR] FingerprintLRCalculator inicializado com priors brasileiros";
}

// ==================== MÉTODOS PRINCIPAIS ====================

LRResult FingerprintLRCalculator::calculateLR(
    const Fragment* fragment1,
    const Fragment* fragment2,
    LRCalculationMode mode,
    const QString& pattern,
    double rarity
) {
    LRResult result;
    result.mode = mode;
    
    if (!fragment1 || !fragment2) {
        qWarning() << "[LR] Fragmentos nulos fornecidos";
        return result;
    }
    
    const auto& minutiae1 = fragment1->minutiae;
    const auto& minutiae2 = fragment2->minutiae;
    
    if (minutiae1.isEmpty() || minutiae2.isEmpty()) {
        qWarning() << "[LR] Fragmentos sem minúcias";
        return result;
    }
    
    // Usar o menor número de minúcias
    int k = std::min(minutiae1.size(), minutiae2.size());
    result.k_minutiae = k;
    
    qDebug() << QString("[LR] Calculando LR com %1 minúcias, modo=%2")
        .arg(k).arg(static_cast<int>(mode));
    
    // Extrair apenas as k primeiras minúcias de cada fragmento
    QVector<Minutia> m1 = minutiae1.mid(0, k);
    QVector<Minutia> m2 = minutiae2.mid(0, k);
    
    // ===== SHAPE =====
    ShapeFeatures shape1 = extractShapeFeatures(m1);
    ShapeFeatures shape2 = extractShapeFeatures(m2);
    result.lr_shape = computeLRShape(shape1, shape2);
    
    // ===== DIRECTION =====
    if (mode == LRCalculationMode::SHAPE_DIRECTION || 
        mode == LRCalculationMode::COMPLETE) {
        DirectionFeatures dir1 = extractDirectionFeatures(m1, shape1.centroid);
        DirectionFeatures dir2 = extractDirectionFeatures(m2, shape2.centroid);
        result.lr_direction = computeLRDirection(dir1, dir2);
    }
    
    // ===== TYPE =====
    if (mode == LRCalculationMode::SHAPE_TYPE || 
        mode == LRCalculationMode::COMPLETE) {
        TypeFeatures type1 = extractTypeFeatures(m1);
        TypeFeatures type2 = extractTypeFeatures(m2);
        result.lr_type = computeLRType(type1, type2);
    }
    
    // ===== P(V=1|Hd) =====
    result.p_v_hd = (rarity > 0) ? rarity : estimatePVHd(k, pattern);
    
    // ===== LR TOTAL =====
    result.lr_total = result.lr_shape * result.lr_direction * result.lr_type * (1.0 / result.p_v_hd);
    
    // Evitar valores muito extremos
    if (result.lr_total > 1e15) result.lr_total = 1e15;
    if (result.lr_total < 1e-15) result.lr_total = 1e-15;
    
    result.log10_lr_total = std::log10(result.lr_total);
    
    // Contribuições
    result.shape_contribution = result.lr_shape * (1.0 / result.p_v_hd);
    result.direction_contribution = result.lr_direction;
    result.type_contribution = result.lr_type;
    
    // Interpretação
    result.interpretation = interpretLR(result.log10_lr_total);
    
    qDebug() << QString("[LR] Resultado: LR_shape=%.2e, LR_dir=%.2e, LR_type=%.2e, p_v_hd=%.2e")
        .arg(result.lr_shape).arg(result.lr_direction)
        .arg(result.lr_type).arg(result.p_v_hd);
    qDebug() << QString("[LR] LR_total = %.2e (log10 = %.2f) - %s")
        .arg(result.lr_total).arg(result.log10_lr_total)
        .arg(result.interpretation);
    
    return result;
}

QMap<QString, LRResult> FingerprintLRCalculator::analyzeSensitivity(
    const Fragment* fragment1,
    const Fragment* fragment2,
    const QString& pattern,
    double rarity
) {
    QMap<QString, LRResult> results;
    
    results["shape_only"] = calculateLR(
        fragment1, fragment2, LRCalculationMode::SHAPE_ONLY, pattern, rarity
    );
    
    results["shape_direction"] = calculateLR(
        fragment1, fragment2, LRCalculationMode::SHAPE_DIRECTION, pattern, rarity
    );
    
    results["shape_type"] = calculateLR(
        fragment1, fragment2, LRCalculationMode::SHAPE_TYPE, pattern, rarity
    );
    
    results["complete"] = calculateLR(
        fragment1, fragment2, LRCalculationMode::COMPLETE, pattern, rarity
    );
    
    qDebug() << "[LR] Análise de sensibilidade completa:";
    qDebug() << QString("  Shape only:      LR = %.2e (log10=%.2f)")
        .arg(results["shape_only"].lr_total)
        .arg(results["shape_only"].log10_lr_total);
    qDebug() << QString("  Shape+Direction: LR = %.2e (log10=%.2f)")
        .arg(results["shape_direction"].lr_total)
        .arg(results["shape_direction"].log10_lr_total);
    qDebug() << QString("  Shape+Type:      LR = %.2e (log10=%.2f)")
        .arg(results["shape_type"].lr_total)
        .arg(results["shape_type"].log10_lr_total);
    qDebug() << QString("  Complete:        LR = %.2e (log10=%.2f)")
        .arg(results["complete"].lr_total)
        .arg(results["complete"].log10_lr_total);
    
    return results;
}

// ==================== EXTRAÇÃO DE FEATURES ====================

ShapeFeatures FingerprintLRCalculator::extractShapeFeatures(const QVector<Minutia>& minutiae) {
    ShapeFeatures features;
    
    if (minutiae.isEmpty()) return features;
    
    // 1. Calcular centroide
    features.centroid = calculateCentroid(minutiae);
    
    // 2. Ordenar minúcias no sentido anti-horário
    QVector<Minutia> ordered = orderCounterClockwise(minutiae, features.centroid);
    
    // 3. Salvar posições ordenadas
    for (const auto& m : ordered) {
        features.orderedPositions.append(QPointF(m.position.x(), m.position.y()));
    }
    
    int k = ordered.size();
    
    // 4. Criar triângulos (par consecutivo + centroide)
    QVector<Triangle> triangles;
    for (int i = 0; i < k; ++i) {
        QPointF v1 = features.orderedPositions[i];
        QPointF v2 = features.orderedPositions[(i + 1) % k];
        QPointF v3 = features.centroid;
        
        triangles.append(Triangle(v1, v2, v3));
    }
    
    // 5. Calcular aspect ratios e form factors
    QVector<double> aspectRatios;
    for (const auto& tri : triangles) {
        aspectRatios.append(tri.aspectRatio());
    }
    
    // 6. Ordenar triângulos pelo aspect ratio (primeiro = menor AR)
    QVector<int> sortedIndices;
    for (int i = 0; i < k; ++i) sortedIndices.append(i);
    
    std::sort(sortedIndices.begin(), sortedIndices.end(), 
        [&aspectRatios](int a, int b) {
            return aspectRatios[a] < aspectRatios[b];
        }
    );
    
    // 7. Extrair form factors na ordem dos triângulos ordenados
    for (int idx : sortedIndices) {
        features.formFactors.append(triangles[idx].formFactor());
        features.aspectRatios.append(aspectRatios[idx]);
    }
    
    qDebug() << QString("[LR-Shape] Extraídos %1 triângulos, centroide=(%.1f, %.1f)")
        .arg(k).arg(features.centroid.x()).arg(features.centroid.y());
    
    return features;
}

DirectionFeatures FingerprintLRCalculator::extractDirectionFeatures(
    const QVector<Minutia>& minutiae,
    const QPointF& centroid
) {
    DirectionFeatures features;
    
    for (const auto& m : minutiae) {
        // Eixo de referência: centroide → minúcia
        double dx = m.position.x() - centroid.x();
        double dy = m.position.y() - centroid.y();
        double axisAngle = std::atan2(dy, dx);
        
        // Direção da minúcia (já em radianos)
        double minutiaAngle = m.angle;
        
        // Ângulo relativo (anti-horário, normalizado para [0, 2π])
        double relativeAngle = normalizeAngle(minutiaAngle - axisAngle);
        
        features.relativeAngles.append(relativeAngle);
        features.absoluteAngles.append(minutiaAngle);
    }
    
    qDebug() << QString("[LR-Direction] Extraídas %1 direções").arg(minutiae.size());
    
    return features;
}

TypeFeatures FingerprintLRCalculator::extractTypeFeatures(const QVector<Minutia>& minutiae) {
    TypeFeatures features;
    
    for (const auto& m : minutiae) {
        // Tipo como string (simplificado)
        QString typeName = "other";
        
        switch (m.type) {
            case MinutiaeType::RIDGE_ENDING_B:
            case MinutiaeType::RIDGE_ENDING_C:
                typeName = "ridge_ending";
                break;
            case MinutiaeType::BIFURCATION:
            case MinutiaeType::BTUS:
            case MinutiaeType::BTUI:
            case MinutiaeType::BTBS:
            case MinutiaeType::BTBI:
                typeName = "bifurcation";
                break;
            case MinutiaeType::CONVERGENCE:
            case MinutiaeType::CTUS:
            case MinutiaeType::CTUI:
            case MinutiaeType::CTCS:
            case MinutiaeType::CTCI:
                typeName = "convergence";
                break;
            case MinutiaeType::FRAGMENT_LARGE:
                typeName = "fragment_big";
                break;
            case MinutiaeType::FRAGMENT_SMALL:
                typeName = "fragment_small";
                break;
            default:
                typeName = "other";
        }
        
        features.types.append(typeName);
        features.typeIndices.append(mapTypeToIndex(m.type));
    }
    
    qDebug() << QString("[LR-Type] Extraídos %1 tipos").arg(minutiae.size());
    
    return features;
}

// ==================== CÁLCULO DOS COMPONENTES ====================

double FingerprintLRCalculator::computeLRShape(
    const ShapeFeatures& observed,
    const ShapeFeatures& reference
) {
    if (observed.formFactors.isEmpty() || reference.formFactors.isEmpty()) {
        return 1.0;
    }
    
    int k = std::min(observed.formFactors.size(), reference.formFactors.size());
    
    // Modelo simplificado: assumir que diferenças seguem distribuição normal
    // Numerador (Hp): esperamos similaridade alta (pequena variância)
    // Denominador (Hd): variância maior (populacional)
    
    double lr_product = 1.0;
    
    for (int i = 0; i < k; ++i) {
        double y = observed.formFactors[i];
        double x = reference.formFactors[i];
        
        // NUMERADOR: p(y | x, Hp, v=1)
        // Modelo de distorção: y ~ N(x, σ_distortion)
        double sigma_hp = distortionStdDev;  // Distorção intra-individual
        double p_numerator = gaussianPDF(y, x, sigma_hp);
        
        // DENOMINADOR: p(y | Hd, v=1)
        // Variabilidade populacional: maior desvio padrão
        double sigma_hd = 5.0 * distortionStdDev;  // 5x maior para população
        double mean_hd = x;  // Média aproximada
        double p_denominator = gaussianPDF(y, mean_hd, sigma_hd);
        
        // Evitar divisão por zero
        if (p_denominator < 1e-15) p_denominator = 1e-15;
        if (p_numerator < 1e-15) p_numerator = 1e-15;
        
        double lr_i = p_numerator / p_denominator;
        
        // Limitar valores extremos por triângulo
        if (lr_i > 1e6) lr_i = 1e6;
        if (lr_i < 1e-6) lr_i = 1e-6;
        
        lr_product *= lr_i;
    }
    
    qDebug() << QString("[LR-Shape] LR_shape = %.2e (k=%1 triângulos)").arg(k).arg(lr_product);
    
    return lr_product;
}

double FingerprintLRCalculator::computeLRDirection(
    const DirectionFeatures& observed,
    const DirectionFeatures& reference
) {
    if (observed.relativeAngles.isEmpty() || reference.relativeAngles.isEmpty()) {
        return 1.0;
    }
    
    int k = std::min(observed.relativeAngles.size(), reference.relativeAngles.size());
    
    double lr_product = 1.0;
    
    for (int i = 0; i < k; ++i) {
        double y_angle = observed.relativeAngles[i];
        double x_angle = reference.relativeAngles[i];
        
        // Diferença angular (circular)
        double diff = angleDifference(y_angle, x_angle);
        
        // NUMERADOR: von Mises com concentração alta (mesma pessoa)
        double kappa_hp = 10.0;  // Alta concentração
        double p_numerator = vonMisesPDF(diff, 0.0, kappa_hp);
        
        // DENOMINADOR: von Mises com concentração baixa (população)
        double kappa_hd = 1.0;  // Baixa concentração (mais disperso)
        double p_denominator = vonMisesPDF(diff, 0.0, kappa_hd);
        
        if (p_denominator < 1e-15) p_denominator = 1e-15;
        if (p_numerator < 1e-15) p_numerator = 1e-15;
        
        double lr_i = p_numerator / p_denominator;
        
        // Limitar valores extremos
        if (lr_i > 1e3) lr_i = 1e3;
        if (lr_i < 1e-3) lr_i = 1e-3;
        
        lr_product *= lr_i;
    }
    
    qDebug() << QString("[LR-Direction] LR_direction = %.2e (k=%1 minúcias)").arg(k).arg(lr_product);
    
    return lr_product;
}

double FingerprintLRCalculator::computeLRType(
    const TypeFeatures& observed,
    const TypeFeatures& reference
) {
    if (observed.types.isEmpty() || reference.types.isEmpty()) {
        return 1.0;
    }
    
    int k = std::min(observed.types.size(), reference.types.size());
    
    double lr_product = 1.0;
    
    for (int i = 0; i < k; ++i) {
        QString y_type = observed.types[i];
        int y_idx = observed.typeIndices[i];
        int x_idx = reference.typeIndices[i];
        
        // NUMERADOR: p(y_type | x_type, Hp, v=1)
        // Usa matriz de confusão de examinadores
        double p_numerator = 0.75;  // Valor padrão (75% de acerto)
        
        if (x_idx >= 0 && x_idx < 3 && y_idx >= 0 && y_idx < 3) {
            p_numerator = populationData.confusionMatrix[x_idx][y_idx];
        }
        
        // DENOMINADOR: p(y_type | Hd, v=1)
        // Usa priors da população brasileira
        double p_denominator = populationData.typeFrequencies.value(y_type, 0.01);
        
        if (p_denominator < 1e-6) p_denominator = 1e-6;
        if (p_numerator < 1e-6) p_numerator = 1e-6;
        
        double lr_i = p_numerator / p_denominator;
        
        // Limitar valores extremos
        if (lr_i > 1e2) lr_i = 1e2;
        if (lr_i < 1e-2) lr_i = 1e-2;
        
        lr_product *= lr_i;
    }
    
    qDebug() << QString("[LR-Type] LR_type = %.2e (k=%1 minúcias)").arg(k).arg(lr_product);
    
    return lr_product;
}

double FingerprintLRCalculator::estimatePVHd(int k_minutiae, const QString& pattern) {
    // Estimativa simplificada de p(v=1|Hd)
    // Em implementação real, usaria busca AFIS em banco de referência
    
    // Fator base: quanto mais minúcias, mais raro
    double base_rarity = std::pow(10.0, -static_cast<double>(k_minutiae) / 5.0);
    
    // Ajuste por padrão (alguns padrões são mais comuns)
    double pattern_factor = 1.0;
    
    if (pattern == "arch") {
        pattern_factor = 0.5;  // Arch é raro (7.33%), então configurações são mais únicas
    } else if (pattern == "whorl") {
        pattern_factor = 1.5;  // Whorl é comum e tem mais minúcias
    } else if (pattern == "left_loop" || pattern == "right_loop") {
        pattern_factor = 1.2;  // Loops são muito comuns
    }
    
    double p_v_hd = base_rarity * pattern_factor * rarityFactor;
    
    // Limitar intervalo razoável
    if (p_v_hd > 0.1) p_v_hd = 0.1;    // Máximo 10%
    if (p_v_hd < 1e-9) p_v_hd = 1e-9;  // Mínimo 1 em 1 bilhão
    
    qDebug() << QString("[LR-AFIS] p(v=1|Hd) estimado = %.2e (k=%1, padrão=%2)")
        .arg(p_v_hd).arg(k_minutiae).arg(pattern.isEmpty() ? "unknown" : pattern);
    
    return p_v_hd;
}

// ==================== UTILITÁRIOS ====================

QPointF FingerprintLRCalculator::calculateCentroid(const QVector<Minutia>& minutiae) {
    if (minutiae.isEmpty()) return QPointF(0, 0);
    
    double sum_x = 0.0, sum_y = 0.0;
    for (const auto& m : minutiae) {
        sum_x += m.position.x();
        sum_y += m.position.y();
    }
    
    return QPointF(sum_x / minutiae.size(), sum_y / minutiae.size());
}

QVector<Minutia> FingerprintLRCalculator::orderCounterClockwise(
    const QVector<Minutia>& minutiae,
    const QPointF& centroid
) {
    QVector<Minutia> ordered = minutiae;
    
    // Ordenar por ângulo polar em relação ao centroide
    std::sort(ordered.begin(), ordered.end(), 
        [&centroid](const Minutia& a, const Minutia& b) {
            double angle_a = std::atan2(a.position.y() - centroid.y(), 
                                        a.position.x() - centroid.x());
            double angle_b = std::atan2(b.position.y() - centroid.y(), 
                                        b.position.x() - centroid.x());
            return angle_a < angle_b;
        }
    );
    
    return ordered;
}

int FingerprintLRCalculator::mapTypeToIndex(MinutiaeType type) {
    // Mapeia para categorias simplificadas: 0=RE, 1=BI, 2=UK
    switch (type) {
        case MinutiaeType::RIDGE_ENDING_B:
        case MinutiaeType::RIDGE_ENDING_C:
            return 0;  // Ridge Ending
            
        case MinutiaeType::BIFURCATION:
        case MinutiaeType::BTUS:
        case MinutiaeType::BTUI:
        case MinutiaeType::BTBS:
        case MinutiaeType::BTBI:
            return 1;  // Bifurcation
            
        case MinutiaeType::CONVERGENCE:
        case MinutiaeType::CTUS:
        case MinutiaeType::CTUI:
        case MinutiaeType::CTCS:
        case MinutiaeType::CTCI:
            return 1;  // Convergence (tratado como BI)
            
        default:
            return 2;  // Unknown/Other
    }
}

QString FingerprintLRCalculator::interpretLR(double log10_lr) {
    // Escala ENFSI (European Network of Forensic Science Institutes)
    if (log10_lr < 0) {
        return "Suporta fortemente Hd (NÃO é o suspeito)";
    } else if (log10_lr < 1) {
        return "Suporte limitado para Hp";
    } else if (log10_lr < 2) {
        return "Suporte moderado para Hp";
    } else if (log10_lr < 4) {
        return "Suporte forte para Hp";
    } else if (log10_lr < 6) {
        return "Suporte muito forte para Hp";
    } else {
        return "Suporte extremamente forte para Hp (É o suspeito)";
    }
}

double FingerprintLRCalculator::normalizeAngle(double angle) {
    // Normaliza para [0, 2π]
    while (angle < 0) angle += 2.0 * M_PI;
    while (angle >= 2.0 * M_PI) angle -= 2.0 * M_PI;
    return angle;
}

double FingerprintLRCalculator::angleDifference(double angle1, double angle2) {
    // Calcula diferença angular considerando periodicidade
    double diff = std::abs(angle1 - angle2);
    if (diff > M_PI) diff = 2.0 * M_PI - diff;
    return diff;
}

// ==================== FUNÇÕES ESTATÍSTICAS ====================

double FingerprintLRCalculator::gaussianPDF(double x, double mean, double stddev) const {
    if (stddev < 1e-9) return 0.0;
    
    double z = (x - mean) / stddev;
    double coefficient = 1.0 / (stddev * std::sqrt(2.0 * M_PI));
    return coefficient * std::exp(-0.5 * z * z);
}

double FingerprintLRCalculator::vonMisesPDF(double x, double mu, double kappa) const {
    // Distribuição von Mises (circular)
    // Aproximação simplificada para I_0(kappa) usando série de Taylor
    
    if (kappa < 1e-6) {
        // Uniforme quando kappa → 0
        return 1.0 / (2.0 * M_PI);
    }
    
    // Bessel I_0(kappa) aproximado
    double i0_kappa = 1.0;
    if (kappa < 3.75) {
        double t = kappa / 3.75;
        double t2 = t * t;
        i0_kappa = 1.0 + 3.5156229*t2 + 3.0899424*t2*t2 + 
                   1.2067492*t2*t2*t2 + 0.2659732*t2*t2*t2*t2;
    } else {
        double t = 3.75 / kappa;
        i0_kappa = (std::exp(kappa) / std::sqrt(kappa)) * 
                   (0.39894228 + 0.01328592*t + 0.00225319*t*t);
    }
    
    double normalization = 1.0 / (2.0 * M_PI * i0_kappa);
    return normalization * std::exp(kappa * std::cos(x - mu));
}

double FingerprintLRCalculator::estimateStdDev(const QVector<double>& samples) const {
    if (samples.size() < 2) return 1.0;
    
    double mean = estimateMean(samples);
    double sum_sq = 0.0;
    
    for (double val : samples) {
        double diff = val - mean;
        sum_sq += diff * diff;
    }
    
    return std::sqrt(sum_sq / (samples.size() - 1));
}

double FingerprintLRCalculator::estimateMean(const QVector<double>& samples) const {
    if (samples.isEmpty()) return 0.0;
    
    double sum = 0.0;
    for (double val : samples) {
        sum += val;
    }
    
    return sum / samples.size();
}

} // namespace FingerprintEnhancer

