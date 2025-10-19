#include "FragmentComparisonDialog.h"
#include "CorrespondenceVisualizationDialog.h"
#include <QSplitter>
#include <QMessageBox>
#include <QScrollBar>
#include <QtConcurrent>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QFormLayout>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QTimer>
#include <QEvent>
#include <QWheelEvent>
#include <QMenu>
#include <QApplication>
#include <cmath>

FragmentComparisonDialog::FragmentComparisonDialog(QWidget *parent)
    : QDialog(parent)
    , project(nullptr)
    , selectedMinutia1(-1)
    , selectedMinutia2(-1)
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
    
    // ==================== PAINEL DE ASSOCIAÇÃO MANUAL ====================
    manualMatchGroup = new QGroupBox("📌 Associação Manual de Minúcias");
    manualMatchGroup->setCheckable(false);
    QVBoxLayout* manualLayout = new QVBoxLayout(manualMatchGroup);
    
    // Checkbox para usar associações manuais
    useManualMatchesCheckBox = new QCheckBox("Usar associações manuais no cálculo do LR");
    useManualMatchesCheckBox->setChecked(false);
    useManualMatchesCheckBox->setToolTip("Quando ativado, as minúcias do fragmento 2 serão reordenadas conforme as associações manuais");
    manualLayout->addWidget(useManualMatchesCheckBox);
    
    // Controles de entrada
    QHBoxLayout* inputLayout = new QHBoxLayout();
    
    QLabel* lblMin1 = new QLabel("Minúcia Frag1:");
    minutia1IndexSpinBox = new QSpinBox();
    minutia1IndexSpinBox->setMinimum(1);
    minutia1IndexSpinBox->setMaximum(1);
    minutia1IndexSpinBox->setToolTip("Índice da minúcia no fragmento 1 (começando em 1)");
    
    QLabel* lblMin2 = new QLabel("↔ Frag2:");
    minutia2IndexSpinBox = new QSpinBox();
    minutia2IndexSpinBox->setMinimum(1);
    minutia2IndexSpinBox->setMaximum(1);
    minutia2IndexSpinBox->setToolTip("Índice da minúcia no fragmento 2 (começando em 1)");
    
    addMatchButton = new QPushButton("➕ Adicionar");
    addMatchButton->setToolTip("Adicionar associação manual");
    
    autoMatchButton = new QPushButton("🔗 Associar Todas");
    autoMatchButton->setToolTip("Associar automaticamente todas as minúcias por índice (1↔1, 2↔2, ...)");
    
    inputLayout->addWidget(lblMin1);
    inputLayout->addWidget(minutia1IndexSpinBox);
    inputLayout->addWidget(lblMin2);
    inputLayout->addWidget(minutia2IndexSpinBox);
    inputLayout->addWidget(addMatchButton);
    inputLayout->addWidget(autoMatchButton);
    inputLayout->addStretch();
    manualLayout->addLayout(inputLayout);
    
    // Lista de associações
    manualMatchList = new QListWidget();
    manualMatchList->setMaximumHeight(100);
    manualMatchList->setToolTip("Clique para destacar minúcias associadas | Botão direito para remover");
    manualMatchList->setContextMenuPolicy(Qt::CustomContextMenu);
    manualLayout->addWidget(manualMatchList);
    
    // Botões de gerenciamento
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    removeMatchButton = new QPushButton("🗑️ Remover Selecionado");
    clearMatchButton = new QPushButton("🧹 Limpar Todos");
    buttonLayout->addWidget(removeMatchButton);
    buttonLayout->addWidget(clearMatchButton);
    buttonLayout->addStretch();
    manualLayout->addLayout(buttonLayout);
    
    leftLayout->addWidget(manualMatchGroup);
    
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
    positionToleranceSpinBox->setRange(0.1, 10.0);
    positionToleranceSpinBox->setValue(3.0);
    positionToleranceSpinBox->setSingleStep(0.1);
    positionToleranceSpinBox->setDecimals(1);
    positionToleranceSpinBox->setSuffix(" mm");
    positionToleranceSpinBox->setToolTip("Distância máxima em milímetros entre minúcias para considerar correspondência");
    paramsLayout->addRow("Tolerância de Posição:", positionToleranceSpinBox);
    
    // Considerar ângulo (checkbox)
    useAngleCheckBox = new QCheckBox("Considerar ângulo das minúcias");
    useAngleCheckBox->setChecked(false);
    useAngleCheckBox->setToolTip("Se desabilitado, apenas posição é considerada no matching");
    paramsLayout->addRow("", useAngleCheckBox);
    
    // Tolerância angular
    angleToleranceSpinBox = new QDoubleSpinBox();
    angleToleranceSpinBox->setRange(0.05, 1.57);  // ~3° a 90°
    angleToleranceSpinBox->setValue(0.3);
    angleToleranceSpinBox->setSingleStep(0.05);
    angleToleranceSpinBox->setDecimals(2);
    angleToleranceSpinBox->setSuffix(" rad");
    angleToleranceSpinBox->setToolTip("Diferença angular máxima em radianos (0.3 rad ≈ 17°)");
    paramsLayout->addRow("Tolerância Angular:", angleToleranceSpinBox);
    
    // Conectar checkbox ao spinbox de ângulo
    connect(useAngleCheckBox, &QCheckBox::toggled, angleToleranceSpinBox, &QWidget::setEnabled);
    angleToleranceSpinBox->setEnabled(false);  // Inicia desabilitado (ângulo não considerado por padrão)
    
    // Score mínimo
    minScoreSpinBox = new QDoubleSpinBox();
    minScoreSpinBox->setRange(0.0, 1.0);
    minScoreSpinBox->setValue(0.0);
    minScoreSpinBox->setSingleStep(0.05);
    minScoreSpinBox->setDecimals(2);
    minScoreSpinBox->setToolTip("Score mínimo de similaridade local para aceitar correspondência");
    paramsLayout->addRow("Score Mínimo:", minScoreSpinBox);
    
    // Separador
    QFrame* line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    paramsLayout->addRow(line);
    
    // Usar peso de tipo
    useTypeWeightingCheckBox = new QCheckBox("Considerar tipo de minúcia");
    useTypeWeightingCheckBox->setChecked(false);
    useTypeWeightingCheckBox->setToolTip("Dar mais peso para minúcias do mesmo tipo");
    paramsLayout->addRow("", useTypeWeightingCheckBox);
    
    // Usar peso de qualidade
    useQualityWeightingCheckBox = new QCheckBox("Considerar qualidade");
    useQualityWeightingCheckBox->setChecked(false);
    useQualityWeightingCheckBox->setToolTip("Considerar qualidade das minúcias no cálculo");
    paramsLayout->addRow("", useQualityWeightingCheckBox);
    
    controlLayout->addWidget(paramsGroup);
    
    // ========== Parâmetros de Likelihood Ratio (LR) ==========
    QGroupBox* lrGroup = new QGroupBox("Likelihood Ratio (LR) - Neumann et al.");
    QFormLayout* lrLayout = new QFormLayout(lrGroup);
    
    // Modo de cálculo do LR
    lrModeComboBox = new QComboBox();
    lrModeComboBox->addItem("Shape Only (Forma)", static_cast<int>(FingerprintEnhancer::LRCalculationMode::SHAPE_ONLY));
    lrModeComboBox->addItem("Shape + Direction", static_cast<int>(FingerprintEnhancer::LRCalculationMode::SHAPE_DIRECTION));
    lrModeComboBox->addItem("Shape + Type", static_cast<int>(FingerprintEnhancer::LRCalculationMode::SHAPE_TYPE));
    lrModeComboBox->addItem("Completo (Shape+Dir+Type)", static_cast<int>(FingerprintEnhancer::LRCalculationMode::COMPLETE));
    lrModeComboBox->setCurrentIndex(3);  // Completo por padrão
    lrModeComboBox->setToolTip("Componentes do LR a calcular:\n"
                                "• Shape: Configuração espacial\n"
                                "• Direction: Ângulos das minúcias\n"
                                "• Type: Tipos das minúcias");
    lrLayout->addRow("Modo de Cálculo:", lrModeComboBox);
    
    // Raridade p(v=1|Hd)
    raritySpinBox = new QDoubleSpinBox();
    raritySpinBox->setRange(1e-9, 0.1);
    raritySpinBox->setValue(0.01);  // 1 em 100 (padrão)
    raritySpinBox->setDecimals(6);
    raritySpinBox->setSingleStep(0.001);
    raritySpinBox->setToolTip("p(v=1|Hd): Probabilidade de encontrar configuração similar na população\n"
                               "Valores típicos: 0.01 (1 em 100) a 0.000001 (1 em 1 milhão)");
    lrLayout->addRow("Raridade p(v=1|Hd):", raritySpinBox);
    
    // Padrão geral (opcional)
    patternLineEdit = new QLineEdit();
    patternLineEdit->setPlaceholderText("arch, left_loop, right_loop, whorl");
    patternLineEdit->setToolTip("Padrão geral da impressão (opcional)\n"
                                 "Usado para ajustar raridade populacional");
    lrLayout->addRow("Padrão (opcional):", patternLineEdit);
    
    controlLayout->addWidget(lrGroup);
    
    // Botão de comparar
    compareButton = new QPushButton("▶ Calcular Score e LR");
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
    resultProbabilityLabel = new QLabel("P(Hp|E): -");
    resultScoreLabel = new QLabel("Score: -");
    resultMatchedLabel = new QLabel("Matches: -");
    resultInterpretationLabel = new QLabel("Interpretação: -");
    resultTimeLabel = new QLabel("Tempo: -");
    
    QFont resultFont;
    resultFont.setPointSize(9);
    resultLikelihoodLabel->setFont(resultFont);
    resultLogLRLabel->setFont(resultFont);
    resultProbabilityLabel->setFont(resultFont);
    resultScoreLabel->setFont(resultFont);
    resultMatchedLabel->setFont(resultFont);
    resultTimeLabel->setFont(resultFont);
    
    resultLikelihoodLabel->setWordWrap(true);
    resultLogLRLabel->setWordWrap(true);
    resultProbabilityLabel->setWordWrap(true);
    resultProbabilityLabel->setToolTip("Probabilidade posterior calculada como P(Hp|E) = LR / (1 + LR)\n"
                                       "Assume priors iguais (50%/50%)");
    resultScoreLabel->setWordWrap(true);
    resultMatchedLabel->setWordWrap(true);
    resultTimeLabel->setWordWrap(true);
    
    QFont interpretFont = resultFont;
    interpretFont.setBold(true);
    resultInterpretationLabel->setFont(interpretFont);
    resultInterpretationLabel->setWordWrap(true);
    
    resultsLayout->addWidget(resultLikelihoodLabel);
    resultsLayout->addWidget(resultLogLRLabel);
    resultsLayout->addWidget(resultProbabilityLabel);
    resultsLayout->addWidget(resultScoreLabel);
    resultsLayout->addWidget(resultMatchedLabel);
    resultsLayout->addWidget(resultInterpretationLabel);
    resultsLayout->addWidget(resultTimeLabel);
    
    // Separador
    QFrame* separator = new QFrame();
    separator->setFrameShape(QFrame::HLine);
    separator->setFrameShadow(QFrame::Sunken);
    resultsLayout->addWidget(separator);
    
    // Labels dos componentes do LR
    QLabel* componentsTitle = new QLabel("Componentes do LR:");
    componentsTitle->setFont(interpretFont);
    resultsLayout->addWidget(componentsTitle);
    
    resultLRShapeLabel = new QLabel("LR_shape: -");
    resultLRDirectionLabel = new QLabel("LR_direction: -");
    resultLRTypeLabel = new QLabel("LR_type: -");
    resultRarityLabel = new QLabel("p(v=1|Hd): -");
    
    resultLRShapeLabel->setFont(resultFont);
    resultLRDirectionLabel->setFont(resultFont);
    resultLRTypeLabel->setFont(resultFont);
    resultRarityLabel->setFont(resultFont);
    
    resultLRShapeLabel->setWordWrap(true);
    resultLRDirectionLabel->setWordWrap(true);
    resultLRTypeLabel->setWordWrap(true);
    resultRarityLabel->setWordWrap(true);
    
    resultsLayout->addWidget(resultLRShapeLabel);
    resultsLayout->addWidget(resultLRDirectionLabel);
    resultsLayout->addWidget(resultLRTypeLabel);
    resultsLayout->addWidget(resultRarityLabel);
    
    rightLayout->addWidget(resultsGroup);
    
    // Espaço expansível
    rightLayout->addStretch();
    
    // ==================== Botões de Ação ====================
    visualizeButton = new QPushButton("🔍 Ver Correspondências");
    visualizeButton->setToolTip("Abrir visualização gráfica das minúcias correspondentes");
    visualizeButton->setEnabled(false);  // Habilitar após comparação
    rightLayout->addWidget(visualizeButton);
    
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
    connect(visualizeButton, &QPushButton::clicked,
            this, &FragmentComparisonDialog::onVisualizeClicked);
    connect(closeButton, &QPushButton::clicked,
            this, &QDialog::accept);
    
    // Conectar sinais de associação manual
    connect(addMatchButton, &QPushButton::clicked,
            this, &FragmentComparisonDialog::onAddManualMatch);
    connect(autoMatchButton, &QPushButton::clicked,
            this, &FragmentComparisonDialog::onAutoMatchByIndex);
    connect(removeMatchButton, &QPushButton::clicked,
            this, &FragmentComparisonDialog::onRemoveManualMatch);
    connect(clearMatchButton, &QPushButton::clicked,
            this, &FragmentComparisonDialog::onClearManualMatches);
    connect(useManualMatchesCheckBox, &QCheckBox::toggled,
            this, &FragmentComparisonDialog::onUseManualMatchesToggled);
    
    // Conectar sinais da lista de associações
    connect(manualMatchList, &QListWidget::itemClicked,
            this, &FragmentComparisonDialog::onManualMatchListClicked);
    connect(manualMatchList, &QListWidget::customContextMenuRequested,
            this, &FragmentComparisonDialog::onManualMatchListContextMenu);
    
    // Conectar sinais de clique dos overlays para seleção interativa
    connect(overlay1, &FingerprintEnhancer::MinutiaeOverlay::minutiaClicked,
            [this](const QString& minutiaId, const QPoint& pos) {
        Q_UNUSED(minutiaId);
        Q_UNUSED(pos);
        QPoint globalPos = overlay1->mapToGlobal(pos);
        onViewer1Clicked(overlay1->mapFromGlobal(globalPos), QApplication::keyboardModifiers());
    });
    
    connect(overlay2, &FingerprintEnhancer::MinutiaeOverlay::minutiaClicked,
            [this](const QString& minutiaId, const QPoint& pos) {
        Q_UNUSED(minutiaId);
        Q_UNUSED(pos);
        QPoint globalPos = overlay2->mapToGlobal(pos);
        onViewer2Clicked(overlay2->mapFromGlobal(globalPos), QApplication::keyboardModifiers());
    });
    
    // Habilitar captura de eventos de mouse nos overlays (sem modo de edição completo)
    overlay1->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    overlay2->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    overlay1->setMouseTracking(true);
    overlay2->setMouseTracking(true);
    
    // Conectar sinais de zoom/scroll dos viewers aos overlays
    connect(viewer1, &ImageViewer::zoomChanged,
            this, &FragmentComparisonDialog::onViewer1ZoomChanged);
    connect(viewer2, &ImageViewer::zoomChanged,
            this, &FragmentComparisonDialog::onViewer2ZoomChanged);
    
    // Conectar sinais das scrollbars para sincronizar overlays
    connect(viewer1->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, &FragmentComparisonDialog::syncOverlay1);
    connect(viewer1->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &FragmentComparisonDialog::syncOverlay1);
    connect(viewer2->horizontalScrollBar(), &QScrollBar::valueChanged,
            this, &FragmentComparisonDialog::syncOverlay2);
    connect(viewer2->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &FragmentComparisonDialog::syncOverlay2);
    
    // Instalar event filters para capturar eventos de scroll
    viewer1->installEventFilter(this);
    viewer2->installEventFilter(this);
}

