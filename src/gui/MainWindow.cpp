#include "MainWindow.h"
#include "ProcessingWorker.h"
#include "RotationDialog.h"
#include "MinutiaEditDialog.h"
#include "FFTFilterDialog.h"
#include "ColorConversionDialog.h"
#include "../core/TranslationManager_Simple.h"
#include "../core/ImageState.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QScrollBar>
#include <QtGui/QActionGroup>
#include <QtGui/QMouseEvent>
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
    , processedImageViewer(nullptr)
    , secondImageViewer(nullptr)
    , leftMinutiaeOverlay(nullptr)
    , rightMinutiaeOverlay(nullptr)
    , activePanel(false)
    , cropTool(nullptr)
    , minutiaeMarker(nullptr)
    , afisMatcher(nullptr)
    , imageState(new ImageState())
    , fragmentManager(nullptr)
    , minutiaeOverlay(nullptr)
    , currentEntityType(ENTITY_NONE)
    , leftPanelEntityType(ENTITY_NONE)
    , rightPanelEntityType(ENTITY_NONE)
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
    viewMenu->addSeparator();
    viewMenu->addAction("&Mostrar/Ocultar Painel Direito", this, &MainWindow::toggleRightPanel, QKeySequence("Ctrl+Shift+P"));
    viewMenu->addSeparator();
    viewMenu->addAction("&Configurar Visualização de Minúcias...", this, &MainWindow::configureMinutiaeDisplay, QKeySequence("Ctrl+Shift+M"));

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
    colorMenu->addAction("Converter para &Escala de Cinza", this, &MainWindow::convertToGrayscale, QKeySequence("Ctrl+Shift+G"));
    colorMenu->addAction("&Inverter Cores", this, &MainWindow::invertColors, QKeySequence("Ctrl+Shift+I"));
    colorMenu->addSeparator();
    colorMenu->addAction("Converter para &RGB", this, &MainWindow::convertToRGB);
    colorMenu->addAction("Converter para &HSV", this, &MainWindow::convertToHSV);
    colorMenu->addAction("Converter para H&SI", this, &MainWindow::convertToHSI);
    colorMenu->addAction("Converter para &Lab", this, &MainWindow::convertToLab);
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
    mainToolBar->addSeparator();

    // Adicionar botões de controle de painéis
    QAction *switchPanelAction = mainToolBar->addAction("◀▶ Chavear Painel");
    connect(switchPanelAction, &QAction::triggered, this, &MainWindow::switchActivePanel);

    QAction *toggleRightPanelAction = mainToolBar->addAction("👁 Painel Direito");
    toggleRightPanelAction->setCheckable(true);
    toggleRightPanelAction->setChecked(false);  // Inicia oculto
    connect(toggleRightPanelAction, &QAction::triggered, this, &MainWindow::toggleRightViewer);

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

QWidget* MainWindow::createViewerContainer(ImageViewer* viewer, FingerprintEnhancer::MinutiaeOverlay* overlay) {
    QWidget *container = new QWidget();
    QStackedLayout *stackedLayout = new QStackedLayout(container);
    stackedLayout->setStackingMode(QStackedLayout::StackAll);

    stackedLayout->addWidget(viewer);
    stackedLayout->addWidget(overlay);

    // Configurar overlay para ser transparente e passar eventos de mouse
    overlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    overlay->setStyleSheet("background-color: transparent;");

    return container;
}

void MainWindow::createCentralWidget() {
    centralWidget = new QWidget();
    setCentralWidget(centralWidget);

    // Inicializar controle de painel ativo
    activePanel = false;  // Começa com painel esquerdo ativo

    // Layout principal com splitter horizontal
    mainSplitter = new QSplitter(Qt::Horizontal);

    // Criar dois visualizadores de imagem
    processedImageViewer = new ImageViewer();
    secondImageViewer = new ImageViewer();
    minutiaeEditor = new MinutiaeEditor();

    // Criar overlays para cada painel
    leftMinutiaeOverlay = new FingerprintEnhancer::MinutiaeOverlay(nullptr);
    rightMinutiaeOverlay = new FingerprintEnhancer::MinutiaeOverlay(nullptr);

    // Criar containers para cada visualizador com seu overlay
    leftViewerContainer = createViewerContainer(processedImageViewer, leftMinutiaeOverlay);
    rightViewerContainer = createViewerContainer(secondImageViewer, rightMinutiaeOverlay);

    // Manter referência ao overlay antigo para compatibilidade
    minutiaeOverlay = leftMinutiaeOverlay;

    // Criar splitter para os dois painéis de visualização
    viewerSplitter = new QSplitter(Qt::Horizontal);
    viewerSplitter->addWidget(leftViewerContainer);
    viewerSplitter->addWidget(rightViewerContainer);
    viewerSplitter->setSizes({500, 500});

    // Adicionar splitter de visualização diretamente ao splitter principal
    mainSplitter->addWidget(viewerSplitter);

    // Esconder o minutiaeEditor por padrão (será usado depois)
    minutiaeEditor->hide();

    // Ocultar painel direito por padrão
    rightViewerContainer->hide();

    // Definir painel esquerdo como ativo visualmente
    leftViewerContainer->setStyleSheet("border: 2px solid #4CAF50;");

    // Layout principal
    QHBoxLayout *mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->addWidget(mainSplitter);
}

