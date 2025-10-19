#include "MinutiaeCatalog.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace FingerprintAnalysis {

// MinutiaeData implementation

QString MinutiaeData::categoryToString() const {
    switch (category) {
        case MinutiaeCategory::RidgeEnding: return "Ridge Ending";
        case MinutiaeCategory::Bifurcation: return "Bifurcation";
        case MinutiaeCategory::Convergence: return "Convergence";
        case MinutiaeCategory::Fragment: return "Fragment";
        case MinutiaeCategory::Enclosure: return "Enclosure";
        case MinutiaeCategory::Break: return "Break";
        case MinutiaeCategory::Overlap: return "Overlap";
        case MinutiaeCategory::Crossbar: return "Crossbar";
        case MinutiaeCategory::Bridge: return "Bridge";
        case MinutiaeCategory::OppositeBifurcation: return "Opposite Bifurcation";
        case MinutiaeCategory::Dock: return "Dock";
        case MinutiaeCategory::Trifurcation: return "Trifurcation";
        case MinutiaeCategory::M: return "M";
        case MinutiaeCategory::Return: return "Return";
        case MinutiaeCategory::Appendage: return "Appendage";
        case MinutiaeCategory::Needle: return "Needle";
        case MinutiaeCategory::Numerical: return "Numerical";
        case MinutiaeCategory::Tripod: return "Tripod";
        case MinutiaeCategory::Emboque: return "Emboque";
        case MinutiaeCategory::Conjugation: return "Conjugation";
        case MinutiaeCategory::Angular: return "Angular";
        case MinutiaeCategory::Other: return "Other";
        default: return "Unknown";
    }
}

QString MinutiaeData::classificationToString() const {
    switch (classification) {
        case MinutiaeClassification::Fundamental: return "Fundamental";
        case MinutiaeClassification::Common: return "Common";
        case MinutiaeClassification::Rare: return "Rare";
        default: return "Unknown";
    }
}

QJsonObject MinutiaeData::toJson() const {
    QJsonObject json;
    json["id"] = id;
    json["name_en"] = nameEnglish;
    json["name_pt"] = namePortuguese;
    json["description_en"] = descriptionEnglish;
    json["description_pt"] = descriptionPortuguese;
    json["image_path"] = imagePath;
    json["category"] = categoryToString();
    json["classification"] = classificationToString();
    json["frequency_general"] = frequencyGeneral;
    
    QJsonObject stats;
    stats["mean"] = statistics.mean;
    stats["median"] = statistics.median;
    stats["std_dev"] = statistics.standardDeviation;
    json["stats"] = stats;
    
    QJsonObject freqPattern;
    for (auto it = frequencyByPattern.begin(); it != frequencyByPattern.end(); ++it) {
        freqPattern[it.key()] = it.value();
    }
    json["frequency_by_pattern"] = freqPattern;
    
    QJsonObject freqFinger;
    for (auto it = frequencyByFinger.begin(); it != frequencyByFinger.end(); ++it) {
        freqFinger[it.key()] = it.value();
    }
    json["frequency_by_finger"] = freqFinger;
    
    return json;
}

MinutiaeData MinutiaeData::fromJson(const QJsonObject& json) {
    MinutiaeData data;
    data.id = json["id"].toInt();
    data.nameEnglish = json["name_en"].toString();
    data.namePortuguese = json["name_pt"].toString();
    data.descriptionEnglish = json["description_en"].toString();
    data.descriptionPortuguese = json["description_pt"].toString();
    data.imagePath = json["image_path"].toString();
    data.frequencyGeneral = json["frequency_general"].toDouble();
    
    // Parse category
    QString categoryStr = json["category"].toString();
    if (categoryStr == "Ridge Ending") data.category = MinutiaeCategory::RidgeEnding;
    else if (categoryStr == "Bifurcation") data.category = MinutiaeCategory::Bifurcation;
    else if (categoryStr == "Convergence") data.category = MinutiaeCategory::Convergence;
    else if (categoryStr == "Fragment") data.category = MinutiaeCategory::Fragment;
    else if (categoryStr == "Enclosure") data.category = MinutiaeCategory::Enclosure;
    else if (categoryStr == "Break") data.category = MinutiaeCategory::Break;
    else if (categoryStr == "Overlap") data.category = MinutiaeCategory::Overlap;
    else if (categoryStr == "Crossbar") data.category = MinutiaeCategory::Crossbar;
    else if (categoryStr == "Bridge") data.category = MinutiaeCategory::Bridge;
    else if (categoryStr == "Opposite Bifurcation") data.category = MinutiaeCategory::OppositeBifurcation;
    else if (categoryStr == "Dock") data.category = MinutiaeCategory::Dock;
    else if (categoryStr == "Trifurcation") data.category = MinutiaeCategory::Trifurcation;
    else if (categoryStr == "M") data.category = MinutiaeCategory::M;
    else if (categoryStr == "Return") data.category = MinutiaeCategory::Return;
    else if (categoryStr == "Appendage") data.category = MinutiaeCategory::Appendage;
    else if (categoryStr == "Needle") data.category = MinutiaeCategory::Needle;
    else if (categoryStr == "Numerical") data.category = MinutiaeCategory::Numerical;
    else if (categoryStr == "Tripod") data.category = MinutiaeCategory::Tripod;
    else if (categoryStr == "Emboque") data.category = MinutiaeCategory::Emboque;
    else if (categoryStr == "Conjugation") data.category = MinutiaeCategory::Conjugation;
    else if (categoryStr == "Angular") data.category = MinutiaeCategory::Angular;
    else data.category = MinutiaeCategory::Other;
    
    // Parse classification
    QString classStr = json["classification"].toString();
    if (classStr == "Fundamental") data.classification = MinutiaeClassification::Fundamental;
    else if (classStr == "Common") data.classification = MinutiaeClassification::Common;
    else data.classification = MinutiaeClassification::Rare;
    
    // Parse statistics
    QJsonObject stats = json["stats"].toObject();
    data.statistics.mean = stats["mean"].toDouble();
    data.statistics.median = stats["median"].toDouble();
    data.statistics.standardDeviation = stats["std_dev"].toDouble();
    
    // Parse frequency by pattern
    QJsonObject freqPattern = json["frequency_by_pattern"].toObject();
    for (const QString& key : freqPattern.keys()) {
        data.frequencyByPattern[key] = freqPattern[key].toDouble();
    }
    
    // Parse frequency by finger
    QJsonObject freqFinger = json["frequency_by_finger"].toObject();
    for (const QString& key : freqFinger.keys()) {
        data.frequencyByFinger[key] = freqFinger[key].toDouble();
    }
    
    return data;
}

