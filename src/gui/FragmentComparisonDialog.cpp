#include "FragmentComparisonDialog.h"
#include <QSplitter>
#include <QMessageBox>
#include <QtConcurrent>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QTimer>
#include <cmath>

FragmentComparisonDialog::FragmentComparisonDialog(QWidget *parent)
    : QDialog(parent)
    , project(nullptr)
    , comparisonWatcher(nullptr)
{
    setWindowTitle("Comparação 1:1 de Fragmentos");
    resize(1400, 800);
    
    setupUI();
    
    // Configurar watcher para processamento assíncrono
    comparisonWatcher = new QFutureWatcher<FragmentComparisonResult>(this);
    connect(comparisonWatcher, &QFutureWatcher<FragmentComparisonResult>::finished,
            this, &FragmentComparisonDialog::onComparisonFinished);
}

FragmentComparisonDialog::~FragmentComparisonDialog() {
    if (comparisonWatcher && comparisonWatcher->isRunning()) {
        comparisonWatcher->cancel();
        comparisonWatcher->waitForFinished();
    }
}

void FragmentComparisonDialog::setupUI() {
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    
    // ==================== PAINEL ESQUERDO: Visualização das Imagens ====================
    QWidget* leftPanel = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    
    // Título dos viewers
    QHBoxLayout* titleLayout = new QHBoxLayout();
    QLabel* title1 = new QLabel("Fragmento 1");
    QFont titleFont = title1->font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    title1->setFont(titleFont);
    title1->setAlignment(Qt::AlignCenter);
    
    QLabel* title2 = new QLabel("Fragmento 2");
    title2->setFont(titleFont);
    title2->setAlignment(Qt::AlignCenter);
    
    titleLayout->addWidget(title1);
    titleLayout->addWidget(title2);
    leftLayout->addLayout(titleLayout);
    
    // Viewers lado a lado
    QSplitter* viewerSplitter = new QSplitter(Qt::Horizontal);
    
    // Viewer 1
    viewer1 = new ImageViewer();
    viewer1->setMinimumSize(400, 400);
    viewerSplitter->addWidget(viewer1);
    
    // Viewer 2
    viewer2 = new ImageViewer();
    viewer2->setMinimumSize(400, 400);
    viewerSplitter->addWidget(viewer2);
    
    viewerSplitter->setSizes(QList<int>() << 500 << 500);
    leftLayout->addWidget(viewerSplitter, 1);
    
    mainLayout->addWidget(leftPanel, 3);  // 75% do espaço
    
    // ==================== PAINEL DIREITO: Controles e Resultados ====================
    QWidget* rightPanel = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightPanel->setMaximumWidth(400);
    
    // ==================== Seleção de Fragmentos ====================
    QGroupBox* selectionGroup = new QGroupBox("Seleção de Fragmentos");
    QVBoxLayout* selectionLayout = new QVBoxLayout(selectionGroup);
    
    // Fragmento 1
    QLabel* label1 = new QLabel("Fragmento 1:");
    fragment1Combo = new QComboBox();
    selectionLayout->addWidget(label1);
    selectionLayout->addWidget(fragment1Combo);
    
    // Fragmento 2
    QLabel* label2 = new QLabel("Fragmento 2:");
    fragment2Combo = new QComboBox();
    selectionLayout->addWidget(label2);
    selectionLayout->addWidget(fragment2Combo);
    
    // Botão de troca
    swapButton = new QPushButton("⇄ Trocar Fragmentos");
    swapButton->setToolTip("Inverter fragmentos 1 e 2");
    selectionLayout->addWidget(swapButton);
    
    rightLayout->addWidget(selectionGroup);
    
    // ==================== Controles e Resultados ====================
    QGroupBox* controlGroup = new QGroupBox("Comparação");
    QVBoxLayout* controlLayout = new QVBoxLayout(controlGroup);
    
    // ========== Parâmetros de Comparação ==========
    QGroupBox* paramsGroup = new QGroupBox("Parâmetros de Matching");
    QFormLayout* paramsLayout = new QFormLayout(paramsGroup);
    
    // Tolerância de posição
    positionToleranceSpinBox = new QDoubleSpinBox();
    positionToleranceSpinBox->setRange(1.0, 100.0);
    positionToleranceSpinBox->setValue(15.0);
    positionToleranceSpinBox->setSuffix(" pixels");
    positionToleranceSpinBox->setToolTip("Distância máxima entre minúcias para considerar correspondência");
    paramsLayout->addRow("Tolerância de Posição:", positionToleranceSpinBox);
    
    // Tolerância angular
    angleToleranceSpinBox = new QDoubleSpinBox();
    angleToleranceSpinBox->setRange(0.05, 1.57);  // ~3° a 90°
    angleToleranceSpinBox->setValue(0.3);
    angleToleranceSpinBox->setSingleStep(0.05);
    angleToleranceSpinBox->setDecimals(2);
    angleToleranceSpinBox->setSuffix(" rad");
    angleToleranceSpinBox->setToolTip("Diferença angular máxima em radianos (0.3 rad ≈ 17°)");
    paramsLayout->addRow("Tolerância Angular:", angleToleranceSpinBox);
    
    // Score mínimo
    minScoreSpinBox = new QDoubleSpinBox();
    minScoreSpinBox->setRange(0.0, 1.0);
    minScoreSpinBox->setValue(0.5);
    minScoreSpinBox->setSingleStep(0.05);
    minScoreSpinBox->setDecimals(2);
    minScoreSpinBox->setToolTip("Score mínimo de similaridade local para aceitar correspondência");
    paramsLayout->addRow("Score Mínimo:", minScoreSpinBox);
    
    // Usar peso de tipo
    useTypeWeightingCheckBox = new QCheckBox("Considerar tipo de minúcia");
    useTypeWeightingCheckBox->setChecked(true);
    useTypeWeightingCheckBox->setToolTip("Dar mais peso para minúcias do mesmo tipo");
    paramsLayout->addRow("", useTypeWeightingCheckBox);
    
    // Usar peso de qualidade
    useQualityWeightingCheckBox = new QCheckBox("Considerar qualidade");
    useQualityWeightingCheckBox->setChecked(true);
    useQualityWeightingCheckBox->setToolTip("Considerar qualidade das minúcias no cálculo");
    paramsLayout->addRow("", useQualityWeightingCheckBox);
    
    controlLayout->addWidget(paramsGroup);
    
    // Botão de comparar
    compareButton = new QPushButton("▶ Calcular Score");
    compareButton->setEnabled(false);
    compareButton->setMinimumHeight(40);
    QFont buttonFont = compareButton->font();
    buttonFont.setPointSize(11);
    buttonFont.setBold(true);
    compareButton->setFont(buttonFont);
    controlLayout->addWidget(compareButton);
    
    // Barra de progresso
    progressBar = new QProgressBar();
    progressBar->setVisible(false);
    progressBar->setRange(0, 0);  // Indeterminado
    controlLayout->addWidget(progressBar);
    
    rightLayout->addWidget(controlGroup);
    
    // ==================== Resultados ====================
    QGroupBox* resultsGroup = new QGroupBox("Resultados");
    QVBoxLayout* resultsLayout = new QVBoxLayout(resultsGroup);
    
    resultLikelihoodLabel = new QLabel("LR: -");
    resultLogLRLabel = new QLabel("Log₁₀(LR): -");
    resultScoreLabel = new QLabel("Score: -");
    resultMatchedLabel = new QLabel("Matches: -");
    resultInterpretationLabel = new QLabel("Interpretação: -");
    resultTimeLabel = new QLabel("Tempo: -");
    
    QFont resultFont;
    resultFont.setPointSize(9);
    resultLikelihoodLabel->setFont(resultFont);
    resultLogLRLabel->setFont(resultFont);
    resultScoreLabel->setFont(resultFont);
    resultMatchedLabel->setFont(resultFont);
    resultTimeLabel->setFont(resultFont);
    
    resultLikelihoodLabel->setWordWrap(true);
    resultLogLRLabel->setWordWrap(true);
    resultScoreLabel->setWordWrap(true);
    resultMatchedLabel->setWordWrap(true);
    resultTimeLabel->setWordWrap(true);
    
    QFont interpretFont = resultFont;
    interpretFont.setBold(true);
    resultInterpretationLabel->setFont(interpretFont);
    resultInterpretationLabel->setWordWrap(true);
    
    resultsLayout->addWidget(resultLikelihoodLabel);
    resultsLayout->addWidget(resultLogLRLabel);
    resultsLayout->addWidget(resultScoreLabel);
    resultsLayout->addWidget(resultMatchedLabel);
    resultsLayout->addWidget(resultInterpretationLabel);
    resultsLayout->addWidget(resultTimeLabel);
    
    rightLayout->addWidget(resultsGroup);
    
    // Espaço expansível
    rightLayout->addStretch();
    
    // ==================== Botões de Ação ====================
    closeButton = new QPushButton("Fechar");
    rightLayout->addWidget(closeButton);
    
    mainLayout->addWidget(rightPanel, 1);  // 25% do espaço
    
    // ==================== Criar overlays DEPOIS dos viewers ====================
    // Os overlays precisam ser criados como filhos dos viewers
    overlay1 = new FingerprintEnhancer::MinutiaeOverlay(viewer1);
    overlay2 = new FingerprintEnhancer::MinutiaeOverlay(viewer2);
    
    // ==================== Conectar Sinais ====================
    connect(fragment1Combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FragmentComparisonDialog::onFragment1Changed);
    connect(fragment2Combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FragmentComparisonDialog::onFragment2Changed);
    connect(swapButton, &QPushButton::clicked,
            this, &FragmentComparisonDialog::onSwapFragments);
    connect(compareButton, &QPushButton::clicked,
            this, &FragmentComparisonDialog::onCompareClicked);
    connect(closeButton, &QPushButton::clicked,
            this, &QDialog::accept);
}

