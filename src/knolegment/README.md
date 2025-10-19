# Catálogo de Minúcias de Impressões Digitais

Este catálogo fornece uma base de conhecimento estruturada sobre minúcias de impressões digitais, baseado no artigo científico **"Standardizing fingerprint minutiae: A comprehensive inventory and statistical analysis based on Brazilian data"** publicado na revista Forensic Science International (2024).

## Visão Geral

O catálogo contém informações detalhadas sobre **54 tipos de minúcias** identificadas em impressões digitais, incluindo:

- **Nomenclatura bilíngue** (inglês e português)
- **Descrições técnicas** de cada minúcia
- **Imagens ilustrativas** extraídas do artigo científico
- **Estatísticas de frequência** baseadas em análise de 600 impressões digitais da população brasileira
- **Classificação** (Fundamental, Comum ou Rara)
- **Distribuição por padrão** de impressão digital (Whorl, Arch, Right Loop, Left Loop)
- **Distribuição por tipo de dedo** (polegar, indicador, médio, anelar, mínimo)

## Estrutura do Projeto

```
minutiae_catalog/
├── MinutiaeCatalog.h          # Arquivo de cabeçalho C++
├── MinutiaeCatalog.cpp        # Implementação C++
├── CMakeLists.txt             # Configuração de compilação CMake
├── example.cpp                # Exemplo de uso
├── minutiae_catalog.json      # Dados do catálogo em formato JSON
├── cropped_images/            # Imagens das minúcias
├── minutiae_images/           # Páginas extraídas do PDF
└── README.md                  # Este arquivo
```

## Classificação das Minúcias

### Minúcias Fundamentais (Fundamental)
Presentes em praticamente todas as impressões digitais (≥99% de ocorrência):
- **Ridge ending-B** (Término de Linha - Início): 100%
- **Ridge ending-C** (Término de Linha - Fim): 99.83%
- **Bifurcation** (Bifurcação): 100%
- **Convergence** (Convergência): 99.67%

### Minúcias Comuns (Common)
Presentes em 10-99% das impressões digitais:
- Small fragment (Fragmento Pequeno): 76.67%
- Large fragment (Fragmento Grande): 66.0%
- Simple break (Interrupção Simples): 62.5%
- Small enclosure (Fechamento Pequeno): 54.67%
- Large enclosure (Fechamento Grande): 50.33%
- Tripod (Tripé): 55.5%
- E outras...

### Minúcias Raras (Rare)
Presentes em menos de 10% das impressões digitais:
- Double bifurcation (Bifurcação Dupla): 15.17%
- Double convergence (Convergência Dupla): 10.5%
- Trifurcation (Trifurcação): 4.0%
- Needle (Agulha): 2.33%
- Numerical (Numérico): 1.67%
- E outras...

## Integração com C++/Qt6

### Requisitos

- **Qt 6.0** ou superior
- **CMake 3.16** ou superior
- **Compilador C++17** ou superior

### Compilação

```bash
mkdir build
cd build
cmake ..
make
```

### Uso Básico

```cpp
#include "MinutiaeCatalog.h"
#include <QDebug>

using namespace FingerprintAnalysis;

int main() {
    // Criar instância do catálogo
    MinutiaeCatalog catalog;
    
    // Carregar dados do arquivo JSON
    catalog.loadFromJson("minutiae_catalog.json");
    
    // Obter todas as minúcias fundamentais
    QVector<MinutiaeData> fundamental = catalog.getFundamentalMinutiae();
    
    // Obter minúcias por categoria
    QVector<MinutiaeData> bifurcations = 
        catalog.getMinutiaeByCategory(MinutiaeCategory::Bifurcation);
    
    // Obter minúcia específica por ID
    const MinutiaeData* minutiae = catalog.getMinutiaeById(1);
    if (minutiae) {
        qDebug() << minutiae->nameEnglish;
        qDebug() << minutiae->namePortuguese;
        qDebug() << "Frequência:" << minutiae->frequencyGeneral << "%";
    }
    
    return 0;
}
```

### API Principal

#### Classe `MinutiaeCatalog`

**Métodos de Carregamento/Salvamento:**
- `bool loadFromJson(const QString& filePath)` - Carrega catálogo de arquivo JSON
- `bool saveToJson(const QString& filePath)` - Salva catálogo em arquivo JSON

**Métodos de Consulta:**
- `QVector<MinutiaeData> getAllMinutiae()` - Retorna todas as minúcias
- `const MinutiaeData* getMinutiaeById(int id)` - Busca minúcia por ID
- `QVector<MinutiaeData> getMinutiaeByCategory(MinutiaeCategory category)` - Filtra por categoria
- `QVector<MinutiaeData> getMinutiaeByClassification(MinutiaeClassification classification)` - Filtra por classificação
- `QVector<MinutiaeData> getFundamentalMinutiae()` - Retorna apenas minúcias fundamentais
- `int getCount()` - Retorna número total de minúcias

