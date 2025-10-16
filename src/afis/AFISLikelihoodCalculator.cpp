#include "AFISLikelihoodCalculator.h"
#include "../core/MinutiaeTypes.h"
#include <cmath>
#include <algorithm>

AFISLikelihoodCalculator::AFISLikelihoodCalculator()
    : config(AFISLikelihoodConfig())
{
}

AFISLikelihoodCalculator::AFISLikelihoodCalculator(const AFISLikelihoodConfig& cfg)
    : config(cfg)
{
}

QVector<QPair<int, int>> AFISLikelihoodCalculator::findCorrespondences(
    const QVector<FingerprintEnhancer::Minutia>& minutiae1,
    const QVector<FingerprintEnhancer::Minutia>& minutiae2) const {
    
    fprintf(stderr, "\n[AFIS-LR] ========== INÍCIO DO MATCHING ==========\n");
    fprintf(stderr, "[AFIS-LR] Minúcias fragmento 1: %d\n", minutiae1.size());
    fprintf(stderr, "[AFIS-LR] Minúcias fragmento 2: %d\n", minutiae2.size());
    fprintf(stderr, "[AFIS-LR] Tolerância posição: %.1f pixels\n", config.positionTolerance);
    fprintf(stderr, "[AFIS-LR] Tolerância ângulo: %.3f radianos (%.1f graus)\n", 
            config.angleTolerance, config.angleTolerance * 180.0 / M_PI);
    fprintf(stderr, "[AFIS-LR] Score mínimo: %.2f\n", config.minMatchScore);
    
    QVector<QPair<int, int>> correspondences;
    QVector<bool> matched2(minutiae2.size(), false);
    
    // Para cada minúcia no conjunto 1, encontrar melhor match no conjunto 2
    for (int i = 0; i < minutiae1.size(); i++) {
        double bestScore = -1.0;
        int bestMatch = -1;
        
        fprintf(stderr, "\n[AFIS-LR] Minúcia 1[%d]: pos=(%d,%d), ângulo=%.2f rad, tipo=%d\n",
                i, minutiae1[i].position.x(), minutiae1[i].position.y(), 
                minutiae1[i].angle, static_cast<int>(minutiae1[i].type));
        
        int candidates = 0;
        for (int j = 0; j < minutiae2.size(); j++) {
            if (matched2[j]) continue;  // Já foi pareada
            
            // Calcular score de similaridade
            double score = computeLocalSimilarity(minutiae1[i], minutiae2[j]);
            
            // Verificar tolerâncias
            double dist = calculateDistance(minutiae1[i].position, minutiae2[j].position);
            double angleDiff = calculateAngleDifference(minutiae1[i].angle, minutiae2[j].angle);
            
            if (dist <= config.positionTolerance && angleDiff <= config.angleTolerance) {
                candidates++;
                fprintf(stderr, "  [AFIS-LR]   Candidato 2[%d]: dist=%.1f, angDiff=%.3f, score=%.3f\n",
                        j, dist, angleDiff, score);
            }
            
            if (dist > config.positionTolerance || angleDiff > config.angleTolerance) {
                continue;  // Fora das tolerâncias
            }
            
            if (score > bestScore) {
                bestScore = score;
                bestMatch = j;
            }
        }
        
        // Aceitar correspondência se score for suficiente
        if (bestMatch >= 0 && bestScore >= config.minMatchScore) {
            correspondences.append(qMakePair(i, bestMatch));
            matched2[bestMatch] = true;
            fprintf(stderr, "  [AFIS-LR] ✓ MATCH: 1[%d] ↔ 2[%d], score=%.3f\n", i, bestMatch, bestScore);
        } else {
            fprintf(stderr, "  [AFIS-LR] ✗ SEM MATCH (candidatos=%d, bestScore=%.3f)\n", candidates, bestScore);
        }
    }
    
    fprintf(stderr, "\n[AFIS-LR] TOTAL DE CORRESPONDÊNCIAS: %d\n", correspondences.size());
    fprintf(stderr, "[AFIS-LR] ========== FIM DO MATCHING ==========\n\n");
    
    return correspondences;
}

