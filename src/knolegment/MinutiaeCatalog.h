#ifndef MINUTIAE_CATALOG_H
#define MINUTIAE_CATALOG_H

#include <QString>
#include <QVector>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>

namespace FingerprintAnalysis {

/**
 * @brief Estrutura de dados estatísticos de uma minúcia
 */
struct MinutiaeStatistics {
    double mean;
    double median;
    double standardDeviation;
    
    MinutiaeStatistics() : mean(0.0), median(0.0), standardDeviation(0.0) {}
    MinutiaeStatistics(double m, double med, double sd) 
        : mean(m), median(med), standardDeviation(sd) {}
};

/**
 * @brief Classificação da minúcia
 */
enum class MinutiaeClassification {
    Fundamental,  // Minúcias fundamentais (>99% de ocorrência)
    Common,       // Minúcias comuns (10-99% de ocorrência)
    Rare          // Minúcias raras (<10% de ocorrência)
};

/**
 * @brief Categoria da minúcia
 */
enum class MinutiaeCategory {
    RidgeEnding,
    Bifurcation,
    Convergence,
    Fragment,
    Enclosure,
    Break,
    Overlap,
    Crossbar,
    Bridge,
    OppositeBifurcation,
    Dock,
    Trifurcation,
    M,
    Return,
    Appendage,
    Needle,
    Numerical,
    Tripod,
    Emboque,
    Conjugation,
    Angular,
    Other
};

/**
 * @brief Estrutura de dados de uma minúcia
 */
struct MinutiaeData {
    int id;
    QString nameEnglish;
    QString namePortuguese;
    QString descriptionEnglish;
    QString descriptionPortuguese;
    QString imagePath;
    MinutiaeCategory category;
    MinutiaeClassification classification;
    double frequencyGeneral;
    MinutiaeStatistics statistics;
    QMap<QString, double> frequencyByPattern;
    QMap<QString, double> frequencyByFinger;
    
    MinutiaeData() : id(0), frequencyGeneral(0.0) {}
    
    /**
     * @brief Converte a categoria para string
     */
    QString categoryToString() const;
    
    /**
     * @brief Converte a classificação para string
     */
    QString classificationToString() const;
    
    /**
     * @brief Serializa para QJsonObject
     */
    QJsonObject toJson() const;
    
    /**
     * @brief Deserializa de QJsonObject
     */
    static MinutiaeData fromJson(const QJsonObject& json);
};

/**
 * @brief Classe principal do catálogo de minúcias
 */
class MinutiaeCatalog {
public:
    MinutiaeCatalog();
    ~MinutiaeCatalog();
    
    /**
     * @brief Carrega o catálogo de um arquivo JSON
     * @param filePath Caminho para o arquivo JSON
     * @return true se carregado com sucesso, false caso contrário
     */
    bool loadFromJson(const QString& filePath);
    
    /**
     * @brief Salva o catálogo em um arquivo JSON
     * @param filePath Caminho para o arquivo JSON
     * @return true se salvo com sucesso, false caso contrário
     */
    bool saveToJson(const QString& filePath) const;
    
    /**
     * @brief Obtém todas as minúcias do catálogo
     * @return Vetor com todas as minúcias
     */
    QVector<MinutiaeData> getAllMinutiae() const;
    
    /**
     * @brief Obtém uma minúcia por ID
     * @param id ID da minúcia
     * @return Dados da minúcia ou nullptr se não encontrada
     */
    const MinutiaeData* getMinutiaeById(int id) const;
    
    /**
     * @brief Obtém minúcias por categoria
     * @param category Categoria desejada
     * @return Vetor com as minúcias da categoria
     */
    QVector<MinutiaeData> getMinutiaeByCategory(MinutiaeCategory category) const;
    
    /**
     * @brief Obtém minúcias por classificação
     * @param classification Classificação desejada
     * @return Vetor com as minúcias da classificação
     */
    QVector<MinutiaeData> getMinutiaeByClassification(MinutiaeClassification classification) const;
    
    /**
     * @brief Obtém minúcias fundamentais (Ridge endings, Bifurcations, Convergences)
     * @return Vetor com as minúcias fundamentais
     */
    QVector<MinutiaeData> getFundamentalMinutiae() const;
    
    /**
     * @brief Obtém o número total de minúcias no catálogo
     * @return Número de minúcias
     */
    int getCount() const;
    
    /**
     * @brief Adiciona uma minúcia ao catálogo
     * @param minutiae Dados da minúcia
     */
    void addMinutiae(const MinutiaeData& minutiae);
    
    /**
     * @brief Remove uma minúcia do catálogo
     * @param id ID da minúcia a ser removida
     * @return true se removida com sucesso, false caso contrário
     */
    bool removeMinutiae(int id);
    
    /**
     * @brief Limpa o catálogo
     */
    void clear();

private:
    QVector<MinutiaeData> m_minutiae;
    QMap<int, int> m_idToIndex;  // Mapeamento de ID para índice no vetor
    
    /**
     * @brief Reconstrói o índice de IDs
     */
    void rebuildIndex();
};

} // namespace FingerprintAnalysis

#endif // MINUTIAE_CATALOG_H

