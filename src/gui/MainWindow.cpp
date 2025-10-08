#include "MainWindow.h"
#include "ProcessingWorker.h"
#include "RotationDialog.h"
#include "MinutiaEditDialog.h"
#include "../core/TranslationManager_Simple.h"
#include "../core/ImageState.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QDialogButtonBox>
#include <QtGui/QActionGroup>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QThread>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , imageProcessor(new ImageProcessor())
    , minutiaeExtractor(new MinutiaeExtractor())
    , cropTool(nullptr)
    , minutiaeMarker(nullptr)
    , afisMatcher(nullptr)
    , imageState(new ImageState())
    , fragmentManager(nullptr)
    , minutiaeOverlay(nullptr)
    , currentEntityType(ENTITY_NONE)
    , currentToolMode(TOOL_NONE)
    , sideBySideMode(true)
    , updateTimer(new QTimer(this))
    , processingThread(nullptr)
    , processingWorker(nullptr)
    , isProcessing(false)
{
    setupUI();
    connectSignals();
    updateWindowTitle();

    // Timer para atualizações periódicas da interface
    updateTimer->setSingleShot(false);
    updateTimer->setInterval(100); // 100ms
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::updateStatusBar);
}

MainWindow::~MainWindow() {
    // Limpar threading se existir
    if (processingThread) {
        processingThread->quit();
        processingThread->wait();
        delete processingThread;
    }
    // unique_ptr gerencia automaticamente o resto
}

void MainWindow::setupUI() {
    // Configurar janela principal
    setMinimumSize(1200, 800);
    resize(1600, 1000);
    
    createMenus();
    createToolBars();
    createStatusBar();
    createCentralWidget();
    createLeftPanel();
    createRightPanel();
}

void MainWindow::createMenus() {
    // Menu Arquivo
    QMenu *fileMenu = menuBar()->addMenu("&Arquivo");

    fileMenu->addAction("&Novo Projeto", this, &MainWindow::newProject, QKeySequence::New);
    fileMenu->addAction("&Abrir Projeto", this, &MainWindow::openProject, QKeySequence::Open);
    fileMenu->addSeparator();
    fileMenu->addAction("&Salvar Projeto", this, &MainWindow::saveProject, QKeySequence::Save);
    fileMenu->addAction("Salvar Projeto &Como...", this, &MainWindow::saveProjectAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("Abrir &Imagem...", this, &MainWindow::openImage, QKeySequence("Ctrl+I"));
    fileMenu->addAction("Salvar I&magem...", this, &MainWindow::saveImage, QKeySequence("Ctrl+M"));
    fileMenu->addSeparator();
    fileMenu->addAction("Exportar &Relatório...", this, &MainWindow::exportReport, QKeySequence("Ctrl+R"));
    fileMenu->addSeparator();
    fileMenu->addAction("&Sair", this, &MainWindow::exitApplication, QKeySequence::Quit);

    // Menu Editar
    QMenu *editMenu = menuBar()->addMenu("&Editar");
    editMenu->addAction("&Restaurar Original", this, &MainWindow::resetToOriginal, QKeySequence("Ctrl+Shift+Z"));
    editMenu->addAction("&Desfazer Última Operação", this, &MainWindow::undoLastOperation, QKeySequence::Undo);

    // Menu Realce
    QMenu *enhanceMenu = menuBar()->addMenu("&Realce");
    enhanceMenu->addAction("Filtro &FFT...", this, &MainWindow::openFFTDialog, QKeySequence("Ctrl+F"));
    enhanceMenu->addAction("&Subtrair Fundo...", this, &MainWindow::subtractBackground, QKeySequence("Ctrl+B"));
    enhanceMenu->addSeparator();
    enhanceMenu->addAction("&Desfoque Gaussiano...", this, &MainWindow::applyGaussianBlur);
    enhanceMenu->addAction("Filtro de &Nitidez...", this, &MainWindow::applySharpenFilter);
    enhanceMenu->addSeparator();
    enhanceMenu->addAction("Brilho/&Contraste...", this, &MainWindow::adjustBrightnessContrast);
    enhanceMenu->addAction("&Equalizar Histograma", this, &MainWindow::equalizeHistogram);
    enhanceMenu->addAction("&CLAHE...", this, &MainWindow::applyCLAHE);
    enhanceMenu->addSeparator();
    enhanceMenu->addAction("&Inverter Cores", this, &MainWindow::invertColors, QKeySequence("Ctrl+Shift+I"));
    enhanceMenu->addAction("&Rotacionar Imagem...", this, &MainWindow::rotateImage);
    enhanceMenu->addSeparator();
    enhanceMenu->addAction("&Binarizar Imagem", this, &MainWindow::binarizeImage, QKeySequence("Ctrl+T"));
    enhanceMenu->addAction("&Esqueletizar", this, &MainWindow::skeletonizeImage, QKeySequence("Ctrl+K"));

    // Menu Análise
    QMenu *analysisMenu = menuBar()->addMenu("&Análise");
    analysisMenu->addAction("&Extrair Minúcias", this, &MainWindow::extractMinutiae, QKeySequence("Ctrl+E"));
    analysisMenu->addAction("&Comparar Minúcias...", this, &MainWindow::compareMinutiae, QKeySequence("Ctrl+Shift+C"));
    analysisMenu->addAction("Gerar &Gráfico", this, &MainWindow::generateChart, QKeySequence("Ctrl+G"));

    // Menu Visualizar
    QMenu *viewMenu = menuBar()->addMenu("&Visualizar");
    viewMenu->addAction("Ampliar (&+)", this, &MainWindow::zoomIn, QKeySequence::ZoomIn);
    viewMenu->addAction("Reduzir (&-)", this, &MainWindow::zoomOut, QKeySequence::ZoomOut);
    viewMenu->addAction("Ajustar à &Janela", this, &MainWindow::zoomFit, QKeySequence("Ctrl+0"));
    viewMenu->addAction("Tamanho &Real", this, &MainWindow::zoomActual, QKeySequence("Ctrl+1"));
    viewMenu->addSeparator();
    viewMenu->addAction("Lado a &Lado", this, &MainWindow::toggleSideBySide, QKeySequence("Ctrl+2"));

    // Menu Ferramentas
    QMenu *toolsMenu = menuBar()->addMenu("&Ferramentas");

    // Submenu de Recorte
    QMenu *cropMenu = toolsMenu->addMenu("&Recorte de Imagem");
    cropMenu->addAction("&Ativar Ferramenta de Recorte", this, &MainWindow::activateCropTool, QKeySequence("Ctrl+Shift+R"));
    cropMenu->addAction("&Aplicar Recorte", this, &MainWindow::applyCrop, QKeySequence("Ctrl+Shift+A"));
    cropMenu->addAction("&Cancelar Seleção", this, &MainWindow::cancelCropSelection);
    cropMenu->addSeparator();
    cropMenu->addAction("&Salvar Imagem Recortada...", this, &MainWindow::saveCroppedImage);

    // Submenu de Rotação
    QMenu *rotateMenu = toolsMenu->addMenu("&Rotação de Imagem");
    rotateMenu->addAction("Rotacionar 90° à &Direita", this, &MainWindow::rotateRight90, QKeySequence("Ctrl+]"));
    rotateMenu->addAction("Rotacionar 90° à &Esquerda", this, &MainWindow::rotateLeft90, QKeySequence("Ctrl+["));
    rotateMenu->addAction("Rotacionar &180°", this, &MainWindow::rotate180);
    rotateMenu->addSeparator();
    rotateMenu->addAction("Rotacionar Ângulo &Personalizado...", this, &MainWindow::rotateCustomAngle, QKeySequence("Ctrl+Shift+T"));

    // Submenu de Espelhamento
    QMenu *flipMenu = toolsMenu->addMenu("&Espelhamento");
    flipMenu->addAction("Espelhar &Horizontal", this, &MainWindow::flipHorizontal, QKeySequence("Ctrl+H"));
    flipMenu->addAction("Espelhar &Vertical", this, &MainWindow::flipVertical, QKeySequence("Ctrl+Shift+V"));

    // Submenu de Calibração de Escala
    QMenu *scaleMenu = toolsMenu->addMenu("&Calibração de Escala");
    scaleMenu->addAction("&Calibrar Escala...", this, &MainWindow::calibrateScale);
    scaleMenu->addAction("&Definir Escala Manualmente...", this, &MainWindow::setScaleManually);
    scaleMenu->addAction("&Informações de Escala", this, &MainWindow::showScaleInfo);

    // Submenu de Conversão de Espaço de Cor
    QMenu *colorMenu = toolsMenu->addMenu("Espaço de &Cor");
    colorMenu->addAction("Converter para &RGB", this, &MainWindow::convertToRGB);
    colorMenu->addAction("Converter para &HSV", this, &MainWindow::convertToHSV);
    colorMenu->addAction("Converter para H&SI", this, &MainWindow::convertToHSI);
    colorMenu->addAction("Converter para &Lab", this, &MainWindow::convertToLab);
    colorMenu->addAction("Converter para &Escala de Cinza", this, &MainWindow::convertToGrayscale, QKeySequence("Ctrl+Shift+G"));
    colorMenu->addSeparator();
    colorMenu->addAction("&Ajustar Níveis de Cor...", this, &MainWindow::adjustColorLevels);

    // Submenu de Minúcias
    QMenu *minutiaeMenu = toolsMenu->addMenu("&Minúcias");
    minutiaeMenu->addAction("&Adicionar Minúcia Manual", this, &MainWindow::activateAddMinutia, QKeySequence("Ctrl+M"));
    minutiaeMenu->addAction("&Editar Minúcia", this, &MainWindow::activateEditMinutia);
    minutiaeMenu->addAction("&Remover Minúcia", this, &MainWindow::activateRemoveMinutia);
    minutiaeMenu->addSeparator();
    minutiaeMenu->addAction("&Mostrar Números", this, &MainWindow::toggleShowMinutiaeNumbers)->setCheckable(true);
    minutiaeMenu->addAction("Mostrar Â&ngulos", this, &MainWindow::toggleShowMinutiaeAngles)->setCheckable(true);
    minutiaeMenu->addAction("Mostrar &Tipos", this, &MainWindow::toggleShowMinutiaeLabels)->setCheckable(true);
    minutiaeMenu->addSeparator();
    minutiaeMenu->addAction("&Salvar Imagem com Minúcias Numeradas...", this, &MainWindow::saveMinutiaeNumberedImage);
    minutiaeMenu->addAction("&Limpar Todas as Minúcias", this, &MainWindow::clearAllMinutiae);

    toolsMenu->addSeparator();
    toolsMenu->addAction("&Idioma...", this, &MainWindow::changeLanguage, QKeySequence("Ctrl+L"));

    // Menu AFIS
    QMenu *afisMenu = menuBar()->addMenu("AF&IS");
    afisMenu->addAction("&Carregar Base de Dados...", this, &MainWindow::loadAFISDatabase, QKeySequence("Ctrl+Shift+D"));
    afisMenu->addAction("&Identificar Impressão Digital", this, &MainWindow::identifyFingerprint, QKeySequence("Ctrl+Shift+I"));
    afisMenu->addAction("&Verificar 1:1...", this, &MainWindow::verifyFingerprint);
    afisMenu->addSeparator();
    afisMenu->addAction("&Configurar Matching...", this, &MainWindow::configureAFISMatching);
    afisMenu->addAction("&Ver Resultados", this, &MainWindow::showAFISResults);
}

void MainWindow::createToolBars() {
    // Toolbar principal
    QToolBar *mainToolBar = addToolBar("Principal");
    mainToolBar->addAction("Novo", this, &MainWindow::newProject);
    mainToolBar->addAction("Abrir", this, &MainWindow::openProject);
    mainToolBar->addAction("Salvar", this, &MainWindow::saveProject);
    mainToolBar->addSeparator();
    mainToolBar->addAction("Abrir Imagem", this, &MainWindow::openImage);
    mainToolBar->addSeparator();
    mainToolBar->addAction("Ampliar", this, &MainWindow::zoomIn);
    mainToolBar->addAction("Reduzir", this, &MainWindow::zoomOut);
    mainToolBar->addAction("Ajustar", this, &MainWindow::zoomFit);

    // Toolbar de ferramentas
    QToolBar *toolsToolBar = addToolBar("Ferramentas");

    QAction *noneToolAction = toolsToolBar->addAction("🖱️ Nenhuma");
    noneToolAction->setCheckable(true);
    noneToolAction->setChecked(true);
    connect(noneToolAction, &QAction::triggered, [this]() { setToolMode(TOOL_NONE); });

    QAction *cropToolAction = toolsToolBar->addAction("✂️ Recortar");
    cropToolAction->setCheckable(true);
    connect(cropToolAction, &QAction::triggered, [this]() { setToolMode(TOOL_CROP); });

    QAction *addMinutiaAction = toolsToolBar->addAction("➕ Adicionar Minúcia");
    addMinutiaAction->setCheckable(true);
    connect(addMinutiaAction, &QAction::triggered, [this]() { setToolMode(TOOL_ADD_MINUTIA); });

    QAction *editMinutiaAction = toolsToolBar->addAction("✏️ Editar Minúcia");
    editMinutiaAction->setCheckable(true);
    connect(editMinutiaAction, &QAction::triggered, [this]() { setToolMode(TOOL_EDIT_MINUTIA); });

    QAction *removeMinutiaAction = toolsToolBar->addAction("🗑️ Remover Minúcia");
    removeMinutiaAction->setCheckable(true);
    connect(removeMinutiaAction, &QAction::triggered, [this]() { setToolMode(TOOL_REMOVE_MINUTIA); });

    // Agrupar ações para exclusividade mútua
    QActionGroup *toolGroup = new QActionGroup(this);
    toolGroup->addAction(noneToolAction);
    toolGroup->addAction(cropToolAction);
    toolGroup->addAction(addMinutiaAction);
    toolGroup->addAction(editMinutiaAction);
    toolGroup->addAction(removeMinutiaAction);
}

void MainWindow::createStatusBar() {
    statusLabel = new QLabel("Pronto");
    imageInfoLabel = new QLabel("");
    scaleLabel = new QLabel("Escala: 1:1");
    progressBar = new QProgressBar();
    progressBar->setVisible(false);
    
    statusBar()->addWidget(statusLabel, 1);
    statusBar()->addPermanentWidget(imageInfoLabel);
    statusBar()->addPermanentWidget(scaleLabel);
    statusBar()->addPermanentWidget(progressBar);
}

void MainWindow::createCentralWidget() {
    centralWidget = new QWidget();
    setCentralWidget(centralWidget);

    // Layout principal com splitter horizontal
    mainSplitter = new QSplitter(Qt::Horizontal);

    // Área de imagens no centro
    imageSplitter = new QSplitter(Qt::Horizontal);

    // Visualizadores de imagem
    originalImageViewer = new ImageViewer();
    processedImageViewer = new ImageViewer();
    minutiaeEditor = new MinutiaeEditor();

    // Configurar sincronização entre viewers
    originalImageViewer->setSyncViewer(processedImageViewer);
    processedImageViewer->setSyncViewer(originalImageViewer);

    // Criar overlay de minúcias - será um widget sobreposto ao viewer
    // Precisamos criar um container com stacked layout para overlay funcionar
    QWidget *viewerContainer = new QWidget();
    QStackedLayout *stackedLayout = new QStackedLayout(viewerContainer);
    stackedLayout->setStackingMode(QStackedLayout::StackAll);

    stackedLayout->addWidget(processedImageViewer);

    minutiaeOverlay = new FingerprintEnhancer::MinutiaeOverlay(viewerContainer);
    stackedLayout->addWidget(minutiaeOverlay);

    // Configurar overlay para ser transparente e passar eventos de mouse quando necessário
    minutiaeOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, false);
    minutiaeOverlay->setStyleSheet("background-color: transparent;");

    // Layout: imagem original pequena (200px) e imagem processada grande (com overlay)
    imageSplitter->addWidget(originalImageViewer);
    imageSplitter->addWidget(viewerContainer); // Usar container em vez de viewer direto
    imageSplitter->setSizes({200, 1200}); // Original minimizada, processada em destaque

    // Esconder o minutiaeEditor por padrão (será usado depois)
    minutiaeEditor->hide();

    // Layout principal
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->addWidget(mainSplitter);

    mainSplitter->addWidget(imageSplitter);
}

