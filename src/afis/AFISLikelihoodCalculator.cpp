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
    
    GeometricTransform transform;
    return findCorrespondencesWithAlignment(minutiae1, minutiae2, transform);
}

QVector<QPair<int, int>> AFISLikelihoodCalculator::findCorrespondencesWithAlignment(
    const QVector<FingerprintEnhancer::Minutia>& minutiae1,
    const QVector<FingerprintEnhancer::Minutia>& minutiae2,
    GeometricTransform& transform) const {
    
    fprintf(stderr, "\n[AFIS-LR] ========== INÍCIO DO MATCHING COM ALINHAMENTO ==========\n");
    fprintf(stderr, "[AFIS-LR] Minúcias fragmento 1: %d\n", static_cast<int>(minutiae1.size()));
    fprintf(stderr, "[AFIS-LR] Minúcias fragmento 2: %d\n", static_cast<int>(minutiae2.size()));
    fprintf(stderr, "[AFIS-LR] Tolerância posição: %.1f pixels\n", config.positionTolerance);
    fprintf(stderr, "[AFIS-LR] Tolerância ângulo: %.3f radianos (%.1f graus)\n", 
            config.angleTolerance, config.angleTolerance * 180.0 / M_PI);
    fprintf(stderr, "[AFIS-LR] Score mínimo: %.2f\n", config.minMatchScore);
    
    // DEBUG: Mostrar amostra de minúcias
    if (!minutiae1.isEmpty()) {
        fprintf(stderr, "[AFIS-LR] DEBUG Amostra Fragmento 1:\n");
        for (int i = 0; i < qMin(3, minutiae1.size()); i++) {
            fprintf(stderr, "[AFIS-LR]   [%d] pos=(%d,%d), angle=%.3f rad, type=%d, quality=%.2f\n",
                    i, minutiae1[i].position.x(), minutiae1[i].position.y(),
                    minutiae1[i].angle, static_cast<int>(minutiae1[i].type), minutiae1[i].quality);
        }
    }
    if (!minutiae2.isEmpty()) {
        fprintf(stderr, "[AFIS-LR] DEBUG Amostra Fragmento 2:\n");
        for (int i = 0; i < qMin(3, minutiae2.size()); i++) {
            fprintf(stderr, "[AFIS-LR]   [%d] pos=(%d,%d), angle=%.3f rad, type=%d, quality=%.2f\n",
                    i, minutiae2[i].position.x(), minutiae2[i].position.y(),
                    minutiae2[i].angle, static_cast<int>(minutiae2[i].type), minutiae2[i].quality);
        }
    }
    
    // PASSO 1: Estimar transformação geométrica (rotação + translação)
    transform = estimateTransform(minutiae1, minutiae2);
    
    fprintf(stderr, "[AFIS-LR] Transformação estimada:\n");
    fprintf(stderr, "[AFIS-LR]   - Escala: %.3f\n", transform.scale);
    fprintf(stderr, "[AFIS-LR]   - Rotação: %.3f rad (%.1f°)\n", 
            transform.rotation, transform.rotation * 180.0 / M_PI);
    fprintf(stderr, "[AFIS-LR]   - Translação: (%.1f, %.1f)\n", 
            transform.translation.x(), transform.translation.y());
    fprintf(stderr, "[AFIS-LR]   - Confiança: %.3f\n", transform.confidence);
    
    // PASSO 2: Encontrar correspondências aplicando a transformação
    QVector<QPair<int, int>> correspondences;
    
    // Validação defensiva de tamanhos
    if (minutiae1.isEmpty() || minutiae2.isEmpty()) {
        fprintf(stderr, "[AFIS-LR] AVISO: Um dos vetores está vazio (size1=%d, size2=%d)\n",
                static_cast<int>(minutiae1.size()), static_cast<int>(minutiae2.size()));
        return correspondences;
    }
    
    QVector<bool> matched2(minutiae2.size(), false);
    
    for (int i = 0; i < minutiae1.size(); i++) {
        // Validação adicional dentro do loop
        if (i < 0 || i >= minutiae1.size()) {
            fprintf(stderr, "[AFIS-LR] ERRO CRÍTICO: índice i=%d fora do range [0,%d)\n",
                    i, static_cast<int>(minutiae1.size()));
            break;
        }
        double bestScore = -1.0;
        int bestMatch = -1;
        
        // Aplicar transformação à minúcia 1
        QPointF transformed1 = applyTransform(minutiae1[i].position, transform);
        double transformedAngle1 = applyRotationToAngle(minutiae1[i].angle, transform);
        
        fprintf(stderr, "\n[AFIS-LR] Minúcia 1[%d]: pos=(%d,%d) → escala×%.2f+rot+trans → (%.1f,%.1f), ângulo=%.2f→%.2f rad, tipo=%d\n",
                i, minutiae1[i].position.x(), minutiae1[i].position.y(),
                transform.scale,
                transformed1.x(), transformed1.y(),
                minutiae1[i].angle, transformedAngle1, static_cast<int>(minutiae1[i].type));
        
        int candidates = 0;
        for (int j = 0; j < minutiae2.size(); j++) {
            // Validação tripla antes de qualquer acesso
            if (j < 0 || j >= minutiae2.size() || j >= matched2.size()) {
                fprintf(stderr, "[AFIS-LR] ERRO: índice j=%d inválido (minutiae2.size=%d, matched2.size=%d)\n",
                        j, static_cast<int>(minutiae2.size()), static_cast<int>(matched2.size()));
                break;
            }
            
            if (matched2[j]) continue;
            
            // Calcular distância após transformação
            QPointF pos2(minutiae2[j].position.x(), minutiae2[j].position.y());
            double dist = calculateDistance(transformed1, pos2);
            double angleDiff = calculateAngleDifference(transformedAngle1, minutiae2[j].angle);
            
            if (dist <= config.positionTolerance && angleDiff <= config.angleTolerance) {
                candidates++;
                
                // Calcular score considerando transformação (consistente com computeLocalSimilarity)
                double distScore = exp(-dist / (config.positionTolerance / 3.0));
                double angleScore = (cos(angleDiff) + 1.0) / 2.0;
                
                // Se ambos têm tipo definido (não OTHER), considerar compatibilidade
                double typeScore = 1.0;  // Padrão: assumir compatível
                if (config.useTypeWeighting) {
                    if (minutiae1[i].type != MinutiaeType::OTHER && 
                        minutiae2[j].type != MinutiaeType::OTHER) {
                        typeScore = (minutiae1[i].type == minutiae2[j].type) ? 1.0 : 0.5;
                    }
                }
                
                // Pesos adaptativos: se ângulo é 0, dar mais peso à distância
                double wDist = 0.6, wAngle = 0.3, wType = 0.1;
                if (minutiae1[i].angle == 0.0f && minutiae2[j].angle == 0.0f) {
                    // Sem ângulo definido - focar APENAS em distância
                    wDist = 0.9;
                    wAngle = 0.05;
                    wType = 0.05;
                }
                
                double score = distScore * wDist + angleScore * wAngle + typeScore * wType;
                
                fprintf(stderr, "  [AFIS-LR]   Candidato 2[%d]: dist=%.1f, angDiff=%.3f, score=%.3f\n",
                        j, dist, angleDiff, score);
                
                if (score > bestScore) {
                    bestScore = score;
                    bestMatch = j;
                }
            }
        }
        
        if (bestMatch >= 0 && bestScore >= config.minMatchScore) {
            // Validar índices antes de adicionar correspondência
            if (bestMatch >= 0 && bestMatch < minutiae2.size() && bestMatch < matched2.size()) {
                correspondences.append(qMakePair(i, bestMatch));
                matched2[bestMatch] = true;
                fprintf(stderr, "  [AFIS-LR] ✓ MATCH: 1[%d] ↔ 2[%d], score=%.3f\n", i, bestMatch, bestScore);
            } else {
                fprintf(stderr, "  [AFIS-LR] ERRO: Índice bestMatch=%d fora do range (size=%d)\n", 
                        bestMatch, static_cast<int>(minutiae2.size()));
            }
        } else {
            fprintf(stderr, "  [AFIS-LR] ✗ SEM MATCH (candidatos=%d, bestScore=%.3f)\n", candidates, bestScore);
        }
    }
    
    fprintf(stderr, "\n[AFIS-LR] TOTAL DE CORRESPONDÊNCIAS: %d\n", static_cast<int>(correspondences.size()));
    fprintf(stderr, "[AFIS-LR] ========== FIM DO MATCHING ==========\n\n");
    
    return correspondences;
}