void MainWindow::createLeftPanel() {
    leftPanel = new QTabWidget();
    leftPanel->setMaximumWidth(300);

    // Tab de gerenciamento de projeto (primeira aba)
    fragmentManager = new FingerprintEnhancer::FragmentManager();
    leftPanel->addTab(fragmentManager, "Projeto");

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

    // Adicionar painel direito ao splitter principal
    mainSplitter->addWidget(rightPanel);

    // Ocultar painel direito por padrão
    rightPanel->hide();

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

    // Configurar menus de contexto nos visualizadores
    processedImageViewer->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(processedImageViewer, &QWidget::customContextMenuRequested,
            [this](const QPoint& pos) { showViewerContextMenu(pos, true); });

    secondImageViewer->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(secondImageViewer, &QWidget::customContextMenuRequested,
            [this](const QPoint& pos) { showViewerContextMenu(pos, false); });

    // Instalar filtros de eventos para detectar cliques e trocar foco
    processedImageViewer->installEventFilter(this);
    secondImageViewer->installEventFilter(this);

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
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::editMinutiaRequested,
            this, &MainWindow::onMinutiaDoubleClicked);
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::deleteMinutiaRequested,
            this, &MainWindow::onDeleteMinutiaRequested);
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::sendToLeftPanelRequested,
            [this](const QString& entityId, bool isFragment) {
                CurrentEntityType type = isFragment ? ENTITY_FRAGMENT : ENTITY_IMAGE;
                loadEntityToPanel(entityId, type, false);  // false = painel esquerdo
            });
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::sendToRightPanelRequested,
            [this](const QString& entityId, bool isFragment) {
                CurrentEntityType type = isFragment ? ENTITY_FRAGMENT : ENTITY_IMAGE;
                loadEntityToPanel(entityId, type, true);   // true = painel direito
            });

    // Conectar sinais dos MinutiaeOverlays
    connect(leftMinutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::minutiaDoubleClicked,
            this, &MainWindow::onMinutiaDoubleClicked);
    connect(leftMinutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::positionChanged,
            this, &MainWindow::onMinutiaPositionChanged);
    connect(rightMinutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::minutiaDoubleClicked,
            this, &MainWindow::onMinutiaDoubleClicked);
    connect(rightMinutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::positionChanged,
            this, &MainWindow::onMinutiaPositionChanged);

    // Manter compatibilidade com overlay antigo
    minutiaeOverlay = leftMinutiaeOverlay;

    // Conectar sinais de zoom/scroll do painel esquerdo
    connect(processedImageViewer, &ImageViewer::zoomChanged,
            leftMinutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::setScaleFactor);
    connect(processedImageViewer, &ImageViewer::scrollChanged,
            leftMinutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::setScrollOffset);
    connect(processedImageViewer, &ImageViewer::imageOffsetChanged,
            leftMinutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::setImageOffset);

    // Conectar sinais de zoom/scroll do painel direito
    connect(secondImageViewer, &ImageViewer::zoomChanged,
            rightMinutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::setScaleFactor);
    connect(secondImageViewer, &ImageViewer::scrollChanged,
            rightMinutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::setScrollOffset);
    connect(secondImageViewer, &ImageViewer::imageOffsetChanged,
            rightMinutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::setImageOffset);
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

            // Auto-selecionar a imagem recém-adicionada
            setCurrentEntity(img->id, ENTITY_IMAGE);

            statusLabel->setText("Imagem carregada e selecionada: " + QFileInfo(fileName).fileName());
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
        cv::Mat processed = imageProcessor->getCurrentImage();

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

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    // Detectar clique esquerdo nos visualizadores para trocar foco
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        // Apenas processar clique esquerdo
        if (mouseEvent->button() == Qt::LeftButton) {
            // Verificar se o clique foi em um dos visualizadores
            if (obj == processedImageViewer) {
                // Clicou no painel esquerdo
                if (activePanel) {  // Se painel direito está ativo
                    setActivePanel(false);  // Mudar para esquerdo
                }
            } else if (obj == secondImageViewer) {
                // Clicou no painel direito (só trocar se estiver visível)
                if (rightViewerContainer->isVisible() && !activePanel) {  // Se painel esquerdo está ativo e direito visível
                    setActivePanel(true);  // Mudar para direito
                }
            }
        }
    }

    // Continuar processamento normal do evento
    return QMainWindow::eventFilter(obj, event);
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
    if (isProcessing) return;
    if (currentEntityType == ENTITY_NONE || currentEntityId.isEmpty()) return;

    runProcessingInThread([value](const cv::Mat& input, int& progress) -> cv::Mat {
        progress = 50;
        cv::Mat result;
        cv::GaussianBlur(input, result, cv::Size(0, 0), value, value);
        progress = 100;
        return result;
    });
}

