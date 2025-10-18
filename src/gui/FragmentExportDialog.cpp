#include "FragmentExportDialog.h"
#include "../core/UserSettings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QFormLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QColorDialog>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QScrollArea>
#include <QWheelEvent>
#include <QEvent>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

FragmentExportDialog::FragmentExportDialog(
    const FingerprintEnhancer::Fragment* frag,
    double currentScale,
    QWidget *parent,
    const QString& defaultDirectory)
    : QDialog(parent),
      fragment(frag),
      scale(currentScale),
      markerColor(255, 0, 0),      // Vermelho
      textColor(255, 0, 0),        // Vermelho
      labelBgColor(255, 255, 255), // Branco
      previewZoomFactor(1.0),      // Zoom inicial: 100%
      defaultExportDirectory(defaultDirectory),
      updatingResolution(false)    // Inicializar flag de atualização
{
    setWindowTitle("💾 Exportar Fragmento");
    setModal(true);
    
    originalWidth = fragment->workingImage.cols;
    originalHeight = fragment->workingImage.rows;
    
    setupUI();
    loadDefaultSettings();
    updatePreview();
}

FragmentExportDialog::~FragmentExportDialog()
{
}

void FragmentExportDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Layout horizontal: Configurações | Preview
    QHBoxLayout *contentLayout = new QHBoxLayout();
    
    // === COLUNA ESQUERDA: Configurações ===
    QVBoxLayout *settingsLayout = new QVBoxLayout();
    
    // Grupo: Arquivo de Saída
    QGroupBox *fileGroup = new QGroupBox("📁 Arquivo de Saída");
    QFormLayout *fileLayout = new QFormLayout(fileGroup);
    
    QHBoxLayout *pathLayout = new QHBoxLayout();
    filePathEdit = new QLineEdit();
    filePathEdit->setMinimumWidth(300);
    browseButton = new QPushButton("📂 Procurar...");
    connect(browseButton, &QPushButton::clicked, this, &FragmentExportDialog::onBrowseClicked);
    pathLayout->addWidget(filePathEdit);
    pathLayout->addWidget(browseButton);
    fileLayout->addRow("Caminho:", pathLayout);
    
    formatComboBox = new QComboBox();
    formatComboBox->addItems({"PNG (*.png)", "JPEG (*.jpg)", "TIFF (*.tif)", "BMP (*.bmp)"});
    connect(formatComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FragmentExportDialog::onFormatChanged);
    fileLayout->addRow("Formato:", formatComboBox);
    
    QHBoxLayout *qualityLayout = new QHBoxLayout();
    qualitySlider = new QSlider(Qt::Horizontal);
    qualitySlider->setRange(0, 100);
    qualitySlider->setValue(95);
    qualityLabel = new QLabel("95%");
    connect(qualitySlider, &QSlider::valueChanged, [this](int value) {
        qualityLabel->setText(QString("%1%").arg(value));
        onAnySettingChanged();
    });
    qualityLayout->addWidget(qualitySlider);
    qualityLayout->addWidget(qualityLabel);
    fileLayout->addRow("Qualidade:", qualityLayout);
    
    settingsLayout->addWidget(fileGroup);
    
    // Grupo: Resolução
    QGroupBox *resGroup = new QGroupBox("📐 Resolução de Saída");
    QFormLayout *resLayout = new QFormLayout(resGroup);
    
    widthSpinBox = new QSpinBox();
    widthSpinBox->setRange(100, 10000);
    widthSpinBox->setSuffix(" px");
    connect(widthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &FragmentExportDialog::onWidthChanged);
    resLayout->addRow("Largura:", widthSpinBox);
    
    heightSpinBox = new QSpinBox();
    heightSpinBox->setRange(100, 10000);
    heightSpinBox->setSuffix(" px");
    connect(heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &FragmentExportDialog::onHeightChanged);
    resLayout->addRow("Altura:", heightSpinBox);
    
    maintainAspectCheck = new QCheckBox("Manter proporção");
    maintainAspectCheck->setChecked(true);
    maintainAspectCheck->setToolTip("Ao modificar largura ou altura, a outra dimensão será ajustada automaticamente");
    resLayout->addRow("", maintainAspectCheck);
    
    resetResolutionButton = new QPushButton("↶ Restaurar Original");
    connect(resetResolutionButton, &QPushButton::clicked, [this]() {
        widthSpinBox->setValue(originalWidth);
        heightSpinBox->setValue(originalHeight);
    });
    resLayout->addRow("", resetResolutionButton);
    
    settingsLayout->addWidget(resGroup);
    
    // Grupo: Marcações de Minúcias
    QGroupBox *markersGroup = new QGroupBox("🎯 Marcações de Minúcias");
    QFormLayout *markersLayout = new QFormLayout(markersGroup);
    
    includeMinutiaeCheck = new QCheckBox("Incluir marcações de minúcias");
    includeMinutiaeCheck->setChecked(true);
    connect(includeMinutiaeCheck, &QCheckBox::toggled, this, &FragmentExportDialog::onAnySettingChanged);
    markersLayout->addRow("", includeMinutiaeCheck);
    
    markerSizeSpinBox = new QSpinBox();
    markerSizeSpinBox->setRange(10, 200);
    markerSizeSpinBox->setValue(40);
    markerSizeSpinBox->setSuffix(" px");
    markerSizeSpinBox->setToolTip("Diâmetro do círculo da marcação");
    connect(markerSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &FragmentExportDialog::onAnySettingChanged);
    markersLayout->addRow("Tamanho marcação:", markerSizeSpinBox);
    
    // Tipo de símbolo
    symbolComboBox = new QComboBox();
    symbolComboBox->addItem("Círculo Simples");
    symbolComboBox->addItem("Círculo com X");
    symbolComboBox->addItem("Círculo com Seta");
    symbolComboBox->addItem("Círculo com Cruz");
    symbolComboBox->addItem("Triângulo");
    symbolComboBox->addItem("Quadrado");
    symbolComboBox->addItem("Losango");
    symbolComboBox->setCurrentIndex(0);
    symbolComboBox->setToolTip("Forma do símbolo da marcação");
    connect(symbolComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FragmentExportDialog::onAnySettingChanged);
    markersLayout->addRow("Tipo de símbolo:", symbolComboBox);
    
    fontSizeSpinBox = new QSpinBox();
    fontSizeSpinBox->setRange(8, 72);
    fontSizeSpinBox->setValue(24);
    fontSizeSpinBox->setSuffix(" pt");
    fontSizeSpinBox->setToolTip("Tamanho da fonte dos rótulos");
    connect(fontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &FragmentExportDialog::onAnySettingChanged);
    markersLayout->addRow("Tamanho fonte:", fontSizeSpinBox);
    
    lineWidthSpinBox = new QSpinBox();
    lineWidthSpinBox->setRange(1, 20);
    lineWidthSpinBox->setValue(2);
    lineWidthSpinBox->setSuffix(" px");
    lineWidthSpinBox->setToolTip("Largura da linha das marcações");
    connect(lineWidthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &FragmentExportDialog::onAnySettingChanged);
    markersLayout->addRow("Largura linha:", lineWidthSpinBox);
    
    // Posição do rótulo
    labelPositionComboBox = new QComboBox();
    labelPositionComboBox->addItem("Acima", static_cast<int>(FingerprintEnhancer::MinutiaLabelPosition::ABOVE));
    labelPositionComboBox->addItem("Abaixo", static_cast<int>(FingerprintEnhancer::MinutiaLabelPosition::BELOW));
    labelPositionComboBox->addItem("Esquerda", static_cast<int>(FingerprintEnhancer::MinutiaLabelPosition::LEFT));
    labelPositionComboBox->addItem("Direita", static_cast<int>(FingerprintEnhancer::MinutiaLabelPosition::RIGHT));
    labelPositionComboBox->addItem("Ocultar", static_cast<int>(FingerprintEnhancer::MinutiaLabelPosition::HIDDEN));
    labelPositionComboBox->setCurrentIndex(0);
    connect(labelPositionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FragmentExportDialog::onAnySettingChanged);
    markersLayout->addRow("Posição rótulo:", labelPositionComboBox);
    
    settingsLayout->addWidget(markersGroup);
    
    // Grupo: Opções de Visualização
    QGroupBox *displayGroup = new QGroupBox("👁️ Opções de Visualização");
    QVBoxLayout *displayLayout = new QVBoxLayout(displayGroup);
    
    showNumbersCheck = new QCheckBox("Mostrar números das minúcias");
    showNumbersCheck->setChecked(true);
    connect(showNumbersCheck, &QCheckBox::toggled, this, &FragmentExportDialog::onAnySettingChanged);
    displayLayout->addWidget(showNumbersCheck);
    
    showLabelTypeCheck = new QCheckBox("Mostrar tipos das minúcias");
    showLabelTypeCheck->setChecked(true);
    connect(showLabelTypeCheck, &QCheckBox::toggled, this, &FragmentExportDialog::onAnySettingChanged);
    displayLayout->addWidget(showLabelTypeCheck);
    
    showAnglesCheck = new QCheckBox("Mostrar direções");
    showAnglesCheck->setChecked(true);
    connect(showAnglesCheck, &QCheckBox::toggled, this, &FragmentExportDialog::onAnySettingChanged);
    displayLayout->addWidget(showAnglesCheck);
    
    // Cores
    QHBoxLayout *colorsLayout = new QHBoxLayout();
    markerColorButton = new QPushButton("🎨 Marcação");
    markerColorButton->setStyleSheet(QString("background-color: rgb(255, 0, 0); color: white;"));
    connect(markerColorButton, &QPushButton::clicked, this, &FragmentExportDialog::onMarkerColorClicked);
    colorsLayout->addWidget(markerColorButton);
    
    textColorButton = new QPushButton("🎨 Texto");
    textColorButton->setStyleSheet(QString("background-color: rgb(255, 0, 0); color: white;"));
    connect(textColorButton, &QPushButton::clicked, this, &FragmentExportDialog::onTextColorClicked);
    colorsLayout->addWidget(textColorButton);
    
    labelBgColorButton = new QPushButton("🎨 Fundo");
    labelBgColorButton->setStyleSheet(QString("background-color: rgb(255, 255, 255); color: black;"));
    connect(labelBgColorButton, &QPushButton::clicked, this, &FragmentExportDialog::onLabelBgColorClicked);
    colorsLayout->addWidget(labelBgColorButton);
    
    displayLayout->addLayout(colorsLayout);
    
    // Opacidade do rótulo
    QHBoxLayout *opacityLayout = new QHBoxLayout();
    QLabel *opacityLabel = new QLabel("Opacidade rótulo:");
    opacityLayout->addWidget(opacityLabel);
    
    labelOpacitySlider = new QSlider(Qt::Horizontal);
    labelOpacitySlider->setRange(0, 100);
    labelOpacitySlider->setValue(100);
    labelOpacitySlider->setToolTip("Ajustar transparência do fundo do rótulo (0% = transparente, 100% = opaco)");
    connect(labelOpacitySlider, &QSlider::valueChanged, this, &FragmentExportDialog::onLabelOpacityChanged);
    opacityLayout->addWidget(labelOpacitySlider, 1);
    
    labelOpacityLabel = new QLabel("100%");
    labelOpacityLabel->setMinimumWidth(45);
    labelOpacityLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    opacityLayout->addWidget(labelOpacityLabel);
    
    displayLayout->addLayout(opacityLayout);
    
    settingsLayout->addWidget(displayGroup);
    
    settingsLayout->addStretch();
    
    // === COLUNA DIREITA: Preview ===
    QVBoxLayout *previewLayout = new QVBoxLayout();
    
    QLabel *previewTitle = new QLabel("📷 Preview da Exportação");
    previewTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    previewLayout->addWidget(previewTitle);
    
    // Controles de Zoom
    QHBoxLayout *zoomLayout = new QHBoxLayout();
    QLabel *zoomTitleLabel = new QLabel("🔍 Zoom:");
    zoomLayout->addWidget(zoomTitleLabel);
    
    previewZoomSlider = new QSlider(Qt::Horizontal);
    previewZoomSlider->setRange(25, 400);  // 25% a 400%
    previewZoomSlider->setValue(100);
    previewZoomSlider->setTickPosition(QSlider::TicksBelow);
    previewZoomSlider->setTickInterval(25);
    previewZoomSlider->setToolTip("Ajustar zoom do preview (não afeta a exportação)\nTambém funciona com scroll do mouse sobre a imagem");
    connect(previewZoomSlider, &QSlider::valueChanged, this, &FragmentExportDialog::onPreviewZoomChanged);
    zoomLayout->addWidget(previewZoomSlider, 1);
    
    previewZoomLabel = new QLabel("100%");
    previewZoomLabel->setMinimumWidth(50);
    previewZoomLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    zoomLayout->addWidget(previewZoomLabel);
    
    previewLayout->addLayout(zoomLayout);
    
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(false);  // Mudado para false para permitir scroll
    scrollArea->setMinimumSize(400, 400);
    
    previewLabel = new QLabel();
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setStyleSheet("QLabel { background-color: #2b2b2b; border: 2px solid #555; }");
    previewLabel->setMinimumSize(380, 380);
    scrollArea->setWidget(previewLabel);
    
    // Instalar event filter para capturar scroll do mouse
    scrollArea->installEventFilter(this);
    
    previewLayout->addWidget(scrollArea);
    
    infoLabel = new QLabel();
    infoLabel->setStyleSheet("QLabel { background-color: #f0f0f0; padding: 8px; border-radius: 4px; }");
    infoLabel->setWordWrap(true);
    previewLayout->addWidget(infoLabel);
    
    // Adicionar colunas ao layout principal
    contentLayout->addLayout(settingsLayout, 1);
    contentLayout->addLayout(previewLayout, 1);
    
    mainLayout->addLayout(contentLayout);
    
    // === BOTÕES INFERIORES ===
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    resetButton = new QPushButton("🔄 Restaurar Padrões");
    connect(resetButton, &QPushButton::clicked, this, &FragmentExportDialog::onResetToDefaults);
    buttonLayout->addWidget(resetButton);
    
    applyDefaultsButton = new QPushButton("✓ Aplicar Padrões");
    applyDefaultsButton->setToolTip("Salvar configurações atuais como padrão de exportação");
    connect(applyDefaultsButton, &QPushButton::clicked, this, &FragmentExportDialog::onApplyDefaults);
    buttonLayout->addWidget(applyDefaultsButton);
    
    buttonLayout->addStretch();
    
    cancelButton = new QPushButton("❌ Cancelar");
    cancelButton->setMinimumWidth(120);
    cancelButton->setMinimumHeight(35);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelButton);
    
    exportButton = new QPushButton("💾 Exportar");
    exportButton->setMinimumWidth(120);
    exportButton->setMinimumHeight(35);
    exportButton->setDefault(true);
    exportButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px 20px; }");
    connect(exportButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(exportButton);
    
    mainLayout->addLayout(buttonLayout);
    
    setMinimumSize(900, 700);
}