void FragmentComparisonDialog::setProject(FingerprintEnhancer::Project* proj) {
    project = proj;
    loadFragmentsList();
}

void FragmentComparisonDialog::loadFragmentsList() {
    fragment1Combo->clear();
    fragment2Combo->clear();
    availableFragments.clear();  // ✅ LIMPAR vetor antes de popular
    
    if (!project) return;
    
    // Coletar APENAS fragmentos COM ESCALA definida
    int totalFragments = 0;
    int fragmentsWithScale = 0;
    
    for (auto& image : project->images) {
        for (auto& fragment : image.fragments) {
            totalFragments++;
            
            // FILTRO: Só aceitar fragmentos com escala definida
            if (!fragment.hasScale()) {
                fprintf(stderr, "[COMPARISON] Fragmento %s IGNORADO: sem escala definida\n",
                        fragment.id.toStdString().c_str());
                continue;
            }
            
            fragmentsWithScale++;
            
            // Obter número de exibição do fragmento (ex: "02-01")
            QString fragmentNumber = QString("%1-%2")
                .arg(image.displayNumber, 2, 10, QChar('0'))
                .arg(fragment.displayNumber, 2, 10, QChar('0'));
            
            // Nome do fragmento ou nome da imagem
            QString fragmentName = fragment.displayName.isEmpty() 
                ? QFileInfo(image.originalFilePath).fileName()
                : fragment.displayName;
            
            // Formato: "02-01: Nome (5 minúcias, 52.7 px/mm)"
            QString displayName = QString("%1: %2 (%3 minúcias, %4 px/mm)")
                .arg(fragmentNumber)
                .arg(fragmentName)
                .arg(fragment.minutiae.size())
                .arg(fragment.pixelsPerMM, 0, 'f', 1);
            
            fragment1Combo->addItem(displayName, fragment.id);
            fragment2Combo->addItem(displayName, fragment.id);
            availableFragments.append(&fragment);  // ✅ ADICIONAR ao vetor também!
            
            fprintf(stderr, "[COMPARISON] Fragmento %s: escala=%.2f px/mm\n",
                    fragment.id.toStdString().c_str(), fragment.pixelsPerMM);
        }
    }
    
    fprintf(stderr, "[COMPARISON] Total de fragmentos: %d, com escala: %d\n",
            totalFragments, fragmentsWithScale);
    fprintf(stderr, "[COMPARISON] Vetor availableFragments.size() = %d\n",
            static_cast<int>(availableFragments.size()));
    
    // Desabilitar botão de comparação se não houver fragmentos suficientes
    bool hasFragments = fragment1Combo->count() >= 2;
    compareButton->setEnabled(hasFragments);
    swapButton->setEnabled(hasFragments);
    
    if (!hasFragments && totalFragments > 0) {
        QMessageBox::information(this, "Sem Fragmentos Disponíveis",
            QString("Nenhum fragmento com escala definida encontrado.\n\n"
                    "Total de fragmentos: %1\n"
                    "Com escala definida: %2\n\n"
                    "Defina a escala dos fragmentos antes de compará-los.")
            .arg(totalFragments).arg(fragmentsWithScale));
    }
}