void FragmentComparisonDialog::setProject(FingerprintEnhancer::Project* proj) {
    project = proj;
    loadFragmentsList();
}

void FragmentComparisonDialog::loadFragmentsList() {
    fragment1Combo->clear();
    fragment2Combo->clear();
    availableFragments.clear();
    
    if (!project) return;
    
    // Coletar todos os fragmentos de todas as imagens
    for (auto& image : project->images) {
        QString imageName = QFileInfo(image.originalFilePath).fileName();
        for (auto& fragment : image.fragments) {
            availableFragments.append(&fragment);
            
            QString displayName = QString("%1 - Fragmento %2 (%3 minúcias)")
                .arg(imageName)
                .arg(fragment.id)
                .arg(fragment.minutiae.size());
            
            fragment1Combo->addItem(displayName);
            fragment2Combo->addItem(displayName);
        }
    }
    
    // Pré-selecionar diferentes fragmentos se houver pelo menos 2
    if (availableFragments.size() >= 2) {
        fragment1Combo->setCurrentIndex(0);
        fragment2Combo->setCurrentIndex(1);
    }
}

void FragmentComparisonDialog::selectFragments(const QString& fragment1Id, const QString& fragment2Id) {
    // Encontrar índices dos fragmentos
    for (int i = 0; i < availableFragments.size(); i++) {
        if (availableFragments[i]->id == fragment1Id) {
            fragment1Combo->setCurrentIndex(i);
        }
        if (availableFragments[i]->id == fragment2Id) {
            fragment2Combo->setCurrentIndex(i);
        }
    }
}