void FragmentExportDialog::loadDefaultSettings()
{
    // Carregar configurações salvas
    auto& settings = FingerprintEnhancer::UserSettings::instance();
    
    // Dimensões originais
    widthSpinBox->setValue(originalWidth);
    heightSpinBox->setValue(originalHeight);
    
    // Caminho padrão - usar diretório do projeto ou Documentos
    QString suggestedName = QString("fragmento_%1_marcado.png")
        .arg(fragment->id.left(8));
    
    QString defaultPath;
    if (!defaultExportDirectory.isEmpty() && QDir(defaultExportDirectory).exists()) {
        defaultPath = defaultExportDirectory;
    } else {
        defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }
    
    filePathEdit->setText(QDir(defaultPath).filePath(suggestedName));
    
    // Carregar configurações de exportação salvas
    markerColor = settings.getExportMarkerColor();
    textColor = settings.getExportTextColor();
    labelBgColor = settings.getExportLabelBgColor();
    
    markerSizeSpinBox->setValue(settings.getExportMarkerSize());
    fontSizeSpinBox->setValue(settings.getExportFontSize());
    lineWidthSpinBox->setValue(settings.getExportLineWidth());
    labelOpacitySlider->setValue(settings.getExportLabelOpacity());
    
    showNumbersCheck->setChecked(settings.getExportShowNumbers());
    showLabelTypeCheck->setChecked(settings.getExportShowTypes());
    showAnglesCheck->setChecked(settings.getExportShowAngles());
    labelPositionComboBox->setCurrentIndex(settings.getExportLabelPosition());
    includeMinutiaeCheck->setChecked(settings.getExportIncludeMinutiae());
    
    // Atualizar botões de cor
    markerColorButton->setStyleSheet(
        QString("background-color: rgb(%1, %2, %3); color: %4;")
            .arg(markerColor.red()).arg(markerColor.green()).arg(markerColor.blue())
            .arg(markerColor.lightness() > 128 ? "black" : "white")
    );
    textColorButton->setStyleSheet(
        QString("background-color: rgb(%1, %2, %3); color: %4;")
            .arg(textColor.red()).arg(textColor.green()).arg(textColor.blue())
            .arg(textColor.lightness() > 128 ? "black" : "white")
    );
    labelBgColorButton->setStyleSheet(
        QString("background-color: rgb(%1, %2, %3); color: %4;")
            .arg(labelBgColor.red()).arg(labelBgColor.green()).arg(labelBgColor.blue())
            .arg(labelBgColor.lightness() > 128 ? "black" : "white")
    );
    labelOpacityLabel->setText(QString("%1%").arg(settings.getExportLabelOpacity()));
    
    // Formato
    QString format = settings.getExportFormat();
    if (format == "PNG") formatComboBox->setCurrentIndex(0);
    else if (format == "JPEG") formatComboBox->setCurrentIndex(1);
    else if (format == "TIFF") formatComboBox->setCurrentIndex(2);
    else if (format == "BMP") formatComboBox->setCurrentIndex(3);
    
    qualitySlider->setValue(settings.getExportQuality());
}

