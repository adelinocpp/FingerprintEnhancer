#include "MainWindow.h"
#include "core/ProjectManager.h"

// Definir categoria de logging
Q_LOGGING_CATEGORY(mainwindow, "mainwindow")

#include "ProcessingWorker.h"
#include "RotationDialog.h"
#include "MinutiaEditDialog.h"
#include "FFTFilterDialog.h"
#include "ColorConversionDialog.h"
#include "ScaleConfigDialog.h"
#include "FragmentExportDialog.h"
#include "ImagePropertiesDialog.h"
#include "FragmentPropertiesDialog.h"
#include "FragmentRegionsOverlay.h"
#include "FragmentComparisonDialog.h"
#include "../core/TranslationManager_Simple.h"
#include "../core/ImageState.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QDialog>
#include <QtWidgets/QMenu>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QScrollBar>
#include <QtGui/QActionGroup>
#include <QtGui/QMouseEvent>
#include <QtCore/QStandardPaths>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QUuid>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
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
    , scaleCalibrationTool(nullptr)
    , leftTopRuler(nullptr)
    , leftLeftRuler(nullptr)
    , rightTopRuler(nullptr)
    , rightLeftRuler(nullptr)
    , rulersVisible(false)
    , imageState(new ImageState())
    , fragmentManager(nullptr)
    , minutiaeOverlay(nullptr)
    , currentEntityType(ENTITY_NONE)
    , leftPanelEntityType(ENTITY_NONE)
    , rightPanelEntityType(ENTITY_NONE)
    , currentToolMode(MODE_NONE)
    , sideBySideMode(true)
    , updateTimer(new QTimer(this))
    , currentProgramState(STATE_NONE)
    , processingThread(nullptr)
    , processingWorker(nullptr)
    , isProcessing(false)
    , imageLoaderWorker(nullptr)
    , isLoadingImages(false)
    , projectSaverWorker(nullptr)
    , isSavingProject(false)
    , saveAction(nullptr)
    , saveAsAction(nullptr)
{
    qCDebug(mainwindow) << "========================================";
    qCDebug(mainwindow) << "🏗️ MainWindow::MainWindow() - Construtor iniciado";
    qCDebug(mainwindow) << "========================================";
    
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
    saveAction = fileMenu->addAction("&Salvar Projeto", this, &MainWindow::saveProject, QKeySequence::Save);
    saveAsAction = fileMenu->addAction("Salvar Projeto &Como...", this, &MainWindow::saveProjectAs, QKeySequence::SaveAs);
    fileMenu->addSeparator();
    fileMenu->addAction("&Editar Informações do Projeto...", this, &MainWindow::editProjectInfo, QKeySequence("Ctrl+Shift+E"));
    fileMenu->addAction("&Limpar Projeto", this, &MainWindow::clearProject, QKeySequence("Ctrl+Shift+N"));
    fileMenu->addSeparator();
    fileMenu->addAction("Abrir &Imagem...", this, &MainWindow::openImage, QKeySequence("Ctrl+O"));
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
    scaleMenu->addAction("📏 &Calibrar Escala Interativa...", this, &MainWindow::calibrateScale, QKeySequence("Ctrl+K"));
    scaleMenu->addAction("⚙️ &Configurar Parâmetros de Escala...", this, &MainWindow::showScaleConfig);
    scaleMenu->addAction("&Definir Escala Manualmente...", this, &MainWindow::setScaleManually);
    scaleMenu->addAction("&Informações de Escala", this, &MainWindow::showScaleInfo);
    scaleMenu->addSeparator();
    QAction *toggleRulersAction = scaleMenu->addAction("Mostrar/Ocultar &Réguas", this, &MainWindow::toggleRulers, QKeySequence("Ctrl+Shift+R"));
    toggleRulersAction->setCheckable(true);
    toggleRulersAction->setChecked(false);

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
    
    // Modo de edição interativa - UNIFICADO com sistema de estados
    editModeAction = minutiaeMenu->addAction("🎯 Modo de Edição &Interativa", this, [this](bool checked) {
        qDebug() << "🎯 Menu: Modo de Edição Interativa -" << (checked ? "ATIVADO" : "DESATIVADO");
        toggleInteractiveEditMode(checked);
    });
    editModeAction->setCheckable(true);
    editModeAction->setChecked(false);
    editModeAction->setShortcut(QKeySequence("Ctrl+I"));
    editModeAction->setToolTip("Ativar modo de edição interativa: selecione minúcia primeiro, depois arraste para mover/rotacionar");
    
    minutiaeMenu->addSeparator();
    minutiaeMenu->addAction("&Editar Minúcia", this, &MainWindow::activateEditMinutia);
    minutiaeMenu->addAction("&Remover Minúcia", this, &MainWindow::activateRemoveMinutia);
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
    switchPanelAction = mainToolBar->addAction("◀▶ Chavear Painel");
    connect(switchPanelAction, &QAction::triggered, this, &MainWindow::switchActivePanel);

    toggleRightPanelAction = mainToolBar->addAction("👁 Painel Direito");
    toggleRightPanelAction->setCheckable(true);
    toggleRightPanelAction->setChecked(false);  // Inicia oculto
    connect(toggleRightPanelAction, &QAction::triggered, this, &MainWindow::toggleRightViewer);

    // Toolbar de ferramentas
    QToolBar *toolsToolBar = addToolBar("Ferramentas");

    noneToolAction = toolsToolBar->addAction("🖱️ Selecionar");
    noneToolAction->setCheckable(true);
    noneToolAction->setChecked(true);
    connect(noneToolAction, &QAction::triggered, [this]() { setToolMode(MODE_NONE); });

    cropToolAction = toolsToolBar->addAction("✂️ Recortar");
    cropToolAction->setCheckable(true);
    connect(cropToolAction, &QAction::triggered, [this]() { setToolMode(MODE_CROP); });

    addMinutiaAction = toolsToolBar->addAction("➕ Adicionar Minúcia");
    addMinutiaAction->setCheckable(true);
    connect(addMinutiaAction, &QAction::triggered, [this]() { setToolMode(MODE_ADD_MINUTIA); });

    editMinutiaToolbarAction = toolsToolBar->addAction("✏️ Editar Minúcia");
    editMinutiaToolbarAction->setCheckable(true);
    connect(editMinutiaToolbarAction, &QAction::triggered, [this](bool checked) {
        qDebug() << "🎯 Toolbar: Modo de Edição Interativa -" << (checked ? "ATIVADO" : "DESATIVADO");
        toggleInteractiveEditMode(checked);
    });

    QAction *removeMinutiaAction = toolsToolBar->addAction("🗑️ Remover Minúcia");
    removeMinutiaAction->setCheckable(true);
    connect(removeMinutiaAction, &QAction::triggered, [this]() { setToolMode(MODE_REMOVE_MINUTIA); });

    toolsToolBar->addSeparator();
    
    QAction *calibrateScaleAction = toolsToolBar->addAction("📏 Calibrar Escala");
    calibrateScaleAction->setCheckable(true);
    calibrateScaleAction->setToolTip("Calibrar escala baseada em distância entre cristas (Ctrl+K)");
    connect(calibrateScaleAction, &QAction::triggered, [this]() { setToolMode(MODE_CALIBRATE_SCALE); });

    // Agrupar ações para exclusividade mútua
    QActionGroup *toolGroup = new QActionGroup(this);
    toolGroup->addAction(noneToolAction);
    toolGroup->addAction(cropToolAction);
    toolGroup->addAction(addMinutiaAction);
    toolGroup->addAction(editMinutiaToolbarAction);
    toolGroup->addAction(removeMinutiaAction);
    toolGroup->addAction(calibrateScaleAction);
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

QWidget* MainWindow::createViewerContainer(ImageViewer* viewer, FingerprintEnhancer::MinutiaeOverlay* overlay, FragmentRegionsOverlay* fragmentOverlay) {
    QWidget *container = new QWidget();
    QGridLayout *gridLayout = new QGridLayout(container);
    gridLayout->setSpacing(0);
    gridLayout->setContentsMargins(0, 0, 0, 0);
    
    // Determinar qual painel (esquerdo ou direito)
    bool isLeftPanel = (viewer == processedImageViewer);
    
    // Criar réguas para este painel
    RulerWidget *topRuler = new RulerWidget(RulerWidget::HORIZONTAL, RulerWidget::TOP, container);
    RulerWidget *leftRuler = new RulerWidget(RulerWidget::VERTICAL, RulerWidget::LEFT, container);
    
    // Armazenar referências
    if (isLeftPanel) {
        leftTopRuler = topRuler;
        leftLeftRuler = leftRuler;
    } else {
        rightTopRuler = topRuler;
        rightLeftRuler = leftRuler;
    }
    
    // Configurar réguas (ocultas por padrão)
    topRuler->setVisible(false);
    leftRuler->setVisible(false);
    
    // Criar widget central com viewer e overlays empilhados
    QWidget *viewerWidget = new QWidget();
    QStackedLayout *stackedLayout = new QStackedLayout(viewerWidget);
    stackedLayout->setStackingMode(QStackedLayout::StackAll);
    stackedLayout->setContentsMargins(0, 0, 0, 0);
    
    stackedLayout->addWidget(viewer);
    stackedLayout->addWidget(fragmentOverlay);  // Fragment regions abaixo
    stackedLayout->addWidget(overlay);          // Minutiae no topo
    
    // Configurar overlays para serem transparentes e passar eventos de mouse
    overlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    overlay->setStyleSheet("background-color: transparent;");
    fragmentOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    fragmentOverlay->setStyleSheet("background-color: transparent;");
    
    // Adicionar ao grid layout
    // [0,0] = canto    [0,1] = régua topo
    // [1,0] = régua esq [1,1] = viewer
    QWidget *cornerWidget = new QWidget();
    cornerWidget->setFixedSize(25, 25);
    cornerWidget->setStyleSheet("background-color: #f0f0f0;");
    
    gridLayout->addWidget(cornerWidget, 0, 0);
    gridLayout->addWidget(topRuler, 0, 1);
    gridLayout->addWidget(leftRuler, 1, 0);
    gridLayout->addWidget(viewerWidget, 1, 1);
    
    // Conectar sinais do viewer para atualizar réguas
    connect(viewer, &ImageViewer::zoomChanged, this, [topRuler, leftRuler](double factor) {
        topRuler->setZoomFactor(factor);
        leftRuler->setZoomFactor(factor);
    });
    
    connect(viewer, &ImageViewer::scrollChanged, this, [topRuler, leftRuler, viewer](QPoint offset) {
        topRuler->setScrollOffset(offset.x());
        leftRuler->setScrollOffset(offset.y());
        topRuler->update();
        leftRuler->update();
    });
    
    connect(viewer, &ImageViewer::imageOffsetChanged, this, [topRuler, leftRuler](QPoint offset) {
        topRuler->setImageOffset(offset.x());
        leftRuler->setImageOffset(offset.y());
    });
    
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
    
    // Criar overlays para regiões de fragmentos
    leftFragmentRegionsOverlay = new FragmentRegionsOverlay(nullptr);
    rightFragmentRegionsOverlay = new FragmentRegionsOverlay(nullptr);

    // Criar containers para cada visualizador com seus overlays
    leftViewerContainer = createViewerContainer(processedImageViewer, leftMinutiaeOverlay, leftFragmentRegionsOverlay);
    rightViewerContainer = createViewerContainer(secondImageViewer, rightMinutiaeOverlay, rightFragmentRegionsOverlay);

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
    
    // Grupo de operações de conversão
    QGroupBox *conversionGroup = new QGroupBox("Conversão e Ajustes");
    QVBoxLayout *conversionLayout = new QVBoxLayout(conversionGroup);
    
    QPushButton *claheButton = new QPushButton("Aplicar CLAHE");
    QPushButton *toGrayscaleButton = new QPushButton("Converter para Cinza");
    QPushButton *invertColorsButton = new QPushButton("Inverter Cores");
    QPushButton *adjustBrightnessContrastButton = new QPushButton("Ajustar Brilho e Contraste");
    
    claheButton->setToolTip("Contrast Limited Adaptive Histogram Equalization");
    toGrayscaleButton->setToolTip("Converter imagem para escala de cinza");
    invertColorsButton->setToolTip("Inverter cores da imagem");
    adjustBrightnessContrastButton->setToolTip("Aplicar valores de brilho e contraste dos sliders acima");
    
    // Conectar sinais dos botões diretamente
    connect(adjustBrightnessContrastButton, &QPushButton::clicked, this, &MainWindow::applyBrightnessContrast);
    connect(claheButton, &QPushButton::clicked, this, &MainWindow::applyCLAHE);
    connect(toGrayscaleButton, &QPushButton::clicked, this, &MainWindow::convertToGrayscale);
    connect(invertColorsButton, &QPushButton::clicked, this, &MainWindow::invertColors);
    
    conversionLayout->addWidget(adjustBrightnessContrastButton);
    conversionLayout->addWidget(claheButton);
    conversionLayout->addWidget(toGrayscaleButton);
    conversionLayout->addWidget(invertColorsButton);
    
    enhancementLayout->addWidget(conversionGroup);
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
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::deleteImageRequested,
            this, &MainWindow::onDeleteImageRequested);
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::duplicateFragmentRequested,
            this, &MainWindow::onDuplicateFragmentRequested);
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::exportImageRequested,
            this, &MainWindow::onExportImageRequested);
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::exportFragmentRequested,
            this, &MainWindow::onExportFragmentRequested);
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::editImagePropertiesRequested,
            this, &MainWindow::onEditImagePropertiesRequested);
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::editFragmentPropertiesRequested,
            this, &MainWindow::onEditFragmentPropertiesRequested);
    connect(fragmentManager, &FingerprintEnhancer::FragmentManager::toggleFragmentRegionsRequested,
            this, &MainWindow::toggleShowFragmentRegions);
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
    connect(leftMinutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::angleChanged,
            this, &MainWindow::onMinutiaAngleChanged);
    connect(leftMinutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::minutiaDeleteRequested,
            this, &MainWindow::onDeleteMinutiaRequested);
    connect(leftMinutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::exitEditModeRequested,
            this, [this]() { 
                qDebug() << "🔑 Saindo do modo de edição (ESC)";
                toggleInteractiveEditMode(false); 
            });
    
    connect(rightMinutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::minutiaDoubleClicked,
            this, &MainWindow::onMinutiaDoubleClicked);
    connect(rightMinutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::positionChanged,
            this, &MainWindow::onMinutiaPositionChanged);
    connect(rightMinutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::angleChanged,
            this, &MainWindow::onMinutiaAngleChanged);
    connect(rightMinutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::minutiaDeleteRequested,
            this, &MainWindow::onDeleteMinutiaRequested);
    connect(rightMinutiaeOverlay, &FingerprintEnhancer::MinutiaeOverlay::exitEditModeRequested,
            this, [this]() { 
                qDebug() << "🔑 Saindo do modo de edição (ESC)";
                toggleInteractiveEditMode(false); 
            });

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
    
    // Conectar sinais de crop selection para auto-save
    connect(processedImageViewer, &ImageViewer::cropSelectionChanged,
            this, [this]() {
                if (!leftPanelEntityId.isEmpty()) {
                    processedImageViewer->saveCropSelectionState(leftPanelEntityId);
                }
            });
    connect(secondImageViewer, &ImageViewer::cropSelectionChanged,
            this, [this]() {
                if (!rightPanelEntityId.isEmpty()) {
                    secondImageViewer->saveCropSelectionState(rightPanelEntityId);
                }
            });
    
    // Conectar sinais para fragment regions overlays do painel esquerdo
    connect(processedImageViewer, &ImageViewer::zoomChanged,
            leftFragmentRegionsOverlay, &FragmentRegionsOverlay::setScaleFactor);
    connect(processedImageViewer, &ImageViewer::scrollChanged,
            leftFragmentRegionsOverlay, &FragmentRegionsOverlay::setScrollOffset);
    connect(processedImageViewer, &ImageViewer::imageOffsetChanged,
            leftFragmentRegionsOverlay, &FragmentRegionsOverlay::setImageOffset);
    
    // Conectar sinais para fragment regions overlays do painel direito
    connect(secondImageViewer, &ImageViewer::zoomChanged,
            rightFragmentRegionsOverlay, &FragmentRegionsOverlay::setScaleFactor);
    connect(secondImageViewer, &ImageViewer::scrollChanged,
            rightFragmentRegionsOverlay, &FragmentRegionsOverlay::setScrollOffset);
    connect(secondImageViewer, &ImageViewer::imageOffsetChanged,
            rightFragmentRegionsOverlay, &FragmentRegionsOverlay::setImageOffset);
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

    // Verificar se há projeto aberto
    if (PM::instance().hasOpenProject()) {
        FingerprintEnhancer::Project* currentProj = PM::instance().getCurrentProject();
        
        // Perguntar sobre salvar SOMENTE se projeto foi modificado
        if (currentProj && currentProj->modified) {
            int result = QMessageBox::question(
                this, 
                "Salvar Projeto Atual?",
                "O projeto atual possui alterações não salvas.\n"
                "Deseja salvar antes de criar um novo projeto?",
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                QMessageBox::Yes
            );
            
            if (result == QMessageBox::Cancel) {
                return;  // Cancelar criação de novo projeto
            }
            
            if (result == QMessageBox::Yes) {
                saveProject();  // Salvar projeto atual
            }
        }
        
        // SEMPRE limpar visualizações e fechar projeto (modificado OU NÃO)
        currentEntityType = ENTITY_NONE;
        currentEntityId.clear();
        leftPanelEntityType = ENTITY_NONE;
        leftPanelEntityId.clear();
        rightPanelEntityType = ENTITY_NONE;
        rightPanelEntityId.clear();
        
        // Limpar viewers
        if (processedImageViewer) {
            processedImageViewer->clearImage();
        }
        if (secondImageViewer) {
            secondImageViewer->clearImage();
        }
        
        // Limpar overlays de minúcias
        if (leftMinutiaeOverlay) {
            leftMinutiaeOverlay->clearMinutiae();
            leftMinutiaeOverlay->setFragment(nullptr);
            leftMinutiaeOverlay->update();
        }
        if (rightMinutiaeOverlay) {
            rightMinutiaeOverlay->clearMinutiae();
            rightMinutiaeOverlay->setFragment(nullptr);
            rightMinutiaeOverlay->update();
        }
        
        // Limpar overlays de regiões de fragmentos
        if (leftFragmentRegionsOverlay) {
            leftFragmentRegionsOverlay->setImage(nullptr);
            leftFragmentRegionsOverlay->update();
        }
        if (rightFragmentRegionsOverlay) {
            rightFragmentRegionsOverlay->setImage(nullptr);
            rightFragmentRegionsOverlay->update();
        }
        
        // Limpar FragmentManager ANTES de fechar projeto
        if (fragmentManager) {
            fragmentManager->setProject(nullptr);
        }
        
        // Fechar projeto anterior
        PM::instance().closeProject();
        
        statusLabel->setText("Projeto anterior fechado");
        updateWindowTitle();
    }

    FingerprintEnhancer::NewProjectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString name = dialog.getProjectName();
        QString caseNumber = dialog.getCaseNumber();

        // WORKAROUND: ProjectManager::closeProject() tem um bug que não limpa o estado interno
        // Tentamos fechar múltiplas vezes, e se falhar, usamos hack com openProject()
        
        // Tentar fechar múltiplas vezes
        int closeAttempts = 5;
        for (int i = 0; i < closeAttempts && PM::instance().hasOpenProject(); i++) {
            PM::instance().closeProject();
        }
        
        bool projectCreated = false;
        
        // Se AINDA houver projeto aberto, usar hack: openProject() em arquivo vazio força reset
        if (PM::instance().hasOpenProject()) {
            // Criar arquivo temporário vazio
            QString tempFile = QDir::temp().filePath("__reset_project__.fpe");
            QFile file(tempFile);
            if (file.open(QIODevice::WriteOnly)) {
                QJsonObject emptyProject;
                emptyProject["version"] = "1.0";
                emptyProject["name"] = "";
                emptyProject["images"] = QJsonArray();
                emptyProject["fragments"] = QJsonArray();
                
                QJsonDocument doc(emptyProject);
                file.write(doc.toJson());
                file.close();
                
                // Hack: openProject() fecha o anterior automaticamente
                PM::instance().openProject(tempFile);
                PM::instance().closeProject();
                QFile::remove(tempFile);
            }
        }
        
        // Tentar criar projeto
        projectCreated = PM::instance().createNewProject(name, caseNumber);
        
        if (projectCreated) {
            fprintf(stderr, "[NEW_PROJECT] Projeto criado com sucesso - limpando painéis\n");
            
            // Limpar estado das entidades
            currentEntityType = ENTITY_NONE;
            currentEntityId.clear();
            leftPanelEntityType = ENTITY_NONE;
            leftPanelEntityId.clear();
            rightPanelEntityType = ENTITY_NONE;
            rightPanelEntityId.clear();
            
            // Forçar limpeza dos viewers (projeto novo está vazio)
            if (processedImageViewer) {
                processedImageViewer->clearImage();
                fprintf(stderr, "[NEW_PROJECT] Painel esquerdo limpo\n");
            }
            if (secondImageViewer) {
                secondImageViewer->clearImage();
                fprintf(stderr, "[NEW_PROJECT] Painel direito limpo\n");
            }
            
            // Limpar overlays de minúcias
            if (leftMinutiaeOverlay) {
                leftMinutiaeOverlay->clearMinutiae();
                leftMinutiaeOverlay->setFragment(nullptr);
                leftMinutiaeOverlay->update();
            }
            if (rightMinutiaeOverlay) {
                rightMinutiaeOverlay->clearMinutiae();
                rightMinutiaeOverlay->setFragment(nullptr);
                rightMinutiaeOverlay->update();
            }
            
            // Limpar overlays de fragmentos
            if (leftFragmentRegionsOverlay) {
                leftFragmentRegionsOverlay->setImage(nullptr);
                leftFragmentRegionsOverlay->update();
            }
            if (rightFragmentRegionsOverlay) {
                rightFragmentRegionsOverlay->setImage(nullptr);
                rightFragmentRegionsOverlay->update();
            }
            
            // Atualizar FragmentManager com projeto vazio
            fragmentManager->setProject(PM::instance().getCurrentProject());
            
            statusLabel->setText("✅ Novo projeto criado: " + name);
            updateWindowTitle();
            fprintf(stderr, "[NEW_PROJECT] Concluído - painéis devem estar limpos\n");
        } else {
            QMessageBox::critical(this, "Erro ao Criar Projeto", 
                QString("Não foi possível criar o projeto.\n\n"
                        "Solução: Feche a aplicação e abra novamente."));
        }
    }
}