double AFISLikelihoodCalculator::computeLocalSimilarity(
    const FingerprintEnhancer::Minutia& m1,
    const FingerprintEnhancer::Minutia& m2) const {
    
    // 1. Score de distância (relativo à tolerância configurada)
    double dist = calculateDistance(m1.position, m2.position);
    // Normalizar pela tolerância: dist=0 → score=1.0, dist=tolerance → score≈0.37
    // dist > tolerance já foi rejeitado no matching
    double distScore = exp(-dist / (config.positionTolerance / 3.0));
    
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
    // Considerar qualidade média das correspondências (campo 'quality' das minúcias)
    // Atenção: isto é diferente da "similaridade local"!
    if (config.useQualityWeighting && !correspondences.isEmpty()) {
        double avgQuality = 0.0;
        int validPairs = 0;
        for (const auto& pair : correspondences) {
            // Validar índices antes de acessar
            if (pair.first < 0 || pair.first >= minutiae1.size() ||
                pair.second < 0 || pair.second >= minutiae2.size()) {
                fprintf(stderr, "[AFIS-LR] ERRO: Índice inválido pair[%d,%d] (tamanhos: %d,%d)\n",
                        pair.first, pair.second, static_cast<int>(minutiae1.size()), static_cast<int>(minutiae2.size()));
                continue;
            }
            
            const auto& m1 = minutiae1[pair.first];
            const auto& m2 = minutiae2[pair.second];
            double pairQuality = (m1.quality + m2.quality) / 2.0;
            avgQuality += pairQuality;
            validPairs++;
            fprintf(stderr, "[AFIS-LR]   Par[%d,%d]: quality campo=%.2f\n", pair.first, pair.second, pairQuality);
        }
        
        if (validPairs == 0) {
            fprintf(stderr, "[AFIS-LR] AVISO: Nenhum par válido para calcular qualidade\n");
        } else {
            avgQuality /= validPairs;
        }
        
        fprintf(stderr, "[AFIS-LR] Qualidade média (campo das minúcias): %.3f\n", avgQuality);
        
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
    
    // Score baseado em:
    // 1. Proporção de minúcias pareadas (completude)
    // 2. Qualidade das correspondências individuais
    
    int minTotal = qMin(minutiae1.size(), minutiae2.size());
    if (minTotal == 0 || correspondences.isEmpty()) return 0.0;
    
    // Fator de completude
    double completeness = static_cast<double>(correspondences.size()) / static_cast<double>(minTotal);
    
    // Similaridade média das correspondências (NÃO é o campo 'quality' das minúcias!)
    // Este é o score de similaridade local considerando distância, ângulo, tipo, etc.
    double avgLocalSimilarity = 0.0;
    int validPairs = 0;
    for (const auto& pair : correspondences) {
        if (pair.first >= 0 && pair.first < minutiae1.size() &&
            pair.second >= 0 && pair.second < minutiae2.size()) {
            // Usar computeLocalSimilarity para cada par
            double pairScore = computeLocalSimilarity(minutiae1[pair.first], minutiae2[pair.second]);
            avgLocalSimilarity += pairScore;
            validPairs++;
        }
    }
    
    if (validPairs == 0) return 0.0;
    avgLocalSimilarity /= validPairs;
    
    // Score final: priorizar completude quando há muitos matches
    // Com 100% de matches, score deve ser próximo de 100%
    // Fórmula: completeness^0.5 * 0.7 + avgLocalSimilarity * 0.3
    // Raiz quadrada suaviza a completude mas mantém 100%→100%
    double completenessContribution = sqrt(completeness) * 0.7;
    double similarityContribution = avgLocalSimilarity * 0.3;
    double finalScore = completenessContribution + similarityContribution;
    
    fprintf(stderr, "[AFIS-SIMILARITY] Completude: %.2f%% (contribui %.2f%%), Similaridade média dos pares: %.2f%% (contribui %.2f%%), Score final: %.2f%%\n",
            completeness * 100, completenessContribution * 100, avgLocalSimilarity * 100, similarityContribution * 100, finalScore * 100);
    
    return finalScore;
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

double AFISLikelihoodCalculator::calculateDistance(const QPointF& p1, const QPointF& p2) const {
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

// ==================== ALINHAMENTO GEOMÉTRICO ====================

AFISLikelihoodCalculator::GeometricTransform AFISLikelihoodCalculator::estimateTransform(
    const QVector<FingerprintEnhancer::Minutia>& minutiae1,
    const QVector<FingerprintEnhancer::Minutia>& minutiae2) const {
    
    fprintf(stderr, "\n[AFIS-TRANSFORM] Estimando transformação geométrica (com escala)...\n");
    if (config.scaleHint > 0.5 && config.scaleHint < 2.0) {
        fprintf(stderr, "[AFIS-TRANSFORM] Hint de escala: %.3f (esperado)\n", config.scaleHint);
    }
    
    GeometricTransform bestTransform;
    int bestMatchCount = 0;
    
    if (minutiae1.size() < 2 || minutiae2.size() < 2) {
        fprintf(stderr, "[AFIS-TRANSFORM] Minúcias insuficientes para estimativa\n");
        return bestTransform;
    }
    
    // RANSAC com PARES de minúcias para estimar escala + rotação + translação
    // Usar DOIS pares de minúcias para estimar escala automaticamente
    // Aumentar iterações para melhor convergência
    int maxIterations = qMin(300, minutiae1.size() * minutiae2.size() * 2);
    
    for (int iter = 0; iter < maxIterations; iter++) {
        // Revalidar tamanhos a cada iteração (pode ter mudado?)
        if (minutiae1.size() < 2 || minutiae2.size() < 2) {
            fprintf(stderr, "[AFIS-TRANSFORM] Tamanhos mudaram durante RANSAC: m1=%d, m2=%d\n",
                    static_cast<int>(minutiae1.size()), static_cast<int>(minutiae2.size()));
            break;
        }
        
        // Selecionar DOIS pares de minúcias aleatórios
        int idx1a = rand() % minutiae1.size();
        int idx1b = rand() % minutiae1.size();
        int maxAttempts = 10;
        while (idx1b == idx1a && maxAttempts-- > 0) idx1b = rand() % minutiae1.size();
        
        int idx2a = rand() % minutiae2.size();
        int idx2b = rand() % minutiae2.size();
        maxAttempts = 10;
        while (idx2b == idx2a && maxAttempts-- > 0) idx2b = rand() % minutiae2.size();
        
        // Validar índices antes de acessar
        if (idx1a < 0 || idx1a >= minutiae1.size() ||
            idx1b < 0 || idx1b >= minutiae1.size() ||
            idx2a < 0 || idx2a >= minutiae2.size() ||
            idx2b < 0 || idx2b >= minutiae2.size()) {
            fprintf(stderr, "[AFIS-TRANSFORM] ERRO: Índice inválido gerado\n");
            continue;
        }
        
        const auto& m1a = minutiae1[idx1a];
        const auto& m1b = minutiae1[idx1b];
        const auto& m2a = minutiae2[idx2a];
        const auto& m2b = minutiae2[idx2b];
        
        // Calcular distâncias entre os pares
        double dist1 = calculateDistance(m1a.position, m1b.position);
        double dist2 = calculateDistance(m2a.position, m2b.position);
        
        if (dist1 < 10.0 || dist2 < 10.0) continue;  // Pares muito próximos
        
        // Estimar escala: dist2 / dist1
        double estimatedScale = dist2 / dist1;
        
        // Limitar escala a valores razoáveis (ampliado para diferenças grandes)
        if (estimatedScale < 0.2 || estimatedScale > 5.0) continue;
        
        // Se temos hint de escala, penalizar transformações muito distantes
        // Isso ajuda com fragmentos de escalas muito diferentes
        if (config.scaleHint > 0.5 && config.scaleHint < 2.0) {
            double scaleDiff = fabs(estimatedScale - config.scaleHint);
            if (scaleDiff > config.scaleHint * 0.5) {
                // Escala muito diferente do esperado - reduzir prioridade
                continue;
            }
        }
        
        // Calcular transformação candidata baseada no primeiro par
        GeometricTransform candidate;
        candidate.scale = estimatedScale;
        
        // Rotação = diferença de ângulos das minúcias
        candidate.rotation = m2a.angle - m1a.angle;
        
        // Normalizar rotação para [-π, π]
        while (candidate.rotation > M_PI) candidate.rotation -= 2 * M_PI;
        while (candidate.rotation < -M_PI) candidate.rotation += 2 * M_PI;
        
        // Aplicar escala + rotação ao ponto m1a e calcular translação
        double cos_r = cos(candidate.rotation);
        double sin_r = sin(candidate.rotation);
        double scaled_x = m1a.position.x() * candidate.scale;
        double scaled_y = m1a.position.y() * candidate.scale;
        double rotated_x = scaled_x * cos_r - scaled_y * sin_r;
        double rotated_y = scaled_x * sin_r + scaled_y * cos_r;
        
        candidate.translation = QPointF(
            m2a.position.x() - rotated_x,
            m2a.position.y() - rotated_y
        );
        
        // Contar quantas minúcias fazem match com esta transformação
        int matchCount = countMatchesAfterTransform(minutiae1, minutiae2, candidate);
        
        if (matchCount > bestMatchCount) {
            bestMatchCount = matchCount;
            bestTransform = candidate;
            
            fprintf(stderr, "[AFIS-TRANSFORM] Iter %d: scale=%.3f, rot=%.2f°, trans=(%.1f,%.1f), matches=%d\n",
                    iter, candidate.scale, candidate.rotation * 180.0 / M_PI,
                    candidate.translation.x(), candidate.translation.y(),
                    matchCount);
        }
    }
    
    // Calcular confiança baseada no número de matches
    int minCount = qMin(minutiae1.size(), minutiae2.size());
    bestTransform.confidence = (minCount > 0) ? 
        static_cast<double>(bestMatchCount) / static_cast<double>(minCount) : 0.0;
    
    fprintf(stderr, "[AFIS-TRANSFORM] Melhor transformação: %d matches (confiança=%.2f)\n",
            bestMatchCount, bestTransform.confidence);
    fprintf(stderr, "[AFIS-TRANSFORM]   Escala: %.3f (frag2/frag1)\n", bestTransform.scale);
    fprintf(stderr, "[AFIS-TRANSFORM]   Rotação: %.3f rad (%.1f°)\n", 
            bestTransform.rotation, bestTransform.rotation * 180.0 / M_PI);
    fprintf(stderr, "[AFIS-TRANSFORM]   Translação: (%.1f, %.1f)\n",
            bestTransform.translation.x(), bestTransform.translation.y());
    
    return bestTransform;
}

QPointF AFISLikelihoodCalculator::applyTransform(const QPoint& point, const GeometricTransform& transform) const {
    // Ordem da transformação afim: ESCALA → ROTAÇÃO → TRANSLAÇÃO
    // P' = scale * R * P + T
    
    // 1. Escala
    double scaled_x = point.x() * transform.scale;
    double scaled_y = point.y() * transform.scale;
    
    // 2. Rotação
    double cos_r = cos(transform.rotation);
    double sin_r = sin(transform.rotation);
    double rotated_x = scaled_x * cos_r - scaled_y * sin_r;
    double rotated_y = scaled_x * sin_r + scaled_y * cos_r;
    
    // 3. Translação
    return QPointF(
        rotated_x + transform.translation.x(),
        rotated_y + transform.translation.y()
    );
}

double AFISLikelihoodCalculator::applyRotationToAngle(double angle, const GeometricTransform& transform) const {
    return angle + transform.rotation;
}

int AFISLikelihoodCalculator::countMatchesAfterTransform(
    const QVector<FingerprintEnhancer::Minutia>& minutiae1,
    const QVector<FingerprintEnhancer::Minutia>& minutiae2,
    const GeometricTransform& transform) const {
    
    int matchCount = 0;
    QVector<bool> matched2(minutiae2.size(), false);
    
    // Validação antes do loop principal
    if (minutiae1.isEmpty() || minutiae2.isEmpty()) {
        return 0;
    }
    
    for (int i = 0; i < minutiae1.size(); i++) {
        if (i < 0 || i >= minutiae1.size()) break;
        
        QPointF transformed1 = applyTransform(minutiae1[i].position, transform);
        double transformedAngle1 = applyRotationToAngle(minutiae1[i].angle, transform);
        
        for (int j = 0; j < minutiae2.size(); j++) {
            if (j < 0 || j >= minutiae2.size() || j >= matched2.size()) break;
            if (matched2[j]) continue;
            
            QPointF pos2(minutiae2[j].position.x(), minutiae2[j].position.y());
            double dist = calculateDistance(transformed1, pos2);
            double angleDiff = calculateAngleDifference(transformedAngle1, minutiae2[j].angle);
            
            if (dist <= config.positionTolerance && angleDiff <= config.angleTolerance) {
                matchCount++;
                if (j < matched2.size()) {
                    matched2[j] = true;
                }
                break;
            }
        }
    }
    
    return matchCount;
}
