#include "FragmentExportDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGridLayout>
#include <QFileDialog>
#include <QColorDialog>
#include <QMessageBox>
#include <QStandardPaths>
#include <QFileInfo>
#include <QScrollArea>
#include <QRegularExpression>
#include <QtCore/QtMath>
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
      updatingResolution(false),
      defaultExportDirectory(defaultDirectory)
{
    setWindowTitle("💾 Exportar Fragmento");
    setModal(true);
    
    originalWidth = fragment->workingImage.cols;
    originalHeight = fragment->workingImage.rows;
    
    setupUI();
    loadDefaultSettings();
    
    // Configurar caminho padrão de exportação
    if (!defaultExportDirectory.isEmpty()) {
        setDefaultExportPath(defaultExportDirectory);
    }
    
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
    qualitySlider->setValue(97);
    qualityLabel = new QLabel("97%");
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
    
    scalePercentSpinBox = new QSpinBox();
    scalePercentSpinBox->setRange(10, 1000);
    scalePercentSpinBox->setValue(100);
    scalePercentSpinBox->setSuffix(" %");
    scalePercentSpinBox->setToolTip("Escala percentual da resolução original");
    connect(scalePercentSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &FragmentExportDialog::onScalePercentChanged);
    resLayout->addRow("Escala:", scalePercentSpinBox);
    
    dpiSpinBox = new QSpinBox();
    dpiSpinBox->setRange(72, 1200);
    dpiSpinBox->setValue(600);
    dpiSpinBox->setSuffix(" DPI");
    dpiSpinBox->setToolTip("Resolução para impressão (600 DPI = padrão alta qualidade)");
    resLayout->addRow("DPI:", dpiSpinBox);
    
    maintainAspectCheck = new QCheckBox("Manter proporção");
    maintainAspectCheck->setChecked(true);
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
    
    // Símbolo
    symbolComboBox = new QComboBox();
    symbolComboBox->addItem("Círculo Simples", (int)FingerprintEnhancer::MinutiaeSymbol::CIRCLE);
    symbolComboBox->addItem("Círculo com X", (int)FingerprintEnhancer::MinutiaeSymbol::CIRCLE_X);
    symbolComboBox->addItem("Círculo com Seta", (int)FingerprintEnhancer::MinutiaeSymbol::CIRCLE_ARROW);
    symbolComboBox->addItem("Círculo com Cruz", (int)FingerprintEnhancer::MinutiaeSymbol::CIRCLE_CROSS);
    symbolComboBox->addItem("Triângulo", (int)FingerprintEnhancer::MinutiaeSymbol::TRIANGLE);
    symbolComboBox->addItem("Quadrado", (int)FingerprintEnhancer::MinutiaeSymbol::SQUARE);
    symbolComboBox->addItem("Losango", (int)FingerprintEnhancer::MinutiaeSymbol::DIAMOND);
    connect(symbolComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FragmentExportDialog::onAnySettingChanged);
    markersLayout->addRow("Símbolo:", symbolComboBox);
    
    markerSizeSpinBox = new QSpinBox();
    markerSizeSpinBox->setRange(10, 200);
    markerSizeSpinBox->setValue(30);
    markerSizeSpinBox->setSuffix(" px");
    markerSizeSpinBox->setToolTip("Diâmetro do círculo da marcação");
    connect(markerSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &FragmentExportDialog::onAnySettingChanged);
    markersLayout->addRow("Tamanho marcação:", markerSizeSpinBox);
    
    fontSizeSpinBox = new QSpinBox();
    fontSizeSpinBox->setRange(8, 72);
    fontSizeSpinBox->setValue(26);
    fontSizeSpinBox->setSuffix(" pt");
    fontSizeSpinBox->setToolTip("Tamanho da fonte dos rótulos");
    connect(fontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &FragmentExportDialog::onAnySettingChanged);
    markersLayout->addRow("Tamanho fonte:", fontSizeSpinBox);
    
    // Posição do rótulo
    labelPositionComboBox = new QComboBox();
    labelPositionComboBox->addItem("À Direita", (int)FingerprintEnhancer::MinutiaLabelPosition::RIGHT);
    labelPositionComboBox->addItem("À Esquerda", (int)FingerprintEnhancer::MinutiaLabelPosition::LEFT);
    labelPositionComboBox->addItem("Acima", (int)FingerprintEnhancer::MinutiaLabelPosition::ABOVE);
    labelPositionComboBox->addItem("Abaixo", (int)FingerprintEnhancer::MinutiaLabelPosition::BELOW);
    labelPositionComboBox->addItem("Oculto", (int)FingerprintEnhancer::MinutiaLabelPosition::HIDDEN);
    connect(labelPositionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &FragmentExportDialog::onAnySettingChanged);
    markersLayout->addRow("Posição do rótulo:", labelPositionComboBox);
    
    // Cor de fundo do rótulo
    labelBgColorButton = new QPushButton("🎨 Cor Fundo Rótulo");
    labelBgColorButton->setStyleSheet("background-color: rgb(255, 255, 255); color: black;");
    connect(labelBgColorButton, &QPushButton::clicked, [this]() {
        QColor color = QColorDialog::getColor(labelBgColor, this, "Cor de Fundo do Rótulo");
        if (color.isValid()) {
            labelBgColor = color;
            labelBgColorButton->setStyleSheet(
                QString("background-color: rgb(%1, %2, %3); color: %4;")
                    .arg(color.red()).arg(color.green()).arg(color.blue())
                    .arg(color.lightness() > 128 ? "black" : "white"));
            displaySettings.labelBackgroundColor = QColor(color.red(), color.green(), color.blue());
            onAnySettingChanged();
        }
    });
    markersLayout->addRow("", labelBgColorButton);
    
    // Opacidade do fundo do rótulo
    QHBoxLayout *opacityLayout = new QHBoxLayout();
    labelOpacitySlider = new QSlider(Qt::Horizontal);
    labelOpacitySlider->setRange(0, 255);
    labelOpacitySlider->setValue(200);
    labelOpacityLabel = new QLabel("78%");
    connect(labelOpacitySlider, &QSlider::valueChanged, [this](int value) {
        labelOpacityLabel->setText(QString("%1%").arg(int(value * 100.0 / 255.0)));
        displaySettings.labelBackgroundOpacity = value;
        onAnySettingChanged();
    });
    opacityLayout->addWidget(labelOpacitySlider);
    opacityLayout->addWidget(labelOpacityLabel);
    markersLayout->addRow("Opacidade rótulo:", opacityLayout);
    
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
    
    showAnglesCheck = new QCheckBox("Mostrar ângulos/direções");
    showAnglesCheck->setChecked(true);
    connect(showAnglesCheck, &QCheckBox::toggled, this, &FragmentExportDialog::onAnySettingChanged);
    displayLayout->addWidget(showAnglesCheck);
    
    // Cores
    QHBoxLayout *colorsLayout = new QHBoxLayout();
    markerColorButton = new QPushButton("🎨 Cor da Marcação");
    markerColorButton->setStyleSheet(QString("background-color: rgb(255, 0, 0); color: white;"));
    connect(markerColorButton, &QPushButton::clicked, this, &FragmentExportDialog::onMarkerColorClicked);
    colorsLayout->addWidget(markerColorButton);
    
    textColorButton = new QPushButton("🎨 Cor do Texto");
    textColorButton->setStyleSheet(QString("background-color: rgb(255, 0, 0); color: white;"));
    connect(textColorButton, &QPushButton::clicked, this, &FragmentExportDialog::onTextColorClicked);
    colorsLayout->addWidget(textColorButton);
    
    displayLayout->addLayout(colorsLayout);
    
    settingsLayout->addWidget(displayGroup);
    
    settingsLayout->addStretch();
    
    // === COLUNA DIREITA: Preview ===
    QVBoxLayout *previewLayout = new QVBoxLayout();
    
    QLabel *previewTitle = new QLabel("📷 Preview da Exportação");
    previewTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    previewLayout->addWidget(previewTitle);
    
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumSize(400, 400);
    
    previewLabel = new QLabel();
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setStyleSheet("QLabel { background-color: #2b2b2b; border: 2px solid #555; }");
    previewLabel->setMinimumSize(380, 380);
    scrollArea->setWidget(previewLabel);
    
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
    
    buttonLayout->addStretch();
    
    cancelButton = new QPushButton("❌ Cancelar");
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(cancelButton);
    
    exportButton = new QPushButton("💾 Exportar");
    exportButton->setDefault(true);
    exportButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px 20px; }");
    connect(exportButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(exportButton);
    
    mainLayout->addLayout(buttonLayout);
    
    setMinimumSize(900, 700);
}