void FragmentComparisonDialog::selectFragments(const QString& fragment1Id, const QString& fragment2Id) {
    for (int i = 0; i < fragment1Combo->count(); i++) {
        if (fragment1Combo->itemData(i).toString() == fragment1Id) {
            fragment1Combo->setCurrentIndex(i);
        }
        if (fragment2Combo->itemData(i).toString() == fragment2Id) {
            fragment2Combo->setCurrentIndex(i);
        }
    }
}

void FragmentComparisonDialog::onFragment1Changed(int index) {
    if (index < 0 || !project) return;
    
    QString fragmentId = fragment1Combo->itemData(index).toString();
    FingerprintEnhancer::Fragment* fragment = project->findFragment(fragmentId);
    if (!fragment) return;
    
    displayFragment(fragment, viewer1, overlay1);
    
    // Atualizar máximo do spinbox de minúcia 1
    minutia1IndexSpinBox->setMaximum(qMax(1, static_cast<int>(fragment->minutiae.size())));
    
    // Limpar associações manuais ao trocar fragmento
    if (!manualMatches.isEmpty()) {
        manualMatches.clear();
        manualMatchList->clear();
        selectedMinutia1 = -1;
        selectedMinutia2 = -1;
        updateOverlayHighlight();
        qDebug() << "[MANUAL MATCH] Associações limpas ao trocar Fragmento 1";
    }
    
    // Habilitar botão se ambos fragmentos estão selecionados
    compareButton->setEnabled(fragment1Combo->currentIndex() >= 0 &&
                             fragment2Combo->currentIndex() >= 0 &&
                             fragment1Combo->currentIndex() != fragment2Combo->currentIndex());
    
    clearResults();
}