void MainWindow::onSharpenStrengthChanged(double value) {
    if (isProcessing) return;
    if (currentEntityType == ENTITY_NONE || currentEntityId.isEmpty()) return;

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
    using PM = FingerprintEnhancer::ProjectManager;

    if (currentEntityType == ENTITY_NONE || currentEntityId.isEmpty()) {
        QMessageBox::warning(this, "Filtro FFT", "Nenhuma imagem ou fragmento selecionado");
        return;
    }

    cv::Mat& workingImg = getCurrentWorkingImage();
    if (workingImg.empty()) {
        QMessageBox::warning(this, "Filtro FFT", "Imagem inválida");
        return;
    }

    // Abrir diálogo interativo de FFT
    FFTFilterDialog dialog(workingImg, this);
    if (dialog.exec() == QDialog::Accepted && dialog.wasAccepted()) {
        cv::Mat filtered = dialog.getFilteredImage();

        // Aplicar imagem filtrada
        filtered.copyTo(workingImg);

        // Recarregar visualização
        loadCurrentEntityToView();

        statusLabel->setText("Filtro FFT interativo aplicado");
        PM::instance().getCurrentProject()->setModified();
    } else {
        statusLabel->setText("Filtro FFT cancelado");
    }
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
void MainWindow::zoomIn() { getActiveViewer()->zoomIn(); }
void MainWindow::zoomOut() { getActiveViewer()->zoomOut(); }
void MainWindow::zoomFit() { getActiveViewer()->zoomToFit(); }
void MainWindow::zoomActual() { getActiveViewer()->zoomToActual(); }
void MainWindow::toggleSideBySide() { sideBySideMode = !sideBySideMode; }

void MainWindow::toggleRightPanel() {
    if (rightPanel->isVisible()) {
        rightPanel->hide();
    } else {
        rightPanel->show();
    }
}

void MainWindow::configureMinutiaeDisplay() {
    using namespace FingerprintEnhancer;

    MinutiaeDisplayDialog dialog(minutiaeDisplaySettings, this);
    if (dialog.exec() == QDialog::Accepted && dialog.wasAccepted()) {
        minutiaeDisplaySettings = dialog.getSettings();

        // Aplicar configurações aos overlays
        leftMinutiaeOverlay->setDisplaySettings(minutiaeDisplaySettings);
        rightMinutiaeOverlay->setDisplaySettings(minutiaeDisplaySettings);

        // Atualizar visualização
        leftMinutiaeOverlay->update();
        rightMinutiaeOverlay->update();

        statusLabel->setText("Configurações de visualização de minúcias atualizadas");
    }
}

// ========== GERENCIAMENTO DE PAINÉIS DUAIS ==========

void MainWindow::switchActivePanel() {
    activePanel = !activePanel;

    // Se estiver alternando para o painel direito e ele estiver oculto, mostrá-lo
    if (activePanel && !rightViewerContainer->isVisible()) {
        toggleRightViewer();
    }

    // Atualizar aparência visual dos containers
    if (activePanel) {
        leftViewerContainer->setStyleSheet("");
        rightViewerContainer->setStyleSheet("border: 2px solid #4CAF50;");
    } else {
        leftViewerContainer->setStyleSheet("border: 2px solid #4CAF50;");
        rightViewerContainer->setStyleSheet("");
    }

    // Atualizar entidade corrente para refletir o painel ativo
    updateActivePanelEntity();
}

void MainWindow::toggleRightViewer() {
    if (rightViewerContainer->isVisible()) {
        rightViewerContainer->hide();

        // Se estava ativo, voltar para o esquerdo
        if (activePanel) {
            activePanel = false;
            leftViewerContainer->setStyleSheet("border: 2px solid #4CAF50;");
            rightViewerContainer->setStyleSheet("");
            updateActivePanelEntity();
        }
    } else {
        rightViewerContainer->show();
    }
}

void MainWindow::setActivePanel(bool rightPanel) {
    if (activePanel != rightPanel) {
        switchActivePanel();
    }
}

ImageViewer* MainWindow::getActiveViewer() {
    return activePanel ? secondImageViewer : processedImageViewer;
}

FingerprintEnhancer::MinutiaeOverlay* MainWindow::getActiveOverlay() {
    return activePanel ? rightMinutiaeOverlay : leftMinutiaeOverlay;
}

void MainWindow::updateActivePanelEntity() {
    if (activePanel) {
        // Painel direito ativo
        currentEntityType = rightPanelEntityType;
        currentEntityId = rightPanelEntityId;
        minutiaeOverlay = rightMinutiaeOverlay;
    } else {
        // Painel esquerdo ativo
        currentEntityType = leftPanelEntityType;
        currentEntityId = leftPanelEntityId;
        minutiaeOverlay = leftMinutiaeOverlay;
    }

    // Atualizar informações na barra de status
    if (!currentEntityId.isEmpty()) {
        statusLabel->setText(QString("Painel %1 ativo: %2")
            .arg(activePanel ? "direito" : "esquerdo")
            .arg(currentEntityType == ENTITY_IMAGE ? "Imagem" : "Fragmento"));
    }
}

void MainWindow::loadEntityToPanel(const QString& entityId, CurrentEntityType type, bool targetPanel) {
    using PM = FingerprintEnhancer::ProjectManager;
    if (!PM::instance().hasOpenProject()) return;

    FingerprintEnhancer::Project* project = PM::instance().getCurrentProject();
    if (!project) return;

    ImageViewer* targetViewer = targetPanel ? secondImageViewer : processedImageViewer;
    FingerprintEnhancer::MinutiaeOverlay* targetOverlay = targetPanel ? rightMinutiaeOverlay : leftMinutiaeOverlay;

    // Atualizar informações do painel
    if (targetPanel) {
        rightPanelEntityType = type;
        rightPanelEntityId = entityId;
    } else {
        leftPanelEntityType = type;
        leftPanelEntityId = entityId;
    }

    // Carregar imagem apropriada
    cv::Mat imageToShow;
    FingerprintEnhancer::Fragment* fragment = nullptr;

    if (type == ENTITY_IMAGE) {
        FingerprintEnhancer::FingerprintImage* img = project->findImage(entityId);
        if (img) {
            imageToShow = img->workingImage.clone();
        }
    } else if (type == ENTITY_FRAGMENT) {
        fragment = project->findFragment(entityId);
        if (fragment) {
            imageToShow = fragment->workingImage.clone();
        }
    }

    if (!imageToShow.empty()) {
        // Converter para QPixmap e exibir
        QImage qimg;
        if (imageToShow.channels() == 1) {
            qimg = QImage(imageToShow.data, imageToShow.cols, imageToShow.rows,
                         imageToShow.step, QImage::Format_Grayscale8).copy();
        } else {
            cv::Mat rgb;
            cv::cvtColor(imageToShow, rgb, cv::COLOR_BGR2RGB);
            qimg = QImage(rgb.data, rgb.cols, rgb.rows,
                         rgb.step, QImage::Format_RGB888).copy();
        }

        targetViewer->setPixmap(QPixmap::fromImage(qimg));

        // Configurar overlay se for fragmento
        if (type == ENTITY_FRAGMENT && fragment) {
            targetOverlay->setFragment(fragment);
            targetOverlay->setScaleFactor(targetViewer->getScaleFactor());

            // Inicializar scroll offset
            QPoint scrollOffset(targetViewer->horizontalScrollBar()->value(),
                               targetViewer->verticalScrollBar()->value());
            targetOverlay->setScrollOffset(scrollOffset);

            // Inicializar image offset (centralização)
            targetOverlay->setImageOffset(targetViewer->getImageOffset());
        } else {
            targetOverlay->setFragment(nullptr);
        }

        targetOverlay->update();
    }

    // Se enviou para painel ativo, atualizar entidade corrente
    if (targetPanel == activePanel) {
        updateActivePanelEntity();
    }
}

void MainWindow::clearActivePanel() {
    ImageViewer* viewer = getActiveViewer();
    FingerprintEnhancer::MinutiaeOverlay* overlay = getActiveOverlay();

    viewer->clearImage();
    overlay->setFragment(nullptr);
    overlay->update();

    // Limpar informações da entidade do painel ativo
    if (activePanel) {
        rightPanelEntityType = ENTITY_NONE;
        rightPanelEntityId.clear();
    } else {
        leftPanelEntityType = ENTITY_NONE;
        leftPanelEntityId.clear();
    }

    updateActivePanelEntity();
    statusLabel->setText(QString("Painel %1 limpo").arg(activePanel ? "direito" : "esquerdo"));
}

void MainWindow::showViewerContextMenu(const QPoint& pos, bool isLeftPanel) {
    QMenu menu(this);

    ImageViewer* viewer = isLeftPanel ? processedImageViewer : secondImageViewer;
    bool isPanelActive = (isLeftPanel && !activePanel) || (!isLeftPanel && activePanel);

    // Se este painel é o ativo, mostrar menu de operações de imagem
    if (isPanelActive) {
        QPoint imagePos = viewer->widgetToImage(pos);

        // Menu baseado no modo de ferramenta ativa
        switch (currentToolMode) {
            case TOOL_NONE:
                // Menu genérico baseado na entidade corrente
                if (currentEntityType == ENTITY_IMAGE) {
                    menu.addAction("📐 Destacar Imagem Inteira como Fragmento",
                                         this, &MainWindow::createFragmentFromWholeImage);
                } else if (currentEntityType == ENTITY_FRAGMENT) {
                    menu.addAction("⚡ Inserção Rápida (sem classificar)", [this, imagePos]() {
                        addMinutiaQuickly(imagePos);
                    });
                    menu.addAction("➕ Adicionar Minúcia Aqui (com diálogo)", [this, imagePos]() {
                        setToolMode(TOOL_ADD_MINUTIA);
                        addMinutiaAtPosition(imagePos);
                    });
                }
                break;

            case TOOL_CROP:
                if (viewer->hasCropSelection()) {
                    menu.addAction("✓ Aplicar Recorte", this, &MainWindow::applyCrop);
                    menu.addAction("Ajustar Seleção", this, &MainWindow::onCropAdjust);
                    menu.addAction("Mover Seleção", this, &MainWindow::onCropMove);
                    menu.addSeparator();
                    menu.addAction("✗ Cancelar Recorte", this, &MainWindow::cancelCropSelection);
                } else {
                    menu.addAction("Desenhe uma área para recortar");
                }
                break;

            case TOOL_ADD_MINUTIA:
                if (currentEntityType == ENTITY_FRAGMENT) {
                    menu.addAction("⚡ Inserção Rápida (sem classificar)", [this, imagePos]() {
                        addMinutiaQuickly(imagePos);
                    });
                    menu.addAction("➕ Adicionar Minúcia Aqui (com diálogo)", [this, imagePos]() {
                        addMinutiaAtPosition(imagePos);
                    });
                }
                break;

            case TOOL_EDIT_MINUTIA:
                menu.addAction("Clique em uma minúcia para editar");
                menu.addSeparator();
                menu.addAction("Adicionar Nova Minúcia", [this, imagePos]() {
                    if (currentEntityType == ENTITY_FRAGMENT) {
                        addMinutiaAtPosition(imagePos);
                    }
                });
                break;

            case TOOL_REMOVE_MINUTIA:
                menu.addAction("Clique em uma minúcia para remover");
                break;

            case TOOL_PAN:
                menu.addAction("Modo Pan ativo");
                break;
        }

        if (menu.actions().count() > 0) {
            menu.addSeparator();
        }
    }

    // Opções de controle de painéis (sempre disponíveis)

    // Opção para tornar este painel ativo (se não for o ativo)
    if (!isPanelActive) {
        menu.addAction("🎯 Tornar Este Painel Ativo", [this, isLeftPanel]() {
            setActivePanel(!isLeftPanel);  // false = esquerdo, true = direito
        });
    }

    // Opção para limpar painel (apenas se for o ativo)
    if (isPanelActive) {
        menu.addAction("🗑 Limpar Este Painel", this, &MainWindow::clearActivePanel);
    }

    // Opção para ocultar painel direito (apenas no painel direito quando visível)
    if (!isLeftPanel && rightViewerContainer->isVisible()) {
        menu.addAction("👁 Ocultar Painel Direito", this, &MainWindow::toggleRightViewer);
    }

    // Mostrar painel direito (apenas no painel esquerdo quando direito está oculto)
    if (isLeftPanel && !rightViewerContainer->isVisible()) {
        menu.addAction("👁 Mostrar Painel Direito", this, &MainWindow::toggleRightViewer);
    }

    if (!menu.isEmpty()) {
        menu.exec(viewer->mapToGlobal(pos));
    }
}

// ========== FIM GERENCIAMENTO DE PAINÉIS DUAIS ==========

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
    if (isProcessing) return;

    // Verificar se há uma entidade selecionada
    if (currentEntityType == ENTITY_NONE || currentEntityId.isEmpty()) {
        return;
    }

    double brightness = brightnessSlider->value();
    double contrast = contrastSlider->value() / 100.0;

    // Obter a imagem de trabalho da entidade atual (imagem ou fragmento)
    cv::Mat& workingImage = getCurrentWorkingImage();
    if (workingImage.empty()) return;

    // Aplicar temporariamente sem modificar o workingImage
    cv::Mat result;
    workingImage.convertTo(result, -1, contrast, brightness);

    processedImageViewer->setImage(result);
}