void FragmentExportDialog::loadDefaultSettings()
{
    updatingResolution = true;
    
    // Dimensões originais
    widthSpinBox->setValue(originalWidth);
    heightSpinBox->setValue(originalHeight);
    scalePercentSpinBox->setValue(100);
    dpiSpinBox->setValue(600);
    
    updatingResolution = false;
    
    // Caminho padrão
    // Sanitizar ID do fragmento (remover caracteres inválidos como {, }, -, etc)
    QString sanitizedId = fragment->id.left(8);
    sanitizedId.replace(QRegularExpression("[^a-zA-Z0-9]"), ""); // Manter apenas alfanuméricos
    if (sanitizedId.isEmpty()) {
        sanitizedId = "fragmento";
    }
    
    QString suggestedName = QString("fragmento_%1_marcado.png").arg(sanitizedId);
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    filePathEdit->setText(QDir(defaultPath).filePath(suggestedName));
    
    // Calcular tamanho das marcações baseado na escala
    if (scale > 0) {
        int markerSize = static_cast<int>(4.0 * scale);  // 4mm
        markerSizeSpinBox->setValue(std::max(20, std::min(100, markerSize)));
        
        int fontSize = static_cast<int>(scale * 2.5);  // Aproximadamente 2.5mm
        fontSizeSpinBox->setValue(std::max(12, std::min(48, fontSize)));
    }
    
    // Configurações de visualização padrão
    displaySettings.symbol = FingerprintEnhancer::MinutiaeSymbol::CIRCLE;
    displaySettings.markerSize = markerSizeSpinBox->value();
    displaySettings.labelFontSize = fontSizeSpinBox->value();
    displaySettings.labelBackgroundColor = QColor(255, 255, 255, 200);
    displaySettings.labelBackgroundOpacity = 200;
    displaySettings.defaultLabelPosition = FingerprintEnhancer::MinutiaLabelPosition::RIGHT;
    displaySettings.showLabelType = true;
    displaySettings.showAngles = true;
    
    // Inicializar combos
    symbolComboBox->setCurrentIndex(0);
    labelPositionComboBox->setCurrentIndex(0);
}