void FragmentExportDialog::onBrowseClicked()
{
    QString filter;
    switch (formatComboBox->currentIndex()) {
        case 0: filter = "PNG (*.png)"; break;
        case 1: filter = "JPEG (*.jpg *.jpeg)"; break;
        case 2: filter = "TIFF (*.tif *.tiff)"; break;
        case 3: filter = "BMP (*.bmp)"; break;
        default: filter = "Todos (*.*)"; break;
    }
    
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Salvar Fragmento",
        filePathEdit->text(),
        filter
    );
    
    if (!fileName.isEmpty()) {
        filePathEdit->setText(fileName);
    }
}

void FragmentExportDialog::onFormatChanged(int index)
{
    // Mostrar/ocultar controle de qualidade (só para JPEG)
    bool isJpeg = (index == 1);
    qualitySlider->setEnabled(isJpeg);
    qualityLabel->setEnabled(isJpeg);
    
    // Atualizar extensão do arquivo
    QString currentPath = filePathEdit->text();
    QFileInfo fileInfo(currentPath);
    QString baseName = fileInfo.completeBaseName();
    QString newExt;
    
    switch (index) {
        case 0: newExt = ".png"; break;
        case 1: newExt = ".jpg"; break;
        case 2: newExt = ".tif"; break;
        case 3: newExt = ".bmp"; break;
    }
    
    QString newPath = fileInfo.dir().filePath(baseName + newExt);
    filePathEdit->setText(newPath);
    
    onAnySettingChanged();
}