void MainWindow::runProcessingInThread(std::function<cv::Mat(const cv::Mat&, int&)> processingFunc) {
    if (isProcessing) {
        QMessageBox::warning(this, "Processing",
            "Another processing operation is already running. Please wait.");
        return;
    }

    // Verificar se há uma entidade selecionada
    if (currentEntityType == ENTITY_NONE || currentEntityId.isEmpty()) {
        QMessageBox::warning(this, "No Image", "Please select an image or fragment first.");
        return;
    }

    cv::Mat& workingImage = getCurrentWorkingImage();
    if (workingImage.empty()) {
        QMessageBox::warning(this, "No Image", "Working image is empty.");
        return;
    }

    isProcessing = true;
    showProcessingProgress("Processing");

    // Criar thread e worker
    processingThread = new QThread();
    processingWorker = new ProcessingWorker();
    processingWorker->moveToThread(processingThread);

    // Configurar worker - usar a imagem de trabalho da entidade atual
    processingWorker->setCustomOperation(processingFunc);
    processingWorker->setInputImage(workingImage);

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
        // Copiar resultado para a workingImage da entidade atual
        cv::Mat& workingImage = getCurrentWorkingImage();
        result.copyTo(workingImage);

        // Recarregar visualização
        loadCurrentEntityToView();

        // Marcar projeto como modificado
        using PM = FingerprintEnhancer::ProjectManager;
        PM::instance().getCurrentProject()->setModified();

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
    // Ativar modo de recorte no visualizador ativo
    ImageViewer* activeViewer = getActiveViewer();
    activeViewer->setCropMode(true);

    statusLabel->setText("Ferramenta de recorte ativada. Clique e arraste para selecionar uma área.");
}