void FragmentExportDialog::setDefaultExportPath(const QString& directory)
{
    // Usar diretório do projeto como base para exportação
    if (directory.isEmpty()) {
        return;
    }
    
    // Sanitizar ID do fragmento
    QString sanitizedId = fragment->id.left(8);
    sanitizedId.replace(QRegularExpression("[^a-zA-Z0-9]"), "");
    if (sanitizedId.isEmpty()) {
        sanitizedId = "fragmento";
    }
    
    // Criar nome sugerido baseado no fragmento
    QString suggestedName = QString("fragmento_%1_marcado.png").arg(sanitizedId);
    QString fullPath = QDir(directory).filePath(suggestedName);
    
    // Atualizar o campo de caminho
    filePathEdit->setText(fullPath);
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
    
    // Usar caminho atual ou diretório do projeto como inicial
    QString initialPath = filePathEdit->text();
    if (initialPath.isEmpty() && !defaultExportDirectory.isEmpty()) {
        QString sanitizedId = fragment->id.left(8);
        sanitizedId.replace(QRegularExpression("[^a-zA-Z0-9]"), "");
        if (sanitizedId.isEmpty()) sanitizedId = "fragmento";
        QString suggestedName = QString("fragmento_%1_marcado.png").arg(sanitizedId);
        initialPath = QDir(defaultExportDirectory).filePath(suggestedName);
    }
    
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Salvar Fragmento",
        initialPath,
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
    if (updatingResolution) return;
    
    updatingResolution = true;
    
    // Atualizar altura mantendo proporção se necessário
    if (maintainAspectCheck->isChecked()) {
        double aspectRatio = (double)originalHeight / originalWidth;
        int newHeight = qRound(value * aspectRatio);
        heightSpinBox->setValue(newHeight);
    }
    
    // Atualizar percentual
    int percent = qRound((double)value / originalWidth * 100.0);
    scalePercentSpinBox->setValue(percent);
    
    updatingResolution = false;
    updatePreview();
}