void FragmentExportDialog::onAnySettingChanged()
{
    updatePreview();
}

void FragmentExportDialog::onWidthChanged(int value)
{
    if (updatingResolution) return;  // Evitar recursão
    
    // Se manter proporção está ativo, ajustar altura proporcionalmente
    if (maintainAspectCheck->isChecked() && originalWidth > 0) {
        updatingResolution = true;
        double aspectRatio = static_cast<double>(originalHeight) / static_cast<double>(originalWidth);
        int newHeight = static_cast<int>(value * aspectRatio);
        heightSpinBox->setValue(newHeight);
        updatingResolution = false;
    }
    
    updatePreview();
}

void FragmentExportDialog::onHeightChanged(int value)
{
    if (updatingResolution) return;  // Evitar recursão
    
    // Se manter proporção está ativo, ajustar largura proporcionalmente
    if (maintainAspectCheck->isChecked() && originalHeight > 0) {
        updatingResolution = true;
        double aspectRatio = static_cast<double>(originalWidth) / static_cast<double>(originalHeight);
        int newWidth = static_cast<int>(value * aspectRatio);
        widthSpinBox->setValue(newWidth);
        updatingResolution = false;
    }
    
    updatePreview();
}

void FragmentExportDialog::onScalePercentChanged(int value)
{
    Q_UNUSED(value);
    updatePreview();
}