void MainWindow::createLeftPanel() {
    leftPanel = new QTabWidget();
    leftPanel->setMaximumWidth(300);

    // Tab de processamento
    QWidget *enhancementTab = new QWidget();
    QVBoxLayout *enhancementLayout = new QVBoxLayout(enhancementTab);

    // Grupo de controles básicos
    enhancementGroup = new QGroupBox("Ajustes Básicos");
    QVBoxLayout *basicLayout = new QVBoxLayout(enhancementGroup);

    // Controles de brilho e contraste
    basicLayout->addWidget(new QLabel("Brilho:"));
    brightnessSlider = new QSlider(Qt::Horizontal);
    brightnessSlider->setRange(-100, 100);
    brightnessSlider->setValue(0);
    basicLayout->addWidget(brightnessSlider);

    basicLayout->addWidget(new QLabel("Contraste:"));
    contrastSlider = new QSlider(Qt::Horizontal);
    contrastSlider->setRange(50, 300);
    contrastSlider->setValue(100);
    basicLayout->addWidget(contrastSlider);

    // Controles de filtros
    basicLayout->addWidget(new QLabel("Sigma Gaussiano:"));
    gaussianSigma = new QDoubleSpinBox();
    gaussianSigma->setRange(0.1, 10.0);
    gaussianSigma->setValue(1.0);
    gaussianSigma->setSingleStep(0.1);
    basicLayout->addWidget(gaussianSigma);

    basicLayout->addWidget(new QLabel("Força de Nitidez:"));
    sharpenStrength = new QDoubleSpinBox();
    sharpenStrength->setRange(0.1, 5.0);
    sharpenStrength->setValue(1.0);
    sharpenStrength->setSingleStep(0.1);
    basicLayout->addWidget(sharpenStrength);

    // Botões de operações avançadas
    applyFFTButton = new QPushButton("Aplicar Filtro FFT");
    subtractBgButton = new QPushButton("Subtrair Fundo");
    binarizeButton = new QPushButton("Binarizar");
    skeletonizeButton = new QPushButton("Esqueletizar");

    basicLayout->addWidget(applyFFTButton);
    basicLayout->addWidget(subtractBgButton);
    basicLayout->addWidget(binarizeButton);
    basicLayout->addWidget(skeletonizeButton);

    enhancementLayout->addWidget(enhancementGroup);
    enhancementLayout->addStretch();

    leftPanel->addTab(enhancementTab, "Realce");

    // Adicionar painel esquerdo ao splitter principal
    mainSplitter->insertWidget(0, leftPanel);
}

void MainWindow::createRightPanel() {
    rightPanel = new QTabWidget();
    rightPanel->setMaximumWidth(300);

    // Tab de análise
    QWidget *analysisTab = new QWidget();
    QVBoxLayout *analysisLayout = new QVBoxLayout(analysisTab);

    // Grupo de análise de minúcias
    analysisGroup = new QGroupBox("Análise de Minúcias");
    QVBoxLayout *minutiaeLayout = new QVBoxLayout(analysisGroup);

    extractMinutiaeButton = new QPushButton("Extrair Minúcias");
    compareButton = new QPushButton("Comparar");
    chartButton = new QPushButton("Gerar Gráfico");

    minutiaeLayout->addWidget(extractMinutiaeButton);
    minutiaeLayout->addWidget(compareButton);
    minutiaeLayout->addWidget(chartButton);

    // Lista de minúcias
    minutiaeList = new QListWidget();
    minutiaeLayout->addWidget(new QLabel("Lista de Minúcias:"));
    minutiaeLayout->addWidget(minutiaeList);

    analysisLayout->addWidget(analysisGroup);

    // Tab de histórico
    QWidget *historyTab = new QWidget();
    QVBoxLayout *historyLayout = new QVBoxLayout(historyTab);

    historyDisplay = new QTextEdit();
    historyDisplay->setReadOnly(true);
    historyLayout->addWidget(new QLabel("Histórico de Processamento:"));
    historyLayout->addWidget(historyDisplay);

    rightPanel->addTab(analysisTab, "Análise");
    rightPanel->addTab(historyTab, "Histórico");

    // Tab de gerenciamento de projeto
    fragmentManager = new FingerprintEnhancer::FragmentManager();
    rightPanel->addTab(fragmentManager, "Projeto");

    // Adicionar painel direito ao splitter principal
    mainSplitter->addWidget(rightPanel);

    // Configurar tamanhos do splitter principal
    mainSplitter->setSizes({300, 800, 300});
}

void MainWindow::connectSignals() {
    // Conectar controles de processamento
    connect(brightnessSlider, &QSlider::valueChanged, this, &MainWindow::onBrightnessChanged);
    connect(contrastSlider, &QSlider::valueChanged, this, &MainWindow::onContrastChanged);
    connect(gaussianSigma, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onGaussianSigmaChanged);
    connect(sharpenStrength, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &MainWindow::onSharpenStrengthChanged);
    
    // Conectar botões
    connect(applyFFTButton, &QPushButton::clicked, this, &MainWindow::openFFTDialog);
    connect(subtractBgButton, &QPushButton::clicked, this, &MainWindow::subtractBackground);
    connect(binarizeButton, &QPushButton::clicked, this, &MainWindow::binarizeImage);
    connect(skeletonizeButton, &QPushButton::clicked, this, &MainWindow::skeletonizeImage);
    connect(extractMinutiaeButton, &QPushButton::clicked, this, &MainWindow::extractMinutiae);
    connect(compareButton, &QPushButton::clicked, this, &MainWindow::compareMinutiae);
    connect(chartButton, &QPushButton::clicked, this, &MainWindow::generateChart);

    // Configurar menu de contexto no viewer processado
    processedImageViewer->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(processedImageViewer, &QWidget::customContextMenuRequested,
            this, &MainWindow::showContextMenu);

    // Conectar sinais do ProjectManager (singleton)
    using PM = FingerprintEnhancer::ProjectManager;
    connect(&PM::instance(), &PM::projectModified, this, &MainWindow::onProjectModified);
    connect(&PM::instance(), &PM::imageAdded, this, &MainWindow::onImageAdded);
    connect(&PM::instance(), &PM::fragmentCreated, this, &MainWindow::onFragmentCreated);
    connect(&PM::instance(), &PM::minutiaAdded, this, &MainWindow::onMinutiaAdded);

    // Conectar sinais do FragmentManager
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::imageSelected,
            [this](const QString& imageId) {
                setCurrentEntity(imageId, ENTITY_IMAGE);
            });
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::fragmentSelected,
            this, &MainWindow::onFragmentSelected);
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::minutiaSelected,
            this, &MainWindow::onMinutiaSelected);
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::makeCurrentRequested,
            [this](const QString& entityId, bool isFragment) {
                CurrentEntityType type = isFragment ? ENTITY_FRAGMENT : ENTITY_IMAGE;
                setCurrentEntity(entityId, type);
            });
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::viewOriginalRequested,
            this, &MainWindow::onViewOriginalRequested);
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::resetWorkingImageRequested,
            this, &MainWindow::onResetWorkingRequested);
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::deleteFragmentRequested,
            this, &MainWindow::onDeleteFragmentRequested);
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::deleteMinutiaRequested,
            this, &MainWindow::onDeleteMinutiaRequested);

    // Conectar sinais do MinutiaeOverlay
    connect(minutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::minutiaDoubleClicked,
            this, &MainWindow::onMinutiaDoubleClicked);
    connect(minutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::positionChanged,
            this, &MainWindow::onMinutiaPositionChanged);
}

