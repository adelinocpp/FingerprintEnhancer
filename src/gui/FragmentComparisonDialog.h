#ifndef FRAGMENTCOMPARISONDIALOG_H
#define FRAGMENTCOMPARISONDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QProgressBar>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFuture>
#include <QFutureWatcher>
#include "../core/ProjectModel.h"
#include "../core/MinutiaeTypes.h"
#include "ImageViewer.h"
#include "MinutiaeOverlay.h"

/**
 * @brief Resultado da comparação 1:1 entre fragmentos
 */
struct FragmentComparisonResult {
    double likelihoodRatio;          // LR (Likelihood Ratio) - razão de verossimilhança
    double logLR;                    // log10(LR) para visualização
    double similarityScore;          // Score de similaridade (0.0 a 1.0)
    int matchedMinutiae;             // Número de minúcias correspondentes
    int totalMinutiaeFragment1;      // Total de minúcias no fragmento 1
    int totalMinutiaeFragment2;      // Total de minúcias no fragmento 2
    QVector<QPair<int, int>> correspondences;  // Pares de índices de minúcias correspondentes
    QString interpretationText;      // Interpretação do resultado
    double executionTimeMs;          // Tempo de execução em ms
    
    FragmentComparisonResult()
        : likelihoodRatio(0.0), logLR(0.0), similarityScore(0.0),
          matchedMinutiae(0), totalMinutiaeFragment1(0),
          totalMinutiaeFragment2(0), executionTimeMs(0.0) {}
};

/**
 * @brief Dialog para comparação 1:1 de fragmentos de impressões digitais
 * 
 * Implementa cálculo de Likelihood Ratio baseado em:
 * - Neumann et al. (2007) - "Quantifying the weight of fingerprint evidence"
 * - Gomes et al. (2024) - Estatísticas de tipos de minúcias (dados brasileiros)
 * - openAFIS - Algoritmos de matching
 */
class FragmentComparisonDialog : public QDialog {
    Q_OBJECT

public:
    explicit FragmentComparisonDialog(QWidget *parent = nullptr);
    ~FragmentComparisonDialog();

    // Definir projeto atual
    void setProject(FingerprintEnhancer::Project* proj);
    
    // Pré-selecionar fragmentos (opcional)
    void selectFragments(const QString& fragment1Id, const QString& fragment2Id);

signals:
    void comparisonCompleted(const FragmentComparisonResult& result);

private slots:
    void onFragment1Changed(int index);
    void onFragment2Changed(int index);
    void onCompareClicked();
    void onComparisonFinished();
    void onSwapFragments();
    
private:
    // UI Components
    QComboBox* fragment1Combo;
    QComboBox* fragment2Combo;
    QPushButton* swapButton;
    QPushButton* compareButton;
    QPushButton* closeButton;
    QProgressBar* progressBar;
    
    // Viewers para exibição lado a lado
    ImageViewer* viewer1;
    ImageViewer* viewer2;
    MinutiaeOverlay* overlay1;
    MinutiaeOverlay* overlay2;
    
    // Labels de resultado
    QLabel* resultLikelihoodLabel;
    QLabel* resultLogLRLabel;
    QLabel* resultScoreLabel;
    QLabel* resultMatchedLabel;
    QLabel* resultInterpretationLabel;
    QLabel* resultTimeLabel;
    
    // Dados
    FingerprintEnhancer::Project* project;
    QVector<FingerprintEnhancer::Fragment*> availableFragments;
    FragmentComparisonResult lastResult;
    
    // Processamento assíncrono
    QFuture<FragmentComparisonResult> comparisonFuture;
    QFutureWatcher<FragmentComparisonResult>* comparisonWatcher;
    
    // Métodos privados
    void setupUI();
    void loadFragmentsList();
    void displayFragment(FingerprintEnhancer::Fragment* fragment,
                        ImageViewer* viewer,
                        MinutiaeOverlay* overlay);
    void clearResults();
    void displayResults(const FragmentComparisonResult& result);
    QString getInterpretationText(double logLR);
    
    // Algoritmo de comparação (executado em thread)
    static FragmentComparisonResult compareFragments(
        const QVector<MinutiaeData>& minutiae1,
        const QVector<MinutiaeData>& minutiae2);
    
    // Métodos de cálculo (baseados nos papers)
    static double calculateLikelihoodRatio(
        const QVector<MinutiaeData>& minutiae1,
        const QVector<MinutiaeData>& minutiae2,
        const QVector<QPair<int, int>>& correspondences);
    
    static QVector<QPair<int, int>> findCorrespondences(
        const QVector<MinutiaeData>& minutiae1,
        const QVector<MinutiaeData>& minutiae2,
        double positionTolerance = 15.0,
        double angleTolerance = 0.3);
    
    static double computeLocalSimilarity(
        const MinutiaeData& m1,
        const MinutiaeData& m2);
};

#endif // FRAGMENTCOMPARISONDIALOG_H