void FragmentExportDialog::updatePreview()
{
    cv::Mat preview = renderPreview();
    
    if (!preview.empty()) {
        QPixmap pixmap = cvMatToQPixmap(preview);
        
        // Aplicar zoom do preview
        int zoomedWidth = static_cast<int>(pixmap.width() * previewZoomFactor);
        int zoomedHeight = static_cast<int>(pixmap.height() * previewZoomFactor);
        
        if (zoomedWidth > 0 && zoomedHeight > 0) {
            pixmap = pixmap.scaled(zoomedWidth, zoomedHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        
        // Ajustar tamanho do label para permitir scroll
        previewLabel->setPixmap(pixmap);
        previewLabel->resize(pixmap.size());
    }
    
    // Atualizar informações
    int outputWidth = widthSpinBox->value();
    int outputHeight = heightSpinBox->value();
    double scaleFactor = (double)outputWidth / originalWidth;
    int minutiaeCount = includeMinutiaeCheck->isChecked() ? fragment->minutiae.size() : 0;
    
    QString info = QString(
        "📊 <b>Informações da Exportação:</b><br>"
        "• Resolução: %1 × %2 pixels<br>"
        "• Escala: %3× do original<br>"
        "• Minúcias: %4<br>"
        "• Formato: %5<br>"
        "• Tamanho estimado: ~%6"
    ).arg(outputWidth)
     .arg(outputHeight)
     .arg(scaleFactor, 0, 'f', 2)
     .arg(minutiaeCount)
     .arg(getFormat())
     .arg(estimateFileSize(outputWidth, outputHeight));
    
    infoLabel->setText(info);
}

QString FragmentExportDialog::estimateFileSize(int width, int height)
{
    long long pixels = (long long)width * height;
    long long bytes;
    
    int formatIdx = formatComboBox->currentIndex();
    if (formatIdx == 1) {  // JPEG
        int quality = qualitySlider->value();
        bytes = pixels * 3 * quality / 400;  // Estimativa rough
    } else if (formatIdx == 0) {  // PNG
        bytes = pixels * 3;  // Assume compressão média
    } else {  // TIFF, BMP
        bytes = pixels * 3;
    }
    
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024);
    } else {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
    }
}

cv::Mat FragmentExportDialog::renderPreview()
{
    // Renderizar em resolução reduzida para preview rápido
    int previewWidth = std::min(400, originalWidth);
    int previewHeight = originalHeight * previewWidth / originalWidth;
    
    return renderExportImage(previewWidth, previewHeight);
}