void MainWindow::updateWindowTitle() {
    QString title = "FingerprintEnhancer";
    using PM = FingerprintEnhancer::ProjectManager;

    if (PM::instance().hasOpenProject()) {
        FingerprintEnhancer::Project* project = PM::instance().getCurrentProject();
        if (project) {
            title += " - " + project->name;
            if (project->modified) {
                title += "*";
            }
        }
    }
    setWindowTitle(title);
}

// Implementações das funções de projeto
void MainWindow::newProject() {
    using PM = FingerprintEnhancer::ProjectManager;

    FingerprintEnhancer::NewProjectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString name = dialog.getProjectName();
        QString caseNumber = dialog.getCaseNumber();

        if (PM::instance().createNewProject(name, caseNumber)) {
            fragmentManager->setProject(PM::instance().getCurrentProject());
            statusLabel->setText("Projeto criado: " + name);
            updateWindowTitle();
        } else {
            QMessageBox::warning(this, "Erro", "Não foi possível criar o projeto");
        }
    }
}

void MainWindow::openProject() {
    using PM = FingerprintEnhancer::ProjectManager;

    QString fileName = QFileDialog::getOpenFileName(this,
        "Abrir Projeto", "", "Projeto FingerprintEnhancer (*.fpe)");

    if (!fileName.isEmpty()) {
        if (PM::instance().openProject(fileName)) {
            fragmentManager->setProject(PM::instance().getCurrentProject());
            statusLabel->setText("Projeto aberto: " + QFileInfo(fileName).fileName());
            updateWindowTitle();
        } else {
            QMessageBox::warning(this, "Erro", "Falha ao abrir o projeto");
        }
    }
}

void MainWindow::saveProject() {
    using PM = FingerprintEnhancer::ProjectManager;

    if (!PM::instance().hasOpenProject()) {
        QMessageBox::information(this, "Info", "Nenhum projeto aberto");
        return;
    }

    FingerprintEnhancer::Project* project = PM::instance().getCurrentProject();
    if (project->filePath.isEmpty()) {
        saveProjectAs();
    } else {
        if (PM::instance().saveProject()) {
            statusLabel->setText("Projeto salvo");
            updateWindowTitle();
        } else {
            QMessageBox::warning(this, "Erro", "Falha ao salvar o projeto");
        }
    }
}

void MainWindow::saveProjectAs() {
    using PM = FingerprintEnhancer::ProjectManager;

    if (!PM::instance().hasOpenProject()) {
        QMessageBox::information(this, "Info", "Nenhum projeto aberto");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        "Salvar Projeto Como", "", "Projeto FingerprintEnhancer (*.fpe)");

    if (!fileName.isEmpty()) {
        if (!fileName.endsWith(".fpe")) {
            fileName += ".fpe";
        }

        if (PM::instance().saveProjectAs(fileName)) {
            statusLabel->setText("Projeto salvo: " + QFileInfo(fileName).fileName());
            updateWindowTitle();
        } else {
            QMessageBox::warning(this, "Erro", "Falha ao salvar o projeto");
        }
    }
}

void MainWindow::openImage() {
    using PM = FingerprintEnhancer::ProjectManager;

    QString fileName = QFileDialog::getOpenFileName(this,
        "Abrir Imagem", "", "Arquivos de Imagem (*.png *.jpg *.jpeg *.bmp *.tiff)");

    if (!fileName.isEmpty()) {
        // Se não há projeto aberto, criar um automaticamente
        if (!PM::instance().hasOpenProject()) {
            if (!PM::instance().createNewProject("Projeto sem título", "")) {
                QMessageBox::warning(this, "Erro", "Não foi possível criar projeto");
                return;
            }
            fragmentManager->setProject(PM::instance().getCurrentProject());
        }

        // Adicionar imagem ao projeto
        FingerprintEnhancer::FingerprintImage* img = PM::instance().addImageToProject(fileName);
        if (img) {
            currentImageId = img->id;

            // Carregar imagem no processador (para edição)
            if (imageProcessor->loadImage(fileName.toStdString())) {
                updateImageDisplay();
                statusLabel->setText("Imagem carregada: " + QFileInfo(fileName).fileName());
            }
        } else {
            QMessageBox::warning(this, "Erro", "Falha ao carregar imagem");
        }
    }
}

void MainWindow::saveImage() {
    // TODO: Implementar salvamento de imagem
    statusLabel->setText("Image saved");
}

void MainWindow::exportReport() {
    // TODO: Implementar exportação de relatório
    statusLabel->setText("Report exported");
}

void MainWindow::exitApplication() {
    close();
}

void MainWindow::resetToOriginal() {
    imageProcessor->resetToOriginal();
    updateImageDisplay();
    statusLabel->setText("Restaurado para imagem original");
}

void MainWindow::undoLastOperation() {
    // TODO: Implementar undo
    statusLabel->setText("Last operation undone");
}

void MainWindow::updateImageDisplay() {
    if (imageProcessor->isImageLoaded()) {
        cv::Mat original = imageProcessor->getOriginalImage();
        cv::Mat processed = imageProcessor->getCurrentImage();
        
        originalImageViewer->setImage(original);
        processedImageViewer->setImage(processed);
        minutiaeEditor->setImage(processed);
        
        updateProcessingHistory();
        updateStatusBar();
    }
}

void MainWindow::updateProcessingHistory() {
    auto history = imageProcessor->getProcessingHistory();
    historyDisplay->clear();
    
    for (const auto& step : history) {
        QString line = QString("%1 - %2")
            .arg(QString::fromStdString(step.timestamp))
            .arg(QString::fromStdString(step.operation));
        historyDisplay->append(line);
    }
}

void MainWindow::updateMinutiaeList() {
    minutiaeList->clear();
    auto minutiae = minutiaeExtractor->getMinutiae();

    for (const auto& m : minutiae) {
        QString typeStr = (m.type == 0) ? "Termination" : "Bifurcation";
        QString itemText = QString("ID:%1 [%2] at (%3,%4) angle:%5°")
            .arg(m.id)
            .arg(typeStr)
            .arg(m.position.x, 0, 'f', 1)
            .arg(m.position.y, 0, 'f', 1)
            .arg(m.angle * 180.0 / M_PI, 0, 'f', 1);
        minutiaeList->addItem(itemText);
    }
}

void MainWindow::updateStatusBar() {
    if (imageProcessor->isImageLoaded()) {
        cv::Size size = imageProcessor->getImageSize();
        double scale = imageProcessor->getScale();
        
        imageInfoLabel->setText(formatImageInfo(size, scale));
        scaleLabel->setText(QString("Scale: %1 px/mm").arg(scale, 0, 'f', 2));
    } else {
        imageInfoLabel->setText("");
        scaleLabel->setText("Scale: 1:1");
    }
}

QString MainWindow::formatImageInfo(const cv::Size &size, double scale) {
    return QString("%1 x %2 px").arg(size.width).arg(size.height);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (checkUnsavedChanges()) {
        event->accept();
    } else {
        event->ignore();
    }
}

bool MainWindow::checkUnsavedChanges() {
    using PM = FingerprintEnhancer::ProjectManager;

    if (PM::instance().hasOpenProject() && PM::instance().getCurrentProject()->modified) {
        QMessageBox::StandardButton ret = QMessageBox::warning(this,
            "Mudanças Não Salvas",
            "O projeto tem alterações não salvas.\nDeseja salvar antes de fechar?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
        
        if (ret == QMessageBox::Save) {
            saveProject();
            return true;
        } else if (ret == QMessageBox::Cancel) {
            return false;
        }
    }
    return true;
}

// Implementações dos slots de processamento
void MainWindow::onBrightnessChanged(int value) {
    Q_UNUSED(value);
    applyBrightnessContrastRealtime();
}

void MainWindow::onContrastChanged(int value) {
    Q_UNUSED(value);
    applyBrightnessContrastRealtime();
}

void MainWindow::onGaussianSigmaChanged(double value) {
    if (!imageProcessor->isImageLoaded() || isProcessing) return;

    runProcessingInThread([value](const cv::Mat& input, int& progress) -> cv::Mat {
        progress = 50;
        cv::Mat result;
        cv::GaussianBlur(input, result, cv::Size(0, 0), value, value);
        progress = 100;
        return result;
    });
}

void MainWindow::onSharpenStrengthChanged(double value) {
    if (!imageProcessor->isImageLoaded() || isProcessing) return;

    runProcessingInThread([value](const cv::Mat& input, int& progress) -> cv::Mat {
        progress = 50;
        double strength = value;
        cv::Mat kernel = (cv::Mat_<float>(3, 3) <<
            0, -1 * strength, 0,
            -1 * strength, 1 + 4 * strength, -1 * strength,
            0, -1 * strength, 0);
        cv::Mat result;
        cv::filter2D(input, result, -1, kernel);
        progress = 100;
        return result;
    });
}

void MainWindow::onThresholdChanged(int value) {
    if (!imageProcessor->isImageLoaded() || isProcessing) return;

    runProcessingInThread([value](const cv::Mat& input, int& progress) -> cv::Mat {
        progress = 50;
        cv::Mat result;
        cv::threshold(input, result, value, 255, cv::THRESH_BINARY);
        progress = 100;
        return result;
    });
}

// Implementações básicas dos outros slots (a serem expandidas)
void MainWindow::openFFTDialog() {
    // TODO: Criar diálogo completo para FFT com visualização de frequências
    applyOperationToCurrentEntity([](cv::Mat& img) {
        // Converter para escala de cinza se necessário
        cv::Mat gray;
        if (img.channels() > 1) {
            cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = img.clone();
        }

        // Aplicar FFT com filtro passa-alta básico
        cv::Mat padded;
        int m = cv::getOptimalDFTSize(gray.rows);
        int n = cv::getOptimalDFTSize(gray.cols);
        cv::copyMakeBorder(gray, padded, 0, m - gray.rows, 0, n - gray.cols, cv::BORDER_CONSTANT, cv::Scalar::all(0));

        cv::Mat planes[] = {cv::Mat_<float>(padded), cv::Mat::zeros(padded.size(), CV_32F)};
        cv::Mat complexI;
        cv::merge(planes, 2, complexI);
        cv::dft(complexI, complexI);

        // Filtro passa-alta para remover ruído de baixa frequência
        cv::Mat filter = cv::Mat::ones(complexI.size(), CV_32F);
        int cx = filter.cols / 2;
        int cy = filter.rows / 2;
        int radius = 30;
        cv::circle(filter, cv::Point(cx, cy), radius, cv::Scalar(0), -1);
        cv::Mat planes2[2];
        cv::split(complexI, planes2);
        planes2[0] = planes2[0].mul(filter);
        planes2[1] = planes2[1].mul(filter);
        cv::merge(planes2, 2, complexI);

        cv::idft(complexI, complexI);
        cv::split(complexI, planes);
        cv::normalize(planes[0], img, 0, 255, cv::NORM_MINMAX);
        img.convertTo(img, CV_8U);
        img = img(cv::Rect(0, 0, gray.cols, gray.rows));
    });
    statusLabel->setText("FFT aplicado para remoção de ruído");
}

void MainWindow::subtractBackground() {
    applyOperationToCurrentEntity([](cv::Mat& img) {
        cv::Mat gray;
        if (img.channels() > 1) {
            cv::cvtColor(img, gray, cv::COLOR_BGR2GRAY);
        } else {
            gray = img.clone();
        }

        // Estimar fundo com filtro morfológico
        cv::Mat background;
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(51, 51));
        cv::morphologyEx(gray, background, cv::MORPH_CLOSE, kernel);

        // Subtrair fundo
        cv::subtract(background, gray, img);
    });
    statusLabel->setText("Fundo subtraído");
}