void MainWindow::openProject() {
    using PM = FingerprintEnhancer::ProjectManager;

    // Verificar se há projeto aberto
    if (PM::instance().hasOpenProject()) {
        FingerprintEnhancer::Project* currentProj = PM::instance().getCurrentProject();
        
        // Perguntar sobre salvar SOMENTE se projeto foi modificado
        if (currentProj && currentProj->modified) {
            int result = QMessageBox::question(
                this,
                "Salvar Projeto Atual?",
                "O projeto atual possui alterações não salvas.\n"
                "Deseja salvar antes de abrir outro projeto?",
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
                QMessageBox::Yes
            );
            
            if (result == QMessageBox::Cancel) {
                return;  // Cancelar abertura de novo projeto
            }
            
            if (result == QMessageBox::Yes) {
                saveProject();  // Salvar projeto atual
            }
        }
        
        // SEMPRE limpar visualizações e fechar projeto anterior (modificado OU NÃO)
        currentEntityType = ENTITY_NONE;
        currentEntityId.clear();
        leftPanelEntityType = ENTITY_NONE;
        leftPanelEntityId.clear();
        rightPanelEntityType = ENTITY_NONE;
        rightPanelEntityId.clear();
        
        // Limpar viewers
        if (processedImageViewer) {
            processedImageViewer->clearImage();
        }
        if (secondImageViewer) {
            secondImageViewer->clearImage();
        }
        
        // Limpar overlays de minúcias
        if (leftMinutiaeOverlay) {
            leftMinutiaeOverlay->clearMinutiae();
            leftMinutiaeOverlay->setFragment(nullptr);
            leftMinutiaeOverlay->update();
        }
        if (rightMinutiaeOverlay) {
            rightMinutiaeOverlay->clearMinutiae();
            rightMinutiaeOverlay->setFragment(nullptr);
            rightMinutiaeOverlay->update();
        }
        
        // Limpar overlays de regiões de fragmentos
        if (leftFragmentRegionsOverlay) {
            leftFragmentRegionsOverlay->setImage(nullptr);
            leftFragmentRegionsOverlay->update();
        }
        if (rightFragmentRegionsOverlay) {
            rightFragmentRegionsOverlay->setImage(nullptr);
            rightFragmentRegionsOverlay->update();
        }
        
        // Limpar FragmentManager
        if (fragmentManager) {
            fragmentManager->setProject(nullptr);
        }
        
        // SEMPRE fechar projeto anterior (modificado ou não)
        PM::instance().closeProject();
    }

    QString fileName = QFileDialog::getOpenFileName(this,
        "Abrir Projeto", "", "Projeto FingerprintEnhancer (*.fpe)");

    if (!fileName.isEmpty()) {
        if (PM::instance().openProject(fileName)) {
            fprintf(stderr, "[OPEN_PROJECT] Projeto aberto - atualizando painéis\n");
            
            // Limpar estado das entidades primeiro
            currentEntityType = ENTITY_NONE;
            currentEntityId.clear();
            leftPanelEntityType = ENTITY_NONE;
            leftPanelEntityId.clear();
            rightPanelEntityType = ENTITY_NONE;
            rightPanelEntityId.clear();
            
            // Forçar limpeza dos viewers primeiro
            if (processedImageViewer) {
                processedImageViewer->clearImage();
            }
            if (secondImageViewer) {
                secondImageViewer->clearImage();
            }
            
            // Limpar overlays
            if (leftMinutiaeOverlay) {
                leftMinutiaeOverlay->clearMinutiae();
                leftMinutiaeOverlay->setFragment(nullptr);
                leftMinutiaeOverlay->update();
            }
            if (rightMinutiaeOverlay) {
                rightMinutiaeOverlay->clearMinutiae();
                rightMinutiaeOverlay->setFragment(nullptr);
                rightMinutiaeOverlay->update();
            }
            if (leftFragmentRegionsOverlay) {
                leftFragmentRegionsOverlay->setImage(nullptr);
                leftFragmentRegionsOverlay->update();
            }
            if (rightFragmentRegionsOverlay) {
                rightFragmentRegionsOverlay->setImage(nullptr);
                rightFragmentRegionsOverlay->update();
            }
            
            // Atualizar FragmentManager
            fragmentManager->setProject(PM::instance().getCurrentProject());
            
            statusLabel->setText("✅ Projeto aberto: " + QFileInfo(fileName).fileName());
            updateWindowTitle();
            fprintf(stderr, "[OPEN_PROJECT] Concluído - painéis atualizados\n");
        } else {
            QMessageBox::warning(this, "Erro", "Falha ao abrir o projeto");
        }
    }
}