void MainWindow::applyCrop() {
    using PM = FingerprintEnhancer::ProjectManager;

    ImageViewer* activeViewer = getActiveViewer();

    if (!activeViewer->hasCropSelection()) {
        QMessageBox::warning(this, "Recorte", "Nenhuma área selecionada.");
        return;
    }

    // Precisa ter uma entidade corrente (Image ou Fragment)
    if (currentEntityType == ENTITY_NONE || currentEntityId.isEmpty()) {
        QMessageBox::warning(this, "Recorte", "Nenhuma imagem ou fragmento selecionado.");
        return;
    }

    // Obter retângulo de seleção
    QRect selection = activeViewer->getCropSelection();

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
    activeViewer->clearCropSelection();
    activeViewer->setCropMode(false);
    setToolMode(TOOL_NONE);

    // Atualizar visualização no gerenciador ANTES de selecionar
    fragmentManager->updateView();

    // Selecionar o fragmento criado automaticamente
    setCurrentEntity(img->fragments.last().id, ENTITY_FRAGMENT);

    statusLabel->setText(QString("Fragmento criado: %1x%2 px")
                        .arg(selection.width()).arg(selection.height()));
}

void MainWindow::cancelCropSelection() {
    ImageViewer* activeViewer = getActiveViewer();
    activeViewer->clearCropSelection();
    activeViewer->setCropMode(false);
    statusLabel->setText("Seleção cancelada.");
}