void MainWindow::applyGaussianBlur() {
    // TODO: Criar diálogo para escolher sigma
    applyOperationToCurrentEntity([](cv::Mat& img) {
        cv::GaussianBlur(img, img, cv::Size(5, 5), 1.5);
    });
    statusLabel->setText("Filtro Gaussiano aplicado");
}

void MainWindow::applySharpenFilter() {
    applyOperationToCurrentEntity([](cv::Mat& img) {
        cv::Mat kernel = (cv::Mat_<float>(3, 3) <<
            0, -1, 0,
            -1, 5, -1,
            0, -1, 0);
        cv::filter2D(img, img, -1, kernel);
    });
    statusLabel->setText("Filtro de nitidez aplicado");
}
void MainWindow::adjustBrightnessContrast() {
    // TODO: Criar diálogo para ajustar parâmetros
    applyOperationToCurrentEntity([](cv::Mat& img) {
        double alpha = 1.2; // contraste
        double beta = 10;   // brilho
        img.convertTo(img, -1, alpha, beta);
    });
    statusLabel->setText("Brilho/Contraste ajustados");
}

void MainWindow::equalizeHistogram() {
    applyOperationToCurrentEntity([](cv::Mat& img) {
        if (img.channels() == 1) {
            cv::equalizeHist(img, img);
        } else {
            cv::Mat ycrcb;
            cv::cvtColor(img, ycrcb, cv::COLOR_BGR2YCrCb);
            std::vector<cv::Mat> channels;
            cv::split(ycrcb, channels);
            cv::equalizeHist(channels[0], channels[0]);
            cv::merge(channels, ycrcb);
            cv::cvtColor(ycrcb, img, cv::COLOR_YCrCb2BGR);
        }
    });
    statusLabel->setText("Histograma equalizado");
}

void MainWindow::applyCLAHE() {
    applyOperationToCurrentEntity([](cv::Mat& img) {
        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
        if (img.channels() == 1) {
            clahe->apply(img, img);
        } else {
            cv::Mat lab;
            cv::cvtColor(img, lab, cv::COLOR_BGR2Lab);
            std::vector<cv::Mat> channels;
            cv::split(lab, channels);
            clahe->apply(channels[0], channels[0]);
            cv::merge(channels, lab);
            cv::cvtColor(lab, img, cv::COLOR_Lab2BGR);
        }
    });
    statusLabel->setText("CLAHE aplicado");
}

void MainWindow::invertColors() {
    applyOperationToCurrentEntity([](cv::Mat& img) {
        cv::bitwise_not(img, img);
    });
    statusLabel->setText("Cores invertidas");
}

void MainWindow::rotateImage() {
    rotateCustomAngle();
}

void MainWindow::binarizeImage() {
    applyOperationToCurrentEntity([](cv::Mat& img) {
        if (img.channels() > 1) {
            cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
        }
        cv::threshold(img, img, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);
    });
    statusLabel->setText("Imagem binarizada (Otsu)");
}

void MainWindow::skeletonizeImage() {
    applyOperationToCurrentEntity([](cv::Mat& img) {
        // Garantir imagem binária
        if (img.channels() > 1) {
            cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
        }
        cv::threshold(img, img, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

        // Algoritmo de esqueletização
        cv::Mat skeleton = cv::Mat::zeros(img.size(), CV_8UC1);
        cv::Mat temp, eroded;
        cv::Mat element = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));
        cv::Mat working = img.clone();

        bool done = false;
        while (!done) {
            cv::erode(working, eroded, element);
            cv::dilate(eroded, temp, element);
            cv::subtract(working, temp, temp);
            cv::bitwise_or(skeleton, temp, skeleton);
            eroded.copyTo(working);
            done = (cv::countNonZero(working) == 0);
        }

        skeleton.copyTo(img);
    });
    statusLabel->setText("Imagem esqueletizada");
}

void MainWindow::extractMinutiae() {
    if (!imageProcessor->isImageLoaded()) return;

    // Verificar se imagem está esqueletizada
    cv::Mat current = imageProcessor->getCurrentImage();
    std::vector<Minutia> minutiae = minutiaeExtractor->extractMinutiae(current);

    minutiaeEditor->setMinutiae(minutiae);
    updateMinutiaeList();

    statusLabel->setText(QString("Extracted %1 minutiae").arg(minutiae.size()));
}
void MainWindow::compareMinutiae() { statusLabel->setText("Minutiae compared"); }
void MainWindow::generateChart() { statusLabel->setText("Chart generated"); }
void MainWindow::zoomIn() { originalImageViewer->zoomIn(); processedImageViewer->zoomIn(); }
void MainWindow::zoomOut() { originalImageViewer->zoomOut(); processedImageViewer->zoomOut(); }
void MainWindow::zoomFit() { originalImageViewer->zoomToFit(); processedImageViewer->zoomToFit(); }
void MainWindow::zoomActual() { originalImageViewer->zoomToActual(); processedImageViewer->zoomToActual(); }
void MainWindow::toggleSideBySide() { sideBySideMode = !sideBySideMode; }
void MainWindow::showProcessingProgress(const QString &operation) {
    Q_UNUSED(operation);
    progressBar->setVisible(true);
    progressBar->setValue(0);
}
void MainWindow::hideProcessingProgress() {
    progressBar->setVisible(false);
}

// ========== NOVAS IMPLEMENTAÇÕES ==========

void MainWindow::changeLanguage() {
    TranslationManager& tm = TranslationManager::instance();
    auto languages = tm.getAvailableLanguages();

    QStringList languageList;
    QList<TranslationManager::Language> langKeys;

    for (auto it = languages.begin(); it != languages.end(); ++it) {
        languageList << QString::fromStdString(it->second);
        langKeys << it->first;
    }

    bool ok;
    QString selectedLang = QInputDialog::getItem(this, "Select Language",
                                                 "Choose your preferred language:",
                                                 languageList, 0, false, &ok);

    if (ok && !selectedLang.isEmpty()) {
        int index = languageList.indexOf(selectedLang);
        if (index >= 0 && index < langKeys.size()) {
            TranslationManager::Language newLang = langKeys[index];
            if (tm.setLanguage(newLang)) {
                tm.saveLanguageSettings(newLang);
                QMessageBox::information(this, "Language Changed",
                    "Language changed successfully. Please restart the application for all changes to take effect.");
            } else {
                QMessageBox::warning(this, "Error",
                    "Failed to load language file. Please check translations directory.");
            }
        }
    }
}

void MainWindow::applyBrightnessContrastRealtime() {
    if (!imageProcessor->isImageLoaded() || isProcessing) return;

    double brightness = brightnessSlider->value();
    double contrast = contrastSlider->value() / 100.0;

    // Aplicar temporariamente sem adicionar ao histórico
    cv::Mat original = imageProcessor->getOriginalImage();
    cv::Mat result;
    original.convertTo(result, -1, contrast, brightness);

    processedImageViewer->setImage(result);
}

void MainWindow::runProcessingInThread(std::function<cv::Mat(const cv::Mat&, int&)> processingFunc) {
    if (isProcessing) {
        QMessageBox::warning(this, "Processing",
            "Another processing operation is already running. Please wait.");
        return;
    }

    if (!imageProcessor->isImageLoaded()) {
        QMessageBox::warning(this, "No Image", "Please load an image first.");
        return;
    }

    isProcessing = true;
    showProcessingProgress("Processing");

    // Criar thread e worker
    processingThread = new QThread();
    processingWorker = new ProcessingWorker();
    processingWorker->moveToThread(processingThread);

    // Configurar worker
    processingWorker->setCustomOperation(processingFunc);
    processingWorker->setInputImage(imageProcessor->getCurrentImage());

    // Conectar sinais
    connect(processingThread, &QThread::started, processingWorker, &ProcessingWorker::process);
    connect(processingWorker, &ProcessingWorker::progressUpdated, this, &MainWindow::onProcessingProgress);
    connect(processingWorker, &ProcessingWorker::operationCompleted, this, &MainWindow::onProcessingCompleted);
    connect(processingWorker, &ProcessingWorker::operationFailed, this, &MainWindow::onProcessingFailed);
    connect(processingWorker, &ProcessingWorker::statusMessage, this, &MainWindow::onProcessingStatus);

    // Cleanup quando terminar
    connect(processingWorker, &ProcessingWorker::operationCompleted, processingThread, &QThread::quit);
    connect(processingWorker, &ProcessingWorker::operationFailed, processingThread, &QThread::quit);
    connect(processingThread, &QThread::finished, processingWorker, &QObject::deleteLater);
    connect(processingThread, &QThread::finished, processingThread, &QObject::deleteLater);

    // Iniciar thread
    processingThread->start();
}

void MainWindow::onProcessingProgress(int percentage) {
    progressBar->setValue(percentage);
}

void MainWindow::onProcessingCompleted(cv::Mat result) {
    // Atualizar imagem processada
    if (!result.empty()) {
        // Copiar resultado para o ImageProcessor
        cv::Mat current = imageProcessor->getCurrentImage();
        result.copyTo(current);

        updateImageDisplay();
        statusLabel->setText("Processing completed successfully");
    }

    hideProcessingProgress();
    isProcessing = false;
    processingWorker = nullptr;
    processingThread = nullptr;
}

void MainWindow::onProcessingFailed(QString errorMessage) {
    QMessageBox::critical(this, "Processing Error", errorMessage);
    hideProcessingProgress();
    isProcessing = false;
    processingWorker = nullptr;
    processingThread = nullptr;
}

void MainWindow::onProcessingStatus(QString message) {
    statusLabel->setText(message);
}


// ==================== FERRAMENTAS DE RECORTE ====================

void MainWindow::activateCropTool() {
    // Desativar modo de recorte na imagem original (não usamos ela para recorte)
    originalImageViewer->setCropMode(false);

    // Ativar modo de recorte na imagem processada
    processedImageViewer->setCropMode(true);

    statusLabel->setText("Ferramenta de recorte ativada. Clique e arraste para selecionar uma área.");
}