double AFISLikelihoodCalculator::computeLocalSimilarity(
    const FingerprintEnhancer::Minutia& m1,
    const FingerprintEnhancer::Minutia& m2) const {
    
    // 1. Score de distância (decai exponencialmente com distância)
    double dist = calculateDistance(m1.position, m2.position);
    double distScore = exp(-dist / 10.0);  // Decai com sigma=10 pixels
    
    // 2. Score de ângulo (baseado em cosseno da diferença)
    double angleDiff = calculateAngleDifference(m1.angle, m2.angle);
    double angleScore = cos(angleDiff);  // 1.0 se ângulos iguais, -1.0 se opostos
    // Mapear de [-1, 1] para [0, 1]
    angleScore = (angleScore + 1.0) / 2.0;
    
    // 3. Score de tipo (minúcias do mesmo tipo têm score maior)
    double typeScore = 0.5;  // Padrão se tipos diferentes
    if (config.useTypeWeighting) {
        if (m1.type == m2.type) {
            typeScore = 1.0;
            // Bonus para tipos raros
            typeScore *= getTypeWeight(m1.type);
        }
    }
    
    // 4. Score de qualidade (média das qualidades)
    double qualityScore = 1.0;
    if (config.useQualityWeighting) {
        qualityScore = (m1.quality + m2.quality) / 2.0;
        // Se qualidade é 0 (não definida), usar 1.0
        if (qualityScore < 0.01) qualityScore = 1.0;
    }
    
    // Score final ponderado (0.0 a 1.0)
    // Pesos: distância 40%, ângulo 30%, tipo 20%, qualidade 10%
    double finalScore = (distScore * 0.4 + angleScore * 0.3 + typeScore * 0.2 + qualityScore * 0.1);
    
    return qBound(0.0, finalScore, 1.0);
}

double AFISLikelihoodCalculator::calculateLikelihoodRatio(
    const QVector<FingerprintEnhancer::Minutia>& minutiae1,
    const QVector<FingerprintEnhancer::Minutia>& minutiae2,
    const QVector<QPair<int, int>>& correspondences) const {
    
    fprintf(stderr, "\n[AFIS-LR] ========== CÁLCULO DE LIKELIHOOD RATIO ==========\n");
    
    // ==================================================================================
    // TODO: Implementar cálculo completo baseado em Neumann et al. (2007, 2012)
    // ==================================================================================
    // 
    // O cálculo completo do LR deve considerar:
    // 1. Probabilidade de correspondência sob H_p (mesma origem)
    // 2. Probabilidade de correspondência sob H_d (origens diferentes)
    // 3. Configuração espacial das minúcias
    // 4. Raridade dos tipos de minúcias
    // 5. Qualidade das correspondências
    // 6. Tamanho da área de sobreposição
    // 7. Número total de minúcias disponíveis
    //
    // Referências:
    // - Neumann, C., et al. (2007). "Computation of likelihood ratios in fingerprint 
    //   identification for configurations of any number of minutiae"
    // - Neumann, C., et al. (2012). "Quantifying the weight of evidence from a forensic 
    //   fingerprint comparison: a new paradigm"
    // ==================================================================================
    
    int n = correspondences.size();  // Número de correspondências
    
    fprintf(stderr, "[AFIS-LR] Número de correspondências: %d\n", n);
    
    if (n == 0) {
        fprintf(stderr, "[AFIS-LR] Nenhuma correspondência - LR = 1e-10\n");
        fprintf(stderr, "[AFIS-LR] ========================================\n\n");
        return 1e-10;  // LR muito baixo (forte evidência de não-match)
    }
    
    // ==================== APROXIMAÇÃO SIMPLES (TEMPORÁRIA) ====================
    // Baseada em análises empíricas de sistemas AFIS:
    // - Cada minúcia correspondente adiciona ~2-3 ordens de magnitude ao LR
    // - LR cresce exponencialmente com número de matches
    
    double baseLR = 10.0;  // LR base para 1 correspondência
    double exponent = n * 2.5;  // Cada correspondência multiplica por ~10^2.5
    
    double LR = pow(baseLR, exponent);
    fprintf(stderr, "[AFIS-LR] LR base (10^(2.5*%d)): %.2e\n", n, LR);
    
    // ==================== FATOR DE COMPLETUDE ====================
    // Penalizar se muitas minúcias não foram pareadas
    // Isso indica possível não-match ou área de sobreposição pequena
    int minTotal = qMin(minutiae1.size(), minutiae2.size());
    double completeness = 1.0;
    if (minTotal > 0) {
        completeness = static_cast<double>(n) / static_cast<double>(minTotal);
        fprintf(stderr, "[AFIS-LR] Fator de completude (%d/%d): %.3f\n", n, minTotal, completeness);
        LR *= completeness;
        fprintf(stderr, "[AFIS-LR] LR após completude: %.2e\n", LR);
    }
    
    // ==================== FATOR DE QUALIDADE ====================
    // Considerar qualidade média das correspondências
    if (config.useQualityWeighting && !correspondences.isEmpty()) {
        double avgQuality = 0.0;
        for (const auto& pair : correspondences) {
            const auto& m1 = minutiae1[pair.first];
            const auto& m2 = minutiae2[pair.second];
            double pairQuality = (m1.quality + m2.quality) / 2.0;
            avgQuality += pairQuality;
            fprintf(stderr, "[AFIS-LR]   Par[%d,%d]: quality=%.2f\n", pair.first, pair.second, pairQuality);
        }
        avgQuality /= correspondences.size();
        
        fprintf(stderr, "[AFIS-LR] Qualidade média: %.3f\n", avgQuality);
        
        // Se qualidade média é razoável, usar como multiplicador
        if (avgQuality > 0.01) {
            LR *= avgQuality;
            fprintf(stderr, "[AFIS-LR] LR após qualidade: %.2e\n", LR);
        }
    }
    
    // Limitar valores extremos
    if (LR > 1e15) LR = 1e15;  // Evitar overflow
    if (LR < 1e-15) LR = 1e-15;
    
    fprintf(stderr, "[AFIS-LR] LR FINAL: %.2e (log10 = %.2f)\n", LR, log10(LR));
    fprintf(stderr, "[AFIS-LR] ========================================\n\n");
    
    return LR;
}