void FragmentComparisonDialog::onFragment2Changed(int index) {
    if (index < 0 || !project) return;
    
    QString fragmentId = fragment2Combo->itemData(index).toString();
    FingerprintEnhancer::Fragment* fragment = project->findFragment(fragmentId);
    if (!fragment) return;
    
    displayFragment(fragment, viewer2, overlay2);
    
    // Atualizar máximo do spinbox de minúcia 2
    minutia2IndexSpinBox->setMaximum(qMax(1, static_cast<int>(fragment->minutiae.size())));
    
    // Limpar associações manuais ao trocar fragmento
    if (!manualMatches.isEmpty()) {
        manualMatches.clear();
        manualMatchList->clear();
        selectedMinutia1 = -1;
        selectedMinutia2 = -1;
        updateOverlayHighlight();
        qDebug() << "[MANUAL MATCH] Associações limpas ao trocar Fragmento 2";
    }
    
    // Habilitar botão se ambos fragmentos estão selecionados
    compareButton->setEnabled(fragment1Combo->currentIndex() >= 0 &&
                             fragment2Combo->currentIndex() >= 0 &&
                             fragment1Combo->currentIndex() != fragment2Combo->currentIndex());
    
    clearResults();
}

void FragmentComparisonDialog::onSwapFragments() {
    // Limpar associações antes de trocar
    if (!manualMatches.isEmpty()) {
        manualMatches.clear();
        manualMatchList->clear();
        selectedMinutia1 = -1;
        selectedMinutia2 = -1;
        updateOverlayHighlight();
        qDebug() << "[MANUAL MATCH] Associações limpas ao trocar fragmentos (swap)";
    }
    
    int temp = fragment1Combo->currentIndex();
    fragment1Combo->setCurrentIndex(fragment2Combo->currentIndex());
    fragment2Combo->setCurrentIndex(temp);
}

void FragmentComparisonDialog::onVisualizeClicked() {
    if (lastResult.matchedMinutiae == 0) {
        QMessageBox::information(this, "Sem Correspondências",
            "Não há correspondências para visualizar.\n"
            "Execute uma comparação primeiro.");
        return;
    }
    
    // Pegar fragmentos atuais
    int idx1 = fragment1Combo->currentIndex();
    int idx2 = fragment2Combo->currentIndex();
    
    if (idx1 < 0 || idx1 >= availableFragments.size() ||
        idx2 < 0 || idx2 >= availableFragments.size()) {
        QMessageBox::warning(this, "Erro", "Fragmentos não encontrados.");
        return;
    }
    
    FingerprintEnhancer::Fragment* frag1 = availableFragments[idx1];
    FingerprintEnhancer::Fragment* frag2 = availableFragments[idx2];
    
    // Criar e mostrar diálogo de visualização
    CorrespondenceVisualizationDialog* vizDialog = new CorrespondenceVisualizationDialog(this);
    vizDialog->setData(
        frag1->workingImage,
        frag2->workingImage,
        frag1->minutiae,
        frag2->minutiae,
        lastResult.correspondences
    );
    
    vizDialog->exec();
    delete vizDialog;
}

void FragmentComparisonDialog::onViewer1ZoomChanged(double factor) {
    syncOverlay1();
}

void FragmentComparisonDialog::onViewer2ZoomChanged(double factor) {
    syncOverlay2();
}

void FragmentComparisonDialog::syncOverlay1() {
    if (!overlay1 || !viewer1) return;
    
    // Pegar posição do scroll do viewer
    QPoint scrollPos(viewer1->horizontalScrollBar()->value(),
                     viewer1->verticalScrollBar()->value());
    
    overlay1->setGeometry(viewer1->rect());
    overlay1->setScaleFactor(viewer1->getScaleFactor());
    overlay1->setScrollOffset(scrollPos);  // ✅ Usar scroll real do viewer
    overlay1->setImageOffset(viewer1->getImageOffset());
    overlay1->update();
    
    fprintf(stderr, "[SYNC] Overlay1: scale=%.2f, scroll=(%d,%d), geometry=%dx%d\n",
            viewer1->getScaleFactor(), scrollPos.x(), scrollPos.y(),
            overlay1->width(), overlay1->height());
}

void FragmentComparisonDialog::syncOverlay2() {
    if (!overlay2 || !viewer2) return;
    
    // Pegar posição do scroll do viewer
    QPoint scrollPos(viewer2->horizontalScrollBar()->value(),
                     viewer2->verticalScrollBar()->value());
    
    overlay2->setGeometry(viewer2->rect());
    overlay2->setScaleFactor(viewer2->getScaleFactor());
    overlay2->setScrollOffset(scrollPos);  // ✅ Usar scroll real do viewer
    overlay2->setImageOffset(viewer2->getImageOffset());
    overlay2->update();
    
    fprintf(stderr, "[SYNC] Overlay2: scale=%.2f, scroll=(%d,%d), geometry=%dx%d\n",
            viewer2->getScaleFactor(), scrollPos.x(), scrollPos.y(),
            overlay2->width(), overlay2->height());
}