void MainWindow::saveProject() {
    using namespace FingerprintEnhancer;
    
    // Verificar se há projeto aberto
    ProjectManager& pm = ProjectManager::instance();
    if (!pm.hasOpenProject()) {
        QMessageBox::warning(this, "Nenhum Projeto",
                           "Não há projeto aberto para salvar.");
        return;
    }

    // Verificar se já está salvando
    if (isSavingProject) {
        QMessageBox::information(this, "Salvamento em Progresso",
                               "Aguarde o salvamento atual terminar.");
        return;
    }

    // Verificar se projeto tem caminho (senão usar saveProjectAs)
    QString projectPath = pm.getCurrentProjectPath();
    if (projectPath.isEmpty()) {
        saveProjectAs();
        return;
    }

    // Criar worker para salvar em thread separada
    projectSaverWorker = new ProjectSaverWorker(this);
    projectSaverWorker->setSaveParameters(""); // Usa caminho atual do projeto
    projectSaverWorker->setTimeout(300000); // 5 minutos

    // Conectar signals
    connect(projectSaverWorker, &ProjectSaverWorker::progressUpdated,
            this, &MainWindow::onSaveProgress);
    connect(projectSaverWorker, &ProjectSaverWorker::saveCompleted,
            this, &MainWindow::onSaveCompleted);
    connect(projectSaverWorker, &ProjectSaverWorker::saveTimeout,
            this, &MainWindow::onSaveTimeout);
    connect(projectSaverWorker, &ProjectSaverWorker::finished,
            projectSaverWorker, &QObject::deleteLater);

    // Configurar interface para saving
    isSavingProject = true;
    progressBar->setVisible(true);
    progressBar->setMaximum(0); // Indeterminate progress
    
    // Desabilitar botões de salvar
    if (saveAction) saveAction->setEnabled(false);
    if (saveAsAction) saveAsAction->setEnabled(false);

    statusLabel->setText("Salvando projeto...");

    // Iniciar salvamento
    projectSaverWorker->start();
}

void MainWindow::saveProjectAs() {
    using namespace FingerprintEnhancer;
    
    // Verificar se há projeto aberto
    ProjectManager& pm = ProjectManager::instance();
    if (!pm.hasOpenProject()) {
        QMessageBox::warning(this, "Nenhum Projeto",
                           "Não há projeto aberto para salvar.");
        return;
    }

    // Verificar se já está salvando
    if (isSavingProject) {
        QMessageBox::information(this, "Salvamento em Progresso",
                               "Aguarde o salvamento atual terminar.");
        return;
    }

    // Obter diretório padrão
    QString defaultDir = getProjectDirectory();
    
    // Diálogo para escolher caminho
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Salvar Projeto Como",
        defaultDir + "/projeto.fpe",
        "Projetos FingerprintEnhancer (*.fpe);;Todos os Arquivos (*.*)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    // Garantir extensão .fpe
    if (!filePath.endsWith(".fpe", Qt::CaseInsensitive)) {
        filePath += ".fpe";
    }

    // Criar worker para salvar em thread separada
    projectSaverWorker = new ProjectSaverWorker(this);
    projectSaverWorker->setSaveParameters(filePath);
    projectSaverWorker->setTimeout(300000); // 5 minutos

    // Conectar signals
    connect(projectSaverWorker, &ProjectSaverWorker::progressUpdated,
            this, &MainWindow::onSaveProgress);
    connect(projectSaverWorker, &ProjectSaverWorker::saveCompleted,
            this, &MainWindow::onSaveCompleted);
    connect(projectSaverWorker, &ProjectSaverWorker::saveTimeout,
            this, &MainWindow::onSaveTimeout);
    connect(projectSaverWorker, &ProjectSaverWorker::finished,
            projectSaverWorker, &QObject::deleteLater);

    // Configurar interface para saving
    isSavingProject = true;
    progressBar->setVisible(true);
    progressBar->setMaximum(0); // Indeterminate progress
    
    // Desabilitar botões de salvar
    if (saveAction) saveAction->setEnabled(false);
    if (saveAsAction) saveAsAction->setEnabled(false);

    statusLabel->setText("Salvando projeto...");

    // Iniciar salvamento
    projectSaverWorker->start();
}

void MainWindow::editProjectInfo() {
    using PM = FingerprintEnhancer::ProjectManager;

    if (!PM::instance().hasOpenProject()) {
        QMessageBox::information(this, "Info", "Nenhum projeto aberto");
        return;
    }

    FingerprintEnhancer::Project* project = PM::instance().getCurrentProject();
    
    // Criar diálogo similar ao NewProjectDialog, mas populado com dados existentes
    QDialog dialog(this);
    dialog.setWindowTitle("Editar Informações do Projeto");
    dialog.setMinimumWidth(400);

    QVBoxLayout* mainLayout = new QVBoxLayout(&dialog);
    QFormLayout* formLayout = new QFormLayout();

    // Campos de entrada
    QLineEdit* projectNameEdit = new QLineEdit(project->name, &dialog);
    QLineEdit* caseNumberEdit = new QLineEdit(project->caseNumber, &dialog);
    QTextEdit* descriptionEdit = new QTextEdit(&dialog);
    descriptionEdit->setPlainText(project->description);
    descriptionEdit->setMaximumHeight(100);

    formLayout->addRow("Nome do Projeto:", projectNameEdit);
    formLayout->addRow("Número do Caso:", caseNumberEdit);
    formLayout->addRow("Descrição:", descriptionEdit);

    mainLayout->addLayout(formLayout);

    // Botões
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton("OK", &dialog);
    QPushButton* cancelButton = new QPushButton("Cancelar", &dialog);

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    // Conectar botões
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    // Validação
    connect(projectNameEdit, &QLineEdit::textChanged, [&]() {
        okButton->setEnabled(!projectNameEdit->text().trimmed().isEmpty());
    });

    // Executar diálogo
    if (dialog.exec() == QDialog::Accepted) {
        QString newName = projectNameEdit->text().trimmed();
        QString newCaseNumber = caseNumberEdit->text().trimmed();
        QString newDescription = descriptionEdit->toPlainText().trimmed();

        if (!newName.isEmpty()) {
            project->name = newName;
            project->caseNumber = newCaseNumber;
            project->description = newDescription;
            project->setModified();

            statusLabel->setText("Informações do projeto atualizadas");
            updateWindowTitle();
        } else {
            QMessageBox::warning(this, "Erro", "O nome do projeto não pode ser vazio");
        }
    }
}

void MainWindow::clearProject() {
    using PM = FingerprintEnhancer::ProjectManager;

    if (!PM::instance().hasOpenProject()) {
        QMessageBox::information(this, "Info", "Nenhum projeto aberto");
        return;
    }

    FingerprintEnhancer::Project* project = PM::instance().getCurrentProject();

    // Verificar se há alterações não salvas
    QString warningMsg = "Deseja realmente limpar o projeto atual?";
    if (project->modified) {
        warningMsg += "\n\nATENÇÃO: Existem alterações não salvas que serão perdidas!";
    }

    int imageCount = project->images.size();
    int totalMinutiae = 0;
    for (const auto& img : project->images) {
        for (const auto& frag : img.fragments) {
            totalMinutiae += frag.getMinutiaeCount();
        }
    }

    if (imageCount > 0) {
        warningMsg += QString("\n\nO projeto contém:\n- %1 imagem(ns)\n- %2 minúcia(s)")
                      .arg(imageCount).arg(totalMinutiae);
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Confirmar Limpeza",
        warningMsg,
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        // Limpar variáveis de estado
        currentEntityType = ENTITY_NONE;
        currentEntityId.clear();
        currentImageId.clear();
        currentFragmentId.clear();
        leftPanelEntityType = ENTITY_NONE;
        leftPanelEntityId.clear();
        rightPanelEntityType = ENTITY_NONE;
        rightPanelEntityId.clear();
        
        // Limpar visualizadores
        processedImageViewer->clearImage();
        secondImageViewer->clearImage();
        
        // Limpar overlays de minúcias
        if (leftMinutiaeOverlay) {
            leftMinutiaeOverlay->clearMinutiae();
            leftMinutiaeOverlay->setFragment(nullptr);
            leftMinutiaeOverlay->update();
        }
        if (rightMinutiaeOverlay) {
            rightMinutiaeOverlay->clearMinutiae();
            rightMinutiaeOverlay->setFragment(nullptr);
            rightMinutiaeOverlay->update();
        }
        if (minutiaeOverlay) {  // Overlay legado
            minutiaeOverlay->setFragment(nullptr);
        }
        
        // Limpar overlays de regiões de fragmentos
        if (leftFragmentRegionsOverlay) {
            leftFragmentRegionsOverlay->setImage(nullptr);
            leftFragmentRegionsOverlay->update();
        }
        if (rightFragmentRegionsOverlay) {
            rightFragmentRegionsOverlay->setImage(nullptr);
            rightFragmentRegionsOverlay->update();
        }

        // Limpar histórico e lista de minúcias
        historyDisplay->clear();
        minutiaeList->clear();

        // Fechar projeto atual
        PM::instance().closeProject();

        // Criar novo projeto vazio
        if (PM::instance().createNewProject("Projeto sem título", "")) {
            // Atualizar árvore de projeto com novo projeto vazio
            fragmentManager->setProject(PM::instance().getCurrentProject());
            fragmentManager->updateView();
            statusLabel->setText("Projeto limpo - Novo projeto criado");
            updateWindowTitle();
        } else {
            QMessageBox::warning(this, "Erro", "Não foi possível criar novo projeto");
        }
    }
}