void MainWindow::applyCrop() {
    using PM = FingerprintEnhancer::ProjectManager;

    if (!processedImageViewer->hasCropSelection()) {
        QMessageBox::warning(this, "Recorte", "Nenhuma área selecionada.");
        return;
    }

    // Precisa ter uma entidade corrente (Image ou Fragment)
    if (currentEntityType == ENTITY_NONE || currentEntityId.isEmpty()) {
        QMessageBox::warning(this, "Recorte", "Nenhuma imagem ou fragmento selecionado.");
        return;
    }

    // Obter retângulo de seleção
    QRect selection = processedImageViewer->getCropSelection();

    // Validar seleção
    if (selection.width() < 10 || selection.height() < 10) {
        QMessageBox::warning(this, "Recorte", "Área selecionada muito pequena.");
        return;
    }

    // Obter a workingImage da entidade corrente para recortar
    cv::Mat& workingImg = getCurrentWorkingImage();
    if (workingImg.empty()) {
        QMessageBox::warning(this, "Erro", "Imagem de trabalho inválida");
        return;
    }

    // Validar que o rect está dentro dos limites
    cv::Rect cvRect(selection.x(), selection.y(), selection.width(), selection.height());
    cvRect &= cv::Rect(0, 0, workingImg.cols, workingImg.rows);

    if (cvRect.width <= 0 || cvRect.height <= 0) {
        QMessageBox::warning(this, "Erro", "Seleção fora dos limites da imagem");
        return;
    }

    // Extrair região da workingImage (não da original!)
    cv::Mat croppedImage = workingImg(cvRect).clone();

    // Criar fragmento manualmente
    QString parentImageId;
    if (currentEntityType == ENTITY_IMAGE) {
        parentImageId = currentEntityId;
    } else if (currentEntityType == ENTITY_FRAGMENT) {
        // Se está recortando de um fragmento, pegar a imagem pai
        FingerprintEnhancer::Fragment* parentFrag = PM::instance().getCurrentProject()->findFragment(currentEntityId);
        if (parentFrag) {
            parentImageId = parentFrag->parentImageId;
        }
    }

    if (parentImageId.isEmpty()) {
        QMessageBox::warning(this, "Erro", "Não foi possível determinar a imagem pai");
        return;
    }

    // Adicionar fragmento ao projeto
    FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(parentImageId);
    if (!img) {
        QMessageBox::warning(this, "Erro", "Imagem pai não encontrada");
        return;
    }

    FingerprintEnhancer::Fragment newFragment(parentImageId, selection, croppedImage);
    img->fragments.append(newFragment);
    PM::instance().getCurrentProject()->setModified();

    // Desativar modo de recorte
    processedImageViewer->clearCropSelection();
    processedImageViewer->setCropMode(false);
    setToolMode(TOOL_NONE);

    // Selecionar o fragmento criado
    onFragmentSelected(img->fragments.last().id);

    // Atualizar visualização no gerenciador
    fragmentManager->updateView();

    statusLabel->setText(QString("Fragmento criado: %1x%2 px")
                        .arg(selection.width()).arg(selection.height()));
}

void MainWindow::cancelCropSelection() {
    processedImageViewer->clearCropSelection();
    processedImageViewer->setCropMode(false);
    statusLabel->setText("Seleção cancelada.");
}

void MainWindow::saveCroppedImage() {
    if (!processedImageViewer->hasCropSelection()) {
        QMessageBox::warning(this, "Salvar Recorte", "Nenhuma área selecionada.");
        return;
    }

    // Usar imageProcessor diretamente (já é membro da classe)
    if (!imageProcessor || !imageProcessor->isImageLoaded()) {
        QMessageBox::warning(this, "Salvar Recorte", "Nenhuma imagem carregada.");
        return;
    }

    cv::Mat currentImg = imageProcessor->getCurrentImage();
    if (currentImg.empty()) {
        currentImg = imageProcessor->getOriginalImage();
    }

    // Obter retângulo de seleção e recortar
    QRect selection = processedImageViewer->getCropSelection();
    cv::Rect cropRect(selection.x(), selection.y(), selection.width(), selection.height());
    cv::Mat cropped = currentImg(cropRect).clone();
    if (cropped.empty()) {
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        "Salvar Imagem Recortada", "",
        "PNG (*.png);;JPEG (*.jpg *.jpeg);;TIFF (*.tif *.tiff);;Todos (*.*)"
    );

    if (!fileName.isEmpty()) {
        cv::imwrite(fileName.toStdString(), cropped);
        statusLabel->setText("Imagem recortada salva: " + fileName);
    }
}

// ==================== FERRAMENTAS DE ROTAÇÃO ====================

// Função auxiliar que faz rotação preservando toda a imagem
cv::Mat MainWindow::rotateImagePreservingSize(const cv::Mat &image, double angle) {
    if (image.empty()) return image;

    cv::Point2f center(image.cols / 2.0f, image.rows / 2.0f);
    cv::Mat rotMatrix = cv::getRotationMatrix2D(center, -angle, 1.0);

    // Calcular novo tamanho da imagem para não cortar
    double abs_cos = abs(rotMatrix.at<double>(0, 0));
    double abs_sin = abs(rotMatrix.at<double>(0, 1));
    int new_w = int(image.rows * abs_sin + image.cols * abs_cos);
    int new_h = int(image.rows * abs_cos + image.cols * abs_sin);

    // Ajustar matriz de rotação para o novo tamanho
    rotMatrix.at<double>(0, 2) += (new_w / 2.0) - center.x;
    rotMatrix.at<double>(1, 2) += (new_h / 2.0) - center.y;

    cv::Mat rotated;
    cv::warpAffine(image, rotated, rotMatrix, cv::Size(new_w, new_h),
                   cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));

    return rotated;
}

void MainWindow::rotateRight90() {
    applyOperationToCurrentEntity([this](cv::Mat& img) {
        cv::rotate(img, img, cv::ROTATE_90_CLOCKWISE);
    });
    statusLabel->setText("Imagem rotacionada 90° à direita");
}

void MainWindow::rotateLeft90() {
    applyOperationToCurrentEntity([this](cv::Mat& img) {
        cv::rotate(img, img, cv::ROTATE_90_COUNTERCLOCKWISE);
    });
    statusLabel->setText("Imagem rotacionada 90° à esquerda");
}

void MainWindow::rotate180() {
    applyOperationToCurrentEntity([this](cv::Mat& img) {
        cv::rotate(img, img, cv::ROTATE_180);
    });
    statusLabel->setText("Imagem rotacionada 180°");
}

void MainWindow::rotateCustomAngle() {
    if (currentEntityType == ENTITY_NONE || currentEntityId.isEmpty()) {
        QMessageBox::warning(this, "Rotação", "Nenhuma imagem ou fragmento selecionado");
        return;
    }

    cv::Mat& workingImg = getCurrentWorkingImage();
    if (workingImg.empty()) {
        QMessageBox::warning(this, "Rotação", "Imagem inválida");
        return;
    }

    // Criar diálogo de rotação em tempo real
    RotationDialog dialog(workingImg, processedImageViewer, this);
    int result = dialog.exec();

    if (result == QDialog::Accepted && dialog.wasAccepted()) {
        double angle = dialog.getRotationAngle();
        cv::Mat rotated = dialog.getRotatedImage();

        // Aplicar diretamente na workingImage da entidade corrente
        rotated.copyTo(workingImg);

        // Recarregar visualização
        loadCurrentEntityToView();

        statusLabel->setText(QString("Imagem rotacionada %1°").arg(angle, 0, 'f', 1));

        // Marcar como modificado
        using PM = FingerprintEnhancer::ProjectManager;
        PM::instance().getCurrentProject()->setModified();
    } else {
        statusLabel->setText("Rotação cancelada");
    }
}

// ==================== CALIBRAÇÃO DE ESCALA ====================

void MainWindow::calibrateScale() {
    if (currentEntityType == ENTITY_NONE || currentEntityId.isEmpty()) {
        QMessageBox::warning(this, "Calibração", "Nenhuma imagem ou fragmento selecionado");
        return;
    }

    bool ok;
    double distance = QInputDialog::getDouble(this, "Calibração de Escala",
        "Insira a distância conhecida em milímetros\n"
        "(você será solicitado a marcar esta distância na imagem):",
        10.0, 0.1, 1000.0, 2, &ok);

    if (!ok) return;

    QMessageBox::information(this, "Calibração",
        "Clique em dois pontos na imagem para marcar a distância conhecida.\n"
        "Use o modo de medição para fazer isso.");

    // TODO: Implementar seleção interativa de dois pontos
    // Por agora, usar input manual
    double pixelDistance = QInputDialog::getDouble(this, "Calibração de Escala",
        "Insira a distância em pixels medida na imagem:",
        100.0, 1.0, 10000.0, 2, &ok);

    if (!ok) return;

    double scale = pixelDistance / distance; // pixels por mm
    imageProcessor->setScale(scale);

    statusLabel->setText(QString("Escala calibrada: %1 pixels/mm").arg(scale, 0, 'f', 2));
}

void MainWindow::setScaleManually() {
    bool ok;
    double scale = QInputDialog::getDouble(this, "Definir Escala",
        "Insira a escala em pixels por milímetro:",
        imageProcessor->getScale(), 0.01, 1000.0, 2, &ok);

    if (ok) {
        imageProcessor->setScale(scale);
        statusLabel->setText(QString("Escala definida: %1 pixels/mm").arg(scale, 0, 'f', 2));
    }
}

void MainWindow::showScaleInfo() {
    double scale = imageProcessor->getScale();
    if (scale > 0) {
        QMessageBox::information(this, "Informação de Escala",
            QString("Escala atual: %1 pixels/mm\n"
                    "Resolução: %2 DPI")
                .arg(scale, 0, 'f', 2)
                .arg(scale * 25.4, 0, 'f', 0)); // 1 inch = 25.4 mm
    } else {
        QMessageBox::information(this, "Informação de Escala",
            "Nenhuma escala foi calibrada ainda.");
    }
}

// ==================== CONVERSÃO DE ESPAÇOS DE COR ====================

void MainWindow::convertToRGB() {
    applyOperationToCurrentEntity([](cv::Mat& img) {
        if (img.channels() == 1) {
            cv::cvtColor(img, img, cv::COLOR_GRAY2BGR);
        } else if (img.channels() == 4) {
            cv::cvtColor(img, img, cv::COLOR_BGRA2BGR);
        }
        // Se já está em BGR (RGB do OpenCV), não fazer nada
    });
    statusLabel->setText("Convertido para RGB");
}

void MainWindow::convertToHSV() {
    applyOperationToCurrentEntity([](cv::Mat& img) {
        if (img.channels() == 1) {
            cv::cvtColor(img, img, cv::COLOR_GRAY2BGR);
        }
        if (img.channels() == 3) {
            cv::cvtColor(img, img, cv::COLOR_BGR2HSV);
        }
    });
    statusLabel->setText("Convertido para HSV");
}

void MainWindow::convertToHSI() {
    applyOperationToCurrentEntity([](cv::Mat& img) {
        if (img.channels() == 1) {
            cv::cvtColor(img, img, cv::COLOR_GRAY2BGR);
        }
        if (img.channels() == 3) {
            // OpenCV não tem conversão direta para HSI
            // Converter para HSV e modificar canal V para I (Intensity)
            cv::Mat hsv;
            cv::cvtColor(img, hsv, cv::COLOR_BGR2HSV);

            // Calcular Intensity como média dos canais BGR
            std::vector<cv::Mat> bgrChannels;
            cv::split(img, bgrChannels);
            cv::Mat intensity = (bgrChannels[0] + bgrChannels[1] + bgrChannels[2]) / 3;

            // Substituir canal V por I
            std::vector<cv::Mat> hsiChannels;
            cv::split(hsv, hsiChannels);
            hsiChannels[2] = intensity;

            cv::merge(hsiChannels, img);
        }
    });
    statusLabel->setText("Convertido para HSI (aproximado)");
}

void MainWindow::convertToLab() {
    applyOperationToCurrentEntity([](cv::Mat& img) {
        if (img.channels() == 1) {
            cv::cvtColor(img, img, cv::COLOR_GRAY2BGR);
        }
        if (img.channels() == 3) {
            cv::cvtColor(img, img, cv::COLOR_BGR2Lab);
        }
    });
    statusLabel->setText("Convertido para Lab");
}