// MinutiaeCatalog implementation

MinutiaeCatalog::MinutiaeCatalog() {
}

MinutiaeCatalog::~MinutiaeCatalog() {
}

bool MinutiaeCatalog::loadFromJson(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file:" << filePath;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        qWarning() << "Invalid JSON document";
        return false;
    }
    
    QJsonObject root = doc.object();
    QJsonArray minutiaeArray = root["minutiae_catalog"].toArray();
    
    m_minutiae.clear();
    m_idToIndex.clear();
    
    for (const QJsonValue& value : minutiaeArray) {
        if (value.isObject()) {
            MinutiaeData minutiae = MinutiaeData::fromJson(value.toObject());
            addMinutiae(minutiae);
        }
    }
    
    return true;
}

bool MinutiaeCatalog::saveToJson(const QString& filePath) const {
    QJsonArray minutiaeArray;
    for (const MinutiaeData& minutiae : m_minutiae) {
        minutiaeArray.append(minutiae.toJson());
    }
    
    QJsonObject root;
    root["minutiae_catalog"] = minutiaeArray;
    
    QJsonDocument doc(root);
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << filePath;
        return false;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    return true;
}

QVector<MinutiaeData> MinutiaeCatalog::getAllMinutiae() const {
    return m_minutiae;
}

const MinutiaeData* MinutiaeCatalog::getMinutiaeById(int id) const {
    if (m_idToIndex.contains(id)) {
        int index = m_idToIndex[id];
        return &m_minutiae[index];
    }
    return nullptr;
}

QVector<MinutiaeData> MinutiaeCatalog::getMinutiaeByCategory(MinutiaeCategory category) const {
    QVector<MinutiaeData> result;
    for (const MinutiaeData& minutiae : m_minutiae) {
        if (minutiae.category == category) {
            result.append(minutiae);
        }
    }
    return result;
}

QVector<MinutiaeData> MinutiaeCatalog::getMinutiaeByClassification(MinutiaeClassification classification) const {
    QVector<MinutiaeData> result;
    for (const MinutiaeData& minutiae : m_minutiae) {
        if (minutiae.classification == classification) {
            result.append(minutiae);
        }
    }
    return result;
}

QVector<MinutiaeData> MinutiaeCatalog::getFundamentalMinutiae() const {
    return getMinutiaeByClassification(MinutiaeClassification::Fundamental);
}

int MinutiaeCatalog::getCount() const {
    return m_minutiae.size();
}

void MinutiaeCatalog::addMinutiae(const MinutiaeData& minutiae) {
    m_minutiae.append(minutiae);
    rebuildIndex();
}

bool MinutiaeCatalog::removeMinutiae(int id) {
    if (m_idToIndex.contains(id)) {
        int index = m_idToIndex[id];
        m_minutiae.remove(index);
        rebuildIndex();
        return true;
    }
    return false;
}

void MinutiaeCatalog::clear() {
    m_minutiae.clear();
    m_idToIndex.clear();
}

void MinutiaeCatalog::rebuildIndex() {
    m_idToIndex.clear();
    for (int i = 0; i < m_minutiae.size(); ++i) {
        m_idToIndex[m_minutiae[i].id] = i;
    }
}

} // namespace FingerprintAnalysis