void FragmentExportDialog::onHeightChanged(int value)
{
    if (updatingResolution) return;
    
    updatingResolution = true;
    
    // Atualizar largura mantendo proporção se necessário
    if (maintainAspectCheck->isChecked()) {
        double aspectRatio = (double)originalWidth / originalHeight;
        int newWidth = qRound(value * aspectRatio);
        widthSpinBox->setValue(newWidth);
    }
    
    // Atualizar percentual (baseado na largura)
    int percent = qRound((double)widthSpinBox->value() / originalWidth * 100.0);
    scalePercentSpinBox->setValue(percent);
    
    updatingResolution = false;
    updatePreview();
}

void FragmentExportDialog::onScalePercentChanged(int value)
{
    if (updatingResolution) return;
    
    updatingResolution = true;
    
    // Calcular novas dimensões baseadas no percentual
    int newWidth = qRound(originalWidth * value / 100.0);
    int newHeight = qRound(originalHeight * value / 100.0);
    
    widthSpinBox->setValue(newWidth);
    heightSpinBox->setValue(newHeight);
    
    updatingResolution = false;
    updatePreview();
}

void FragmentExportDialog::updatePreview()
{
    cv::Mat preview = renderPreview();
    
    if (!preview.empty()) {
        QPixmap pixmap = cvMatToQPixmap(preview);
        
        // Redimensionar para caber no preview (máx 380x380)
        if (pixmap.width() > 380 || pixmap.height() > 380) {
            pixmap = pixmap.scaled(380, 380, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        
        previewLabel->setPixmap(pixmap);
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
    
    // Converter para BGR se necessário
    if (result.channels() == 1) {
        cv::cvtColor(result, result, cv::COLOR_GRAY2BGR);
    }
    
    // Redimensionar se necessário
    if (width != result.cols || height != result.rows) {
        cv::resize(result, result, cv::Size(width, height), 0, 0, cv::INTER_LINEAR);
    }
    
    // Desenhar minúcias se habilitado
    if (includeMinutiaeCheck->isChecked() && !fragment->minutiae.empty()) {
        double scaleFactor = (double)width / originalWidth;
        
        // Atualizar configurações de exibição
        displaySettings.markerSize = markerSizeSpinBox->value();
        displaySettings.labelFontSize = fontSizeSpinBox->value();
        displaySettings.symbol = static_cast<FingerprintEnhancer::MinutiaeSymbol>(
            symbolComboBox->currentData().toInt());
        displaySettings.defaultLabelPosition = static_cast<FingerprintEnhancer::MinutiaLabelPosition>(
            labelPositionComboBox->currentData().toInt());
        displaySettings.showLabelType = showLabelTypeCheck->isChecked();
        displaySettings.showAngles = showAnglesCheck->isChecked();
        
        int markerRadius = displaySettings.markerSize * scaleFactor / 2;
        double fontSize = displaySettings.labelFontSize * scaleFactor / 30.0;
        int fontThickness = std::max(1, (int)(markerRadius / 8.0));
        
        cv::Scalar markerColorCV(markerColor.blue(), markerColor.green(), markerColor.red());
        cv::Scalar textColorCV(textColor.blue(), textColor.green(), textColor.red());
        cv::Scalar labelBgColorCV(
            labelBgColor.blue(), 
            labelBgColor.green(), 
            labelBgColor.red(), 
            displaySettings.labelBackgroundOpacity
        );
        
        for (int i = 0; i < fragment->minutiae.size(); ++i) {
            const auto& m = fragment->minutiae[i];
            cv::Point center(
                (int)(m.position.x() * scaleFactor),
                (int)(m.position.y() * scaleFactor)
            );
            
            // Desenhar símbolo da minúcia
            drawMinutiaSymbol(result, center, markerRadius, m.angle, markerColorCV);
            
            // Desenhar ângulo adicional se habilitado
            if (displaySettings.showAngles && 
                displaySettings.symbol != FingerprintEnhancer::MinutiaeSymbol::CIRCLE_ARROW) {
                double angle = m.angle * M_PI / 180.0;
                int lineLength = markerRadius * 2;
                cv::Point endPoint(
                    center.x + (int)(lineLength * std::cos(angle)),
                    center.y - (int)(lineLength * std::sin(angle))
                );
                cv::line(result, center, endPoint, markerColorCV, 2, cv::LINE_AA);
            }
            
            // Desenhar rótulo baseado nas configurações
            FingerprintEnhancer::MinutiaLabelPosition labelPos = displaySettings.defaultLabelPosition;
            
            if (labelPos != FingerprintEnhancer::MinutiaLabelPosition::HIDDEN) {
                QString numberLabel;
                QString typeLabel;
                
                if (showNumbersCheck->isChecked()) {
                    numberLabel = QString(" %1 ").arg(i + 1);
                }
                
                if (showLabelTypeCheck->isChecked()) {
                    typeLabel = QString(" %1 ").arg(m.getTypeAbbreviation());
                }
                
                drawMinutiaLabels(result, center, numberLabel, typeLabel, labelPos,
                                markerRadius, fontSize, fontThickness, 
                                textColorCV, labelBgColorCV, scaleFactor);
            }
        }
    }
    
    return result;
}

void FragmentExportDialog::drawMinutiaSymbol(cv::Mat& img, const cv::Point& center, 
                                             int radius, float angle, const cv::Scalar& color)
{
    using MS = FingerprintEnhancer::MinutiaeSymbol;
    MS symbol = static_cast<MS>(symbolComboBox->currentData().toInt());
    
    switch (symbol) {
        case MS::CIRCLE:
            cv::circle(img, center, radius, color, 2, cv::LINE_AA);
            break;
            
        case MS::CIRCLE_X: {
            cv::circle(img, center, radius, color, 2, cv::LINE_AA);
            int offset = radius / 2;
            cv::line(img, cv::Point(center.x - offset, center.y - offset),
                    cv::Point(center.x + offset, center.y + offset), color, 2, cv::LINE_AA);
            cv::line(img, cv::Point(center.x - offset, center.y + offset),
                    cv::Point(center.x + offset, center.y - offset), color, 2, cv::LINE_AA);
            break;
        }
        
        case MS::CIRCLE_ARROW: {
            cv::circle(img, center, radius, color, 2, cv::LINE_AA);
            double rad = angle * M_PI / 180.0;
            int arrowLen = radius * 1.5;
            cv::Point endPt(center.x + (int)(arrowLen * std::cos(rad)),
                           center.y - (int)(arrowLen * std::sin(rad)));
            cv::line(img, center, endPt, color, 2, cv::LINE_AA);
            // Pontas da seta
            int headSize = radius / 3;
            double angle1 = rad + M_PI * 3.0 / 4.0;
            double angle2 = rad - M_PI * 3.0 / 4.0;
            cv::Point head1(endPt.x + (int)(headSize * std::cos(angle1)),
                          endPt.y - (int)(headSize * std::sin(angle1)));
            cv::Point head2(endPt.x + (int)(headSize * std::cos(angle2)),
                          endPt.y - (int)(headSize * std::sin(angle2)));
            cv::line(img, endPt, head1, color, 2, cv::LINE_AA);
            cv::line(img, endPt, head2, color, 2, cv::LINE_AA);
            break;
        }
        
        case MS::CIRCLE_CROSS: {
            cv::circle(img, center, radius, color, 2, cv::LINE_AA);
            int offset = radius / 2;
            cv::line(img, cv::Point(center.x - offset, center.y),
                    cv::Point(center.x + offset, center.y), color, 2, cv::LINE_AA);
            cv::line(img, cv::Point(center.x, center.y - offset),
                    cv::Point(center.x, center.y + offset), color, 2, cv::LINE_AA);
            break;
        }
        
        case MS::TRIANGLE: {
            int h = radius;
            std::vector<cv::Point> pts = {
                cv::Point(center.x, center.y - h),
                cv::Point(center.x - h, center.y + h/2),
                cv::Point(center.x + h, center.y + h/2)
            };
            cv::polylines(img, pts, true, color, 2, cv::LINE_AA);
            break;
        }
        
        case MS::SQUARE: {
            cv::Rect rect(center.x - radius, center.y - radius, radius * 2, radius * 2);
            cv::rectangle(img, rect, color, 2, cv::LINE_AA);
            break;
        }
        
        case MS::DIAMOND: {
            std::vector<cv::Point> pts = {
                cv::Point(center.x, center.y - radius),
                cv::Point(center.x + radius, center.y),
                cv::Point(center.x, center.y + radius),
                cv::Point(center.x - radius, center.y)
            };
            cv::polylines(img, pts, true, color, 2, cv::LINE_AA);
            break;
        }
    }
}

void FragmentExportDialog::drawMinutiaLabels(cv::Mat& img, const cv::Point& center,
                                             const QString& numberLabel, const QString& typeLabel,
                                             FingerprintEnhancer::MinutiaLabelPosition labelPos,
                                             int markerRadius, double fontSize, int fontThickness,
                                             const cv::Scalar& textColor, const cv::Scalar& bgColor,
                                             double scaleFactor)
{
    using MLP = FingerprintEnhancer::MinutiaLabelPosition;
    int margin = 5 * scaleFactor;
    
    // Desenhar número
    if (!numberLabel.isEmpty()) {
        cv::Size textSize = cv::getTextSize(numberLabel.toStdString(),
                                           cv::FONT_HERSHEY_SIMPLEX, fontSize,
                                           fontThickness, nullptr);
        cv::Point textPos;
        
        switch (labelPos) {
            case MLP::RIGHT:
                textPos = cv::Point(center.x + markerRadius + margin, 
                                  center.y - markerRadius);
                break;
            case MLP::LEFT:
                textPos = cv::Point(center.x - markerRadius - margin - textSize.width,
                                  center.y - markerRadius);
                break;
            case MLP::ABOVE:
                textPos = cv::Point(center.x - textSize.width / 2,
                                  center.y - markerRadius - margin - textSize.height);
                break;
            case MLP::BELOW:
                textPos = cv::Point(center.x - textSize.width / 2,
                                  center.y + markerRadius + margin + textSize.height);
                break;
            default:
                return;
        }
        
        // Desenhar fundo com opacidade
        cv::Rect textBg(textPos.x - 2, textPos.y - textSize.height - 2,
                       textSize.width + 4, textSize.height + 4);
        if (textBg.x >= 0 && textBg.y >= 0 &&
            textBg.x + textBg.width < img.cols && 
            textBg.y + textBg.height < img.rows) {
            cv::Mat roi = img(textBg);
            cv::Mat overlay = roi.clone();
            cv::rectangle(overlay, cv::Point(0, 0), 
                        cv::Point(overlay.cols, overlay.rows),
                        bgColor, cv::FILLED);
            double alpha = displaySettings.labelBackgroundOpacity / 255.0;
            cv::addWeighted(overlay, alpha, roi, 1.0 - alpha, 0, roi);
        }
        
        // Desenhar texto
        cv::putText(img, numberLabel.toStdString(), textPos,
                   cv::FONT_HERSHEY_SIMPLEX, fontSize, textColor,
                   fontThickness, cv::LINE_AA);
        
        // Desenhar tipo abaixo do número (se houver)
        if (!typeLabel.isEmpty()) {
            cv::Size typeSize = cv::getTextSize(typeLabel.toStdString(),
                                               cv::FONT_HERSHEY_SIMPLEX, fontSize,
                                               fontThickness, nullptr);
            cv::Point typePos = textPos;
            typePos.y += textSize.height + 5;
            
            // Fundo do tipo
            cv::Rect typeBg(typePos.x - 2, typePos.y - typeSize.height - 2,
                           typeSize.width + 4, typeSize.height + 4);
            if (typeBg.x >= 0 && typeBg.y >= 0 &&
                typeBg.x + typeBg.width < img.cols && 
                typeBg.y + typeBg.height < img.rows) {
                cv::Mat roi = img(typeBg);
                cv::Mat overlay = roi.clone();
                cv::rectangle(overlay, cv::Point(0, 0), 
                            cv::Point(overlay.cols, overlay.rows),
                            bgColor, cv::FILLED);
                double alpha = displaySettings.labelBackgroundOpacity / 255.0;
                cv::addWeighted(overlay, alpha, roi, 1.0 - alpha, 0, roi);
            }
            
            // Texto do tipo
            cv::putText(img, typeLabel.toStdString(), typePos,
                       cv::FONT_HERSHEY_SIMPLEX, fontSize, textColor,
                       fontThickness, cv::LINE_AA);
        }
    }
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

void FragmentExportDialog::onResetToDefaults()
{
    loadDefaultSettings();
    
    // Cores
    markerColor = QColor(255, 0, 0);
    textColor = QColor(255, 0, 0);
    labelBgColor = QColor(255, 255, 255);
    markerColorButton->setStyleSheet("background-color: rgb(255, 0, 0); color: white;");
    textColorButton->setStyleSheet("background-color: rgb(255, 0, 0); color: white;");
    labelBgColorButton->setStyleSheet("background-color: rgb(255, 255, 255); color: black;");
    
    // Tamanhos
    markerSizeSpinBox->setValue(30);
    fontSizeSpinBox->setValue(26);
    
    // Checkboxes
    includeMinutiaeCheck->setChecked(true);
    showNumbersCheck->setChecked(true);
    showLabelTypeCheck->setChecked(true);
    showAnglesCheck->setChecked(true);
    maintainAspectCheck->setChecked(true);
    
    // Formato
    formatComboBox->setCurrentIndex(0);  // PNG
    qualitySlider->setValue(97);
    
    // Opacidade
    labelOpacitySlider->setValue(200);
    
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
        errorMessage = "Caminho do arquivo não especificado";
        return false;
    }
    
    try {
        // Renderizar imagem na resolução final
        int width = widthSpinBox->value();
        int height = heightSpinBox->value();
        
        cv::Mat finalImage = renderExportImage(width, height);
        
        if (finalImage.empty()) {
            errorMessage = "Erro ao renderizar imagem";
            return false;
        }
        
        // Preparar parâmetros de salvamento
        std::vector<int> params;
        
        int formatIdx = formatComboBox->currentIndex();
        if (formatIdx == 1) {  // JPEG
            params.push_back(cv::IMWRITE_JPEG_QUALITY);
            params.push_back(qualitySlider->value());
        } else if (formatIdx == 0) {  // PNG
            params.push_back(cv::IMWRITE_PNG_COMPRESSION);
            params.push_back(9);  // Máxima compressão
        }
        
        // Salvar imagem
        bool success = cv::imwrite(filePath.toStdString(), finalImage, params);
        
        if (!success) {
            errorMessage = "Falha ao salvar arquivo. Verifique as permissões e o caminho.";
            return false;
        }
        
        return true;
        
    } catch (const cv::Exception& e) {
        errorMessage = QString("Erro OpenCV: %1").arg(e.what());
        return false;
    } catch (const std::exception& e) {
        errorMessage = QString("Erro: %1").arg(e.what());
        return false;
    }
}