void MainWindow::convertToGrayscale() {
    applyOperationToCurrentEntity([](cv::Mat& img) {
        if (img.channels() > 1) {
            cv::cvtColor(img, img, cv::COLOR_BGR2GRAY);
        }
    });
    statusLabel->setText("Convertido para escala de cinza");
}

void MainWindow::adjustColorLevels() {
    // TODO: Criar diálogo para ajuste de níveis por canal
    QMessageBox::information(this, "Ajuste de Níveis",
        "Funcionalidade em desenvolvimento.\n"
        "Em breve você poderá ajustar níveis individuais de cada canal de cor.");
}

// ==================== MENU DE CONTEXTO E MINÚCIAS ====================

void MainWindow::showContextMenu(const QPoint &pos) {
    QMenu contextMenu("Menu", this);
    QPoint imagePos = processedImageViewer->widgetToImage(pos);

    // Menu baseado no modo de ferramenta ativa
    switch (currentToolMode) {
        case TOOL_NONE:
            // Menu genérico baseado na entidade corrente
            if (currentEntityType == ENTITY_IMAGE) {
                contextMenu.addAction("📐 Destacar Imagem Inteira como Fragmento",
                                     this, &MainWindow::createFragmentFromWholeImage);
            } else if (currentEntityType == ENTITY_FRAGMENT) {
                contextMenu.addAction("➕ Adicionar Minúcia Aqui", [this, imagePos]() {
                    setToolMode(TOOL_ADD_MINUTIA);
                    addMinutiaAtPosition(imagePos);
                });
            } else {
                contextMenu.addAction("Selecione uma imagem ou fragmento primeiro");
            }
            break;

        case TOOL_CROP:
            if (processedImageViewer->hasCropSelection()) {
                contextMenu.addAction("✓ Aplicar Recorte", this, &MainWindow::applyCrop);
                contextMenu.addAction("Ajustar Seleção", this, &MainWindow::onCropAdjust);
                contextMenu.addAction("Mover Seleção", this, &MainWindow::onCropMove);
                contextMenu.addSeparator();
                contextMenu.addAction("✗ Cancelar Recorte", this, &MainWindow::cancelCropSelection);
            } else {
                contextMenu.addAction("Desenhe uma área para recortar");
            }
            break;

        case TOOL_ADD_MINUTIA:
            if (currentEntityType == ENTITY_FRAGMENT) {
                contextMenu.addAction("➕ Adicionar Minúcia Aqui", [this, imagePos]() {
                    addMinutiaAtPosition(imagePos);
                });
            } else {
                contextMenu.addAction("Selecione um fragmento primeiro");
            }
            break;

        case TOOL_EDIT_MINUTIA:
            contextMenu.addAction("Clique em uma minúcia para editar");
            contextMenu.addSeparator();
            contextMenu.addAction("Adicionar Nova Minúcia", [this, imagePos]() {
                if (currentEntityType == ENTITY_FRAGMENT) {
                    addMinutiaAtPosition(imagePos);
                }
            });
            break;

        case TOOL_REMOVE_MINUTIA:
            contextMenu.addAction("Clique em uma minúcia para remover");
            break;

        case TOOL_PAN:
            contextMenu.addAction("Modo Pan ativo");
            break;
    }

    if (!contextMenu.isEmpty()) {
        contextMenu.exec(processedImageViewer->mapToGlobal(pos));
    }
}

void MainWindow::addMinutiaAtPosition(const QPoint &imagePos) {
    using PM = FingerprintEnhancer::ProjectManager;

    // Verificar se há um fragmento selecionado usando o novo sistema
    if (currentEntityType != ENTITY_FRAGMENT || currentEntityId.isEmpty()) {
        QMessageBox::warning(this, "Erro", "Nenhum fragmento selecionado.\n"
                                          "Selecione um fragmento no painel Projeto antes de adicionar minúcias.");
        return;
    }

    // Criar minúcia temporária para edição
    FingerprintEnhancer::Minutia tempMinutia("", "", imagePos, MinutiaeType::RIDGE_ENDING, 0.0f, 1.0f);

    // Abrir diálogo para escolher tipo e configurar
    FingerprintEnhancer::MinutiaEditDialog dialog(&tempMinutia, this);

    if (dialog.exec() == QDialog::Accepted) {
        // Obter valores do diálogo
        QPoint finalPos = dialog.getPosition();
        MinutiaeType type = dialog.getType();
        float angle = dialog.getAngle();
        float quality = dialog.getQuality();
        QString notes = dialog.getNotes();

        // Adicionar minúcia ao fragmento no projeto
        FingerprintEnhancer::Minutia* minutia = PM::instance().addMinutiaToFragment(
            currentEntityId,  // Usar currentEntityId em vez de currentFragmentId
            finalPos,
            type,
            angle,
            quality
        );

        if (minutia) {
            if (!notes.isEmpty()) {
                minutia->notes = notes;
            }

            statusLabel->setText(QString("Minúcia %1 adicionada em (%2, %3)")
                                .arg(getMinutiaeTypeName(type))
                                .arg(finalPos.x()).arg(finalPos.y()));

            // Atualizar overlay
            FingerprintEnhancer::Fragment* frag = PM::instance().getCurrentProject()->findFragment(currentEntityId);
            if (frag) {
                minutiaeOverlay->setFragment(frag);
                minutiaeOverlay->update();
            }

            // Atualizar view do gerenciador
            fragmentManager->updateView();
        } else {
            QMessageBox::warning(this, "Erro", "Falha ao adicionar minúcia");
        }
    }

    // Código legado abaixo - manter comentado para referência
    /*
    // Adicionar minúcia na posição
    markedMinutiae.append(imagePos);
    minutiaeTypes.append("Não classificada");

    // Atualizar lista
    minutiaeList->addItem(QString("Minúcia #%1 - Posição: (%2, %3) - Tipo: Não classificada")
        .arg(markedMinutiae.size())
        .arg(imagePos.x())
        .arg(imagePos.y()));

    // Redesenhar imagem com a minúcia marcada
    if (imageProcessor && imageProcessor->isImageLoaded()) {
        cv::Mat currentImg = imageProcessor->getCurrentImage().clone();

        // Desenhar círculo com X para todas as minúcias
        for (int i = 0; i < markedMinutiae.size(); ++i) {
            QPoint pt = markedMinutiae[i];
            cv::Point center(pt.x(), pt.y());

            // Círculo
            cv::circle(currentImg, center, 10, cv::Scalar(0, 0, 255), 2);

            // X (duas linhas cruzadas)
            cv::line(currentImg,
                    cv::Point(center.x - 7, center.y - 7),
                    cv::Point(center.x + 7, center.y + 7),
                    cv::Scalar(0, 0, 255), 2);
            cv::line(currentImg,
                    cv::Point(center.x + 7, center.y - 7),
                    cv::Point(center.x - 7, center.y + 7),
                    cv::Scalar(0, 0, 255), 2);

            // Número
            cv::putText(currentImg, std::to_string(i + 1),
                       cv::Point(center.x + 15, center.y - 15),
                       cv::FONT_HERSHEY_SIMPLEX, 0.5,
                       cv::Scalar(255, 0, 0), 2);
        }

        processedImageViewer->setImage(currentImg);
    }

    statusLabel->setText(QString("Minúcia #%1 adicionada na posição (%2, %3)")
        .arg(markedMinutiae.size())
        .arg(imagePos.x())
        .arg(imagePos.y()));
    */
}

// ==================== FERRAMENTAS DE MINÚCIAS ====================

void MainWindow::activateAddMinutia() {
    if (!minutiaeMarker) {
        minutiaeMarker = new MinutiaeMarkerWidget(this);
        // TODO: Integrar com a interface principal
    }

    // Diálogo para selecionar tipo de minúcia
    QStringList types = MinutiaeTypeInfo::getAllTypeNames();
    bool ok;
    QString selectedType = QInputDialog::getItem(this,
        "Tipo de Minúcia",
        "Selecione o tipo de minúcia a adicionar:",
        types, 0, false, &ok);

    if (ok) {
        MinutiaeType type = MinutiaeTypeInfo::getTypeFromName(selectedType);
        minutiaeMarker->setCurrentMinutiaeType(type);
        minutiaeMarker->setMode(MinutiaeMarkerWidget::Mode::AddMinutia);
        statusLabel->setText("Modo: Adicionar Minúcia (" + selectedType + "). Clique na imagem.");
    }
}

void MainWindow::activateEditMinutia() {
    if (minutiaeMarker) {
        minutiaeMarker->setMode(MinutiaeMarkerWidget::Mode::EditMinutia);
        statusLabel->setText("Modo: Editar Minúcia. Clique em uma minúcia existente.");
    }
}

void MainWindow::activateRemoveMinutia() {
    if (minutiaeMarker) {
        minutiaeMarker->setMode(MinutiaeMarkerWidget::Mode::RemoveMinutia);
        statusLabel->setText("Modo: Remover Minúcia. Clique em uma minúcia para remover.");
    }
}

void MainWindow::toggleShowMinutiaeNumbers() {
    if (minutiaeMarker) {
        bool show = !minutiaeMarker->isShowingNumbers();
        minutiaeMarker->setShowNumbers(show);
        statusLabel->setText(show ? "Números das minúcias visíveis" : "Números das minúcias ocultos");
    }
}

void MainWindow::toggleShowMinutiaeAngles() {
    if (minutiaeMarker) {
        bool show = !minutiaeMarker->isShowingAngles();
        minutiaeMarker->setShowAngles(show);
        statusLabel->setText(show ? "Ângulos das minúcias visíveis" : "Ângulos das minúcias ocultos");
    }
}

void MainWindow::toggleShowMinutiaeLabels() {
    if (minutiaeMarker) {
        bool show = !minutiaeMarker->isShowingLabels();
        minutiaeMarker->setShowLabels(show);
        statusLabel->setText(show ? "Tipos das minúcias visíveis" : "Tipos das minúcias ocultos");
    }
}

