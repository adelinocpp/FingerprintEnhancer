# Sistema AFIS - Automated Fingerprint Identification System

## Visão Geral

Este módulo implementa um sistema básico de identificação automatizada de impressões digitais (AFIS). O sistema permite comparar um fragmento de impressão digital com uma base de dados de impressões completas e retornar um ranking de similaridade.

## Arquitetura

### Componentes Principais

1. **AFISMatcher**: Classe principal que gerencia todo o processo de matching
2. **MinutiaeTypes**: Sistema de classificação de 56 tipos de minúcias (54 do estudo + Core + Delta)
3. **MinutiaeExtractor**: Extração automática de minúcias das imagens

## Uso Básico

```cpp
#include "afis/AFISMatcher.h"

// Criar matcher
AFISMatcher matcher;

// Configurar
AFISMatchConfig config;
config.minMatchedMinutiae = 12;  // Mínimo de 12 pontos correspondentes
config.minSimilarityScore = 0.3;  // Score mínimo de 30%
matcher.setConfig(config);

// Carregar base de dados de impressões digitais
matcher.loadDatabase("/caminho/para/diretorio/com/digitais");

// Identificar impressão digital
cv::Mat fragmento = cv::imread("fragmento.png", cv::IMREAD_GRAYSCALE);
QVector<AFISMatchResult> results = matcher.identifyFingerprintFromImage(fragmento, 10);

// Processar resultados
for (const auto& result : results) {
    qDebug() << "Candidato:" << result.candidateId;
    qDebug() << "Similaridade:" << result.similarityScore;
    qDebug() << "Minúcias correspondentes:" << result.matchedMinutiae;
    qDebug() << "Confiança:" << result.confidenceLevel;
}
```

## Integração com openAFIS

### Planejamento para Integração Futura

Este sistema foi projetado para ser expandido com integração ao [openAFIS](https://github.com/neilharan/openafis), um framework open-source para AFIS.

### Passos para Integração com openAFIS:

1. **Clonar o repositório openAFIS**:
   ```bash
   cd /path/to/FingerprintEnhancer/third_party
   git clone https://github.com/neilharan/openafis.git
   ```

2. **Compilar openAFIS** (seguir instruções do projeto)

3. **Adicionar no CMakeLists.txt**:
   ```cmake
   # OpenAFIS
   add_subdirectory(third_party/openafis)
   include_directories(third_party/openafis/include)

   # Linkar com AFISCore
   target_link_libraries(AFISCore openafis)
   ```

4. **Criar adapter para openAFIS**:
   - Converter MinutiaeData para formato openAFIS
   - Utilizar algoritmos de matching do openAFIS
   - Manter interface atual do AFISMatcher

### Estrutura Proposta

```
src/afis/
├── AFISMatcher.h/cpp          # Interface principal (atual)
├── OpenAFISAdapter.h/cpp      # Adapter para openAFIS (futuro)
├── AFISDatabase.h/cpp         # Gerenciamento de base de dados (futuro)
└── README_AFIS.md            # Esta documentação
```

## Formatos de Imagem Suportados

O sistema suporta os seguintes formatos:
- PNG
- JPEG/JPG
- TIFF/TIF
- BMP
- RAW (através do OpenCV)

## Configuração Avançada

### Parâmetros de Matching

- **positionTolerance**: Tolerância de posição em pixels (padrão: 15.0)
- **angleTolerance**: Tolerância de ângulo em radianos (padrão: 0.3)
- **minMatchedMinutiae**: Mínimo de minúcias correspondentes (padrão: 12)
- **minSimilarityScore**: Score mínimo para considerar match (padrão: 0.3)
- **useQualityWeighting**: Usar peso de qualidade das minúcias (padrão: true)
- **performGeometricValidation**: Validar geometria do matching (padrão: true)
- **maxCandidates**: Máximo de candidatos a retornar (padrão: 10)

### Exemplo de Configuração Customizada

```cpp
AFISMatchConfig config;
config.positionTolerance = 20.0;      // Mais permissivo
config.angleTolerance = 0.4;          // Mais permissivo
config.minMatchedMinutiae = 8;        // Menos restritivo para fragmentos pequenos
config.minSimilarityScore = 0.25;     // Threshold mais baixo
config.maxCandidates = 20;            // Mais resultados
matcher.setConfig(config);
```

## Melhorias Futuras

### Curto Prazo
- [ ] Implementar validação geométrica robusta (RANSAC)
- [ ] Adicionar cache de features extraídas
- [ ] Otimizar busca em base de dados grande (indexação)
- [ ] Suporte a operações batch

### Médio Prazo
- [ ] Integração com openAFIS
- [ ] Suporte a formato ANSI/NIST-ITL
- [ ] API REST para uso em servidor
- [ ] Interface web para visualização de resultados

### Longo Prazo
- [ ] Machine Learning para melhor matching
- [ ] Suporte a impressões digitais latentes
- [ ] Sistema distribuído para bases de dados massivas
- [ ] Integração com padrões internacionais (INTERPOL, FBI)

## Performance

### Benchmarks Estimados

| Tamanho da Base | Tempo de Identificação |
|----------------|------------------------|
| 100 impressões | ~0.5s                  |
| 1,000 impressões | ~5s                  |
| 10,000 impressões | ~50s                |

*Nota: Tempos aproximados em CPU moderna. Pode variar com qualidade das imagens e configurações.*

## Referências

- Gomes et al. (2024). "Standardizing fingerprint minutiae: A comprehensive inventory and statistical analysis based on Brazilian data". Forensic Science International 364.
- [openAFIS GitHub](https://github.com/neilharan/openafis)
- NIST Special Database 4 (Fingerprint Database)

## Licença

Este módulo segue a mesma licença do projeto principal FingerprintEnhancer.