cv::Mat FragmentExportDialog::renderExportImage(int width, int height)
{
    cv::Mat result = fragment->workingImage.clone();
    
    // Converter para BGR se necessário (garantir 3 canais)
    if (result.channels() == 1) {
        cv::cvtColor(result, result, cv::COLOR_GRAY2BGR);
    } else if (result.channels() == 4) {
        // PNG com canal alpha - converter para BGR (remover alpha)
        cv::cvtColor(result, result, cv::COLOR_BGRA2BGR);
    }
    
    // Redimensionar se necessário
    if (width != result.cols || height != result.rows) {
        cv::resize(result, result, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);
    }
    
    // Desenhar minúcias se habilitado
    if (includeMinutiaeCheck->isChecked() && !fragment->minutiae.empty()) {
        double scaleFactor = (double)width / originalWidth;
        int markerRadius = markerSizeSpinBox->value() * scaleFactor / 2;
        double fontSize = fontSizeSpinBox->value() * scaleFactor / 30.0;
        int fontThickness = std::max(1, (int)(markerRadius / 8.0));
        int lineWidth = lineWidthSpinBox->value();
        
        cv::Scalar markerColorCV(markerColor.blue(), markerColor.green(), markerColor.red());
        cv::Scalar textColorCV(textColor.blue(), textColor.green(), textColor.red());
        cv::Scalar bgColorCV(labelBgColor.blue(), labelBgColor.green(), labelBgColor.red());
        
        for (int i = 0; i < fragment->minutiae.size(); ++i) {
            const auto& m = fragment->minutiae[i];
            cv::Point center(
                (int)(m.position.x() * scaleFactor),
                (int)(m.position.y() * scaleFactor)
            );
            
            // Desenhar círculo
            cv::circle(result, center, markerRadius, markerColorCV, lineWidth, cv::LINE_AA);
            
            // Desenhar linha de direção
            if (showAnglesCheck->isChecked()) {
                double angle = m.angle * M_PI / 180.0;
                int lineLength = markerRadius + markerRadius / 2;
                cv::Point endPoint(
                    center.x + (int)(lineLength * std::cos(angle)),
                    center.y + (int)(lineLength * std::sin(angle))
                );
                cv::line(result, center, endPoint, markerColorCV, lineWidth, cv::LINE_AA);
            }
            
            // Desenhar rótulo
            QString label;
            if (showNumbersCheck->isChecked() && showLabelTypeCheck->isChecked()) {
                label = QString("%1-%2").arg(i + 1).arg(m.getTypeAbbreviation());
            } else if (showNumbersCheck->isChecked()) {
                label = QString::number(i + 1);
            } else if (showLabelTypeCheck->isChecked()) {
                label = m.getTypeAbbreviation();
            }
            
            if (!label.isEmpty()) {
                // Obter posição do rótulo
                int labelPosIndex = labelPositionComboBox->currentIndex();
                FingerprintEnhancer::MinutiaLabelPosition labelPos = 
                    static_cast<FingerprintEnhancer::MinutiaLabelPosition>(
                        labelPositionComboBox->itemData(labelPosIndex).toInt());
                
                // Se HIDDEN, não desenhar rótulo
                if (labelPos == FingerprintEnhancer::MinutiaLabelPosition::HIDDEN) {
                    continue;
                }
                
                cv::Size textSize = cv::getTextSize(
                    label.toStdString(),
                    cv::FONT_HERSHEY_SIMPLEX,
                    fontSize,
                    fontThickness,
                    nullptr
                );
                
                cv::Point textPos;
                int offset = markerRadius + (int)(fontSize * 20);
                
                switch (labelPos) {
                    case FingerprintEnhancer::MinutiaLabelPosition::ABOVE:
                        textPos = cv::Point(center.x - textSize.width / 2, center.y - offset);
                        break;
                    case FingerprintEnhancer::MinutiaLabelPosition::BELOW:
                        textPos = cv::Point(center.x - textSize.width / 2, center.y + offset + textSize.height);
                        break;
                    case FingerprintEnhancer::MinutiaLabelPosition::LEFT:
                        textPos = cv::Point(center.x - offset - textSize.width, center.y + textSize.height / 2);
                        break;
                    case FingerprintEnhancer::MinutiaLabelPosition::RIGHT:
                    default:
                        textPos = cv::Point(center.x + offset, center.y + textSize.height / 2);
                        break;
                }
                
                // Fundo do texto com opacidade
                cv::Rect textBg(
                    std::max(0, textPos.x - 2),
                    std::max(0, textPos.y - textSize.height - 2),
                    std::min(textSize.width + 4, result.cols - textPos.x + 2),
                    textSize.height + 4
                );
                
                if (textBg.x >= 0 && textBg.y >= 0 &&
                    textBg.x + textBg.width <= result.cols && 
                    textBg.y + textBg.height <= result.rows &&
                    textBg.width > 0 && textBg.height > 0) {
                    
                    // Aplicar opacidade
                    double opacity = labelOpacitySlider->value() / 100.0;
                    if (opacity > 0.01) {
                        cv::Mat roi = result(textBg);
                        cv::Mat bgOverlay(textBg.size(), result.type(), bgColorCV);
                        cv::addWeighted(bgOverlay, opacity, roi, 1.0 - opacity, 0, roi);
                    }
                }
                
                // Texto
                cv::putText(result, label.toStdString(), textPos,
                           cv::FONT_HERSHEY_SIMPLEX, fontSize,
                           textColorCV, fontThickness, cv::LINE_AA);
            }
        }
    }
    
    return result;
}

QPixmap FragmentExportDialog::cvMatToQPixmap(const cv::Mat& mat)
{
    if (mat.empty()) {
        return QPixmap();
    }
    
    cv::Mat rgb;
    if (mat.channels() == 1) {
        cv::cvtColor(mat, rgb, cv::COLOR_GRAY2RGB);
    } else if (mat.channels() == 3) {
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    } else {
        rgb = mat;
    }
    
    QImage qImg(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888);
    return QPixmap::fromImage(qImg.copy());
}