void MainWindow::openImage() {
    using namespace FingerprintEnhancer;
    
    // Verificar se há projeto aberto, senão criar automaticamente
    ProjectManager& pm = ProjectManager::instance();
    if (!pm.hasOpenProject()) {
        // Criar projeto automático com nome baseado na data/hora
        QString projectName = QString("Projeto %1").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm"));
        
        if (!pm.createNewProject(projectName, "")) {
            QMessageBox::warning(this, "Erro", 
                               "Não foi possível criar projeto automaticamente.\n"
                               "Por favor, crie um projeto manualmente primeiro.");
            return;
        }
        
        // Atualizar interface com novo projeto
        fragmentManager->setProject(pm.getCurrentProject());
        updateWindowTitle();
        statusLabel->setText(QString("✓ Projeto criado automaticamente: %1").arg(projectName));
    }

    // Verificar se já está carregando imagens
    if (isLoadingImages) {
        QMessageBox::information(this, "Carregamento em Progresso",
                               "Aguarde o carregamento das imagens anteriores terminar.");
        return;
    }

    // Obter diretório do projeto como padrão
    QString defaultDir = getProjectDirectory();

    // Permitir seleção múltipla de arquivos
    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        "Selecionar Imagens para Adicionar ao Projeto",
        defaultDir,
        "Imagens (*.png *.jpg *.jpeg *.bmp *.tiff *.tif);;Todos os Arquivos (*.*)"
    );

    if (filePaths.isEmpty()) {
        return;
    }

    // Criar worker para carregar imagens em thread separada
    imageLoaderWorker = new ImageLoaderWorker(filePaths, this);
    
    // Conectar signals
    connect(imageLoaderWorker, &ImageLoaderWorker::progressUpdated,
            this, &MainWindow::onImageLoadProgress);
    connect(imageLoaderWorker, &ImageLoaderWorker::imageLoaded,
            this, &MainWindow::onImageLoaded);
    connect(imageLoaderWorker, &ImageLoaderWorker::loadingFailed,
            this, &MainWindow::onImageLoadingFailed);
    connect(imageLoaderWorker, &ImageLoaderWorker::allImagesLoaded,
            this, &MainWindow::onAllImagesLoaded);
    connect(imageLoaderWorker, &ImageLoaderWorker::finished,
            imageLoaderWorker, &QObject::deleteLater);

    // Configurar interface para loading
    isLoadingImages = true;
    progressBar->setVisible(true);
    progressBar->setMaximum(filePaths.size());
    progressBar->setValue(0);
    statusLabel->setText(QString("Carregando %1 imagem(ns)...").arg(filePaths.size()));

    // Iniciar carregamento
    imageLoaderWorker->start();
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