double AFISLikelihoodCalculator::calculateSimilarityScore(
    const QVector<FingerprintEnhancer::Minutia>& minutiae1,
    const QVector<FingerprintEnhancer::Minutia>& minutiae2,
    const QVector<QPair<int, int>>& correspondences) const {
    
    // Score simples: proporção de minúcias pareadas
    int minTotal = qMin(minutiae1.size(), minutiae2.size());
    if (minTotal == 0) return 0.0;
    
    return static_cast<double>(correspondences.size()) / static_cast<double>(minTotal);
}

QString AFISLikelihoodCalculator::getInterpretationText(double logLR) {
    // Baseado em Neumann et al. (2007) e diretrizes forenses internacionais
    // Escala verbal de força da evidência (ENFSI, 2015)
    
    if (logLR >= 6.0) {
        return "Evidência EXTREMAMENTE FORTE de mesma origem";
    } else if (logLR >= 4.0) {
        return "Evidência MUITO FORTE de mesma origem";
    } else if (logLR >= 2.0) {
        return "Evidência FORTE de mesma origem";
    } else if (logLR >= 1.0) {
        return "Evidência MODERADA de mesma origem";
    } else if (logLR >= 0.5) {
        return "Evidência LEVE de mesma origem";
    } else if (logLR > -0.5) {
        return "Evidência INCONCLUSIVA";
    } else if (logLR > -1.0) {
        return "Evidência LEVE de origens diferentes";
    } else if (logLR > -2.0) {
        return "Evidência MODERADA de origens diferentes";
    } else if (logLR > -4.0) {
        return "Evidência FORTE de origens diferentes";
    } else if (logLR > -6.0) {
        return "Evidência MUITO FORTE de origens diferentes";
    } else {
        return "Evidência EXTREMAMENTE FORTE de origens diferentes";
    }
}

// ==================== FUNÇÕES AUXILIARES ====================

double AFISLikelihoodCalculator::calculateDistance(const QPoint& p1, const QPoint& p2) const {
    double dx = p1.x() - p2.x();
    double dy = p1.y() - p2.y();
    return sqrt(dx * dx + dy * dy);
}

double AFISLikelihoodCalculator::calculateAngleDifference(double angle1, double angle2) const {
    double diff = fabs(angle1 - angle2);
    // Considerar periodicidade (0 = 2π)
    if (diff > M_PI) {
        diff = 2 * M_PI - diff;
    }
    return diff;
}

double AFISLikelihoodCalculator::getTypeWeight(MinutiaeType type) const {
    // TODO: Integrar com estatísticas de Gomes et al. (2024)
    // Por enquanto, dar peso maior para tipos menos comuns
    
    // Tipos mais comuns (frequência alta): peso normal
    if (type == MinutiaeType::RIDGE_ENDING_B ||
        type == MinutiaeType::RIDGE_ENDING_C ||
        type == MinutiaeType::BIFURCATION) {
        return 1.0;
    }
    
    // Tipos raros: peso maior (evidência mais forte)
    if (type == MinutiaeType::TRIPOD ||
        type == MinutiaeType::TRIFURCATION_B ||
        type == MinutiaeType::TRIFURCATION_C ||
        type == MinutiaeType::CORE ||
        type == MinutiaeType::DELTA) {
        return 1.5;
    }
    
    // Tipos de raridade intermediária
    return 1.2;
}