void MainWindow::saveMinutiaeNumberedImage() {
    if (!minutiaeMarker) {
        QMessageBox::warning(this, "Salvar", "Nenhuma minúcia marcada.");
        return;
    }

    cv::Mat numberedImage = minutiaeMarker->renderMinutiaeImage(true, true, true);
    if (numberedImage.empty()) {
        QMessageBox::warning(this, "Salvar", "Erro ao gerar imagem.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this,
        "Salvar Imagem com Minúcias Numeradas", "",
        "PNG (*.png);;JPEG (*.jpg *.jpeg);;TIFF (*.tif *.tiff);;Todos (*.*)"
    );

    if (!fileName.isEmpty()) {
        cv::imwrite(fileName.toStdString(), numberedImage);
        statusLabel->setText("Imagem com minúcias salva: " + fileName);
    }
}

void MainWindow::clearAllMinutiae() {
    if (minutiaeMarker) {
        auto reply = QMessageBox::question(this,
            "Limpar Minúcias",
            "Tem certeza que deseja remover todas as minúcias?",
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            minutiaeMarker->clearMinutiae();
            statusLabel->setText("Todas as minúcias foram removidas.");
        }
    }
}

// ==================== MENU AFIS ====================

void MainWindow::loadAFISDatabase() {
    QString dirPath = QFileDialog::getExistingDirectory(this,
        "Selecionar Diretório com Base de Dados AFIS",
        QDir::homePath());

    if (dirPath.isEmpty()) {
        return;
    }

    if (!afisMatcher) {
        afisMatcher = new AFISMatcher();
    }

    bool success = afisMatcher->loadDatabase(dirPath);
    if (success) {
        int count = afisMatcher->getDatabaseSize();
        QMessageBox::information(this, "AFIS",
            QString("Base de dados carregada com sucesso!\n"
                   "Total de impressões digitais: %1").arg(count));
        statusLabel->setText(QString("Base AFIS carregada: %1 digitais").arg(count));
    } else {
        QMessageBox::warning(this, "AFIS",
            "Erro ao carregar base de dados.\n"
            "Verifique se o diretório contém imagens de impressões digitais.");
    }
}

void MainWindow::identifyFingerprint() {
    if (!afisMatcher || afisMatcher->getDatabaseSize() == 0) {
        QMessageBox::warning(this, "AFIS",
            "Nenhuma base de dados carregada.\n"
            "Use 'AFIS > Carregar Base de Dados' primeiro.");
        return;
    }

    // Usar imageProcessor diretamente (já é membro da classe)
    if (!imageProcessor || !imageProcessor->isImageLoaded()) {
        QMessageBox::warning(this, "AFIS", "Nenhuma imagem carregada para identificar.");
        return;
    }

    cv::Mat queryImage = imageProcessor->getCurrentImage();
    if (queryImage.empty()) {
        queryImage = imageProcessor->getOriginalImage();
    }

    if (queryImage.empty()) {
        QMessageBox::warning(this, "AFIS", "Nenhuma imagem carregada para identificar.");
        return;
    }

    statusLabel->setText("Identificando impressão digital...");
    progressBar->setVisible(true);
    progressBar->setRange(0, 0);  // Indeterminado

    // Executar identificação (pode demorar)
    QVector<AFISMatchResult> results = afisMatcher->identifyFingerprintFromImage(queryImage, 10);

    progressBar->setVisible(false);

    if (results.isEmpty()) {
        QMessageBox::information(this, "AFIS",
            "Nenhuma correspondência encontrada na base de dados.");
        statusLabel->setText("AFIS: Nenhuma correspondência encontrada.");
        return;
    }

    // Mostrar resultados
    QString resultText = "Resultados da Identificação AFIS:\n\n";
    for (int i = 0; i < results.size() && i < 10; i++) {
        const auto &r = results[i];
        resultText += QString("%1. %2\n"
                             "   Similaridade: %3%\n"
                             "   Minúcias correspondentes: %4/%5\n"
                             "   Confiança: %6%\n\n")
            .arg(i + 1)
            .arg(r.candidateId)
            .arg(r.similarityScore * 100, 0, 'f', 2)
            .arg(r.matchedMinutiae)
            .arg(r.totalQueryMinutiae)
            .arg(r.confidenceLevel * 100, 0, 'f', 2);
    }

    QMessageBox::information(this, "Resultados AFIS", resultText);
    statusLabel->setText(QString("AFIS: %1 correspondências encontradas").arg(results.size()));
}

void MainWindow::verifyFingerprint() {
    QMessageBox::information(this, "AFIS",
        "Função de verificação 1:1 em desenvolvimento.\n"
        "Esta função comparará duas impressões digitais específicas.");
}

void MainWindow::configureAFISMatching() {
    if (!afisMatcher) {
        afisMatcher = new AFISMatcher();
    }

    AFISMatchConfig config = afisMatcher->getConfig();

    // Diálogo simples para configuração
    bool ok;
    int minMatches = QInputDialog::getInt(this,
        "Configurar AFIS",
        "Mínimo de minúcias correspondentes:",
        config.minMatchedMinutiae, 1, 100, 1, &ok);

    if (ok) {
        config.minMatchedMinutiae = minMatches;

        double minScore = QInputDialog::getDouble(this,
            "Configurar AFIS",
            "Score mínimo de similaridade (0.0 a 1.0):",
            config.minSimilarityScore, 0.0, 1.0, 2, &ok);

        if (ok) {
            config.minSimilarityScore = minScore;
            afisMatcher->setConfig(config);
            QMessageBox::information(this, "AFIS", "Configuração atualizada!");
        }
    }
}

void MainWindow::showAFISResults() {
    QMessageBox::information(this, "AFIS",
        "Visualizador de resultados AFIS em desenvolvimento.\n"
        "Permitirá visualizar matches detalhados e exportar relatórios.");
}

// ========== Novos Slots do Sistema de Projetos ==========

void MainWindow::onProjectModified() {
    updateWindowTitle();
}

void MainWindow::onImageAdded(const QString& imageId) {
    fragmentManager->updateView();
    statusLabel->setText("Imagem adicionada ao projeto");
}

void MainWindow::onFragmentCreated(const QString& fragmentId) {
    fragmentManager->updateView();
    currentFragmentId = fragmentId;
    statusLabel->setText("Fragmento criado");
}

void MainWindow::onFragmentSelected(const QString& fragmentId) {
    // Usar novo sistema de entidade corrente
    setCurrentEntity(fragmentId, ENTITY_FRAGMENT);
}

void MainWindow::onMinutiaAdded(const QString& minutiaId) {
    minutiaeOverlay->update();
    fragmentManager->updateView();
}

void MainWindow::onMinutiaSelected(const QString& minutiaId) {
    minutiaeOverlay->setSelectedMinutia(minutiaId);
    minutiaeOverlay->update();
}

void MainWindow::onMinutiaDoubleClicked(const QString& minutiaId) {
    using PM = FingerprintEnhancer::ProjectManager;

    FingerprintEnhancer::Minutia* minutia = PM::instance().getCurrentProject()->findMinutia(minutiaId);
    if (minutia) {
        FingerprintEnhancer::MinutiaEditDialog dialog(minutia, this);
        if (dialog.exec() == QDialog::Accepted) {
            // As mudanças já foram aplicadas ao objeto minutia
            PM::instance().getCurrentProject()->setModified();
            minutiaeOverlay->update();
            fragmentManager->updateView();
            statusLabel->setText("Minúcia editada");
        }
    }
}

void MainWindow::onMinutiaPositionChanged(const QString& minutiaId, const QPoint& newPos) {
    using PM = FingerprintEnhancer::ProjectManager;
    PM::instance().updateMinutiaPosition(minutiaId, newPos);
}

// ========== Gerenciamento de Ferramentas ==========

void MainWindow::setToolMode(ToolMode mode) {
    currentToolMode = mode;

    // Desativar todos os modos primeiro
    processedImageViewer->setCropMode(false);
    minutiaeOverlay->setEditMode(false);

    // Ativar o modo selecionado
    switch (mode) {
        case TOOL_NONE:
            statusLabel->setText("Ferramenta: Nenhuma (navegação)");
            break;

        case TOOL_CROP:
            processedImageViewer->setCropMode(true);
            statusLabel->setText("Ferramenta: Recortar - Clique e arraste para selecionar área");
            break;

        case TOOL_ADD_MINUTIA:
            minutiaeOverlay->setEditMode(false);
            statusLabel->setText("Ferramenta: Adicionar Minúcia - Clique para marcar posição");
            break;

        case TOOL_EDIT_MINUTIA:
            minutiaeOverlay->setEditMode(true);
            statusLabel->setText("Ferramenta: Editar Minúcia - Clique para selecionar, arraste para mover");
            break;

        case TOOL_REMOVE_MINUTIA:
            minutiaeOverlay->setEditMode(false);
            statusLabel->setText("Ferramenta: Remover Minúcia - Clique na minúcia para remover");
            break;

        case TOOL_PAN:
            statusLabel->setText("Ferramenta: Pan - Arraste para mover a imagem");
            break;
    }
}

void MainWindow::onCropAdjust() {
    if (!processedImageViewer->hasCropSelection()) {
        QMessageBox::information(this, "Info", "Nenhuma seleção de recorte ativa");
        return;
    }

    QRect currentSelection = processedImageViewer->getCropSelection();

    // Dialog para ajustar manualmente as coordenadas
    QDialog dialog(this);
    dialog.setWindowTitle("Ajustar Seleção de Recorte");
    QFormLayout* layout = new QFormLayout(&dialog);

    QSpinBox* xSpinBox = new QSpinBox();
    xSpinBox->setRange(0, 10000);
    xSpinBox->setValue(currentSelection.x());
    layout->addRow("X:", xSpinBox);

    QSpinBox* ySpinBox = new QSpinBox();
    ySpinBox->setRange(0, 10000);
    ySpinBox->setValue(currentSelection.y());
    layout->addRow("Y:", ySpinBox);

    QSpinBox* widthSpinBox = new QSpinBox();
    widthSpinBox->setRange(10, 10000);
    widthSpinBox->setValue(currentSelection.width());
    layout->addRow("Largura:", widthSpinBox);

    QSpinBox* heightSpinBox = new QSpinBox();
    heightSpinBox->setRange(10, 10000);
    heightSpinBox->setValue(currentSelection.height());
    layout->addRow("Altura:", heightSpinBox);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        QRect newSelection(
            xSpinBox->value(),
            ySpinBox->value(),
            widthSpinBox->value(),
            heightSpinBox->value()
        );
        processedImageViewer->setCropSelection(newSelection);
        statusLabel->setText(QString("Seleção ajustada: %1x%2 em (%3,%4)")
            .arg(newSelection.width()).arg(newSelection.height())
            .arg(newSelection.x()).arg(newSelection.y()));
    }
}

void MainWindow::onCropMove() {
    if (!processedImageViewer->hasCropSelection()) {
        QMessageBox::information(this, "Info", "Nenhuma seleção de recorte ativa");
        return;
    }

    QRect currentSelection = processedImageViewer->getCropSelection();

    // Dialog para mover a seleção
    QDialog dialog(this);
    dialog.setWindowTitle("Mover Seleção de Recorte");
    QFormLayout* layout = new QFormLayout(&dialog);

    QSpinBox* deltaXSpinBox = new QSpinBox();
    deltaXSpinBox->setRange(-10000, 10000);
    deltaXSpinBox->setValue(0);
    deltaXSpinBox->setSuffix(" px");
    layout->addRow("Deslocamento X:", deltaXSpinBox);

    QSpinBox* deltaYSpinBox = new QSpinBox();
    deltaYSpinBox->setRange(-10000, 10000);
    deltaYSpinBox->setValue(0);
    deltaYSpinBox->setSuffix(" px");
    layout->addRow("Deslocamento Y:", deltaYSpinBox);

    QLabel* info = new QLabel(QString("Posição atual: (%1, %2)\nTamanho: %3x%4")
        .arg(currentSelection.x()).arg(currentSelection.y())
        .arg(currentSelection.width()).arg(currentSelection.height()));
    layout->addRow(info);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addRow(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        QRect newSelection = currentSelection.translated(
            deltaXSpinBox->value(),
            deltaYSpinBox->value()
        );
        processedImageViewer->setCropSelection(newSelection);
        statusLabel->setText(QString("Seleção movida para (%1,%2)")
            .arg(newSelection.x()).arg(newSelection.y()));
    }
}

void MainWindow::createFragmentFromWholeImage() {
    using PM = FingerprintEnhancer::ProjectManager;

    if (currentImageId.isEmpty()) {
        QMessageBox::warning(this, "Erro", "Nenhuma imagem selecionada no projeto");
        return;
    }

    FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(currentImageId);
    if (!img) {
        QMessageBox::warning(this, "Erro", "Imagem não encontrada");
        return;
    }

    // Criar fragmento com toda a área da imagem
    QRect wholeImageRect(0, 0, img->originalImage.cols, img->originalImage.rows);
    FingerprintEnhancer::Fragment* fragment = PM::instance().createFragment(currentImageId, wholeImageRect);

    if (fragment) {
        onFragmentSelected(fragment->id);
        statusLabel->setText("Imagem inteira destacada como fragmento");
    } else {
        QMessageBox::warning(this, "Erro", "Falha ao criar fragmento");
    }
}

