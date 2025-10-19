#ifndef FRAGMENTCOMPARISONDIALOG_H
#define FRAGMENTCOMPARISONDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QComboBox>
#include <QPushButton>
#include <QProgressBar>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QFuture>
#include <QFutureWatcher>
#include "../core/ProjectModel.h"
#include "../core/MinutiaeTypes.h"
#include "../afis/AFISLikelihoodCalculator.h"
#include "../afis/FingerprintLRCalculator.h"
#include "ImageViewer.h"
#include "MinutiaeOverlay.h"

namespace FingerprintEnhancer {
    class Fragment;
    class Minutia;
}

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
    
    // Componentes detalhados do LR (Neumann et al.)
    FingerprintEnhancer::LRResult lrDetails;
    
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
    
    void setProject(FingerprintEnhancer::Project* project);
    
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
    
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
    void onVisualizeClicked();
    
    // Associação manual de minúcias
    void onAddManualMatch();
    void onRemoveManualMatch();
    void onClearManualMatches();
    void onUseManualMatchesToggled(bool checked);
    void onAutoMatchByIndex();
    void onManualMatchListClicked(QListWidgetItem* item);
    void onManualMatchListContextMenu(const QPoint& pos);
    
    // Seleção interativa
    void onViewer1Clicked(QPoint pos, Qt::KeyboardModifiers modifiers);
    void onViewer2Clicked(QPoint pos, Qt::KeyboardModifiers modifiers);
    void onViewerContextMenu(const QPoint& pos, int viewerIndex);
    void highlightAssociationInViewers(int matchIndex);
    
    // Sincronização de overlays com zoom/scroll
    void onViewer1ZoomChanged(double factor);
    void onViewer2ZoomChanged(double factor);
    void syncOverlay1();
    void syncOverlay2();
    
private:
    // UI Components
    QComboBox* fragment1Combo;
    QComboBox* fragment2Combo;
    QPushButton* swapButton;
    QPushButton* compareButton;
    QPushButton* visualizeButton;
    QPushButton* closeButton;
    QProgressBar* progressBar;
    
    // Campos de configuração AFIS
    QDoubleSpinBox* positionToleranceSpinBox;
    QCheckBox* useAngleCheckBox;
    QDoubleSpinBox* angleToleranceSpinBox;
    QDoubleSpinBox* minScoreSpinBox;
    QCheckBox* useTypeWeightingCheckBox;
    QCheckBox* useQualityWeightingCheckBox;
    
    // Campos de configuração LR (Neumann et al.)
    QComboBox* lrModeComboBox;
    QDoubleSpinBox* raritySpinBox;
    QLineEdit* patternLineEdit;
    
    // Viewers para exibição lado a lado
    ImageViewer* viewer1;
    ImageViewer* viewer2;
    FingerprintEnhancer::MinutiaeOverlay* overlay1;
    FingerprintEnhancer::MinutiaeOverlay* overlay2;
    
    // Labels de resultado
    QLabel* resultLikelihoodLabel;
    QLabel* resultLogLRLabel;
    QLabel* resultProbabilityLabel;  // P(H1|E) = LR/(1+LR)
    QLabel* resultScoreLabel;
    QLabel* resultMatchedLabel;
    QLabel* resultInterpretationLabel;
    QLabel* resultTimeLabel;
    
    // Labels dos componentes LR (Neumann et al.)
    QLabel* resultLRShapeLabel;
    QLabel* resultLRDirectionLabel;
    QLabel* resultLRTypeLabel;
    QLabel* resultRarityLabel;
    
    // Painel de associação manual
    QGroupBox* manualMatchGroup;
    QListWidget* manualMatchList;
    QSpinBox* minutia1IndexSpinBox;
    QSpinBox* minutia2IndexSpinBox;
    QPushButton* addMatchButton;
    QPushButton* autoMatchButton;
    QPushButton* removeMatchButton;
    QPushButton* clearMatchButton;
    QCheckBox* useManualMatchesCheckBox;
    QVector<QPair<int, int>> manualMatches;
    
    // Seleção interativa
    int selectedMinutia1;  // -1 se nenhuma selecionada
    int selectedMinutia2;  // -1 se nenhuma selecionada
    void updateOverlayHighlight();
    int findNearestMinutia(const QVector<FingerprintEnhancer::Minutia>& minutiae, 
                           QPointF scenePos, double maxDistance = 15.0);
    
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
                        FingerprintEnhancer::MinutiaeOverlay* overlay);
    void clearResults();
    void displayResults(const FragmentComparisonResult& result);
    static QString getInterpretationText(double logLR);
    
    // Conversão de coordenadas considerando zoom/scroll
    QPointF viewportToScene(const QPoint& viewportPos, ImageViewer* viewer);
    
    // Algoritmo de comparação (executado em thread)
    static FragmentComparisonResult compareFragments(
        const QVector<FingerprintEnhancer::Minutia>& minutiae1,
        const QVector<FingerprintEnhancer::Minutia>& minutiae2,
        const AFISLikelihoodConfig& config = AFISLikelihoodConfig());
};

#endif // FRAGMENTCOMPARISONDIALOG_H