void FragmentComparisonDialog::onFragment1Changed(int index) {
    if (index < 0 || index >= availableFragments.size()) return;
    
    FingerprintEnhancer::Fragment* fragment = availableFragments[index];
    displayFragment(fragment, viewer1, overlay1);
    
    // Habilitar botão se ambos fragmentos estão selecionados
    compareButton->setEnabled(fragment1Combo->currentIndex() >= 0 &&
                             fragment2Combo->currentIndex() >= 0 &&
                             fragment1Combo->currentIndex() != fragment2Combo->currentIndex());
    
    clearResults();
}

void FragmentComparisonDialog::onFragment2Changed(int index) {
    if (index < 0 || index >= availableFragments.size()) return;
    
    FingerprintEnhancer::Fragment* fragment = availableFragments[index];
    displayFragment(fragment, viewer2, overlay2);
    
    // Habilitar botão se ambos fragmentos estão selecionados
    compareButton->setEnabled(fragment1Combo->currentIndex() >= 0 &&
                             fragment2Combo->currentIndex() >= 0 &&
                             fragment1Combo->currentIndex() != fragment2Combo->currentIndex());
    
    clearResults();
}

void FragmentComparisonDialog::onSwapFragments() {
    int temp = fragment1Combo->currentIndex();
    fragment1Combo->setCurrentIndex(fragment2Combo->currentIndex());
    fragment2Combo->setCurrentIndex(temp);
}

