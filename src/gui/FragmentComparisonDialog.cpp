#include "FragmentComparisonDialog.h"
#include <QSplitter>
#include <QMessageBox>
#include <QtConcurrent>
#include <QElapsedTimer>
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
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // ==================== Seleção de Fragmentos ====================
    QGroupBox* selectionGroup = new QGroupBox("Seleção de Fragmentos");
    QHBoxLayout* selectionLayout = new QHBoxLayout(selectionGroup);
    
    // Fragmento 1
    QVBoxLayout* frag1Layout = new QVBoxLayout();
    frag1Layout->addWidget(new QLabel("Fragmento 1:"));
    fragment1Combo = new QComboBox();
    frag1Layout->addWidget(fragment1Combo);
    selectionLayout->addLayout(frag1Layout);
    
    // Botão de troca
    swapButton = new QPushButton("⇄");
    swapButton->setToolTip("Trocar fragmentos");
    swapButton->setMaximumWidth(50);
    selectionLayout->addWidget(swapButton);
    
    // Fragmento 2
    QVBoxLayout* frag2Layout = new QVBoxLayout();
    frag2Layout->addWidget(new QLabel("Fragmento 2:"));
    fragment2Combo = new QComboBox();
    frag2Layout->addWidget(fragment2Combo);
    selectionLayout->addLayout(frag2Layout);
    
    mainLayout->addWidget(selectionGroup);
    
    // ==================== Visualização Lado a Lado ====================
    QSplitter* viewerSplitter = new QSplitter(Qt::Horizontal);
    
    // Viewer 1
    QWidget* viewer1Container = new QWidget();
    QVBoxLayout* viewer1Layout = new QVBoxLayout(viewer1Container);
    viewer1Layout->setContentsMargins(0, 0, 0, 0);
    viewer1 = new ImageViewer();
    overlay1 = new MinutiaeOverlay(viewer1);
    viewer1Layout->addWidget(viewer1);
    viewerSplitter->addWidget(viewer1Container);
    
    // Viewer 2
    QWidget* viewer2Container = new QWidget();
    QVBoxLayout* viewer2Layout = new QVBoxLayout(viewer2Container);
    viewer2Layout->setContentsMargins(0, 0, 0, 0);
    viewer2 = new ImageViewer();
    overlay2 = new MinutiaeOverlay(viewer2);
    viewer2Layout->addWidget(viewer2);
    viewerSplitter->addWidget(viewer2Container);
    
    mainLayout->addWidget(viewerSplitter, 1);
    
    // ==================== Controles e Resultados ====================
    QGroupBox* controlGroup = new QGroupBox("Comparação");
    QVBoxLayout* controlLayout = new QVBoxLayout(controlGroup);
    
    // Botão de comparar
    compareButton = new QPushButton("Calcular Score de Similaridade");
    compareButton->setEnabled(false);
    compareButton->setMinimumHeight(40);
    QFont buttonFont = compareButton->font();
    buttonFont.setPointSize(12);
    buttonFont.setBold(true);
    compareButton->setFont(buttonFont);
    controlLayout->addWidget(compareButton);
    
    // Barra de progresso
    progressBar = new QProgressBar();
    progressBar->setVisible(false);
    progressBar->setRange(0, 0);  // Indeterminado
    controlLayout->addWidget(progressBar);
    
    // Resultados
    QGroupBox* resultsGroup = new QGroupBox("Resultados");
    QVBoxLayout* resultsLayout = new QVBoxLayout(resultsGroup);
    
    resultLikelihoodLabel = new QLabel("Likelihood Ratio (LR): -");
    resultLogLRLabel = new QLabel("Log₁₀(LR): -");
    resultScoreLabel = new QLabel("Score de Similaridade: -");
    resultMatchedLabel = new QLabel("Minúcias Correspondentes: -");
    resultInterpretationLabel = new QLabel("Interpretação: -");
    resultTimeLabel = new QLabel("Tempo de Cálculo: -");
    
    QFont resultFont;
    resultFont.setPointSize(10);
    resultLikelihoodLabel->setFont(resultFont);
    resultLogLRLabel->setFont(resultFont);
    resultScoreLabel->setFont(resultFont);
    resultMatchedLabel->setFont(resultFont);
    resultTimeLabel->setFont(resultFont);
    
    QFont interpretFont = resultFont;
    interpretFont.setBold(true);
    resultInterpretationLabel->setFont(interpretFont);
    
    resultsLayout->addWidget(resultLikelihoodLabel);
    resultsLayout->addWidget(resultLogLRLabel);
    resultsLayout->addWidget(resultScoreLabel);
    resultsLayout->addWidget(resultMatchedLabel);
    resultsLayout->addWidget(resultInterpretationLabel);
    resultsLayout->addWidget(resultTimeLabel);
    
    controlLayout->addWidget(resultsGroup);
    mainLayout->addWidget(controlGroup);
    
    // ==================== Botões de Ação ====================
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    closeButton = new QPushButton("Fechar");
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);
    
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
        for (auto& fragment : image.fragments) {
            availableFragments.append(&fragment);
            
            QString displayName = QString("%1 - Fragmento %2 (%3 minúcias)")
                .arg(image.name)
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
                                               MinutiaeOverlay* overlay) {
    if (!fragment) return;
    
    // Exibir imagem do fragmento
    viewer->setImage(fragment->workingImage);
    viewer->zoomToFit();
    
    // Exibir minúcias
    overlay->setMinutiae(fragment->minutiae);
    overlay->setEditMode(false);  // Apenas visualização
    overlay->update();
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
    
    // Executar comparação em thread separada
    QVector<MinutiaeData> minutiae1 = frag1->minutiae;
    QVector<MinutiaeData> minutiae2 = frag2->minutiae;
    
    comparisonFuture = QtConcurrent::run([minutiae1, minutiae2]() {
        return compareFragments(minutiae1, minutiae2);
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
    const QVector<MinutiaeData>& minutiae1,
    const QVector<MinutiaeData>& minutiae2) {
    
    QElapsedTimer timer;
    timer.start();
    
    FragmentComparisonResult result;
    result.totalMinutiaeFragment1 = minutiae1.size();
    result.totalMinutiaeFragment2 = minutiae2.size();
    
    // 1. Encontrar correspondências entre minúcias
    result.correspondences = findCorrespondences(minutiae1, minutiae2);
    result.matchedMinutiae = result.correspondences.size();
    
    // 2. Calcular Likelihood Ratio
    result.likelihoodRatio = calculateLikelihoodRatio(minutiae1, minutiae2, result.correspondences);
    result.logLR = (result.likelihoodRatio > 0) ? log10(result.likelihoodRatio) : -100.0;
    
    // 3. Calcular score de similaridade simples (0.0 a 1.0)
    int minMinutiae = qMin(minutiae1.size(), minutiae2.size());
    result.similarityScore = (minMinutiae > 0) ?
        static_cast<double>(result.matchedMinutiae) / static_cast<double>(minMinutiae) : 0.0;
    
    // 4. Interpretação
    result.interpretationText = getInterpretationText(result.logLR);
    
    result.executionTimeMs = timer.elapsed();
    
    return result;
}

QVector<QPair<int, int>> FragmentComparisonDialog::findCorrespondences(
    const QVector<MinutiaeData>& minutiae1,
    const QVector<MinutiaeData>& minutiae2,
    double positionTolerance,
    double angleTolerance) {
    
    QVector<QPair<int, int>> correspondences;
    QVector<bool> matched2(minutiae2.size(), false);
    
    // Para cada minúcia no fragmento 1, encontrar melhor match no fragmento 2
    for (int i = 0; i < minutiae1.size(); i++) {
        double bestScore = -1.0;
        int bestMatch = -1;
        
        for (int j = 0; j < minutiae2.size(); j++) {
            if (matched2[j]) continue;  // Já foi pareada
            
            double score = computeLocalSimilarity(minutiae1[i], minutiae2[j]);
            
            // Verificar tolerâncias
            double distSq = pow(minutiae1[i].position.x - minutiae2[j].position.x, 2) +
                           pow(minutiae1[i].position.y - minutiae2[j].position.y, 2);
            double angleDiff = fabs(minutiae1[i].angle - minutiae2[j].angle);
            if (angleDiff > M_PI) angleDiff = 2 * M_PI - angleDiff;
            
            if (sqrt(distSq) > positionTolerance || angleDiff > angleTolerance) {
                continue;  // Fora das tolerâncias
            }
            
            if (score > bestScore) {
                bestScore = score;
                bestMatch = j;
            }
        }
        
        if (bestMatch >= 0 && bestScore > 0.5) {  // Threshold mínimo
            correspondences.append(qMakePair(i, bestMatch));
            matched2[bestMatch] = true;
        }
    }
    
    return correspondences;
}

double FragmentComparisonDialog::computeLocalSimilarity(
    const MinutiaeData& m1,
    const MinutiaeData& m2) {
    
    // Distância euclidiana
    double dx = m1.position.x - m2.position.x;
    double dy = m1.position.y - m2.position.y;
    double dist = sqrt(dx * dx + dy * dy);
    
    // Diferença angular
    double angleDiff = fabs(m1.angle - m2.angle);
    if (angleDiff > M_PI) angleDiff = 2 * M_PI - angleDiff;
    
    // Score baseado em distância e ângulo
    double distScore = exp(-dist / 10.0);  // Decai com distância
    double angleScore = cos(angleDiff);     // 1.0 se ângulos iguais, -1.0 se opostos
    
    // Peso do tipo de minúcia (minúcias do mesmo tipo têm score maior)
    double typeScore = (m1.type == m2.type) ? 1.0 : 0.5;
    
    // Peso da qualidade
    double qualityScore = (m1.quality + m2.quality) / 2.0;
    
    // Score final (0.0 a 1.0)
    double score = (distScore * 0.4 + angleScore * 0.3 + typeScore * 0.2 + qualityScore * 0.1);
    
    return qMax(0.0, score);
}

double FragmentComparisonDialog::calculateLikelihoodRatio(
    const QVector<MinutiaeData>& minutiae1,
    const QVector<MinutiaeData>& minutiae2,
    const QVector<QPair<int, int>>& correspondences) {
    
    // TODO: Implementar cálculo completo baseado em Neumann et al. (2007)
    // Por enquanto, usar aproximação simples
    
    int n = correspondences.size();  // Número de correspondências
    
    if (n == 0) {
        return 1e-10;  // LR muito baixo (evidência de não-match)
    }
    
    // Estimativa simples: LR cresce exponencialmente com número de matches
    // Baseado em análises empíricas de AFIS
    // Cada minúcia correspondente adiciona ~2-3 ordens de magnitude ao LR
    
    double baseLR = 10.0;  // LR base para 1 correspondência
    double exponent = n * 2.5;  // Cada correspondência multiplica por ~10^2.5
    
    double LR = pow(baseLR, exponent);
    
    // Fator de qualidade: reduzir LR se poucas minúcias no total
    int minTotal = qMin(minutiae1.size(), minutiae2.size());
    if (minTotal > 0) {
        double completeness = static_cast<double>(n) / static_cast<double>(minTotal);
        LR *= completeness;  // Penalizar se muitas minúcias não foram pareadas
    }
    
    return LR;
}
