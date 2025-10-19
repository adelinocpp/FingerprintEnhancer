#ifndef MINUTIAE_QUERY_DIALOG_H
#define MINUTIAE_QUERY_DIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QTableWidget>
#include <QPushButton>
#include <QSplitter>
#include <QTextEdit>
#include <QGroupBox>
#include "../knolegment/MinutiaeCatalog.h"

/**
 * @brief Diálogo para consultar tipos de minúcias do catálogo
 */
class MinutiaeQueryDialog : public QDialog {
    Q_OBJECT

public:
    explicit MinutiaeQueryDialog(QWidget *parent = nullptr);
    ~MinutiaeQueryDialog() = default;

private slots:
    void onSearchTextChanged(const QString& text);
    void onCategoryFilterChanged(int index);
    void onClassificationFilterChanged(int index);
    void onMinutiaeSelected(int row, int column);
    void loadCatalogData();

private:
    void applyFilters();
    void setupUI();
    void populateTable(const QVector<FingerprintAnalysis::MinutiaeData>& minutiae);
    void updateMinutiaeDetails(const FingerprintAnalysis::MinutiaeData& minutiae);

    FingerprintAnalysis::MinutiaeCatalog catalog;
    
    QLineEdit* searchEdit;
    QComboBox* categoryCombo;
    QComboBox* classificationCombo;
    QTableWidget* minutiaeTable;
    QTextEdit* detailsText;
    QLabel* imageLabel;
    QPushButton* closeButton;
    
    QVector<FingerprintAnalysis::MinutiaeData> currentMinutiae;
};

#endif // MINUTIAE_QUERY_DIALOG_H