void FragmentExportDialog::onMarkerColorClicked()
{
    QColor color = QColorDialog::getColor(markerColor, this, "Escolher Cor da Marcação");
    if (color.isValid()) {
        markerColor = color;
        markerColorButton->setStyleSheet(
            QString("background-color: rgb(%1, %2, %3); color: %4;")
                .arg(color.red())
                .arg(color.green())
                .arg(color.blue())
                .arg(color.lightness() > 128 ? "black" : "white")
        );
        updatePreview();
    }
}

void FragmentExportDialog::onTextColorClicked()
{
    QColor color = QColorDialog::getColor(textColor, this, "Escolher Cor do Texto");
    if (color.isValid()) {
        textColor = color;
        textColorButton->setStyleSheet(
            QString("background-color: rgb(%1, %2, %3); color: %4;")
                .arg(color.red())
                .arg(color.green())
                .arg(color.blue())
                .arg(color.lightness() > 128 ? "black" : "white")
        );
        updatePreview();
    }
}

void FragmentExportDialog::onLabelBgColorClicked()
{
    QColor color = QColorDialog::getColor(labelBgColor, this, "Escolher Cor do Fundo do Rótulo");
    if (color.isValid()) {
        labelBgColor = color;
        labelBgColorButton->setStyleSheet(
            QString("background-color: rgb(%1, %2, %3); color: %4;")
                .arg(color.red())
                .arg(color.green())
                .arg(color.blue())
                .arg(color.lightness() > 128 ? "black" : "white")
        );
        updatePreview();
    }
}

void FragmentExportDialog::onLabelOpacityChanged(int value)
{
    labelOpacityLabel->setText(QString("%1%").arg(value));
    updatePreview();
}

void FragmentExportDialog::onResetToDefaults()
{
    loadDefaultSettings();
    markerColor = QColor(255, 0, 0);
    textColor = QColor(255, 0, 0);
    markerColorButton->setStyleSheet("background-color: rgb(255, 0, 0); color: white;");
    textColorButton->setStyleSheet("background-color: rgb(255, 0, 0); color: white;");
    
    includeMinutiaeCheck->setChecked(true);
    showNumbersCheck->setChecked(true);
    showLabelTypeCheck->setChecked(true);
    showAnglesCheck->setChecked(true);
    maintainAspectCheck->setChecked(true);
    
    formatComboBox->setCurrentIndex(0);  // PNG
    qualitySlider->setValue(95);
    
    lineWidthSpinBox->setValue(2);
    labelPositionComboBox->setCurrentIndex(0);
    
    // Resetar zoom do preview
    previewZoomSlider->setValue(100);
    previewZoomFactor = 1.0;
    previewZoomLabel->setText("100%");
    
    updatePreview();
}

void FragmentExportDialog::onApplyDefaults()
{
    // Salvar TODAS as configurações atuais como padrão de exportação
    auto& settings = FingerprintEnhancer::UserSettings::instance();
    
    // Cores
    settings.setExportMarkerColor(markerColor);
    settings.setExportTextColor(textColor);
    settings.setExportLabelBgColor(labelBgColor);
    
    // Tamanhos
    settings.setExportMarkerSize(markerSizeSpinBox->value());
    settings.setExportFontSize(fontSizeSpinBox->value());
    settings.setExportLineWidth(lineWidthSpinBox->value());
    
    // Opacidade
    settings.setExportLabelOpacity(labelOpacitySlider->value());
    
    // Opções de exibição
    settings.setExportShowNumbers(showNumbersCheck->isChecked());
    settings.setExportShowTypes(showLabelTypeCheck->isChecked());
    settings.setExportShowAngles(showAnglesCheck->isChecked());
    settings.setExportLabelPosition(labelPositionComboBox->currentData().toInt());
    settings.setExportIncludeMinutiae(includeMinutiaeCheck->isChecked());
    
    // Formato e qualidade
    QString format;
    switch (formatComboBox->currentIndex()) {
        case 0: format = "PNG"; break;
        case 1: format = "JPEG"; break;
        case 2: format = "TIFF"; break;
        case 3: format = "BMP"; break;
        default: format = "PNG";
    }
    settings.setExportFormat(format);
    settings.setExportQuality(qualitySlider->value());
    
    // Gravar no arquivo .ini
    settings.save();
    
    fprintf(stderr, "[EXPORT_SETTINGS] Configurações de exportação salvas:\n");
    fprintf(stderr, "  - Formato: %s, Qualidade: %d%%\n", format.toStdString().c_str(), qualitySlider->value());
    fprintf(stderr, "  - Marcação: rgb(%d,%d,%d), Texto: rgb(%d,%d,%d)\n",
            markerColor.red(), markerColor.green(), markerColor.blue(),
            textColor.red(), textColor.green(), textColor.blue());
    fprintf(stderr, "  - Tamanhos: marcação=%dpx, fonte=%dpt, linha=%dpx\n",
            markerSizeSpinBox->value(), fontSizeSpinBox->value(), lineWidthSpinBox->value());
    
    QMessageBox::information(this, "Padrões Salvos", 
                            "As configurações atuais foram salvas como padrão de exportação.\n\n"
                            "Estes valores serão carregados automaticamente na próxima vez que\n"
                            "você abrir a janela de exportação.");
}