void MainWindow::applyBrightnessContrast() {
    // Aplicar valores dos sliders de brilho e contraste
    int brightness = brightnessSlider->value();  // -100 a 100
    int contrast = contrastSlider->value();       // 50 a 300
    
    applyOperationToCurrentEntity([brightness, contrast](cv::Mat& img) {
        // Converter contraste de 50-300 para 0.5-3.0 (alpha)
        double alpha = contrast / 100.0;
        // Brightness já está em -100 a 100 (beta)
        double beta = static_cast<double>(brightness);
        
        img.convertTo(img, -1, alpha, beta);
    });
    
    statusLabel->setText(QString("✅ Brilho: %1 | Contraste: %2% aplicados")
                        .arg(brightness)
                        .arg(contrast));
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
        auto oldSettings = minutiaeDisplaySettings;
        minutiaeDisplaySettings = dialog.getSettings();

        // Aplicar configurações aos overlays
        leftMinutiaeOverlay->setDisplaySettings(minutiaeDisplaySettings);
        rightMinutiaeOverlay->setDisplaySettings(minutiaeDisplaySettings);

        // Se a posição padrão mudou, perguntar se quer aplicar a todas as minúcias do projeto
        if (oldSettings.defaultLabelPosition != minutiaeDisplaySettings.defaultLabelPosition) {
            QMessageBox::StandardButton reply = QMessageBox::question(this,
                "Aplicar Posição do Rótulo",
                "Deseja aplicar a nova posição padrão do rótulo a TODAS as minúcias do projeto?",
                QMessageBox::Yes | QMessageBox::No);
            
            if (reply == QMessageBox::Yes) {
                // Aplicar a todas as minúcias de todos os fragmentos do projeto
                using PM = FingerprintEnhancer::ProjectManager;
                Project* project = PM::instance().getCurrentProject();
                if (project) {
                    int totalMinutiae = 0;
                    for (auto& image : project->images) {
                        for (auto& fragment : image.fragments) {
                            for (auto& minutia : fragment.minutiae) {
                                minutia.labelPosition = minutiaeDisplaySettings.defaultLabelPosition;
                                totalMinutiae++;
                            }
                        }
                    }
                    qDebug() << "✅ Posição do rótulo aplicada a" << totalMinutiae << "minúcias em todo o projeto";
                    statusLabel->setText(QString("Posição do rótulo aplicada a %1 minúcias do projeto").arg(totalMinutiae));
                    PM::instance().getCurrentProject()->setModified();
                }
            }
        }

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
        
        // Ajustar painel esquerdo ao novo tamanho
        processedImageViewer->zoomToFit();
    } else {
        rightViewerContainer->show();
        
        // Ajustar ambos os painéis ao novo tamanho
        QTimer::singleShot(100, [this]() {
            processedImageViewer->zoomToFit();
            secondImageViewer->zoomToFit();
        });
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

    // Selecionar entidade na árvore de projeto
    if (!currentEntityId.isEmpty()) {
        if (currentEntityType == ENTITY_IMAGE) {
            fragmentManager->selectImage(currentEntityId);
        } else if (currentEntityType == ENTITY_FRAGMENT) {
            fragmentManager->selectFragment(currentEntityId);
        }
    }

    // Atualizar informações na barra de status
    if (!currentEntityId.isEmpty()) {
        statusLabel->setText(QString("✓ Painel %1 ativo: %2 selecionado")
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
    FragmentRegionsOverlay* targetFragmentOverlay = targetPanel ? rightFragmentRegionsOverlay : leftFragmentRegionsOverlay;

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
        
        // Restaurar seleção de crop salva (se existir)
        targetViewer->restoreCropSelectionState(entityId);

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
            
            // Limpar fragment regions overlay (não mostra em fragmentos)
            targetFragmentOverlay->setImage(nullptr);
        } else {
            targetOverlay->setFragment(nullptr);
        }
        
        // Configurar fragment regions overlay se for imagem
        if (type == ENTITY_IMAGE) {
            FingerprintEnhancer::FingerprintImage* img = project->findImage(entityId);
            if (img) {
                targetFragmentOverlay->setImage(img);
                targetFragmentOverlay->setScaleFactor(targetViewer->getScaleFactor());
                
                // Inicializar offsets
                QPoint scrollOffset(targetViewer->horizontalScrollBar()->value(),
                                   targetViewer->verticalScrollBar()->value());
                targetFragmentOverlay->setScrollOffset(scrollOffset);
                targetFragmentOverlay->setImageOffset(targetViewer->getImageOffset());
            }
        }

        targetOverlay->update();
        targetFragmentOverlay->update();
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
            case MODE_NONE:
                // Menu genérico baseado na entidade corrente
                if (currentEntityType == ENTITY_IMAGE) {
                    menu.addAction("📐 Destacar Imagem Inteira como Fragmento",
                                         this, &MainWindow::createFragmentFromWholeImage);
                    menu.addSeparator();
                    
                    // Opção para exibir/ocultar regiões de fragmentos
                    bool showing = leftFragmentRegionsOverlay->isShowingRegions();
                    QString toggleText = showing ? "✗ Ocultar Regiões de Fragmentos" : "📍 Exibir Regiões de Fragmentos";
                    menu.addAction(toggleText, this, &MainWindow::toggleShowFragmentRegions);
                } else if (currentEntityType == ENTITY_FRAGMENT) {
                    menu.addAction("⚡ Inserção Rápida (sem classificar)", [this, imagePos]() {
                        addMinutiaQuickly(imagePos);
                    });
                    menu.addAction("➕ Adicionar Minúcia Aqui (com diálogo)", [this, imagePos]() {
                        setToolMode(MODE_ADD_MINUTIA);
                        addMinutiaAtPosition(imagePos);
                    });
                }
                break;

            case MODE_CROP:
                if (viewer->hasCropSelection()) {
                    menu.addAction("✓ Aplicar Recorte", this, &MainWindow::applyCrop);
                    menu.addAction("Ajustar Seleção", this, &MainWindow::onCropAdjust);
                    menu.addSeparator();
                    menu.addAction("✗ Cancelar Recorte", this, &MainWindow::cancelCropSelection);
                } else {
                    menu.addAction("Desenhe uma área para recortar");
                }
                break;

            case MODE_ADD_MINUTIA:
                if (currentEntityType == ENTITY_FRAGMENT) {
                    menu.addAction("⚡ Inserção Rápida (sem classificar)", [this, imagePos]() {
                        addMinutiaQuickly(imagePos);
                    });
                    menu.addAction("➕ Adicionar Minúcia Aqui (com diálogo)", [this, imagePos]() {
                        addMinutiaAtPosition(imagePos);
                    });
                }
                break;

            case MODE_EDIT_MINUTIA:
                menu.addAction("Clique em uma minúcia para editar");
                menu.addSeparator();
                menu.addAction("Adicionar Nova Minúcia", [this, imagePos]() {
                    if (currentEntityType == ENTITY_FRAGMENT) {
                        addMinutiaAtPosition(imagePos);
                    }
                });
                break;

            case MODE_REMOVE_MINUTIA:
                menu.addAction("Clique em uma minúcia para remover");
                break;

            case MODE_PAN:
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

    // CONVERTER seleção (em coords rotacionadas atuais) para coordenadas ORIGINAIS
    QRect originalSpaceRect = convertRotatedToOriginalCoords(
        selection, 
        img->currentRotationAngle,
        QSize(img->workingImage.cols, img->workingImage.rows),  // Tamanho atual (rotacionado)
        QSize(img->originalImage.cols, img->originalImage.rows)  // Tamanho original
    );
    
    // Criar fragmento COM coordenadas no espaço ORIGINAL
    // Mas a imagem do fragmento vem do workingImage rotacionado atual
    FingerprintEnhancer::Fragment newFragment(img->id, originalSpaceRect, croppedImage);
    newFragment.sourceRotationAngle = img->currentRotationAngle;  // Salvar ângulo atual
    
    img->fragments.append(newFragment);
    PM::instance().getCurrentProject()->setModified();
    
    FingerprintEnhancer::Fragment* newFragmentPtr = &img->fragments.last();

    // Desativar modo de recorte e limpar seleção salva
    activeViewer->saveCropSelectionState(currentEntityId);  // Salvar seleção vazia
    activeViewer->clearCropSelection();
    activeViewer->setCropMode(false);
    setToolMode(MODE_NONE);

    // Atualizar visualização no gerenciador ANTES de selecionar
    fragmentManager->updateView();

    // Selecionar o fragmento criado automaticamente
    QString newFragmentId = newFragmentPtr->id;
    setCurrentEntity(newFragmentId, ENTITY_FRAGMENT);
    
    // Selecionar na árvore de projeto
    fragmentManager->selectFragment(newFragmentId);
    
    // Aplicar ajuste ao tamanho do painel
    activeViewer->zoomToFit();

    statusLabel->setText(QString("✓ Fragmento criado e selecionado: %1×%2 px")
                        .arg(selection.width()).arg(selection.height()));
}

void MainWindow::cancelCropSelection() {
    ImageViewer* activeViewer = getActiveViewer();
    
    // Limpar seleção salva
    if (!currentEntityId.isEmpty()) {
        activeViewer->saveCropSelectionState(currentEntityId);  // Salvar seleção vazia
    }
    
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
    } else if (currentEntityType == ENTITY_IMAGE && !currentEntityId.isEmpty()) {
        // Rotacionar IMAGEM - atualizar apenas ângulo da imagem
        FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(currentEntityId);
        if (img) {
            // Rotacionar imagem
            cv::rotate(img->workingImage, img->workingImage, cv::ROTATE_90_CLOCKWISE);
            // Incrementar ângulo da imagem (fragmentos mantêm sourceRect em coords originais)
            img->currentRotationAngle = fmod(img->currentRotationAngle + 90.0, 360.0);
            if (img->currentRotationAngle < 0) img->currentRotationAngle += 360.0;
            loadCurrentEntityToView();
            PM::instance().getCurrentProject()->setModified();
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
    } else if (currentEntityType == ENTITY_IMAGE && !currentEntityId.isEmpty()) {
        // Rotacionar IMAGEM - atualizar ângulo acumulado
        FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(currentEntityId);
        if (img) {
            // Rotacionar imagem
            cv::rotate(img->workingImage, img->workingImage, cv::ROTATE_90_COUNTERCLOCKWISE);
            // Atualizar ângulo acumulado
            img->currentRotationAngle = fmod(img->currentRotationAngle - 90.0, 360.0);
            if (img->currentRotationAngle < 0) img->currentRotationAngle += 360.0;
            loadCurrentEntityToView();
            PM::instance().getCurrentProject()->setModified();
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
    } else if (currentEntityType == ENTITY_IMAGE && !currentEntityId.isEmpty()) {
        // Rotacionar IMAGEM - atualizar ângulo acumulado
        FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(currentEntityId);
        if (img) {
            // Rotacionar imagem
            cv::rotate(img->workingImage, img->workingImage, cv::ROTATE_180);
            // Atualizar ângulo acumulado
            img->currentRotationAngle = fmod(img->currentRotationAngle + 180.0, 360.0);
            if (img->currentRotationAngle < 0) img->currentRotationAngle += 360.0;
            loadCurrentEntityToView();
            PM::instance().getCurrentProject()->setModified();
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
    
    // Pegar fragmento e overlay se estiver rotacionando um fragmento
    FingerprintEnhancer::Fragment* currentFrag = nullptr;
    FingerprintEnhancer::MinutiaeOverlay* activeOverlay = nullptr;
    FingerprintEnhancer::FingerprintImage* currentImg = nullptr;
    FragmentRegionsOverlay* fragmentOverlay = nullptr;
    
    if (currentEntityType == ENTITY_FRAGMENT) {
        currentFrag = PM::instance().getCurrentProject()->findFragment(currentEntityId);
        activeOverlay = getActiveOverlay();
    } else if (currentEntityType == ENTITY_IMAGE) {
        // Se rotacionando IMAGEM, passar imagem e overlay de fragmentos
        currentImg = PM::instance().getCurrentProject()->findImage(currentEntityId);
        fragmentOverlay = activePanel ? rightFragmentRegionsOverlay : leftFragmentRegionsOverlay;
        fprintf(stderr, "[ROTATION] Rotacionando IMAGEM - fragmentOverlay: %p\n", (void*)fragmentOverlay);
    }

    // Criar diálogo de rotação em tempo real (não-modal para permitir zoom/scroll)
    RotationDialog* dialog = new RotationDialog(workingImg, activeViewer, 
                                                 currentFrag, activeOverlay, 
                                                 currentImg, fragmentOverlay, 
                                                 this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    
    // DESABILITAR navegação durante rotação para evitar troca de imagem
    fprintf(stderr, "[ROTATION] Desabilitando árvore de projeto e controles de painel\n");
    if (fragmentManager) {
        fragmentManager->setEnabled(false);
    }
    if (switchPanelAction) {
        switchPanelAction->setEnabled(false);
    }
    if (toggleRightPanelAction) {
        toggleRightPanelAction->setEnabled(false);
    }
    
    // Conectar sinal de aceitação
    connect(dialog, &QDialog::accepted, this, [this, dialog]() {
        if (dialog->wasAccepted()) {
            double angle = dialog->getRotationAngle();
            cv::Mat rotated = dialog->getRotatedImage();

            cv::Mat& workingImg = getCurrentWorkingImage();
            using PM = FingerprintEnhancer::ProjectManager;

            // Aplicar imagem rotacionada
            // IMPORTANTE: As minúcias já foram rotacionadas em tempo real pelo dialog!
            // Não rotacionar novamente aqui
            if (currentEntityType == ENTITY_FRAGMENT) {
                FingerprintEnhancer::Fragment* frag = PM::instance().getCurrentProject()->findFragment(currentEntityId);
                if (frag) {
                    rotated.copyTo(frag->workingImage);
                    // Minúcias já foram rotacionadas em tempo real, não fazer nada aqui
                    fprintf(stderr, "[ROTATION] Fragmento rotacionado %.1f graus\n", angle);
                }
            } else if (currentEntityType == ENTITY_IMAGE) {
                // Aplicar na imagem E atualizar ângulo acumulado
                FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(currentEntityId);
                if (img) {
                    // Atualizar ângulo acumulado da imagem (normalizar para [0, 360))
                    img->currentRotationAngle = fmod(img->currentRotationAngle + angle, 360.0);
                    if (img->currentRotationAngle < 0) img->currentRotationAngle += 360.0;
                    
                    // Se ângulo acumulado é ~0°, usar imagem original sem borda
                    if (fabs(img->currentRotationAngle) < 0.1 || fabs(img->currentRotationAngle - 360.0) < 0.1) {
                        fprintf(stderr, "[ROTATION] Ângulo acumulado ~0° - restaurando imagem ORIGINAL (sem borda)\n");
                        img->originalImage.copyTo(img->workingImage);  // ✅ Restaurar imagem original
                        img->currentRotationAngle = 0.0;  // Zerar ângulo
                    } else {
                        rotated.copyTo(img->workingImage);
                        fprintf(stderr, "[ROTATION] Imagem rotacionada %.1f graus - ângulo acumulado: %.1f\n", 
                                angle, img->currentRotationAngle);
                    }
                }
            } else {
                // Fallback
                rotated.copyTo(workingImg);
            }

            // Recarregar visualização
            loadCurrentEntityToView();

            statusLabel->setText(QString("✅ Imagem rotacionada %1°").arg(angle, 0, 'f', 1));

            // Marcar como modificado
            PM::instance().getCurrentProject()->setModified();
        }
        
        // REABILITAR navegação após rotação
        fprintf(stderr, "[ROTATION] Reabilitando árvore de projeto e controles de painel\n");
        if (fragmentManager) {
            fragmentManager->setEnabled(true);
        }
        if (switchPanelAction) {
            switchPanelAction->setEnabled(true);
        }
        if (toggleRightPanelAction) {
            toggleRightPanelAction->setEnabled(true);
        }
    });
    
    // Conectar sinal de rejeição
    connect(dialog, &QDialog::rejected, this, [this]() {
        statusLabel->setText("❌ Rotação cancelada");
        
        // REABILITAR navegação após cancelamento
        fprintf(stderr, "[ROTATION] Reabilitando árvore de projeto e controles de painel\n");
        if (fragmentManager) {
            fragmentManager->setEnabled(true);
        }
        if (switchPanelAction) {
            switchPanelAction->setEnabled(true);
        }
        if (toggleRightPanelAction) {
            toggleRightPanelAction->setEnabled(true);
        }
    });
    
    // Mostrar dialog não-modal
    dialog->show();
    statusLabel->setText("🔄 Modo de Rotação Interativa - Ajuste o ângulo e use zoom/scroll livremente");
}

// ==================== CALIBRAÇÃO DE ESCALA ====================

void MainWindow::calibrateScale() {
    // Perguntar espaçamento típico entre cristas antes de ativar
    bool ok;
    double ridgeSpacing = QInputDialog::getDouble(this, "Calibração de Escala",
        "Distância típica entre cristas (mm):\n"
        "(Valor padrão: 0.4545mm para impressões digitais)",
        0.4545, 0.1, 5.0, 4, &ok);
    
    if (!ok) return;
    
    // Criar ferramenta se não existir
    if (!scaleCalibrationTool) {
        ImageViewer* activeViewer = getActiveViewer();
        if (!activeViewer) {
            QMessageBox::warning(this, "Calibração de Escala", "Visualizador não disponível");
            return;
        }
        
        scaleCalibrationTool = new ScaleCalibrationTool(activeViewer);
        
        // Conectar sinais
        connect(scaleCalibrationTool, &ScaleCalibrationTool::calibrationCompleted,
                this, &MainWindow::onCalibrationCompleted);
        connect(scaleCalibrationTool, &ScaleCalibrationTool::calibrationCancelled,
                this, &MainWindow::onCalibrationCancelled);
        connect(scaleCalibrationTool, &ScaleCalibrationTool::lineDrawn,
                this, &MainWindow::onCalibrationLineDrawn);
        connect(scaleCalibrationTool, &ScaleCalibrationTool::ridgeCountChanged,
                this, &MainWindow::onCalibrationRidgeCountChanged);
    }
    
    scaleCalibrationTool->setDefaultRidgeSpacing(ridgeSpacing);
    
    // Ativar modo
    setToolMode(MODE_CALIBRATE_SCALE);
    
    QMessageBox::information(this, "Calibração de Escala",
        "Instruções:\n\n"
        "1. Desenhe uma linha perpendicular através das cristas\n"
        "2. Clique em cada crista ao longo da linha (mínimo 2)\n"
        "3. Pressione ENTER para concluir\n"
        "4. ESC para cancelar ou clique em outra ferramenta\n\n"
        "Dica: Quanto mais cristas marcar, mais precisa será a calibração!");
}

void MainWindow::activateScaleCalibrationMode() {
    if (currentEntityType == ENTITY_NONE || currentEntityId.isEmpty()) {
        QMessageBox::warning(this, "Calibração de Escala", 
                            "Nenhuma imagem ou fragmento selecionado");
        setToolMode(MODE_NONE);
        return;
    }

    ImageViewer* activeViewer = getActiveViewer();
    if (!activeViewer) {
        QMessageBox::warning(this, "Calibração de Escala", "Visualizador não disponível");
        setToolMode(MODE_NONE);
        return;
    }
    
    // Criar ferramenta se não existir
    if (!scaleCalibrationTool) {
        scaleCalibrationTool = new ScaleCalibrationTool(activeViewer);
        
        // Conectar sinais
        connect(scaleCalibrationTool, &ScaleCalibrationTool::calibrationCompleted,
                this, &MainWindow::onCalibrationCompleted);
        connect(scaleCalibrationTool, &ScaleCalibrationTool::calibrationCancelled,
                this, &MainWindow::onCalibrationCancelled);
        connect(scaleCalibrationTool, &ScaleCalibrationTool::lineDrawn,
                this, &MainWindow::onCalibrationLineDrawn);
        connect(scaleCalibrationTool, &ScaleCalibrationTool::ridgeCountChanged,
                this, &MainWindow::onCalibrationRidgeCountChanged);
    }
    
    // Configurar ferramenta com parâmetros atuais
    scaleCalibrationTool->setParent(activeViewer);
    scaleCalibrationTool->setGeometry(activeViewer->rect());
    scaleCalibrationTool->setZoomFactor(activeViewer->getZoomFactor());
    
    // Configurar scroll offset (x e y)
    QPoint scrollOffset(activeViewer->horizontalScrollBar()->value(),
                        activeViewer->verticalScrollBar()->value());
    scaleCalibrationTool->setScrollOffset(scrollOffset);
    
    // Configurar image offset
    scaleCalibrationTool->setImageOffset(activeViewer->getImageOffset());
    
    // Conectar sinais para atualização dinâmica durante calibração
    // Zoom
    connect(activeViewer, &ImageViewer::zoomChanged, scaleCalibrationTool, 
            &ScaleCalibrationTool::setZoomFactor, Qt::UniqueConnection);
    
    // Scroll
    connect(activeViewer, &ImageViewer::scrollChanged, scaleCalibrationTool,
            &ScaleCalibrationTool::setScrollOffset, Qt::UniqueConnection);
    
    // Image offset
    connect(activeViewer, &ImageViewer::imageOffsetChanged, scaleCalibrationTool,
            &ScaleCalibrationTool::setImageOffset, Qt::UniqueConnection);
    
    // Ativar ferramenta
    scaleCalibrationTool->activate();
}

void MainWindow::deactivateScaleCalibrationMode() {
    if (scaleCalibrationTool && scaleCalibrationTool->isActive()) {
        scaleCalibrationTool->deactivate();
    }
}

void MainWindow::showScaleConfig() {
    ScaleConfigDialog dialog(this);
    
    // Configurar valores atuais
    double currentScale = imageProcessor->getScale();
    if (currentScale > 0) {
        dialog.setPixelsPerMm(currentScale);
    }
    
    // Sempre definir espaçamento padrão
    dialog.setRidgeSpacing(0.4545);
    
    if (dialog.exec() == QDialog::Accepted) {
        // Se modificou pixels/mm ou DPI, aplicar à imagem
        double newScale = dialog.getPixelsPerMm();
        if (newScale > 0 && std::abs(newScale - currentScale) > 0.01) {
            imageProcessor->setScale(newScale);
            
            // Atualizar réguas
            if (leftTopRuler) leftTopRuler->setScale(newScale);
            if (leftLeftRuler) leftLeftRuler->setScale(newScale);
            if (rightTopRuler) rightTopRuler->setScale(newScale);
            if (rightLeftRuler) rightLeftRuler->setScale(newScale);
            
            statusLabel->setText(QString("Escala configurada: %1 px/mm (%2 DPI)")
                                .arg(newScale, 0, 'f', 2)
                                .arg(dialog.getDPI(), 0, 'f', 0));
            
            QMessageBox::information(this, "Configuração de Escala",
                QString("Escala atualizada:\n\n"
                        "Pixels/mm: %1\n"
                        "DPI: %2\n"
                        "Distância entre cristas: %3 mm\n"
                        "Cristas por 10mm: %4")
                    .arg(newScale, 0, 'f', 2)
                    .arg(dialog.getDPI(), 0, 'f', 0)
                    .arg(dialog.getRidgeSpacing(), 0, 'f', 4)
                    .arg(dialog.getRidgesPer10mm(), 0, 'f', 2));
        }
    }
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

void MainWindow::toggleRulers() {
    rulersVisible = !rulersVisible;
    
    if (leftTopRuler) leftTopRuler->setVisible(rulersVisible);
    if (leftLeftRuler) leftLeftRuler->setVisible(rulersVisible);
    if (rightTopRuler) rightTopRuler->setVisible(rulersVisible);
    if (rightLeftRuler) rightLeftRuler->setVisible(rulersVisible);
    
    statusLabel->setText(rulersVisible ? "Réguas exibidas" : "Réguas ocultas");
}

void MainWindow::onCalibrationCompleted(double scale, double confidence) {
    if (scaleCalibrationTool) {
        scaleCalibrationTool->deactivate();
    }
    
    // Aplicar escala
    imageProcessor->setScale(scale);
    
    // SALVAR ESCALA NO FRAGMENTO (se estiver calibrando um fragmento)
    QString activeEntityId = activePanel ? rightPanelEntityId : leftPanelEntityId;
    CurrentEntityType activeEntityType = activePanel ? rightPanelEntityType : leftPanelEntityType;
    
    bool savedToFragment = false;
    FingerprintEnhancer::ProjectManager& pm = FingerprintEnhancer::ProjectManager::instance();
    if (activeEntityType == ENTITY_FRAGMENT && pm.hasOpenProject()) {
        FingerprintEnhancer::Project* proj = pm.getCurrentProject();
        FingerprintEnhancer::Fragment* fragment = proj ? proj->findFragment(activeEntityId) : nullptr;
        if (fragment) {
            fragment->pixelsPerMM = scale;
            fragment->dpi = scale * 25.4;  // Salvar DPI também
            fragment->modifiedAt = QDateTime::currentDateTime();
            proj->setModified();
            savedToFragment = true;
            fprintf(stderr, "[CALIBRATION] Escala %.2f px/mm (%.1f DPI) salva no fragmento %s\n",
                    scale, fragment->dpi, fragment->id.toStdString().c_str());
        }
    }
    
    // Atualizar réguas com nova escala
    if (leftTopRuler) leftTopRuler->setScale(scale);
    if (leftLeftRuler) leftLeftRuler->setScale(scale);
    if (rightTopRuler) rightTopRuler->setScale(scale);
    if (rightLeftRuler) rightLeftRuler->setScale(scale);
    
    // Mostrar resultado
    double dpi = scale * 25.4;
    QString contextInfo = savedToFragment ? 
        "\n\n✓ Escala salva no fragmento" : 
        "\n\n⚠️ Calibração em imagem digital (não salva no fragmento)";
    
    QString message = QString(
        "✓ Calibração concluída com sucesso!\n\n"
        "Escala: %1 pixels/mm\n"
        "Resolução: %2 DPI\n"
        "Confiança: %3%%"
        "%4\n\n"
        "As réguas foram atualizadas.")
        .arg(scale, 0, 'f', 2)
        .arg(dpi, 0, 'f', 0)
        .arg(confidence, 0, 'f', 0)
        .arg(contextInfo);
    
    QMessageBox::information(this, "Calibração Concluída", message);
    
    statusLabel->setText(QString("Escala calibrada: %1 px/mm (%2 DPI)%3")
                        .arg(scale, 0, 'f', 2)
                        .arg(dpi, 0, 'f', 0)
                        .arg(savedToFragment ? " [Fragmento]" : ""));
    
    // Sugerir exibir réguas se estiverem ocultas
    if (!rulersVisible) {
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
            "Exibir Réguas",
            "Deseja exibir as réguas métricas nos painéis?",
            QMessageBox::Yes | QMessageBox::No);
        
        if (reply == QMessageBox::Yes) {
            toggleRulers();
        }
    }
    
    // Voltar ao modo normal
    setToolMode(MODE_NONE);
}

void MainWindow::onCalibrationCancelled() {
    statusLabel->setText("Calibração de escala cancelada");
    setToolMode(MODE_NONE);
}

void MainWindow::onCalibrationLineDrawn(QPoint start, QPoint end, double distance) {
    statusLabel->setText(QString("Linha desenhada: %1 pixels - Agora clique nas cristas")
                        .arg(distance, 0, 'f', 1));
}

void MainWindow::onCalibrationRidgeCountChanged(int count) {
    statusLabel->setText(QString("📏 Cristas marcadas: %1 (mínimo 2 requerido)")
                        .arg(count));
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
        case MODE_NONE:
            // Menu genérico baseado na entidade corrente
            if (currentEntityType == ENTITY_IMAGE) {
                contextMenu.addAction("📐 Destacar Imagem Inteira como Fragmento",
                                     this, &MainWindow::createFragmentFromWholeImage);
            } else if (currentEntityType == ENTITY_FRAGMENT) {
                contextMenu.addAction("⚡ Inserção Rápida (sem classificar)", [this, imagePos]() {
                    addMinutiaQuickly(imagePos);
                });
                contextMenu.addAction("➕ Adicionar Minúcia Aqui (com diálogo)", [this, imagePos]() {
                    setToolMode(MODE_ADD_MINUTIA);
                    addMinutiaAtPosition(imagePos);
                });
            } else {
                contextMenu.addAction("Selecione uma imagem ou fragmento primeiro");
            }
            break;

        case MODE_CROP:
            if (processedImageViewer->hasCropSelection()) {
                contextMenu.addAction("✓ Aplicar Recorte", this, &MainWindow::applyCrop);
                contextMenu.addAction("Ajustar Seleção", this, &MainWindow::onCropAdjust);
                contextMenu.addSeparator();
                contextMenu.addAction("✗ Cancelar Recorte", this, &MainWindow::cancelCropSelection);
            } else {
                contextMenu.addAction("Desenhe uma área para recortar");
            }
            break;

        case MODE_ADD_MINUTIA:
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

        case MODE_EDIT_MINUTIA:
            contextMenu.addAction("Clique em uma minúcia para editar");
            contextMenu.addSeparator();
            contextMenu.addAction("Adicionar Nova Minúcia", [this, imagePos]() {
                if (currentEntityType == ENTITY_FRAGMENT) {
                    addMinutiaAtPosition(imagePos);
                }
            });
            break;

        case MODE_REMOVE_MINUTIA:
            contextMenu.addAction("Clique em uma minúcia para remover");
            break;

        case MODE_PAN:
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
    setToolMode(MODE_ADD_MINUTIA);
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

void MainWindow::toggleShowFragmentRegions() {
    // Alternar estado
    bool newState = !leftFragmentRegionsOverlay->isShowingRegions();
    
    leftFragmentRegionsOverlay->setShowRegions(newState);
    rightFragmentRegionsOverlay->setShowRegions(newState);
    
    if (newState) {
        statusLabel->setText("✓ Exibindo regiões de origem dos fragmentos");
    } else {
        statusLabel->setText("✗ Regiões de fragmentos ocultadas");
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
    using PM = FingerprintEnhancer::ProjectManager;
    
    // Abrir dialog de comparação 1:1 de fragmentos
    FragmentComparisonDialog* dialog = new FragmentComparisonDialog(this);
    dialog->setProject(PM::instance().getCurrentProject());
    dialog->exec();
    dialog->deleteLater();
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

void MainWindow::onImageSelected(const QString& imageId) {
    // Carregar imagem no painel ativo
    setCurrentEntity(imageId, ENTITY_IMAGE);
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
    using PM = FingerprintEnhancer::ProjectManager;
    
    // Encontrar a minúcia e seu fragmento pai
    FingerprintEnhancer::Minutia* minutia = PM::instance().getCurrentProject()->findMinutia(minutiaId);
    if (!minutia) {
        return;
    }
    
    // Encontrar o fragmento que contém esta minúcia
    QString fragmentId;
    for (auto& img : PM::instance().getCurrentProject()->images) {
        for (auto& frag : img.fragments) {
            if (frag.findMinutia(minutiaId)) {
                fragmentId = frag.id;
                break;
            }
        }
        if (!fragmentId.isEmpty()) break;
    }
    
    if (!fragmentId.isEmpty()) {
        // Carregar o fragmento pai no painel ativo
        setCurrentEntity(fragmentId, ENTITY_FRAGMENT);
        
        // Selecionar a minúcia no overlay
        FingerprintEnhancer::MinutiaeOverlay* activeOverlay = getActiveOverlay();
        activeOverlay->setSelectedMinutia(minutiaId);
        activeOverlay->update();
        
        statusLabel->setText(QString("Minúcia selecionada: %1 (%2)")
                            .arg(minutia->getTypeName())
                            .arg(minutiaId.left(8)));
    }
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

void MainWindow::onMinutiaAngleChanged(const QString& minutiaId, float newAngle) {
    using PM = FingerprintEnhancer::ProjectManager;
    
    FingerprintEnhancer::Minutia* minutia = PM::instance().getCurrentProject()->findMinutia(minutiaId);
    if (minutia) {
        minutia->angle = newAngle;
        minutia->modifiedAt = QDateTime::currentDateTime();
        PM::instance().getCurrentProject()->setModified();
        
        // Atualizar ambos os overlays
        leftMinutiaeOverlay->update();
        rightMinutiaeOverlay->update();
        
        statusLabel->setText(QString("Ângulo da minúcia: %1°").arg(static_cast<int>(newAngle)));
    }
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
    
    // Tornar overlays visíveis (exceto no modo crop onde serão escondidos)
    leftMinutiaeOverlay->setVisible(true);
    rightMinutiaeOverlay->setVisible(true);
    
    // Desativar calibração se ativa
    if (mode != MODE_CALIBRATE_SCALE) {
        deactivateScaleCalibrationMode();
    }
    
    // Atualizar estado dos botões da toolbar
    noneToolAction->setChecked(mode == MODE_NONE);
    cropToolAction->setChecked(mode == MODE_CROP);
    addMinutiaAction->setChecked(mode == MODE_ADD_MINUTIA);

    // Obter viewer e overlay ativos
    ImageViewer* activeViewer = getActiveViewer();
    FingerprintEnhancer::MinutiaeOverlay* activeOverlay = getActiveOverlay();

    // Ativar o modo selecionado apenas no painel ativo
    switch (mode) {
        case MODE_NONE:
            statusLabel->setText("Ferramenta: Selecionar (navegação)");
            break;

        case MODE_CROP:
            activeViewer->setCropMode(true);
            // Desabilitar overlay completamente no modo crop
            if (activeOverlay) {
                activeOverlay->setVisible(false);
                activeOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);
                activeOverlay->lower();  // Colocar overlay atrás
            }
            statusLabel->setText("Ferramenta: Recortar - Clique e arraste para selecionar área");
            break;

        case MODE_ADD_MINUTIA:
            activeOverlay->setEditMode(false);
            statusLabel->setText("Ferramenta: Adicionar Minúcia - Clique para marcar posição");
            break;

        case MODE_EDIT_MINUTIA:
            activeOverlay->setEditMode(true);
            statusLabel->setText("Ferramenta: Editar Minúcia - Clique para selecionar, arraste para mover");
            break;

        case MODE_REMOVE_MINUTIA:
            activeOverlay->setEditMode(false);
            statusLabel->setText("Ferramenta: Remover Minúcia - Clique na minúcia para remover");
            break;

        case MODE_PAN:
            statusLabel->setText("Ferramenta: Pan - Arraste para mover a imagem");
            break;
            
        case MODE_CALIBRATE_SCALE:
            // Ativar ferramenta de calibração
            activateScaleCalibrationMode();
            statusLabel->setText("📏 Calibração de Escala - Desenhe linha através das cristas");
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
    // Função não mais usada - movimento agora é feito clicando e arrastando dentro da seleção
    return;
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
        fragmentManager->selectFragment(fragment->id);
        getActiveViewer()->zoomToFit();
        statusLabel->setText("✓ Imagem inteira destacada como fragmento e selecionada");
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
    
    // Atualizar estado do programa baseado no tipo de entidade
    if (type == ENTITY_IMAGE) {
        setProgramState(STATE_IMAGE);
    } else if (type == ENTITY_FRAGMENT) {
        setProgramState(STATE_FRAGMENT);
    } else {
        setProgramState(STATE_NONE);
    }

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
    FragmentRegionsOverlay* activeFragmentOverlay = activePanel ? rightFragmentRegionsOverlay : leftFragmentRegionsOverlay;

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
    
    // Configurar fragment regions overlay se for imagem
    if (currentEntityType == ENTITY_IMAGE) {
        FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(currentEntityId);
        if (img) {
            activeFragmentOverlay->setImage(img);
            activeFragmentOverlay->setScaleFactor(activeViewer->getScaleFactor());
            
            // Inicializar offsets
            QPoint scrollOffset(activeViewer->horizontalScrollBar()->value(),
                               activeViewer->verticalScrollBar()->value());
            activeFragmentOverlay->setScrollOffset(scrollOffset);
            activeFragmentOverlay->setImageOffset(activeViewer->getImageOffset());
            activeFragmentOverlay->update();
        }
    } else {
        activeFragmentOverlay->setImage(nullptr);
        activeFragmentOverlay->update();
    }
    
    // Aplicar ajuste ao tamanho do painel
    activeViewer->zoomToFit();
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

QRect MainWindow::convertRotatedToOriginalCoords(const QRect& rotatedRect, double currentAngle,
                                                  const QSize& currentSize, const QSize& originalSize) {
    // Se não há rotação, retornar direto
    if (fabs(currentAngle) < 0.1) {
        return rotatedRect;
    }
    
    // Normalizar ângulo para múltiplos de 90°
    int angle90 = static_cast<int>(round(currentAngle / 90.0)) % 4;
    if (angle90 < 0) angle90 += 4;
    
    // Converter para coordenadas originais baseado no ângulo
    QRect result;
    
    switch (angle90) {
        case 0: // 0° - sem rotação
            result = rotatedRect;
            break;
            
        case 1: // 90° CW (ou -270°)
            // Após 90° CW: x' = y, y' = width_orig - x - w
            result = QRect(
                currentSize.height() - rotatedRect.y() - rotatedRect.height(),
                rotatedRect.x(),
                rotatedRect.height(),
                rotatedRect.width()
            );
            break;
            
        case 2: // 180°
            // Após 180°: x' = w - x - w_rect, y' = h - y - h_rect
            result = QRect(
                currentSize.width() - rotatedRect.x() - rotatedRect.width(),
                currentSize.height() - rotatedRect.y() - rotatedRect.height(),
                rotatedRect.width(),
                rotatedRect.height()
            );
            break;
            
        case 3: // 270° CW (ou -90° / 90° CCW)
            // Após 270° CW: x' = width_curr - y - h, y' = x
            result = QRect(
                rotatedRect.y(),
                currentSize.width() - rotatedRect.x() - rotatedRect.width(),
                rotatedRect.height(),
                rotatedRect.width()
            );
            break;
    }
    
    return result;
}

QRect MainWindow::convertOriginalToRotatedCoords(const QRect& originalRect, double currentAngle,
                                                  const QSize& originalSize, const QSize& currentSize) {
    // Se não há rotação, retornar direto
    if (fabs(currentAngle) < 0.1) {
        return originalRect;
    }
    
    // Normalizar ângulo para múltiplos de 90°
    int angle90 = static_cast<int>(round(currentAngle / 90.0)) % 4;
    if (angle90 < 0) angle90 += 4;
    
    // Converter de coordenadas originais para rotacionadas
    QRect result;
    
    switch (angle90) {
        case 0: // 0° - sem rotação
            result = originalRect;
            break;
            
        case 1: // 90° CW
            // x_rot = y_orig, y_rot = width_orig - x_orig - w_orig
            result = QRect(
                originalRect.y(),
                originalSize.width() - originalRect.x() - originalRect.width(),
                originalRect.height(),
                originalRect.width()
            );
            break;
            
        case 2: // 180°
            result = QRect(
                originalSize.width() - originalRect.x() - originalRect.width(),
                originalSize.height() - originalRect.y() - originalRect.height(),
                originalRect.width(),
                originalRect.height()
            );
            break;
            
        case 3: // 270° CW (90° CCW)
            result = QRect(
                originalSize.height() - originalRect.y() - originalRect.height(),
                originalRect.x(),
                originalRect.height(),
                originalRect.width()
            );
            break;
    }
    
    return result;
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
    } else if (currentEntityType == ENTITY_IMAGE && !currentEntityId.isEmpty()) {
        FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(currentEntityId);
        if (img) {
            cv::flip(img->workingImage, img->workingImage, 1); // 1 = horizontal
            img->addFlipHorizontal(); // Registrar no histórico
            loadCurrentEntityToView();
            PM::instance().getCurrentProject()->setModified();
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
    } else if (currentEntityType == ENTITY_IMAGE && !currentEntityId.isEmpty()) {
        FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(currentEntityId);
        if (img) {
            cv::flip(img->workingImage, img->workingImage, 0); // 0 = vertical
            img->addFlipVertical(); // Registrar no histórico
            loadCurrentEntityToView();
            PM::instance().getCurrentProject()->setModified();
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

    // Selecionar item anterior antes de remover
    fragmentManager->selectPreviousItem();
    
    // Remover fragmento da imagem pai
    FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(parentImageId);
    if (img) {
        img->removeFragment(fragmentId);
        PM::instance().getCurrentProject()->setModified();

        // Atualizar view do gerenciador
        fragmentManager->updateView();

        statusLabel->setText(QString("✓ Fragmento excluído (%1 minúcia(s) removida(s))").arg(minutiaeCount));
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

void MainWindow::onDeleteImageRequested(const QString& imageId) {
    using PM = FingerprintEnhancer::ProjectManager;

    FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(imageId);
    if (!img) {
        QMessageBox::warning(this, "Erro", "Imagem não encontrada");
        return;
    }

    int fragmentCount = img->fragments.size();
    int totalMinutiae = 0;
    for (const auto& frag : img->fragments) {
        totalMinutiae += frag.getMinutiaeCount();
    }

    QString message = QString("Deseja realmente remover esta imagem do projeto?\\n\\nImagem: %1")
                      .arg(QFileInfo(img->originalFilePath).fileName());
    
    if (fragmentCount > 0 || totalMinutiae > 0) {
        message += QString("\\n\\nAVISO: Esta imagem possui:\\n- %1 fragmento(s)\\n- %2 minúcia(s)\\n\\nTodos serão excluídos!")
                  .arg(fragmentCount).arg(totalMinutiae);
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Confirmar Remoção",
        message,
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply != QMessageBox::Yes) {
        return;
    }

    // Se alguma entidade desta imagem está sendo visualizada, limpar
    bool needsClear = false;
    if (currentEntityType == ENTITY_IMAGE && currentEntityId == imageId) {
        needsClear = true;
    } else if (currentEntityType == ENTITY_FRAGMENT) {
        FingerprintEnhancer::Fragment* frag = PM::instance().getCurrentProject()->findFragment(currentEntityId);
        if (frag && frag->parentImageId == imageId) {
            needsClear = true;
        }
    }

    if (needsClear) {
        currentEntityType = ENTITY_NONE;
        currentEntityId.clear();
        currentImageId.clear();
        currentFragmentId.clear();
        processedImageViewer->clearImage();
        secondImageViewer->clearImage();
        minutiaeOverlay->setFragment(nullptr);
        leftMinutiaeOverlay->setFragment(nullptr);
        rightMinutiaeOverlay->setFragment(nullptr);
    }

    // Selecionar item anterior antes de remover
    fragmentManager->selectPreviousItem();
    
    // Remover imagem do projeto
    PM::instance().getCurrentProject()->removeImage(imageId);
    PM::instance().getCurrentProject()->setModified();

    // Atualizar view do gerenciador
    fragmentManager->updateView();

    statusLabel->setText(QString("✓ Imagem removida (%1 fragmento(s) e %2 minúcia(s) excluídos)")
                        .arg(fragmentCount).arg(totalMinutiae));
}

void MainWindow::onSendToLeftPanel(const QString& entityId, bool isFragment) {
    CurrentEntityType type = isFragment ? ENTITY_FRAGMENT : ENTITY_IMAGE;
    loadEntityToPanel(entityId, type, false);  // false = painel esquerdo
}

void MainWindow::onSendToRightPanel(const QString& entityId, bool isFragment) {
    CurrentEntityType type = isFragment ? ENTITY_FRAGMENT : ENTITY_IMAGE;
    loadEntityToPanel(entityId, type, true);   // true = painel direito
}

void MainWindow::onDuplicateFragmentRequested(const QString& fragmentId) {
    using PM = FingerprintEnhancer::ProjectManager;

    FingerprintEnhancer::Fragment* originalFrag = PM::instance().getCurrentProject()->findFragment(fragmentId);
    if (!originalFrag) {
        QMessageBox::warning(this, "Erro", "Fragmento não encontrado");
        return;
    }

    // Encontrar a imagem pai
    FingerprintEnhancer::FingerprintImage* parentImg = PM::instance().getCurrentProject()->findImage(originalFrag->parentImageId);
    if (!parentImg) {
        QMessageBox::warning(this, "Erro", "Imagem pai não encontrada");
        return;
    }

    // Criar cópia do fragmento
    FingerprintEnhancer::Fragment newFragment;
    newFragment.id = QUuid::createUuid().toString();
    newFragment.parentImageId = originalFrag->parentImageId;
    newFragment.sourceRect = originalFrag->sourceRect;
    newFragment.originalImage = originalFrag->originalImage.clone();
    newFragment.workingImage = originalFrag->workingImage.clone();
    newFragment.createdAt = QDateTime::currentDateTime();
    newFragment.modifiedAt = QDateTime::currentDateTime();
    
    // Copiar minúcias
    for (const auto& minutia : originalFrag->minutiae) {
        FingerprintEnhancer::Minutia newMinutia = minutia;
        newMinutia.id = QUuid::createUuid().toString();
        newMinutia.createdAt = QDateTime::currentDateTime();
        newMinutia.modifiedAt = QDateTime::currentDateTime();
        newFragment.minutiae.push_back(newMinutia);
    }

    // Adicionar fragmento à imagem (diretamente ao vetor)
    parentImg->fragments.append(newFragment);
    parentImg->modifiedAt = QDateTime::currentDateTime();
    PM::instance().getCurrentProject()->setModified();

    // Atualizar view
    fragmentManager->updateView();

    statusLabel->setText(QString("Fragmento duplicado (%1 minúcia(s) copiada(s))")
                        .arg(originalFrag->getMinutiaeCount()));
}

void MainWindow::onExportImageRequested(const QString& imageId) {
    using PM = FingerprintEnhancer::ProjectManager;

    FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(imageId);
    if (!img) {
        QMessageBox::warning(this, "Erro", "Imagem não encontrada");
        return;
    }

    QFileInfo originalFileInfo(img->originalFilePath);
    QString suggestedName = originalFileInfo.baseName() + "_processada." + originalFileInfo.suffix();
    
    // Usar diretório do projeto como padrão
    QString projectDir = getProjectDirectory();
    QString defaultPath = projectDir + "/" + suggestedName;

    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Exportar Imagem Processada",
        defaultPath,
        "PNG (*.png);;JPEG (*.jpg *.jpeg);;TIFF (*.tif *.tiff);;BMP (*.bmp);;Todos (*.*)"
    );

    if (fileName.isEmpty()) {
        return;
    }

    // Usar workingImage (processada) em vez da original
    cv::Mat imageToSave = img->workingImage;
    
    if (imageToSave.empty()) {
        QMessageBox::warning(this, "Erro", "Imagem de trabalho vazia");
        return;
    }

    try {
        if (cv::imwrite(fileName.toStdString(), imageToSave)) {
            statusLabel->setText(QString("Imagem exportada: %1").arg(QFileInfo(fileName).fileName()));
            QMessageBox::information(this, "Sucesso", 
                QString("Imagem exportada com sucesso!\n\nArquivo: %1\nTamanho: %2×%3 pixels")
                    .arg(fileName)
                    .arg(imageToSave.cols)
                    .arg(imageToSave.rows));
        } else {
            QMessageBox::warning(this, "Erro", "Não foi possível salvar a imagem");
        }
    } catch (const cv::Exception& e) {
        QMessageBox::critical(this, "Erro", QString("Erro ao exportar imagem: %1").arg(e.what()));
    }
}

void MainWindow::onExportFragmentRequested(const QString& fragmentId) {
    // TODO: Implementar exportação avançada com FragmentExportDialog
    QMessageBox::information(this, "Exportar Fragmento", 
        "Funcionalidade de exportação avançada temporariamente desativada.\n"
        "Use o botão Exportar do menu principal.");
}

void MainWindow::onEditImagePropertiesRequested(const QString& imageId) {
    using PM = FingerprintEnhancer::ProjectManager;
    
    FingerprintEnhancer::FingerprintImage* img = PM::instance().getCurrentProject()->findImage(imageId);
    if (!img) {
        QMessageBox::warning(this, "Erro", "Imagem não encontrada");
        return;
    }
    
    FingerprintEnhancer::ImagePropertiesDialog dialog(img, this);
    if (dialog.exec() == QDialog::Accepted) {
        // Atualizar comentários
        img->notes = dialog.getComments();
        img->modifiedAt = QDateTime::currentDateTime();
        PM::instance().getCurrentProject()->setModified();
        
        // Atualizar visualização da árvore
        fragmentManager->updateView();
        
        statusLabel->setText("Propriedades da imagem atualizadas");
    }
}

void MainWindow::onEditFragmentPropertiesRequested(const QString& fragmentId) {
    using PM = FingerprintEnhancer::ProjectManager;
    
    FingerprintEnhancer::Fragment* frag = PM::instance().getCurrentProject()->findFragment(fragmentId);
    if (!frag) {
        QMessageBox::warning(this, "Erro", "Fragmento não encontrado");
        return;
    }
    
    FingerprintEnhancer::FragmentPropertiesDialog dialog(frag, this);
    if (dialog.exec() == QDialog::Accepted) {
        // Atualizar comentários
        frag->notes = dialog.getComments();
        frag->modifiedAt = QDateTime::currentDateTime();
        PM::instance().getCurrentProject()->setModified();
        
        // Atualizar visualização da árvore
        fragmentManager->updateView();
        
        statusLabel->setText("Propriedades do fragmento atualizadas");
    }
}

// ========== Gerenciamento de Estados do Programa ==========

void MainWindow::setProgramState(ProgramState newState) {
    qDebug() << "🔄 setProgramState:" << currentProgramState << "→" << newState;
    
    if (currentProgramState == newState) {
        qDebug() << "  ℹ️  Estado já é" << newState << ", ignorando";
        return;
    }
    
    currentProgramState = newState;
    updateUIForCurrentState();
    
    qDebug() << "  ✅ Estado mudou para:" << newState;
}

void MainWindow::updateUIForCurrentState() {
    qDebug() << "🎨 updateUIForCurrentState - Estado:" << currentProgramState;
    
    ImageViewer* activeViewer = getActiveViewer();
    FingerprintEnhancer::MinutiaeOverlay* activeOverlay = getActiveOverlay();
    
    switch (currentProgramState) {
        case STATE_NONE:
            qDebug() << "  📭 STATE_NONE - Nenhuma seleção";
            // Overlay passa eventos (não captura)
            if (activeOverlay) {
                activeOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);
                activeOverlay->setEditMode(false);
            }
            // ImageViewer ativo normalmente
            if (activeViewer) {
                activeViewer->setEnabled(true);
            }
            statusLabel->setText("Pronto");
            break;
            
        case STATE_IMAGE:
            qDebug() << "  🖼️  STATE_IMAGE - Imagem selecionada";
            // Overlay passa eventos
            if (activeOverlay) {
                activeOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);
                activeOverlay->setEditMode(false);
            }
            // ImageViewer: zoom, pan, crop, realces
            if (activeViewer) {
                activeViewer->setEnabled(true);
            }
            statusLabel->setText("Imagem selecionada - Operações: zoom, pan, crop, realces");
            break;
            
        case STATE_FRAGMENT:
            qDebug() << "  📄 STATE_FRAGMENT - Fragmento selecionado";
            // Overlay passa eventos (permite zoom/pan)
            if (activeOverlay) {
                activeOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);
                activeOverlay->setEditMode(false);
            }
            // ImageViewer: tudo de imagem + adicionar minúcias
            if (activeViewer) {
                activeViewer->setEnabled(true);
            }
            statusLabel->setText("Fragmento selecionado - Adicione minúcias ou edite imagem");
            break;
            
        case STATE_MINUTIA_EDITING:
            qDebug() << "  ✏️  STATE_MINUTIA_EDITING - Editando minúcia";
            // Overlay CAPTURA eventos (não passa para ImageViewer)
            if (activeOverlay) {
                activeOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, false);
                activeOverlay->setEditMode(true);
            }
            // ImageViewer: O overlay vai capturar eventos, então zoom/pan com mouse ficam desabilitados automaticamente
            // (scroll bars continuam funcionando)
            statusLabel->setText("🎯 MODO DE EDIÇÃO DE MINÚCIA ATIVO - Arraste para mover/rotacionar");
            break;
    }
    
    qDebug() << "  ✅ UI atualizada para estado:" << currentProgramState;
}

void MainWindow::enableMinutiaEditingMode(bool enable) {
    qDebug() << "🎯 enableMinutiaEditingMode:" << enable;
    
    if (enable) {
        // Só permite se há uma minúcia selecionada
        FingerprintEnhancer::MinutiaeOverlay* overlay = getActiveOverlay();
        if (overlay && !overlay->getSelectedMinutiaId().isEmpty()) {
            setProgramState(STATE_MINUTIA_EDITING);
            qDebug() << "  ✅ Modo de edição de minúcia ATIVADO";
        } else {
            qDebug() << "  ⚠️  Nenhuma minúcia selecionada, não pode ativar modo de edição";
            QMessageBox::information(this, "Edição de Minúcia", 
                "Selecione uma minúcia primeiro (clique na árvore ou duplo clique na imagem)");
        }
    } else {
        // Volta para o estado anterior (fragmento ou imagem)
        if (currentEntityType == ENTITY_FRAGMENT) {
            setProgramState(STATE_FRAGMENT);
        } else if (currentEntityType == ENTITY_IMAGE) {
            setProgramState(STATE_IMAGE);
        } else {
            setProgramState(STATE_NONE);
        }
        qDebug() << "  ✅ Modo de edição de minúcia DESATIVADO";
    }
}

void MainWindow::toggleInteractiveEditMode(bool enable) {
    fprintf(stderr, "[MAINWINDOW] 🎯 toggleInteractiveEditMode: %s\n", enable ? "true" : "false");
    fflush(stderr);
    
    FingerprintEnhancer::MinutiaeOverlay* activeOverlay = getActiveOverlay();
    if (!activeOverlay) {
        fprintf(stderr, "[MAINWINDOW]   ⚠️  Nenhum overlay ativo\n");
        fflush(stderr);
        return;
    }
    
    fprintf(stderr, "[MAINWINDOW]   📊 Estado do overlay antes:\n");
    fprintf(stderr, "[MAINWINDOW]      - editMode atual: %s\n", activeOverlay->isEditMode() ? "true" : "false");
    fprintf(stderr, "[MAINWINDOW]      - tem fragmento: %s\n", (activeOverlay->getFragment() != nullptr) ? "true" : "false");
    fflush(stderr);
    
    if (enable) {
        // Ativar modo de edição interativa no overlay
        activeOverlay->setEditMode(true);
        activeOverlay->setFocus();  // Dar foco ao overlay para receber eventos de teclado
        statusLabel->setText("🎯 Modo de Edição Interativa ATIVO - Clique em minúcia e botão direito para opções");
        
        fprintf(stderr, "[MAINWINDOW]   ✅ Modo de edição interativa ATIVADO no overlay\n");
        fprintf(stderr, "[MAINWINDOW]   📊 Estado do overlay depois:\n");
        fprintf(stderr, "[MAINWINDOW]      - editMode agora: %s\n", activeOverlay->isEditMode() ? "true" : "false");
        fprintf(stderr, "[MAINWINDOW]      - fragmento: %s\n", (activeOverlay->getFragment() != nullptr) ? "true" : "false");
        fflush(stderr);
        
        // Sincronizar checkboxes
        if (editModeAction && !editModeAction->isChecked()) {
            editModeAction->blockSignals(true);
            editModeAction->setChecked(true);
            editModeAction->blockSignals(false);
        }
        if (editMinutiaToolbarAction && !editMinutiaToolbarAction->isChecked()) {
            editMinutiaToolbarAction->blockSignals(true);
            editMinutiaToolbarAction->setChecked(true);
            editMinutiaToolbarAction->blockSignals(false);
        }
    } else {
        // Desativar modo de edição
        activeOverlay->setEditMode(false);
        statusLabel->setText("Modo de edição interativa desativado");
        qDebug() << "  ✅ Modo de edição interativa DESATIVADO";
        
        // Sincronizar checkboxes
        if (editModeAction && editModeAction->isChecked()) {
            editModeAction->blockSignals(true);
            editModeAction->setChecked(false);
            editModeAction->blockSignals(false);
        }
        if (editMinutiaToolbarAction && editMinutiaToolbarAction->isChecked()) {
            editMinutiaToolbarAction->blockSignals(true);
            editMinutiaToolbarAction->setChecked(false);
            editMinutiaToolbarAction->blockSignals(false);
        }
    }
}
// ============================================================================
// OPERAÇÕES ASSÍNCRONAS - Carregamento e Salvamento
// ============================================================================

QString MainWindow::getProjectDirectory() const {
    using namespace FingerprintEnhancer;
    ProjectManager& pm = ProjectManager::instance();
    
    if (!pm.hasOpenProject()) {
        return QDir::homePath();
    }
    
    QString projectPath = pm.getCurrentProjectPath();
    if (projectPath.isEmpty()) {
        return QDir::homePath();
    }
    
    QFileInfo fileInfo(projectPath);
    return fileInfo.absolutePath();
}

void MainWindow::onImageLoadProgress(int current, int total, const QString& currentFile) {
    progressBar->setValue(current);
    statusLabel->setText(QString("Carregando %1/%2: %3").arg(current).arg(total).arg(currentFile));
    QApplication::processEvents();
}

void MainWindow::onImageLoaded(const QString& filePath, const cv::Mat& image) {
    using namespace FingerprintEnhancer;
    ProjectManager& pm = ProjectManager::instance();
    FingerprintImage* img = pm.addImageToProject(filePath);
    
    if (!img) {
        qWarning() << "Falha ao adicionar imagem ao projeto:" << filePath;
    }
}

void MainWindow::onImageLoadingFailed(const QString& filePath, const QString& error) {
    QFileInfo fileInfo(filePath);
    qWarning() << "Falha ao carregar" << fileInfo.fileName() << ":" << error;
    statusLabel->setText(QString("Erro: %1").arg(fileInfo.fileName()));
}

void MainWindow::onAllImagesLoaded(int successCount, int failCount) {
    isLoadingImages = false;
    progressBar->setVisible(false);
    
    QString message;
    if (failCount == 0) {
        message = QString("✓ %1 imagem(ns) carregada(s) com sucesso!").arg(successCount);
        statusLabel->setText(message);
        QMessageBox::information(this, "Carregamento Concluído", message);
    } else {
        message = QString("Carregamento concluído: %1 sucesso, %2 falha(s)")
                    .arg(successCount).arg(failCount);
        statusLabel->setText(message);
        QMessageBox::warning(this, "Carregamento Concluído", message);
    }

    if (fragmentManager) {
        fragmentManager->updateView();
    }
    updateStatusBar();
}

void MainWindow::onSaveProgress(const QString& message) {
    statusLabel->setText(message);
    QApplication::processEvents();
}

void MainWindow::onSaveCompleted(bool success, const QString& message) {
    isSavingProject = false;
    progressBar->setVisible(false);
    
    if (saveAction) saveAction->setEnabled(true);
    if (saveAsAction) saveAsAction->setEnabled(true);

    if (success) {
        statusLabel->setText("✓ Projeto salvo com sucesso!");
        QMessageBox::information(this, "Salvamento Concluído", message);
        updateWindowTitle();
    } else {
        statusLabel->setText("✗ Falha ao salvar projeto");
        QMessageBox::critical(this, "Erro ao Salvar", message);
    }
}

void MainWindow::onSaveTimeout() {
    statusLabel->setText("✗ Timeout: Salvamento cancelado");
    QMessageBox::critical(this, "Timeout",
                         "O salvamento do projeto demorou mais de 5 minutos e foi cancelado.\n"
                         "Isso pode indicar um projeto muito grande ou problemas no disco.");
}