bool FragmentComparisonDialog::eventFilter(QObject* obj, QEvent* event) {
    // Capturar eventos de scroll e resize dos viewers (NÃO Paint para evitar loop)
    if (obj == viewer1 || obj == viewer2) {
        if (event->type() == QEvent::Wheel || 
            event->type() == QEvent::Resize) {
            
            // Sincronizar overlay correspondente (delay para evitar múltiplas chamadas)
            QTimer::singleShot(10, this, [this, obj]() {
                if (obj == viewer1) {
                    syncOverlay1();
                } else if (obj == viewer2) {
                    syncOverlay2();
                }
            });
        }
    }
    
    return QDialog::eventFilter(obj, event);
}

void FragmentComparisonDialog::displayFragment(FingerprintEnhancer::Fragment* fragment,
                                               ImageViewer* viewer,
                                               FingerprintEnhancer::MinutiaeOverlay* overlay) {
    if (!fragment) {
        fprintf(stderr, "[DISPLAY] Fragmento nulo - nada a exibir\n");
        return;
    }
    
    fprintf(stderr, "[DISPLAY] Exibindo fragmento %s com %d minúcias\n",
            fragment->id.toStdString().c_str(), static_cast<int>(fragment->minutiae.size()));
    
    // Exibir imagem do fragmento
    viewer->setImage(fragment->workingImage);
    viewer->zoomToFit();
    
    // Configurar overlay de minúcias
    overlay->setFragment(fragment);
    overlay->setEditMode(false);  // Apenas visualização
    
    // Garantir que minúcias sejam exibidas com configurações padrão
    overlay->setShowLabels(true);
    overlay->setShowAngles(true);
    
    // Sincronizar overlay com viewer (imediatamente e com delay)
    auto syncFunc = [this, viewer, overlay]() {
        overlay->setGeometry(viewer->rect());
        overlay->setScaleFactor(viewer->getScaleFactor());
        overlay->setScrollOffset(QPoint(0, 0));
        overlay->setImageOffset(viewer->getImageOffset());
        overlay->raise();
        overlay->setVisible(true);
        overlay->update();
        
        fprintf(stderr, "[DISPLAY] Overlay atualizado: geometry=%dx%d, scale=%.2f\n",
                overlay->width(), overlay->height(), viewer->getScaleFactor());
    };
    
    // Sincronizar imediatamente
    syncFunc();
    
    // E novamente após processamento da imagem
    QTimer::singleShot(100, syncFunc);
    QTimer::singleShot(300, syncFunc);
    
    fprintf(stderr, "[DISPLAY] Fragmento exibido com sucesso\n");
}

void FragmentComparisonDialog::clearResults() {
    resultLikelihoodLabel->setText("Likelihood Ratio (LR): -");
    resultLogLRLabel->setText("Log₁₀(LR): -");
    resultScoreLabel->setText("Score de Similaridade: -");
    resultMatchedLabel->setText("Minúcias Correspondentes: -");
    resultInterpretationLabel->setText("Interpretação: -");
    resultTimeLabel->setText("Tempo de Cálculo: -");
    visualizeButton->setEnabled(false);
}