void MainWindow::saveCroppedImage() {
    ImageViewer* activeViewer = getActiveViewer();

    if (!activeViewer->hasCropSelection()) {
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
    QRect selection = activeViewer->getCropSelection();
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
    using PM = FingerprintEnhancer::ProjectManager;

    if (currentEntityType == ENTITY_FRAGMENT && !currentEntityId.isEmpty()) {
        FingerprintEnhancer::Fragment* frag = PM::instance().getCurrentProject()->findFragment(currentEntityId);
        if (frag) {
            cv::Size oldSize = frag->workingImage.size();
            cv::rotate(frag->workingImage, frag->workingImage, cv::ROTATE_90_CLOCKWISE);
            cv::Size newSize = frag->workingImage.size();
            frag->rotateMinutiae(-90.0, oldSize, newSize); // -90 porque OpenCV rotaciona no sentido horário
            loadCurrentEntityToView();
        }
    } else {
        applyOperationToCurrentEntity([](cv::Mat& img) {
            cv::rotate(img, img, cv::ROTATE_90_CLOCKWISE);
        });
    }
    statusLabel->setText("Imagem rotacionada 90° à direita");
}

void MainWindow::rotateLeft90() {
    using PM = FingerprintEnhancer::ProjectManager;

    if (currentEntityType == ENTITY_FRAGMENT && !currentEntityId.isEmpty()) {
        FingerprintEnhancer::Fragment* frag = PM::instance().getCurrentProject()->findFragment(currentEntityId);
        if (frag) {
            cv::Size oldSize = frag->workingImage.size();
            cv::rotate(frag->workingImage, frag->workingImage, cv::ROTATE_90_COUNTERCLOCKWISE);
            cv::Size newSize = frag->workingImage.size();
            frag->rotateMinutiae(90.0, oldSize, newSize);
            loadCurrentEntityToView();
        }
    } else {
        applyOperationToCurrentEntity([](cv::Mat& img) {
            cv::rotate(img, img, cv::ROTATE_90_COUNTERCLOCKWISE);
        });
    }
    statusLabel->setText("Imagem rotacionada 90° à esquerda");
}

void MainWindow::rotate180() {
    using PM = FingerprintEnhancer::ProjectManager;

    if (currentEntityType == ENTITY_FRAGMENT && !currentEntityId.isEmpty()) {
        FingerprintEnhancer::Fragment* frag = PM::instance().getCurrentProject()->findFragment(currentEntityId);
        if (frag) {
            cv::Size oldSize = frag->workingImage.size();
            cv::rotate(frag->workingImage, frag->workingImage, cv::ROTATE_180);
            cv::Size newSize = frag->workingImage.size();
            frag->rotateMinutiae(180.0, oldSize, newSize);
            loadCurrentEntityToView();
        }
    } else {
        applyOperationToCurrentEntity([](cv::Mat& img) {
            cv::rotate(img, img, cv::ROTATE_180);
        });
    }
    statusLabel->setText("Imagem rotacionada 180°");
}

void MainWindow::rotateCustomAngle() {
    using PM = FingerprintEnhancer::ProjectManager;

    if (currentEntityType == ENTITY_NONE || currentEntityId.isEmpty()) {
        QMessageBox::warning(this, "Rotação", "Nenhuma imagem ou fragmento selecionado");
        return;
    }

    cv::Mat& workingImg = getCurrentWorkingImage();
    if (workingImg.empty()) {
        QMessageBox::warning(this, "Rotação", "Imagem inválida");
        return;
    }

    // Usar visualizador do painel ativo
    ImageViewer* activeViewer = getActiveViewer();

    // Criar diálogo de rotação em tempo real
    RotationDialog dialog(workingImg, activeViewer, this);
    int result = dialog.exec();

    if (result == QDialog::Accepted && dialog.wasAccepted()) {
        double angle = dialog.getRotationAngle();
        cv::Mat rotated = dialog.getRotatedImage();

        // Se for fragmento, atualizar minúcias
        if (currentEntityType == ENTITY_FRAGMENT) {
            FingerprintEnhancer::Fragment* frag = PM::instance().getCurrentProject()->findFragment(currentEntityId);
            if (frag) {
                cv::Size oldSize = workingImg.size();
                cv::Size newSize = rotated.size();
                rotated.copyTo(frag->workingImage);
                frag->rotateMinutiae(angle, oldSize, newSize);
            }
        } else {
            // Aplicar diretamente na workingImage da entidade corrente
            rotated.copyTo(workingImg);
        }

        // Recarregar visualização
        loadCurrentEntityToView();

        statusLabel->setText(QString("Imagem rotacionada %1°").arg(angle, 0, 'f', 1));

        // Marcar como modificado
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
    using PM = FingerprintEnhancer::ProjectManager;

    if (currentEntityType == ENTITY_NONE || currentEntityId.isEmpty()) {
        QMessageBox::warning(this, "Conversão RGB", "Nenhuma imagem ou fragmento selecionado");
        return;
    }

    cv::Mat& workingImg = getCurrentWorkingImage();
    if (workingImg.empty()) {
        QMessageBox::warning(this, "Conversão RGB", "Imagem inválida");
        return;
    }

    ColorConversionDialog dialog(workingImg, COLOR_SPACE_RGB, this);
    if (dialog.exec() == QDialog::Accepted && dialog.wasAccepted()) {
        cv::Mat converted = dialog.getConvertedImage();
        converted.copyTo(workingImg);
        loadCurrentEntityToView();
        statusLabel->setText("Convertido para RGB com ajustes aplicados");
        PM::instance().getCurrentProject()->setModified();
    } else {
        statusLabel->setText("Conversão RGB cancelada");
    }
}

void MainWindow::convertToHSV() {
    using PM = FingerprintEnhancer::ProjectManager;

    if (currentEntityType == ENTITY_NONE || currentEntityId.isEmpty()) {
        QMessageBox::warning(this, "Conversão HSV", "Nenhuma imagem ou fragmento selecionado");
        return;
    }

    cv::Mat& workingImg = getCurrentWorkingImage();
    if (workingImg.empty()) {
        QMessageBox::warning(this, "Conversão HSV", "Imagem inválida");
        return;
    }

    ColorConversionDialog dialog(workingImg, COLOR_SPACE_HSV, this);
    if (dialog.exec() == QDialog::Accepted && dialog.wasAccepted()) {
        cv::Mat converted = dialog.getConvertedImage();
        converted.copyTo(workingImg);
        loadCurrentEntityToView();
        statusLabel->setText("Convertido para HSV com ajustes aplicados");
        PM::instance().getCurrentProject()->setModified();
    } else {
        statusLabel->setText("Conversão HSV cancelada");
    }
}

void MainWindow::convertToHSI() {
    using PM = FingerprintEnhancer::ProjectManager;

    if (currentEntityType == ENTITY_NONE || currentEntityId.isEmpty()) {
        QMessageBox::warning(this, "Conversão HSI", "Nenhuma imagem ou fragmento selecionado");
        return;
    }

    cv::Mat& workingImg = getCurrentWorkingImage();
    if (workingImg.empty()) {
        QMessageBox::warning(this, "Conversão HSI", "Imagem inválida");
        return;
    }

    ColorConversionDialog dialog(workingImg, COLOR_SPACE_HSI, this);
    if (dialog.exec() == QDialog::Accepted && dialog.wasAccepted()) {
        cv::Mat converted = dialog.getConvertedImage();
        converted.copyTo(workingImg);
        loadCurrentEntityToView();
        statusLabel->setText("Convertido para HSI com ajustes aplicados");
        PM::instance().getCurrentProject()->setModified();
    } else {
        statusLabel->setText("Conversão HSI cancelada");
    }
}

void MainWindow::convertToLab() {
    using PM = FingerprintEnhancer::ProjectManager;

    if (currentEntityType == ENTITY_NONE || currentEntityId.isEmpty()) {
        QMessageBox::warning(this, "Conversão Lab", "Nenhuma imagem ou fragmento selecionado");
        return;
    }

    cv::Mat& workingImg = getCurrentWorkingImage();
    if (workingImg.empty()) {
        QMessageBox::warning(this, "Conversão Lab", "Imagem inválida");
        return;
    }

    ColorConversionDialog dialog(workingImg, COLOR_SPACE_LAB, this);
    if (dialog.exec() == QDialog::Accepted && dialog.wasAccepted()) {
        cv::Mat converted = dialog.getConvertedImage();
        converted.copyTo(workingImg);
        loadCurrentEntityToView();
        statusLabel->setText("Convertido para Lab com ajustes aplicados");
        PM::instance().getCurrentProject()->setModified();
    } else {
        statusLabel->setText("Conversão Lab cancelada");
    }
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

    // DEBUG: Log do estado atual
    qDebug() << "showContextMenu called:";
    qDebug() << "  currentToolMode:" << currentToolMode;
    qDebug() << "  currentEntityType:" << currentEntityType;
    qDebug() << "  currentEntityId:" << currentEntityId;

    // Menu baseado no modo de ferramenta ativa
    switch (currentToolMode) {
        case TOOL_NONE:
            // Menu genérico baseado na entidade corrente
            if (currentEntityType == ENTITY_IMAGE) {
                contextMenu.addAction("📐 Destacar Imagem Inteira como Fragmento",
                                     this, &MainWindow::createFragmentFromWholeImage);
            } else if (currentEntityType == ENTITY_FRAGMENT) {
                contextMenu.addAction("⚡ Inserção Rápida (sem classificar)", [this, imagePos]() {
                    addMinutiaQuickly(imagePos);
                });
                contextMenu.addAction("➕ Adicionar Minúcia Aqui (com diálogo)", [this, imagePos]() {
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
                contextMenu.addAction("⚡ Inserção Rápida (sem classificar)", [this, imagePos]() {
                    addMinutiaQuickly(imagePos);
                });
                contextMenu.addAction("➕ Adicionar Minúcia Aqui (com diálogo)", [this, imagePos]() {
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

    // Obter dimensões do fragmento para validação
    cv::Mat& workingImg = getCurrentWorkingImage();
    if (workingImg.empty()) {
        QMessageBox::warning(this, "Erro", "Imagem inválida");
        return;
    }

    // Validar posição dentro dos limites da imagem
    if (imagePos.x() < 0 || imagePos.x() >= workingImg.cols ||
        imagePos.y() < 0 || imagePos.y() >= workingImg.rows) {
        QMessageBox::warning(this, "Posição Inválida",
            QString("A posição (%1, %2) está fora dos limites da imagem.\n"
                    "Limites: 0 a %3 (largura), 0 a %4 (altura)")
                .arg(imagePos.x()).arg(imagePos.y())
                .arg(workingImg.cols - 1).arg(workingImg.rows - 1));
        return;
    }

    // Criar minúcia temporária para edição - tipo padrão: OTHER (Não Classificada)
    FingerprintEnhancer::Minutia tempMinutia(imagePos, MinutiaeType::OTHER);

    // Abrir diálogo para escolher tipo e configurar
    FingerprintEnhancer::MinutiaEditDialog dialog(&tempMinutia, this);

    if (dialog.exec() == QDialog::Accepted) {
        // Obter valores do diálogo
        QPoint finalPos = dialog.getPosition();
        MinutiaeType type = dialog.getType();
        float angle = dialog.getAngle();
        float quality = dialog.getQuality();
        QString notes = dialog.getNotes();

        // Adicionar minúcia ao fragmento no projeto (método só aceita 3 parâmetros)
        FingerprintEnhancer::Minutia* minutia = PM::instance().addMinutiaToFragment(
            currentEntityId,  // Usar currentEntityId em vez de currentFragmentId
            finalPos,
            type
        );

        if (minutia) {
            // Atualizar ângulo e qualidade manualmente
            minutia->angle = angle;
            minutia->quality = quality;

            if (!notes.isEmpty()) {
                minutia->notes = notes;
            }

            statusLabel->setText(QString("Minúcia %1 adicionada em (%2, %3)")
                                .arg(MinutiaeTypeInfo::getTypeName(type))
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
}

void MainWindow::addMinutiaQuickly(const QPoint &imagePos) {
    using PM = FingerprintEnhancer::ProjectManager;

    // Verificar se há um fragmento selecionado
    if (currentEntityType != ENTITY_FRAGMENT || currentEntityId.isEmpty()) {
        QMessageBox::warning(this, "Erro", "Nenhum fragmento selecionado");
        return;
    }

    // Validar posição
    cv::Mat& workingImg = getCurrentWorkingImage();
    if (workingImg.empty()) return;

    if (imagePos.x() < 0 || imagePos.x() >= workingImg.cols ||
        imagePos.y() < 0 || imagePos.y() >= workingImg.rows) {
        QMessageBox::warning(this, "Posição Inválida",
            "A posição está fora dos limites da imagem");
        return;
    }

    // Adicionar minúcia diretamente como OTHER (Não Classificada)
    FingerprintEnhancer::Minutia* minutia = PM::instance().addMinutiaToFragment(
        currentEntityId,
        imagePos,
        MinutiaeType::OTHER
    );

    if (minutia) {
        statusLabel->setText(QString("Minúcia não classificada #%1 adicionada em (%2, %3) - Edite depois")
                            .arg(minutia->id.right(8))
                            .arg(imagePos.x()).arg(imagePos.y()));

        // Atualizar overlay
        FingerprintEnhancer::Fragment* frag = PM::instance().getCurrentProject()->findFragment(currentEntityId);
        if (frag) {
            minutiaeOverlay->setFragment(frag);
            minutiaeOverlay->update();
        }

        // Atualizar view do gerenciador
        fragmentManager->updateView();
    }
}

// ==================== FERRAMENTAS DE MINÚCIAS ====================

void MainWindow::activateAddMinutia() {
    // Verificar se há um fragmento selecionado
    if (currentEntityType != ENTITY_FRAGMENT) {
        QMessageBox::information(this, "Adicionar Minúcia",
            "Para adicionar minúcias, você precisa primeiro:\n\n"
            "1. Criar um fragmento (recorte de uma imagem)\n"
            "2. Selecionar o fragmento no painel Projeto (clique nele)\n"
            "3. Depois use:\n"
            "   • Menu Ferramentas → Minúcias → Adicionar Minúcia Manual\n"
            "   • OU clique com botão direito na imagem → 'Adicionar Minúcia Aqui'");
        return;
    }

    // Ativar modo de adição de minúcias
    setToolMode(TOOL_ADD_MINUTIA);
    statusLabel->setText("✚ Modo: Adicionar Minúcia ATIVO. Clique com BOTÃO DIREITO na posição desejada e escolha 'Adicionar Minúcia Aqui'.");
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

    // Desativar todos os modos em ambos os viewers
    processedImageViewer->setCropMode(false);
    secondImageViewer->setCropMode(false);

    // Desativar edit mode em ambos os overlays
    leftMinutiaeOverlay->setEditMode(false);
    rightMinutiaeOverlay->setEditMode(false);

    // Obter viewer e overlay ativos
    ImageViewer* activeViewer = getActiveViewer();
    FingerprintEnhancer::MinutiaeOverlay* activeOverlay = getActiveOverlay();

    // Ativar o modo selecionado apenas no painel ativo
    switch (mode) {
        case TOOL_NONE:
            statusLabel->setText("Ferramenta: Nenhuma (navegação)");
            break;

        case TOOL_CROP:
            activeViewer->setCropMode(true);
            statusLabel->setText("Ferramenta: Recortar - Clique e arraste para selecionar área");
            break;

        case TOOL_ADD_MINUTIA:
            activeOverlay->setEditMode(false);
            statusLabel->setText("Ferramenta: Adicionar Minúcia - Clique para marcar posição");
            break;

        case TOOL_EDIT_MINUTIA:
            activeOverlay->setEditMode(true);
            statusLabel->setText("Ferramenta: Editar Minúcia - Clique para selecionar, arraste para mover");
            break;

        case TOOL_REMOVE_MINUTIA:
            activeOverlay->setEditMode(false);
            statusLabel->setText("Ferramenta: Remover Minúcia - Clique na minúcia para remover");
            break;

        case TOOL_PAN:
            statusLabel->setText("Ferramenta: Pan - Arraste para mover a imagem");
            break;
    }
}

void MainWindow::onCropAdjust() {
    ImageViewer* activeViewer = getActiveViewer();

    if (!activeViewer->hasCropSelection()) {
        QMessageBox::information(this, "Info", "Nenhuma seleção de recorte ativa");
        return;
    }

    QRect currentSelection = activeViewer->getCropSelection();

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
        activeViewer->setCropSelection(newSelection);
        statusLabel->setText(QString("Seleção ajustada: %1x%2 em (%3,%4)")
            .arg(newSelection.width()).arg(newSelection.height())
            .arg(newSelection.x()).arg(newSelection.y()));
    }
}

void MainWindow::onCropMove() {
    ImageViewer* activeViewer = getActiveViewer();

    if (!activeViewer->hasCropSelection()) {
        QMessageBox::information(this, "Info", "Nenhuma seleção de recorte ativa");
        return;
    }

    QRect currentSelection = activeViewer->getCropSelection();

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
        activeViewer->setCropSelection(newSelection);
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

    // Atualizar informações do painel ativo
    if (activePanel) {
        rightPanelEntityType = type;
        rightPanelEntityId = entityId;
    } else {
        leftPanelEntityType = type;
        leftPanelEntityId = entityId;
    }

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

    // Usar visualizador e overlay do painel ativo
    ImageViewer* activeViewer = getActiveViewer();
    FingerprintEnhancer::MinutiaeOverlay* activeOverlay = getActiveOverlay();

    if (currentEntityType == ENTITY_NONE || currentEntityId.isEmpty()) {
        activeViewer->clearImage();
        activeOverlay->setFragment(nullptr);
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

    activeViewer->setPixmap(QPixmap::fromImage(qimg));

    // Configurar overlay se for fragmento
    if (currentEntityType == ENTITY_FRAGMENT && fragment) {
        activeOverlay->setFragment(fragment);
        activeOverlay->setScaleFactor(activeViewer->getScaleFactor());

        // Inicializar scroll offset
        QPoint scrollOffset(activeViewer->horizontalScrollBar()->value(),
                           activeViewer->verticalScrollBar()->value());
        activeOverlay->setScrollOffset(scrollOffset);

        // Inicializar image offset (centralização)
        activeOverlay->setImageOffset(activeViewer->getImageOffset());

        activeOverlay->raise();
        activeOverlay->update();
    } else {
        activeOverlay->setFragment(nullptr);
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
    using PM = FingerprintEnhancer::ProjectManager;

    if (currentEntityType == ENTITY_FRAGMENT && !currentEntityId.isEmpty()) {
        FingerprintEnhancer::Fragment* frag = PM::instance().getCurrentProject()->findFragment(currentEntityId);
        if (frag) {
            int imageWidth = frag->workingImage.cols;
            cv::flip(frag->workingImage, frag->workingImage, 1); // 1 = horizontal
            frag->flipMinutiaeHorizontal(imageWidth);
            loadCurrentEntityToView();
        }
    } else {
        applyOperationToCurrentEntity([](cv::Mat& img) {
            cv::flip(img, img, 1); // 1 = horizontal
        });
    }
    statusLabel->setText("Espelhamento horizontal aplicado");
}

void MainWindow::flipVertical() {
    using PM = FingerprintEnhancer::ProjectManager;

    if (currentEntityType == ENTITY_FRAGMENT && !currentEntityId.isEmpty()) {
        FingerprintEnhancer::Fragment* frag = PM::instance().getCurrentProject()->findFragment(currentEntityId);
        if (frag) {
            int imageHeight = frag->workingImage.rows;
            cv::flip(frag->workingImage, frag->workingImage, 0); // 0 = vertical
            frag->flipMinutiaeVertical(imageHeight);
            loadCurrentEntityToView();
        }
    } else {
        applyOperationToCurrentEntity([](cv::Mat& img) {
            cv::flip(img, img, 0); // 0 = vertical
        });
    }
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