void FragmentComparisonDialog::displayFragment(FingerprintEnhancer::Fragment* fragment,
                                               ImageViewer* viewer,
                                               FingerprintEnhancer::MinutiaeOverlay* overlay) {
    if (!fragment) {
        fprintf(stderr, "[DISPLAY] Fragmento nulo - nada a exibir\n");
        return;
    }
    
    fprintf(stderr, "[DISPLAY] Exibindo fragmento %s com %d minúcias\n",
            fragment->id.toStdString().c_str(), fragment->minutiae.size());
    
    // Exibir imagem do fragmento
    viewer->setImage(fragment->workingImage);
    viewer->zoomToFit();
    
    // Configurar overlay de minúcias
    overlay->setFragment(fragment);
    overlay->setEditMode(false);  // Apenas visualização
    
    // Garantir que minúcias sejam exibidas com configurações padrão
    overlay->setShowLabels(true);
    overlay->setShowAngles(true);
    
    // Sincronizar overlay com viewer
    QTimer::singleShot(100, [this, viewer, overlay]() {
        // Dar tempo para o viewer processar a imagem
        overlay->setGeometry(viewer->rect());
        overlay->setScaleFactor(viewer->getScaleFactor());
        overlay->setScrollOffset(QPoint(0, 0));
        overlay->setImageOffset(viewer->getImageOffset());
        
        // Forçar atualização
        overlay->raise();
        overlay->setVisible(true);
        overlay->update();
        
        fprintf(stderr, "[DISPLAY] Overlay atualizado: geometry=%dx%d, scale=%.2f\n",
                overlay->width(), overlay->height(), viewer->getScaleFactor());
    });
    
    fprintf(stderr, "[DISPLAY] Fragmento exibido com sucesso\n");
}

void FragmentComparisonDialog::clearResults() {
    resultLikelihoodLabel->setText("Likelihood Ratio (LR): -");
    resultLogLRLabel->setText("Log₁₀(LR): -");
    resultScoreLabel->setText("Score de Similaridade: -");
    resultMatchedLabel->setText("Minúcias Correspondentes: -");
    resultInterpretationLabel->setText("Interpretação: -");
    resultTimeLabel->setText("Tempo de Cálculo: -");
}

void FragmentComparisonDialog::onCompareClicked() {
    int idx1 = fragment1Combo->currentIndex();
    int idx2 = fragment2Combo->currentIndex();
    
    if (idx1 < 0 || idx2 < 0 || idx1 == idx2) {
        QMessageBox::warning(this, "Erro", "Selecione dois fragmentos diferentes.");
        return;
    }
    
    FingerprintEnhancer::Fragment* frag1 = availableFragments[idx1];
    FingerprintEnhancer::Fragment* frag2 = availableFragments[idx2];
    
    if (frag1->minutiae.isEmpty() || frag2->minutiae.isEmpty()) {
        QMessageBox::warning(this, "Erro",
            "Ambos os fragmentos devem ter minúcias marcadas.\n"
            "Use o editor de minúcias para marcar os pontos característicos.");
        return;
    }
    
    // Desabilitar controles durante processamento
    compareButton->setEnabled(false);
    fragment1Combo->setEnabled(false);
    fragment2Combo->setEnabled(false);
    swapButton->setEnabled(false);
    progressBar->setVisible(true);
    clearResults();
    
    // Ler configurações da UI
    AFISLikelihoodConfig config;
    config.positionTolerance = positionToleranceSpinBox->value();
    config.angleTolerance = angleToleranceSpinBox->value();
    config.minMatchScore = minScoreSpinBox->value();
    config.useTypeWeighting = useTypeWeightingCheckBox->isChecked();
    config.useQualityWeighting = useQualityWeightingCheckBox->isChecked();
    
    fprintf(stderr, "\n[COMPARISON] ========== INICIANDO COMPARAÇÃO ==========\n");
    fprintf(stderr, "[COMPARISON] Fragmento 1: %s (%d minúcias)\n", 
            frag1->id.toStdString().c_str(), frag1->minutiae.size());
    fprintf(stderr, "[COMPARISON] Fragmento 2: %s (%d minúcias)\n", 
            frag2->id.toStdString().c_str(), frag2->minutiae.size());
    fprintf(stderr, "[COMPARISON] Configurações:\n");
    fprintf(stderr, "[COMPARISON]   - Tolerância posição: %.1f pixels\n", config.positionTolerance);
    fprintf(stderr, "[COMPARISON]   - Tolerância ângulo: %.3f rad (%.1f°)\n", 
            config.angleTolerance, config.angleTolerance * 180.0 / M_PI);
    fprintf(stderr, "[COMPARISON]   - Score mínimo: %.2f\n", config.minMatchScore);
    fprintf(stderr, "[COMPARISON]   - Usar tipo: %s\n", config.useTypeWeighting ? "SIM" : "NÃO");
    fprintf(stderr, "[COMPARISON]   - Usar qualidade: %s\n", config.useQualityWeighting ? "SIM" : "NÃO");
    fprintf(stderr, "[COMPARISON] ==============================================\n\n");
    
    // Executar comparação em thread separada
    QVector<FingerprintEnhancer::Minutia> minutiae1 = frag1->minutiae;
    QVector<FingerprintEnhancer::Minutia> minutiae2 = frag2->minutiae;
    
    comparisonFuture = QtConcurrent::run([minutiae1, minutiae2, config]() {
        return compareFragments(minutiae1, minutiae2, config);
    });
    
    comparisonWatcher->setFuture(comparisonFuture);
}