**Métodos de Modificação:**
- `void addMinutiae(const MinutiaeData& minutiae)` - Adiciona nova minúcia
- `bool removeMinutiae(int id)` - Remove minúcia por ID
- `void clear()` - Limpa o catálogo

#### Estrutura `MinutiaeData`

**Campos:**
- `int id` - Identificador único
- `QString nameEnglish` - Nome em inglês
- `QString namePortuguese` - Nome em português
- `QString descriptionEnglish` - Descrição em inglês
- `QString descriptionPortuguese` - Descrição em português
- `QString imagePath` - Caminho para imagem ilustrativa
- `MinutiaeCategory category` - Categoria da minúcia
- `MinutiaeClassification classification` - Classificação (Fundamental/Comum/Rara)
- `double frequencyGeneral` - Frequência geral de ocorrência (%)
- `MinutiaeStatistics statistics` - Estatísticas (média, mediana, desvio padrão)
- `QMap<QString, double> frequencyByPattern` - Frequência por padrão de impressão
- `QMap<QString, double> frequencyByFinger` - Frequência por tipo de dedo

**Métodos:**
- `QString categoryToString()` - Converte categoria para string
- `QString classificationToString()` - Converte classificação para string
- `QJsonObject toJson()` - Serializa para JSON
- `static MinutiaeData fromJson(const QJsonObject& json)` - Deserializa de JSON

#### Enumerações

**MinutiaeCategory:**
- `RidgeEnding`, `Bifurcation`, `Convergence`, `Fragment`, `Enclosure`, `Break`, `Overlap`, `Crossbar`, `Bridge`, `OppositeBifurcation`, `Dock`, `Trifurcation`, `M`, `Return`, `Appendage`, `Needle`, `Numerical`, `Tripod`, `Emboque`, `Conjugation`, `Angular`, `Other`

**MinutiaeClassification:**
- `Fundamental` - Minúcias fundamentais (≥99% de ocorrência)
- `Common` - Minúcias comuns (10-99% de ocorrência)
- `Rare` - Minúcias raras (<10% de ocorrência)

## Formato do Arquivo JSON

O arquivo `minutiae_catalog.json` segue a seguinte estrutura:

```json
{
    "minutiae_catalog": [
        {
            "id": 1,
            "name_en": "Ridge ending-B",
            "name_pt": "Término de Linha (Início)",
            "description_en": "Beginning of the line.",
            "description_pt": "Início da linha papilar.",
            "image_path": "cropped_images/ridge_ending_b.png",
            "category": "Ridge Ending",
            "classification": "Fundamental",
            "frequency_general": 100.0,
            "stats": {
                "mean": 18.5,
                "median": 17.0,
                "std_dev": 7.74
            },
            "frequency_by_pattern": {
                "Whorl": 22.815,
                "Arch": 22.239,
                "Right Loop": 23.185,
                "Left Loop": 28.458
            },
            "frequency_by_finger": {
                "RT": 29.014,
                "RI": 25.092,
                "RM": 28.706,
                "RR": 24.383,
                "RL": 24.155,
                "LT": 15.995,
                "LI": 18.672,
                "LM": 16.309,
                "LR": 16.690,
                "LL": 14.965
            }
        }
    ]
}
```

## Estatísticas Gerais

Com base na análise de 600 impressões digitais da população brasileira:

- **Média de minúcias por impressão digital**: 82.28
  - Masculino: 86.62
  - Feminino: 77.94

- **Distribuição por padrão geral (L1D)**:
  - Whorl (Verticilo): 31.33%
  - Left Loop (Loop Esquerdo): 31.33%
  - Right Loop (Loop Direito): 30.0%
  - Arch (Arco): 7.33%

- **Características adicionais**:
  - Poros: presentes em 65.83% das impressões
  - Cicatrizes: presentes em 35.83% das impressões
  - Cristas incipientes: presentes em 20.5% das impressões

## Referência Científica

Este catálogo é baseado no artigo:

**Gomes, G.A.S., Marouelli de Oliveira, L.P., Carvalho, D.S., Brito, F.C.A., & Matsushita, R.Y.** (2024). Standardizing fingerprint minutiae: A comprehensive inventory and statistical analysis based on Brazilian data. *Forensic Science International*, 364, 112233. https://doi.org/10.1016/j.forsciint.2024.112233

## Licença

Este catálogo é fornecido para fins educacionais e de pesquisa. Os dados estatísticos são derivados do artigo científico publicado mencionado acima.

## Autores

- **Catálogo e Implementação**: Manus AI
- **Dados Científicos**: Gomes et al. (2024)

## Versão

- **Versão do Catálogo**: 1.0
- **Data**: Outubro de 2024