void FragmentComparisonDialog::onCompareClicked() {
    int idx1 = fragment1Combo->currentIndex();
    int idx2 = fragment2Combo->currentIndex();
    
    fprintf(stderr, "\n[COMPARISON] ========== BOTÃO COMPARAR CLICADO ==========\n");
    fprintf(stderr, "[COMPARISON] idx1=%d, idx2=%d, availableFragments.size()=%d\n",
            idx1, idx2, static_cast<int>(availableFragments.size()));
    
    if (idx1 < 0 || idx2 < 0 || idx1 == idx2) {
        QMessageBox::warning(this, "Erro", "Selecione dois fragmentos diferentes.");
        return;
    }
    
    // Validação crítica de índices antes de acessar availableFragments
    if (idx1 >= availableFragments.size() || idx2 >= availableFragments.size()) {
        fprintf(stderr, "[COMPARISON] ERRO CRÍTICO: Índices fora do range! idx1=%d, idx2=%d, size=%d\n",
                idx1, idx2, static_cast<int>(availableFragments.size()));
        QMessageBox::critical(this, "Erro Interno",
            QString("Erro ao acessar fragmentos (índices: %1, %2, tamanho: %3).\n"
                    "Tente reabrir a janela de comparação.")
            .arg(idx1).arg(idx2).arg(availableFragments.size()));
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
    double toleranceMM = positionToleranceSpinBox->value();
    
    // Calcular escala média dos dois fragmentos
    double avgScale = (frag1->pixelsPerMM + frag2->pixelsPerMM) / 2.0;
    
    // Converter tolerância de mm para pixels usando escala média
    double tolerancePixels = toleranceMM * avgScale;
    
    AFISLikelihoodConfig config;
    config.positionTolerance = tolerancePixels;
    
    // Se "usar ângulo" não estiver marcado, ignorar ângulo (tolerância muito alta)
    if (useAngleCheckBox->isChecked()) {
        config.angleTolerance = angleToleranceSpinBox->value();
    } else {
        config.angleTolerance = M_PI;  // 180° - praticamente ignora ângulo
    }
    
    config.minMatchScore = minScoreSpinBox->value();
    config.useTypeWeighting = useTypeWeightingCheckBox->isChecked();
    config.useQualityWeighting = useQualityWeightingCheckBox->isChecked();
    
    // Calcular hint de escala esperada (frag2 / frag1)
    // Isso ajuda o RANSAC a convergir mais rápido com diferenças grandes de escala
    if (frag1->pixelsPerMM > 0.1 && frag2->pixelsPerMM > 0.1) {
        config.scaleHint = frag2->pixelsPerMM / frag1->pixelsPerMM;
    }
    
    fprintf(stderr, "\n[COMPARISON] ========== INICIANDO COMPARAÇÃO ==========\n");
    fprintf(stderr, "[COMPARISON] Fragmento 1: %s (%d minúcias, escala=%.2f px/mm)\n", 
            frag1->id.toStdString().c_str(), static_cast<int>(frag1->minutiae.size()), frag1->pixelsPerMM);
    fprintf(stderr, "[COMPARISON] Fragmento 2: %s (%d minúcias, escala=%.2f px/mm)\n", 
            frag2->id.toStdString().c_str(), static_cast<int>(frag2->minutiae.size()), frag2->pixelsPerMM);
    fprintf(stderr, "[COMPARISON] Configurações:\n");
    fprintf(stderr, "[COMPARISON]   - Tolerância posição: %.1f mm (%.1f pixels, escala média=%.2f px/mm)\n", 
            toleranceMM, tolerancePixels, avgScale);
    
    if (useAngleCheckBox->isChecked()) {
        fprintf(stderr, "[COMPARISON]   - Tolerância ângulo: %.3f rad (%.1f°)\n", 
                config.angleTolerance, config.angleTolerance * 180.0 / M_PI);
    } else {
        fprintf(stderr, "[COMPARISON]   - Ângulo: IGNORADO (não considerado no matching)\n");
    }
    
    fprintf(stderr, "[COMPARISON]   - Score mínimo: %.2f\n", config.minMatchScore);
    fprintf(stderr, "[COMPARISON]   - Usar tipo: %s\n", config.useTypeWeighting ? "SIM" : "NÃO");
    fprintf(stderr, "[COMPARISON]   - Usar qualidade: %s\n", config.useQualityWeighting ? "SIM" : "NÃO");
    fprintf(stderr, "[COMPARISON]   - Hint de escala: %.3f (razão %.2f:%.2f px/mm)\n", 
            config.scaleHint, frag1->pixelsPerMM, frag2->pixelsPerMM);
    fprintf(stderr, "[COMPARISON] ==============================================\n\n");
    
    // Ler configurações do LR
    FingerprintEnhancer::LRCalculationMode lrMode = 
        static_cast<FingerprintEnhancer::LRCalculationMode>(lrModeComboBox->currentData().toInt());
    double rarity = raritySpinBox->value();
    QString pattern = patternLineEdit->text().trimmed().toLower();
    
    fprintf(stderr, "[COMPARISON] ========== CONFIGURAÇÃO LR ==========\n");
    fprintf(stderr, "[COMPARISON]   - Modo: %d\n", static_cast<int>(lrMode));
    fprintf(stderr, "[COMPARISON]   - Raridade p(v=1|Hd): %.2e\n", rarity);
    fprintf(stderr, "[COMPARISON]   - Padrão: %s\n", pattern.isEmpty() ? "não especificado" : pattern.toStdString().c_str());
    fprintf(stderr, "[COMPARISON] ==============================================\n\n");
    
    // Executar comparação em thread separada
    QVector<FingerprintEnhancer::Minutia> minutiae1 = frag1->minutiae;
    QVector<FingerprintEnhancer::Minutia> minutiae2 = frag2->minutiae;
    
    // Capturar ponteiros para fragmentos (para calcular LR)
    FingerprintEnhancer::Fragment* frag1Ptr = frag1;
    FingerprintEnhancer::Fragment* frag2Ptr = frag2;
    
    comparisonFuture = QtConcurrent::run([minutiae1, minutiae2, config, frag1Ptr, frag2Ptr, lrMode, rarity, pattern]() {
        // Primeiro calcular matching AFIS
        FragmentComparisonResult result = compareFragments(minutiae1, minutiae2, config);
        
        // Depois calcular LR usando Neumann et al.
        FingerprintEnhancer::FingerprintLRCalculator lrCalc;
        result.lrDetails = lrCalc.calculateLR(frag1Ptr, frag2Ptr, lrMode, pattern, rarity);
        
        // Atualizar resultado com valores do LR
        result.likelihoodRatio = result.lrDetails.lr_total;
        result.logLR = result.lrDetails.log10_lr_total;
        result.interpretationText = result.lrDetails.interpretation;
        
        return result;
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
    
    // Probabilidade P(Hp|E) = LR / (1 + LR)
    // Assume priors iguais (50%/50%)
    double probability = result.likelihoodRatio / (1.0 + result.likelihoodRatio);
    resultProbabilityLabel->setText(QString("P(Hp|E): %1% (priors iguais)")
        .arg(probability * 100.0, 0, 'f', 2));
    
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
    
    // Componentes detalhados do LR (Neumann et al.)
    if (result.lrDetails.k_minutiae > 0) {
        resultLRShapeLabel->setText(QString("LR_shape: %1 (configuração espacial)")
            .arg(result.lrDetails.lr_shape, 0, 'e', 2));
        
        resultLRDirectionLabel->setText(QString("LR_direction: %1 (ângulos)")
            .arg(result.lrDetails.lr_direction, 0, 'e', 2));
        
        resultLRTypeLabel->setText(QString("LR_type: %1 (tipos)")
            .arg(result.lrDetails.lr_type, 0, 'e', 2));
        
        resultRarityLabel->setText(QString("p(v=1|Hd): %1 (raridade)")
            .arg(result.lrDetails.p_v_hd, 0, 'e', 2));
    } else {
        resultLRShapeLabel->setText("LR_shape: -");
        resultLRDirectionLabel->setText("LR_direction: -");
        resultLRTypeLabel->setText("LR_type: -");
        resultRarityLabel->setText("p(v=1|Hd): -");
    }
    
    // Habilitar botão de visualização se houver correspondências
    visualizeButton->setEnabled(result.matchedMinutiae > 0);
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

// ==================== ASSOCIAÇÃO MANUAL DE MINÚCIAS ====================

void FragmentComparisonDialog::onAddManualMatch() {
    int idx1 = minutia1IndexSpinBox->value() - 1;  // Converter para 0-indexed
    int idx2 = minutia2IndexSpinBox->value() - 1;
    
    // Validar índices
    int frag1Idx = fragment1Combo->currentIndex();
    int frag2Idx = fragment2Combo->currentIndex();
    
    if (frag1Idx < 0 || frag1Idx >= availableFragments.size() ||
        frag2Idx < 0 || frag2Idx >= availableFragments.size()) {
        QMessageBox::warning(this, "Erro", "Selecione dois fragmentos válidos.");
        return;
    }
    
    FingerprintEnhancer::Fragment* frag1 = availableFragments[frag1Idx];
    FingerprintEnhancer::Fragment* frag2 = availableFragments[frag2Idx];
    
    if (idx1 < 0 || idx1 >= frag1->minutiae.size() ||
        idx2 < 0 || idx2 >= frag2->minutiae.size()) {
        QMessageBox::warning(this, "Erro", "Índices de minúcias inválidos.");
        return;
    }
    
    // Verificar se já existe
    for (const auto& match : manualMatches) {
        if (match.first == idx1 || match.second == idx2) {
            QMessageBox::warning(this, "Duplicado",
                QString("Já existe uma associação envolvendo estas minúcias.\n"
                        "Minúcia %1 do Frag1 ou Minúcia %2 do Frag2 já foram associadas.")
                .arg(idx1 + 1).arg(idx2 + 1));
            return;
        }
    }
    
    // Adicionar
    manualMatches.append(QPair<int, int>(idx1, idx2));
    manualMatchList->addItem(QString("%1 ↔ %2").arg(idx1 + 1).arg(idx2 + 1));
    
    qDebug() << QString("[MANUAL MATCH] Adicionada: Frag1[%1] ↔ Frag2[%2]")
        .arg(idx1).arg(idx2);
}

void FragmentComparisonDialog::onRemoveManualMatch() {
    int currentRow = manualMatchList->currentRow();
    if (currentRow < 0 || currentRow >= manualMatches.size()) {
        QMessageBox::information(this, "Nenhuma Seleção",
            "Selecione uma associação para remover.");
        return;
    }
    
    manualMatches.removeAt(currentRow);
    delete manualMatchList->takeItem(currentRow);
    
    qDebug() << QString("[MANUAL MATCH] Removida associação na posição %1").arg(currentRow);
}

void FragmentComparisonDialog::onClearManualMatches() {
    if (manualMatches.isEmpty()) return;
    
    auto reply = QMessageBox::question(this, "Confirmar Limpeza",
        QString("Deseja remover todas as %1 associações manuais?")
        .arg(manualMatches.size()),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        manualMatches.clear();
        manualMatchList->clear();
        qDebug() << "[MANUAL MATCH] Todas as associações foram limpas";
    }
}

void FragmentComparisonDialog::onUseManualMatchesToggled(bool checked) {
    qDebug() << QString("[MANUAL MATCH] Usar associações manuais: %1")
        .arg(checked ? "SIM" : "NÃO");
    
    if (checked && manualMatches.isEmpty()) {
        QMessageBox::information(this, "Nenhuma Associação",
            "Adicione associações manuais antes de ativar esta opção.");
        useManualMatchesCheckBox->setChecked(false);
    }
}

// ==================== SELEÇÃO INTERATIVA DE MINÚCIAS ====================

QPointF FragmentComparisonDialog::viewportToScene(const QPoint& viewportPos, ImageViewer* viewer) {
    // Converter posição do viewport para coordenadas da imagem (scene)
    QPoint widgetPos = viewer->viewport()->mapToParent(viewportPos);
    QPoint imagePos = viewer->widgetToImage(widgetPos);
    return QPointF(imagePos);
}

int FragmentComparisonDialog::findNearestMinutia(
    const QVector<FingerprintEnhancer::Minutia>& minutiae,
    QPointF scenePos, 
    double maxDistance) {
    
    int nearestIdx = -1;
    double minDist = maxDistance;
    
    for (int i = 0; i < minutiae.size(); ++i) {
        QPointF minutiaPos(minutiae[i].position.x(), minutiae[i].position.y());
        double dist = QLineF(scenePos, minutiaPos).length();
        
        if (dist < minDist) {
            minDist = dist;
            nearestIdx = i;
        }
    }
    
    return nearestIdx;
}

void FragmentComparisonDialog::updateOverlayHighlight() {
    // Atualizar destaque visual nos overlays
    int frag1Idx = fragment1Combo->currentIndex();
    int frag2Idx = fragment2Combo->currentIndex();
    
    if (frag1Idx < 0 || frag1Idx >= availableFragments.size() ||
        frag2Idx < 0 || frag2Idx >= availableFragments.size()) {
        return;
    }
    
    FingerprintEnhancer::Fragment* frag1 = availableFragments[frag1Idx];
    FingerprintEnhancer::Fragment* frag2 = availableFragments[frag2Idx];
    
    // Limpar seleção anterior
    overlay1->clearSelection();
    overlay2->clearSelection();
    
    // Destacar minúcias selecionadas
    if (selectedMinutia1 >= 0 && selectedMinutia1 < frag1->minutiae.size()) {
        overlay1->setSelectedMinutia(frag1->minutiae[selectedMinutia1].id);
    }
    
    if (selectedMinutia2 >= 0 && selectedMinutia2 < frag2->minutiae.size()) {
        overlay2->setSelectedMinutia(frag2->minutiae[selectedMinutia2].id);
    }
    
    overlay1->update();
    overlay2->update();
}

void FragmentComparisonDialog::onViewer1Clicked(QPoint pos, Qt::KeyboardModifiers modifiers) {
    int frag1Idx = fragment1Combo->currentIndex();
    if (frag1Idx < 0 || frag1Idx >= availableFragments.size()) return;
    
    FingerprintEnhancer::Fragment* frag1 = availableFragments[frag1Idx];
    
    // Converter posição do clique para coordenadas da imagem
    QPointF scenePos = viewportToScene(pos, viewer1);
    
    // Encontrar minúcia mais próxima
    int minutiaIdx = findNearestMinutia(frag1->minutiae, scenePos, 15.0);
    
    if (minutiaIdx >= 0) {
        qDebug() << QString("[INTERACTIVE] Frag1: Minúcia %1 clicada").arg(minutiaIdx + 1);
        
        selectedMinutia1 = minutiaIdx;
        minutia1IndexSpinBox->setValue(minutiaIdx + 1);
        
        // Se já tinha uma minúcia do frag2 selecionada, criar associação automaticamente
        if (selectedMinutia2 >= 0) {
            onAddManualMatch();
            selectedMinutia1 = -1;
            selectedMinutia2 = -1;
        }
        
        updateOverlayHighlight();
    }
}

void FragmentComparisonDialog::onViewer2Clicked(QPoint pos, Qt::KeyboardModifiers modifiers) {
    int frag2Idx = fragment2Combo->currentIndex();
    if (frag2Idx < 0 || frag2Idx >= availableFragments.size()) return;
    
    FingerprintEnhancer::Fragment* frag2 = availableFragments[frag2Idx];
    
    // Converter posição do clique para coordenadas da imagem
    QPointF scenePos = viewportToScene(pos, viewer2);
    
    // Encontrar minúcia mais próxima
    int minutiaIdx = findNearestMinutia(frag2->minutiae, scenePos, 15.0);
    
    if (minutiaIdx >= 0) {
        qDebug() << QString("[INTERACTIVE] Frag2: Minúcia %1 clicada (Shift=%2)")
            .arg(minutiaIdx + 1).arg((modifiers & Qt::ShiftModifier) ? "SIM" : "NÃO");
        
        selectedMinutia2 = minutiaIdx;
        minutia2IndexSpinBox->setValue(minutiaIdx + 1);
        
        // Se Shift está pressionado E tem minúcia do frag1 selecionada, criar associação
        if ((modifiers & Qt::ShiftModifier) && selectedMinutia1 >= 0) {
            onAddManualMatch();
            selectedMinutia1 = -1;
            selectedMinutia2 = -1;
        }
        
        updateOverlayHighlight();
    }
}

void FragmentComparisonDialog::onViewerContextMenu(const QPoint& pos, int viewerIndex) {
    Q_UNUSED(pos);
    Q_UNUSED(viewerIndex);
    
    QMenu contextMenu(this);
    
    QAction* addAction = contextMenu.addAction("➕ Adicionar Associação");
    addAction->setEnabled(selectedMinutia1 >= 0 && selectedMinutia2 >= 0);
    connect(addAction, &QAction::triggered, this, &FragmentComparisonDialog::onAddManualMatch);
    
    contextMenu.addSeparator();
    
    QAction* clearSelection = contextMenu.addAction("Limpar Seleção");
    connect(clearSelection, &QAction::triggered, [this]() {
        selectedMinutia1 = -1;
        selectedMinutia2 = -1;
        updateOverlayHighlight();
    });
    
    contextMenu.exec(QCursor::pos());
}

void FragmentComparisonDialog::onManualMatchListClicked(QListWidgetItem* item) {
    int row = manualMatchList->row(item);
    if (row < 0 || row >= manualMatches.size()) return;
    
    qDebug() << QString("[MANUAL MATCH] Clicado na associação %1 da lista").arg(row);
    highlightAssociationInViewers(row);
}

void FragmentComparisonDialog::onManualMatchListContextMenu(const QPoint& pos) {
    QListWidgetItem* item = manualMatchList->itemAt(pos);
    if (!item) return;
    
    int row = manualMatchList->row(item);
    if (row < 0 || row >= manualMatches.size()) return;
    
    QMenu contextMenu(this);
    
    QAction* removeAction = contextMenu.addAction("🗑️ Remover Esta Associação");
    connect(removeAction, &QAction::triggered, [this, row]() {
        if (row < manualMatches.size()) {
            QPair<int, int> match = manualMatches[row];
            manualMatches.removeAt(row);
            delete manualMatchList->takeItem(row);
            
            qDebug() << QString("[MANUAL MATCH] Removida associação %1↔%2 via menu de contexto")
                .arg(match.first + 1).arg(match.second + 1);
        }
    });
    
    contextMenu.addSeparator();
    
    QAction* highlightAction = contextMenu.addAction("🔍 Destacar Minúcias");
    connect(highlightAction, &QAction::triggered, [this, row]() {
        highlightAssociationInViewers(row);
    });
    
    contextMenu.exec(manualMatchList->mapToGlobal(pos));
}

void FragmentComparisonDialog::highlightAssociationInViewers(int matchIndex) {
    if (matchIndex < 0 || matchIndex >= manualMatches.size()) return;
    
    int frag1Idx = fragment1Combo->currentIndex();
    int frag2Idx = fragment2Combo->currentIndex();
    
    if (frag1Idx < 0 || frag1Idx >= availableFragments.size() ||
        frag2Idx < 0 || frag2Idx >= availableFragments.size()) {
        return;
    }
    
    FingerprintEnhancer::Fragment* frag1 = availableFragments[frag1Idx];
    FingerprintEnhancer::Fragment* frag2 = availableFragments[frag2Idx];
    
    QPair<int, int> match = manualMatches[matchIndex];
    
    // Validar índices
    if (match.first < 0 || match.first >= frag1->minutiae.size() ||
        match.second < 0 || match.second >= frag2->minutiae.size()) {
        return;
    }
    
    // Atualizar seleção
    selectedMinutia1 = match.first;
    selectedMinutia2 = match.second;
    
    // Atualizar spinboxes
    minutia1IndexSpinBox->setValue(match.first + 1);
    minutia2IndexSpinBox->setValue(match.second + 1);
    
    // Destacar nos overlays
    updateOverlayHighlight();
    
    qDebug() << QString("[MANUAL MATCH] Destacando associação: Frag1[%1] ↔ Frag2[%2]")
        .arg(match.first + 1).arg(match.second + 1);
}

void FragmentComparisonDialog::onAutoMatchByIndex() {
    int frag1Idx = fragment1Combo->currentIndex();
    int frag2Idx = fragment2Combo->currentIndex();
    
    if (frag1Idx < 0 || frag1Idx >= availableFragments.size() ||
        frag2Idx < 0 || frag2Idx >= availableFragments.size()) {
        QMessageBox::warning(this, "Erro", "Selecione dois fragmentos válidos.");
        return;
    }
    
    FingerprintEnhancer::Fragment* frag1 = availableFragments[frag1Idx];
    FingerprintEnhancer::Fragment* frag2 = availableFragments[frag2Idx];
    
    int minCount = qMin(frag1->minutiae.size(), frag2->minutiae.size());
    
    if (minCount == 0) {
        QMessageBox::information(this, "Sem Minúcias",
            "Um ou ambos os fragmentos não possuem minúcias.");
        return;
    }
    
    auto reply = QMessageBox::question(this, "Confirmar Associação Automática",
        QString("Associar automaticamente %1 minúcias por índice?\n\n"
                "Fragmento 1: %2 minúcias\n"
                "Fragmento 2: %3 minúcias\n\n"
                "Serão criadas %1 associações (1↔1, 2↔2, ...)")
        .arg(minCount).arg(frag1->minutiae.size()).arg(frag2->minutiae.size()),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        manualMatches.clear();
        manualMatchList->clear();
        
        for (int i = 0; i < minCount; ++i) {
            manualMatches.append(QPair<int, int>(i, i));
            manualMatchList->addItem(QString("%1 ↔ %2").arg(i + 1).arg(i + 1));
        }
        
        qDebug() << QString("[MANUAL MATCH] Associação automática: %1 pares criados").arg(minCount);
        
        QMessageBox::information(this, "Sucesso",
            QString("%1 associações criadas automaticamente.").arg(minCount));
    }
}