void FragmentComparisonDialog::onComparisonFinished() {
    // Reabilitar controles
    compareButton->setEnabled(true);
    fragment1Combo->setEnabled(true);
    fragment2Combo->setEnabled(true);
    swapButton->setEnabled(true);
    progressBar->setVisible(false);
    
    // Obter resultado
    FragmentComparisonResult result = comparisonFuture.result();
    lastResult = result;
    
    // Exibir resultados
    displayResults(result);
    
    emit comparisonCompleted(result);
}

void FragmentComparisonDialog::displayResults(const FragmentComparisonResult& result) {
    // Likelihood Ratio
    if (result.likelihoodRatio > 1e10) {
        resultLikelihoodLabel->setText(QString("Likelihood Ratio (LR): > 10¹⁰ (muito alto)"));
    } else {
        resultLikelihoodLabel->setText(QString("Likelihood Ratio (LR): %1")
            .arg(result.likelihoodRatio, 0, 'e', 2));
    }
    
    // Log LR
    resultLogLRLabel->setText(QString("Log₁₀(LR): %1")
        .arg(result.logLR, 0, 'f', 2));
    
    // Score de similaridade
    resultScoreLabel->setText(QString("Score de Similaridade: %1%")
        .arg(result.similarityScore * 100.0, 0, 'f', 2));
    
    // Minúcias correspondentes
    resultMatchedLabel->setText(QString("Minúcias Correspondentes: %1 de %2 vs %3")
        .arg(result.matchedMinutiae)
        .arg(result.totalMinutiaeFragment1)
        .arg(result.totalMinutiaeFragment2));
    
    // Interpretação
    resultInterpretationLabel->setText(QString("Interpretação: %1")
        .arg(result.interpretationText));
    
    // Tempo
    resultTimeLabel->setText(QString("Tempo de Cálculo: %1 ms")
        .arg(result.executionTimeMs, 0, 'f', 1));
}

QString FragmentComparisonDialog::getInterpretationText(double logLR) {
    // Baseado em Neumann et al. (2007) e práticas forenses
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
    } else {
        return "Evidência MUITO FORTE de origens diferentes";
    }
}

// ==================== ALGORITMO DE COMPARAÇÃO ====================

FragmentComparisonResult FragmentComparisonDialog::compareFragments(
    const QVector<FingerprintEnhancer::Minutia>& minutiae1,
    const QVector<FingerprintEnhancer::Minutia>& minutiae2,
    const AFISLikelihoodConfig& config) {
    
    QElapsedTimer timer;
    timer.start();
    
    FragmentComparisonResult result;
    result.totalMinutiaeFragment1 = minutiae1.size();
    result.totalMinutiaeFragment2 = minutiae2.size();
    
    // Criar calculadora com configuração
    AFISLikelihoodCalculator calculator(config);
    
    // 1. Encontrar correspondências entre minúcias
    result.correspondences = calculator.findCorrespondences(minutiae1, minutiae2);
    result.matchedMinutiae = result.correspondences.size();
    
    // 2. Calcular Likelihood Ratio
    result.likelihoodRatio = calculator.calculateLikelihoodRatio(minutiae1, minutiae2, result.correspondences);
    result.logLR = (result.likelihoodRatio > 0) ? log10(result.likelihoodRatio) : -100.0;
    
    // 3. Calcular score de similaridade simples (0.0 a 1.0)
    result.similarityScore = calculator.calculateSimilarityScore(minutiae1, minutiae2, result.correspondences);
    
    // 4. Interpretação
    result.interpretationText = AFISLikelihoodCalculator::getInterpretationText(result.logLR);
    
    result.executionTimeMs = timer.elapsed();
    
    return result;
}

// Funções de cálculo migradas para AFISLikelihoodCalculator em src/afis/
