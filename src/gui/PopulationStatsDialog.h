#ifndef POPULATION_STATS_DIALOG_H
#define POPULATION_STATS_DIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QLabel>
#include "../knolegment/MinutiaeCatalog.h"

/**
 * @brief Diálogo para exibir estatísticas populacionais de minúcias
 */
class PopulationStatsDialog : public QDialog {
    Q_OBJECT

public:
    explicit PopulationStatsDialog(QWidget *parent = nullptr);
    ~PopulationStatsDialog() = default;

private slots:
    void loadStatistics();

private:
    void setupUI();
    void createOverviewTab();
    void createClassificationTab();
    void createFrequencyTab();
    void createChartsTab();
    
    void populateOverview();
    void populateClassificationTable();
    void populateFrequencyTable();
    void createFrequencyChart();

    FingerprintAnalysis::MinutiaeCatalog catalog;
    
    QTabWidget* tabWidget;
    QTextEdit* overviewText;
    QTableWidget* classificationTable;
    QTableWidget* frequencyTable;
    QTextEdit* chartText;
    QPushButton* closeButton;
};

#endif // POPULATION_STATS_DIALOG_H