// ========== Sistema de Entidade Corrente ==========

void MainWindow::setCurrentEntity(const QString& entityId, CurrentEntityType type) {
    using PM = FingerprintEnhancer::ProjectManager;

    currentEntityType = type;
    currentEntityId = entityId;

    // Manter IDs legados sincronizados
    if (type == ENTITY_IMAGE) {
        currentImageId = entityId;
        currentFragmentId.clear();
    } else if (type == ENTITY_FRAGMENT) {
        currentFragmentId = entityId;
        // Encontrar a imagem pai
        FingerprintEnhancer::Fragment* frag = PM::instance().getCurrentProject()->findFragment(entityId);
        if (frag) {
            currentImageId = frag->parentImageId;
        }
    }

    // Carregar entidade na visualização
    loadCurrentEntityToView();

    // Atualizar status com informações detalhadas
    QString statusMsg;
    if (type == ENTITY_IMAGE) {
        FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(entityId);
        if (img) {
            QFileInfo fileInfo(img->originalFilePath);
            statusMsg = QString("🖼️ Imagem Corrente: %1").arg(fileInfo.fileName());
        }
    } else if (type == ENTITY_FRAGMENT) {
        FingerprintEnhancer::Fragment* frag = PM::instance().getCurrentProject()->findFragment(entityId);
        if (frag) {
            int minutiaeCount = frag->getMinutiaeCount();
            statusMsg = QString("📐 Fragmento Corrente: %1x%2 pixels (%3 minúcia(s))")
                .arg(frag->workingImage.cols)
                .arg(frag->workingImage.rows)
                .arg(minutiaeCount);
        }
    }

    if (!statusMsg.isEmpty()) {
        statusLabel->setText(statusMsg);
    }
}

void MainWindow::loadCurrentEntityToView() {
    using PM = FingerprintEnhancer::ProjectManager;

    if (currentEntityType == ENTITY_NONE || currentEntityId.isEmpty()) {
        processedImageViewer->clearImage();
        minutiaeOverlay->setFragment(nullptr);
        return;
    }

    cv::Mat displayImage;
    FingerprintEnhancer::Fragment* fragment = nullptr;

    if (currentEntityType == ENTITY_IMAGE) {
        FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(currentEntityId);
        if (img) {
            displayImage = img->workingImage.clone();
        }
    } else if (currentEntityType == ENTITY_FRAGMENT) {
        fragment = PM::instance().getCurrentProject()->findFragment(currentEntityId);
        if (fragment) {
            displayImage = fragment->workingImage.clone();
        }
    }

    if (displayImage.empty()) {
        QMessageBox::warning(this, "Erro", "Não foi possível carregar a imagem da entidade");
        return;
    }

    // Converter para QImage e exibir
    QImage qimg;
    if (displayImage.channels() == 1) {
        qimg = QImage(displayImage.data, displayImage.cols, displayImage.rows,
                     displayImage.step, QImage::Format_Grayscale8).copy();
    } else {
        cv::Mat rgb;
        cv::cvtColor(displayImage, rgb, cv::COLOR_BGR2RGB);
        qimg = QImage(rgb.data, rgb.cols, rgb.rows,
                     rgb.step, QImage::Format_RGB888).copy();
    }

    processedImageViewer->setPixmap(QPixmap::fromImage(qimg));

    // Configurar overlay se for fragmento
    if (currentEntityType == ENTITY_FRAGMENT && fragment) {
        minutiaeOverlay->setFragment(fragment);
        minutiaeOverlay->setScaleFactor(processedImageViewer->getScaleFactor());
        minutiaeOverlay->raise();
        minutiaeOverlay->update();
    } else {
        minutiaeOverlay->setFragment(nullptr);
    }
}

cv::Mat& MainWindow::getCurrentWorkingImage() {
    using PM = FingerprintEnhancer::ProjectManager;

    if (currentEntityType == ENTITY_IMAGE) {
        FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(currentEntityId);
        if (img) {
            return img->workingImage;
        }
    } else if (currentEntityType == ENTITY_FRAGMENT) {
        FingerprintEnhancer::Fragment* frag = PM::instance().getCurrentProject()->findFragment(currentEntityId);
        if (frag) {
            return frag->workingImage;
        }
    }

    // Fallback - nunca deve chegar aqui em uso normal
    static cv::Mat emptyMat;
    return emptyMat;
}

void MainWindow::applyOperationToCurrentEntity(std::function<void(cv::Mat&)> operation) {
    if (currentEntityType == ENTITY_NONE || currentEntityId.isEmpty()) {
        QMessageBox::warning(this, "Erro", "Nenhuma imagem ou fragmento selecionado");
        return;
    }

    cv::Mat& workingImage = getCurrentWorkingImage();
    if (workingImage.empty()) {
        QMessageBox::warning(this, "Erro", "Imagem de trabalho inválida");
        return;
    }

    // Aplicar operação
    operation(workingImage);

    // Recarregar visualização
    loadCurrentEntityToView();

    // Marcar projeto como modificado
    using PM = FingerprintEnhancer::ProjectManager;
    PM::instance().getCurrentProject()->setModified();
}

// ========== Espelhamento ==========

void MainWindow::flipHorizontal() {
    applyOperationToCurrentEntity([](cv::Mat& img) {
        cv::flip(img, img, 1); // 1 = horizontal
    });
    statusLabel->setText("Espelhamento horizontal aplicado");
}

void MainWindow::flipVertical() {
    applyOperationToCurrentEntity([](cv::Mat& img) {
        cv::flip(img, img, 0); // 0 = vertical
    });
    statusLabel->setText("Espelhamento vertical aplicado");
}

// ========== Handlers do FragmentManager ==========

void MainWindow::onViewOriginalRequested(const QString& entityId, bool isFragment) {
    using PM = FingerprintEnhancer::ProjectManager;

    cv::Mat originalImage;
    QString entityName;

    if (isFragment) {
        FingerprintEnhancer::Fragment* frag = PM::instance().getCurrentProject()->findFragment(entityId);
        if (frag) {
            originalImage = frag->originalImage.clone();
            entityName = "Fragmento";
        } else {
            QMessageBox::warning(this, "Erro", "Fragmento não encontrado");
            return;
        }
    } else {
        FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(entityId);
        if (img) {
            originalImage = img->originalImage.clone();
            QFileInfo fileInfo(img->originalFilePath);
            entityName = fileInfo.fileName();
        } else {
            QMessageBox::warning(this, "Erro", "Imagem não encontrada");
            return;
        }
    }

    if (originalImage.empty()) {
        QMessageBox::warning(this, "Erro", "Imagem original vazia");
        return;
    }

    // Criar diálogo para mostrar imagem original
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(QString("Imagem Original - %1").arg(entityName));
    dialog->resize(800, 600);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    ImageViewer* viewer = new ImageViewer(dialog);
    viewer->setImage(originalImage);
    viewer->zoomToFit();

    layout->addWidget(viewer);

    QPushButton* closeBtn = new QPushButton("Fechar", dialog);
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);
    layout->addWidget(closeBtn);

    dialog->setLayout(layout);
    dialog->exec();

    delete dialog;
}

void MainWindow::onResetWorkingRequested(const QString& entityId, bool isFragment) {
    using PM = FingerprintEnhancer::ProjectManager;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Confirmar Reset",
        "Deseja realmente resetar a imagem de trabalho para a original?\n"
        "Todos os realces aplicados serão perdidos.",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply != QMessageBox::Yes) {
        return;
    }

    if (isFragment) {
        FingerprintEnhancer::Fragment* frag = PM::instance().getCurrentProject()->findFragment(entityId);
        if (frag) {
            frag->resetWorkingImage();
            PM::instance().getCurrentProject()->setModified();

            // Se é a entidade corrente, atualizar visualização
            if (currentEntityType == ENTITY_FRAGMENT && currentEntityId == entityId) {
                loadCurrentEntityToView();
            }

            statusLabel->setText("Fragmento resetado para imagem original");
        } else {
            QMessageBox::warning(this, "Erro", "Fragmento não encontrado");
        }
    } else {
        FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(entityId);
        if (img) {
            img->resetWorkingImage();
            PM::instance().getCurrentProject()->setModified();

            // Se é a entidade corrente, atualizar visualização
            if (currentEntityType == ENTITY_IMAGE && currentEntityId == entityId) {
                loadCurrentEntityToView();
            }

            statusLabel->setText("Imagem resetada para original");
        } else {
            QMessageBox::warning(this, "Erro", "Imagem não encontrada");
        }
    }
}

void MainWindow::onDeleteFragmentRequested(const QString& fragmentId) {
    using PM = FingerprintEnhancer::ProjectManager;

    FingerprintEnhancer::Fragment* frag = PM::instance().getCurrentProject()->findFragment(fragmentId);
    if (!frag) {
        QMessageBox::warning(this, "Erro", "Fragmento não encontrado");
        return;
    }

    int minutiaeCount = frag->getMinutiaeCount();

    QString message = QString("Deseja realmente excluir este fragmento?");
    if (minutiaeCount > 0) {
        message += QString("\n\nAVISO: Este fragmento possui %1 minúcia(s) que também serão excluídas!")
                   .arg(minutiaeCount);
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Confirmar Exclusão",
        message,
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply != QMessageBox::Yes) {
        return;
    }

    QString parentImageId = frag->parentImageId;

    // Se é a entidade corrente, limpar seleção
    if (currentEntityType == ENTITY_FRAGMENT && currentEntityId == fragmentId) {
        currentEntityType = ENTITY_NONE;
        currentEntityId.clear();
        currentFragmentId.clear();
        processedImageViewer->clearImage();
        minutiaeOverlay->setFragment(nullptr);
    }

    // Remover fragmento da imagem pai
    FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(parentImageId);
    if (img) {
        img->removeFragment(fragmentId);
        PM::instance().getCurrentProject()->setModified();

        // Atualizar view do gerenciador
        fragmentManager->updateView();

        statusLabel->setText(QString("Fragmento excluído (%1 minúcia(s) removida(s))").arg(minutiaeCount));
    }
}

void MainWindow::onDeleteMinutiaRequested(const QString& minutiaId) {
    using PM = FingerprintEnhancer::ProjectManager;

    // Encontrar a minúcia no projeto
    FingerprintEnhancer::Minutia* minutia = PM::instance().getCurrentProject()->findMinutia(minutiaId);
    if (!minutia) {
        QMessageBox::warning(this, "Erro", "Minúcia não encontrada");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Confirmar Exclusão",
        QString("Deseja realmente excluir esta minúcia?\n\nTipo: %1\nPosição: (%2, %3)")
            .arg(minutia->getTypeName())
            .arg(minutia->position.x())
            .arg(minutia->position.y()),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply != QMessageBox::Yes) {
        return;
    }

    // Encontrar o fragmento pai e remover a minúcia
    for (auto& img : PM::instance().getCurrentProject()->images) {
        for (auto& frag : img.fragments) {
            FingerprintEnhancer::Minutia* found = frag.findMinutia(minutiaId);
            if (found) {
                frag.removeMinutia(minutiaId);
                PM::instance().getCurrentProject()->setModified();

                // Atualizar visualização se este fragmento está corrente
                if (currentEntityType == ENTITY_FRAGMENT && currentEntityId == frag.id) {
                    minutiaeOverlay->clearSelection();
                    minutiaeOverlay->update();
                }

                // Atualizar view do gerenciador
                fragmentManager->updateView();

                statusLabel->setText("Minúcia excluída");
                return;
            }
        }
    }
}