bool FragmentExportDialog::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);
        
        // Scroll do mouse = Zoom (sem necessidade de Ctrl)
        int delta = wheelEvent->angleDelta().y();
        int currentZoom = previewZoomSlider->value();
        int newZoom = currentZoom + (delta > 0 ? 10 : -10);
        newZoom = qBound(25, newZoom, 400);
        previewZoomSlider->setValue(newZoom);
        return true;  // Evento consumido - bloqueia scroll normal
    }
    
    return QDialog::eventFilter(obj, event);
}

void FragmentExportDialog::onPreviewZoomChanged(int value)
{
    previewZoomFactor = value / 100.0;
    previewZoomLabel->setText(QString("%1%").arg(value));
    updatePreview();
}

QString FragmentExportDialog::getFormat() const
{
    switch (formatComboBox->currentIndex()) {
        case 0: return "PNG";
        case 1: return "JPEG";
        case 2: return "TIFF";
        case 3: return "BMP";
        default: return "PNG";
    }
}

bool FragmentExportDialog::exportImage(QString& errorMessage)
{
    QString filePath = filePathEdit->text();
    
    if (filePath.isEmpty()) {
        errorMessage = "Caminho de arquivo não especificado";
        return false;
    }
    
    // Verificar se diretório existe
    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.dir();
    if (!dir.exists()) {
        errorMessage = QString("Diretório não existe: %1").arg(dir.path());
        return false;
    }
    
    // Renderizar imagem final
    int outputWidth = widthSpinBox->value();
    int outputHeight = heightSpinBox->value();
    
    cv::Mat exportImage = renderExportImage(outputWidth, outputHeight);
    
    if (exportImage.empty()) {
        errorMessage = "Falha ao renderizar imagem de exportação";
        return false;
    }
    
    // Parâmetros de salvamento
    std::vector<int> params;
    QString format = getFormat();
    
    if (format == "JPEG") {
        params.push_back(cv::IMWRITE_JPEG_QUALITY);
        params.push_back(qualitySlider->value());
    } else if (format == "PNG") {
        params.push_back(cv::IMWRITE_PNG_COMPRESSION);
        params.push_back(9);  // Máxima compressão
    }
    
    // Salvar
    try {
        if (!cv::imwrite(filePath.toStdString(), exportImage, params)) {
            errorMessage = "OpenCV não conseguiu salvar a imagem";
            return false;
        }
    } catch (const cv::Exception& e) {
        errorMessage = QString("Exceção do OpenCV: %1").arg(e.what());
        return false;
    }
    
    // Salvar configurações de exportação para próximas vezes
    saveCurrentSettings();
    
    return true;
}

void FragmentExportDialog::saveCurrentSettings()
{
    auto& settings = FingerprintEnhancer::UserSettings::instance();
    
    // Salvar cores
    settings.setExportMarkerColor(markerColor);
    settings.setExportTextColor(textColor);
    settings.setExportLabelBgColor(labelBgColor);
    
    // Salvar tamanhos
    settings.setExportMarkerSize(markerSizeSpinBox->value());
    settings.setExportFontSize(fontSizeSpinBox->value());
    settings.setExportLineWidth(lineWidthSpinBox->value());
    settings.setExportLabelOpacity(labelOpacitySlider->value());
    
    // Salvar opções de exibição
    settings.setExportShowNumbers(showNumbersCheck->isChecked());
    settings.setExportShowTypes(showLabelTypeCheck->isChecked());
    settings.setExportShowAngles(showAnglesCheck->isChecked());
    settings.setExportLabelPosition(labelPositionComboBox->currentIndex());
    settings.setExportIncludeMinutiae(includeMinutiaeCheck->isChecked());
    
    // Salvar formato
    settings.setExportFormat(getFormat());
    settings.setExportQuality(qualitySlider->value());
    
    // Gravar no disco
    settings.save();
}

void FragmentExportDialog::setSuggestedFileName(const QString& name) {
    // Sugerir nome de arquivo com caminho completo
    QString currentPath = filePathEdit->text();
    QFileInfo fileInfo(currentPath);
    
    // Se já tem um diretório, manter, senão usar padrão
    QString directory = fileInfo.absolutePath();
    if (directory == "." || directory.isEmpty()) {
        directory = defaultExportDirectory;
    }
    
    // Obter extensão do formato selecionado
    QString extension = ".png";  // padrão
    QString format = getFormat();
    if (format.contains("jpg", Qt::CaseInsensitive)) {
        extension = ".jpg";
    } else if (format.contains("tif", Qt::CaseInsensitive)) {
        extension = ".tif";
    } else if (format.contains("bmp", Qt::CaseInsensitive)) {
        extension = ".bmp";
    }
    
    // Montar caminho completo
    QString suggestedPath = QDir(directory).filePath(name + extension);
    filePathEdit->setText(suggestedPath);
}
