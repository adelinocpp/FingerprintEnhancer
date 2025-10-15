#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLoggingCategory>

// Categoria de logging para debug do MainWindow
Q_DECLARE_LOGGING_CATEGORY(mainwindow)

#include <QtWidgets/QMenuBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QStackedLayout>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QTabWidget>
#include <QtCore/QTimer>

#include "../core/ProjectManager.h"
#include "../core/ImageProcessor.h"
#include "../core/MinutiaeExtractor.h"
#include "ImageViewer.h"
#include "MinutiaeEditor.h"
#include "CropTool.h"
#include "MinutiaeMarkerWidget.h"
#include "FragmentManager.h"
#include "MinutiaeOverlay.h"
#include "MinutiaEditDialog.h"
#include "NewProjectDialog.h"
#include "MinutiaeDisplayDialog.h"
#include "ScaleCalibrationTool.h"
#include "RulerWidget.h"
#include "ImageLoaderWorker.h"
#include "ProjectSaverWorker.h"
#include "../afis/AFISMatcher.h"

/**
 * @brief Janela principal da aplicação FingerprintEnhancer
 * 
 * Interface principal que integra todas as funcionalidades de processamento
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    // Estados do programa baseado no que está selecionado
    enum ProgramState {
        STATE_NONE,              // Nenhuma seleção
        STATE_IMAGE,             // Imagem selecionada
        STATE_FRAGMENT,          // Fragmento selecionado
        STATE_MINUTIA_EDITING    // Minúcia em edição interativa
    };
    
    enum ToolMode {
        MODE_NONE,
        MODE_CROP,
        MODE_ADD_MINUTIA,
        MODE_EDIT_MINUTIA,
        MODE_REMOVE_MINUTIA,
        MODE_PAN,
        MODE_CALIBRATE_SCALE
    };

    enum CurrentEntityType {
        ENTITY_NONE = 0,
        ENTITY_IMAGE,      // Imagem completa de arquivo
        ENTITY_FRAGMENT    // Fragmento recortado
    };

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    // Menu File
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void editProjectInfo();
    void clearProject();
    void openImage();
    void saveImage();
    void exportReport();
    void exitApplication();
    
    // Menu Edit
    void resetToOriginal();
    void undoLastOperation();
    
    // Menu Enhancement
    void openFFTDialog();
    void subtractBackground();
    void applyGaussianBlur();
    void applySharpenFilter();
    void adjustBrightnessContrast();
    void applyBrightnessContrast();  // Aplica valores dos sliders
    void equalizeHistogram();
    void applyCLAHE();
    void invertColors();
    void rotateImage();
    void binarizeImage();
    void skeletonizeImage();
    
    // Menu Analysis
    void extractMinutiae();
    void compareMinutiae();
    void generateChart();
    
    // Menu View
    void zoomIn();
    void zoomOut();
    void zoomFit();
    void zoomActual();
    void toggleSideBySide();
    void toggleRightPanel();
    void configureMinutiaeDisplay();

    // Menu Tools
    void changeLanguage();

    // Ferramentas de Recorte
    void activateCropTool();
    void applyCrop();
    void cancelCropSelection();
    void saveCroppedImage();

    // Ferramentas de Rotação
    void rotateRight90();
    void rotateLeft90();
    void rotate180();
    void rotateCustomAngle();

    // Calibração de Escala
    void calibrateScale();
    void activateScaleCalibrationMode();
    void deactivateScaleCalibrationMode();
    void showScaleConfig();
    void setScaleManually();
    void showScaleInfo();
    void toggleRulers();
    
    // Slots para ferramenta de calibração
    void onCalibrationCompleted(double scale, double confidence);
    void onCalibrationCancelled();
    void onCalibrationLineDrawn(QPoint start, QPoint end, double distance);
    void onCalibrationRidgeCountChanged(int count);

    // Conversão de Espaços de Cor
    void convertToRGB();
    void convertToHSV();
    void convertToHSI();
    void convertToLab();
    void convertToGrayscale();
    void adjustColorLevels();

    // Ferramentas de Minúcias
    void activateAddMinutia();
    void activateEditMinutia();
    void activateRemoveMinutia();
    void toggleShowMinutiaeNumbers();
    void toggleShowMinutiaeAngles();
    void toggleShowMinutiaeLabels();
    void saveMinutiaeNumberedImage();
    void clearAllMinutiae();

    // Menu AFIS
    void loadAFISDatabase();
    void identifyFingerprint();
    void verifyFingerprint();
    void configureAFISMatching();
    void showAFISResults();

    // Controles de processamento
    void onBrightnessChanged(int value);
    void onContrastChanged(int value);
    void onGaussianSigmaChanged(double value);
    void onSharpenStrengthChanged(double value);
    void onThresholdChanged(int value);
    
    // Atualização da interface
    void updateImageDisplay();
    void updateProcessingHistory();
    void updateMinutiaeList();

    // Menu de contexto
    void showContextMenu(const QPoint &pos);
    void addMinutiaAtPosition(const QPoint &imagePos);
    void addMinutiaQuickly(const QPoint &imagePos); // Inserção rápida sem diálogo

    // Função auxiliar
    cv::Mat rotateImagePreservingSize(const cv::Mat &image, double angle);
    void updateStatusBar();

    // Processing com threading
    void onProcessingProgress(int percentage);
    void onProcessingCompleted(cv::Mat result);
    void onProcessingFailed(QString errorMessage);
    void onProcessingStatus(QString message);

    // Image loading com threading
    void onImageLoadProgress(int current, int total, const QString& currentFile);
    void onImageLoaded(const QString& filePath, const cv::Mat& image);
    void onImageLoadingFailed(const QString& filePath, const QString& error);
    void onAllImagesLoaded(int successCount, int failCount);

    // Project saving com threading
    void onSaveProgress(const QString& message);
    void onSaveCompleted(bool success, const QString& message);
    void onSaveTimeout();

    // Slots para gerenciamento de projeto
    void onProjectModified();
    void onImageAdded(const QString& imageId);
    void onImageSelected(const QString& imageId);
    void onFragmentCreated(const QString& fragmentId);
    void onFragmentSelected(const QString& fragmentId);
    void onMinutiaAdded(const QString& minutiaId);
    void onMinutiaSelected(const QString& minutiaId);
    void onMinutiaDoubleClicked(const QString& minutiaId);
    void onMinutiaPositionChanged(const QString& minutiaId, const QPoint& newPos);
    void onMinutiaAngleChanged(const QString& minutiaId, float newAngle);
    void onDeleteImageRequested(const QString& imageId);
    void onSendToLeftPanel(const QString& entityId, bool isFragment);
    void onSendToRightPanel(const QString& entityId, bool isFragment);
    void onDuplicateFragmentRequested(const QString& fragmentId);
    void onExportImageRequested(const QString& imageId);
    void onExportFragmentRequested(const QString& fragmentId);
    void onEditImagePropertiesRequested(const QString& imageId);
    void onEditFragmentPropertiesRequested(const QString& fragmentId);

    // Slots para ferramentas
    void setToolMode(ToolMode mode);
    void onCropAdjust();
    void onCropMove();
    void createFragmentFromWholeImage();

    // Slots para gerenciamento de entidade corrente
    void setCurrentEntity(const QString& entityId, CurrentEntityType type);
    void loadCurrentEntityToView();
    cv::Mat& getCurrentWorkingImage();  // Retorna referência para workingImage da entidade corrente
    void applyOperationToCurrentEntity(std::function<void(cv::Mat&)> operation);

    // Gerenciamento de painéis duais
    void switchActivePanel();
    void toggleRightViewer();
    void setActivePanel(bool rightPanel);  // false = esquerdo, true = direito
    ImageViewer* getActiveViewer();
    FingerprintEnhancer::MinutiaeOverlay* getActiveOverlay();
    void loadEntityToPanel(const QString& entityId, CurrentEntityType type, bool targetPanel);
    void updateActivePanelEntity();
    QWidget* createViewerContainer(ImageViewer* viewer, FingerprintEnhancer::MinutiaeOverlay* overlay);
    void clearActivePanel();
    void showViewerContextMenu(const QPoint& pos, bool isLeftPanel);

    // Espelhamento
    void flipHorizontal();
    void flipVertical();

    // Handlers do FragmentManager
    void onViewOriginalRequested(const QString& entityId, bool isFragment);
    void onResetWorkingRequested(const QString& entityId, bool isFragment);
    void onDeleteFragmentRequested(const QString& fragmentId);
    void onDeleteMinutiaRequested(const QString& minutiaId);
    
    // Gerenciamento de estados do programa
    void setProgramState(ProgramState newState);
    ProgramState getProgramState() const { return currentProgramState; }
    void updateUIForCurrentState();
    void enableMinutiaEditingMode(bool enable);
    void toggleInteractiveEditMode(bool enable);

private:
    // Componentes principais (sem ProjectManager - usar singleton)
    ImageProcessor* imageProcessor;
    MinutiaeExtractor* minutiaeExtractor;

    // Layout central
    QWidget *centralWidget;
    QSplitter *mainSplitter;

    // Visualizadores de imagem (dois painéis)
    ImageViewer *processedImageViewer;  // Painel esquerdo
    ImageViewer *secondImageViewer;     // Painel direito
    MinutiaeEditor *minutiaeEditor;

    // Containers e overlays para os dois painéis
    QWidget *leftViewerContainer;
    QWidget *rightViewerContainer;
    FingerprintEnhancer::MinutiaeOverlay *leftMinutiaeOverlay;
    FingerprintEnhancer::MinutiaeOverlay *rightMinutiaeOverlay;

    // Controle de painéis
    QSplitter *viewerSplitter;
    bool activePanel;  // false = esquerdo, true = direito

    // Painéis laterais
    QTabWidget *leftPanel;
    QTabWidget *rightPanel;
    
    // Painel de processamento
    QGroupBox *enhancementGroup;
    QSlider *brightnessSlider;
    QSlider *contrastSlider;
    QDoubleSpinBox *gaussianSigma;
    QDoubleSpinBox *sharpenStrength;
    QSlider *thresholdSlider;
    QPushButton *applyFFTButton;
    QPushButton *subtractBgButton;
    QPushButton *binarizeButton;
    QPushButton *skeletonizeButton;
    
    // Painel de análise
    QGroupBox *analysisGroup;
    QPushButton *extractMinutiaeButton;
    QPushButton *compareButton;
    QPushButton *chartButton;
    QListWidget *minutiaeList;
    
    // Histórico de processamento
    QTextEdit *historyDisplay;

    // Ferramentas especializadas
    class CropTool *cropTool;
    class MinutiaeMarkerWidget *minutiaeMarker;
    class AFISMatcher *afisMatcher;
    class ScaleCalibrationTool *scaleCalibrationTool;

    // Réguas métricas
    class RulerWidget *leftTopRuler;
    class RulerWidget *leftLeftRuler;
    class RulerWidget *rightTopRuler;
    class RulerWidget *rightLeftRuler;
    bool rulersVisible;

    // Gerenciador de estado de imagem
    class ImageState *imageState;

    // Novos componentes do sistema de projetos
    FingerprintEnhancer::FragmentManager *fragmentManager;
    FingerprintEnhancer::MinutiaeOverlay *minutiaeOverlay;  // Deprecated - usar leftMinutiaeOverlay/rightMinutiaeOverlay

    // Sistema de Entidade Corrente (para cada painel)
    CurrentEntityType currentEntityType;  // Tipo da entidade sendo exibida/editada no painel ativo
    QString currentEntityId;               // ID da entidade (Image ou Fragment) no painel ativo

    // Entidades para cada painel
    CurrentEntityType leftPanelEntityType;
    QString leftPanelEntityId;
    CurrentEntityType rightPanelEntityType;
    QString rightPanelEntityId;

    // IDs legados (manter por compatibilidade temporária)
    QString currentImageId;
    QString currentFragmentId;

    // Modo de ferramenta ativa
    ToolMode currentToolMode;

    // Configurações de visualização de minúcias
    FingerprintEnhancer::MinutiaeDisplaySettings minutiaeDisplaySettings;

    // Lista de minúcias marcadas (legado - será removido)
    QVector<QPoint> markedMinutiae;
    QVector<QString> minutiaeTypes;

    // Barra de status
    QLabel *statusLabel;
    QLabel *imageInfoLabel;
    QLabel *scaleLabel;
    QProgressBar *progressBar;
    
    // Actions para sincronização
    QAction *editModeAction;           // Menu: Modo de Edição Interativa
    QAction *editMinutiaToolbarAction; // Toolbar: Editar Minúcia
    
    // Actions da toolbar de ferramentas
    QAction *noneToolAction;           // Ferramenta: Selecionar
    QAction *cropToolAction;           // Ferramenta: Recortar
    QAction *addMinutiaAction;         // Ferramenta: Adicionar Minúcia
    
    // Métodos de inicialização
    void setupUI();
    void createMenus();
    void createToolBars();
    void createStatusBar();
    void createCentralWidget();
    void createLeftPanel();
    void createRightPanel();
    void connectSignals();
    
    // Métodos auxiliares
    void updateWindowTitle();
    bool checkUnsavedChanges();
    void showProcessingProgress(const QString &operation);
    void hideProcessingProgress();
    QString formatImageInfo(const cv::Size &size, double scale);
    void runProcessingInThread(std::function<cv::Mat(const cv::Mat&, int&)> processingFunc);
    void applyBrightnessContrastRealtime();
    
    // Membros para controle de estado
    bool sideBySideMode;
    QString currentProjectPath;
    QTimer *updateTimer;
    ProgramState currentProgramState;

    // Threading
    QThread *processingThread;
    class ProcessingWorker *processingWorker;
    bool isProcessing;

    // Image loading threading
    class FingerprintEnhancer::ImageLoaderWorker *imageLoaderWorker;
    bool isLoadingImages;

    // Project saving threading
    class FingerprintEnhancer::ProjectSaverWorker *projectSaverWorker;
    bool isSavingProject;
    QAction *saveAction;
    QAction *saveAsAction;

    // Helper methods
    QString getProjectDirectory() const;
};

#endif // MAINWINDOW_H

