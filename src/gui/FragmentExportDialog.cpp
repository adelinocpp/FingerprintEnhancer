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
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

FragmentExportDialog::FragmentExportDialog(
    const FingerprintEnhancer::Fragment* frag,
    double currentScale,
    QWidget *parent)
    : QDialog(parent),
      fragment(frag),
      scale(currentScale),
      markerColor(0, 255, 0),      // Verde - mesmo da visualização
      textColor(255, 255, 0)       // Amarelo - mesmo da visualização
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
            this, &FragmentExportDialog::onAnySettingChanged);
    resLayout->addRow("Largura:", widthSpinBox);
    
    heightSpinBox = new QSpinBox();
    heightSpinBox->setRange(100, 10000);
    heightSpinBox->setSuffix(" px");
    connect(heightSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &FragmentExportDialog::onAnySettingChanged);
    resLayout->addRow("Altura:", heightSpinBox);
    
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
    
    markerSizeSpinBox = new QSpinBox();
    markerSizeSpinBox->setRange(10, 200);
    markerSizeSpinBox->setValue(40);
    markerSizeSpinBox->setSuffix(" px");
    markerSizeSpinBox->setToolTip("Diâmetro do círculo da marcação");
    connect(markerSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &FragmentExportDialog::onAnySettingChanged);
    markersLayout->addRow("Tamanho marcação:", markerSizeSpinBox);
    
    fontSizeSpinBox = new QSpinBox();
    fontSizeSpinBox->setRange(8, 72);
    fontSizeSpinBox->setValue(24);
    fontSizeSpinBox->setSuffix(" pt");
    fontSizeSpinBox->setToolTip("Tamanho da fonte dos rótulos");
    connect(fontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &FragmentExportDialog::onAnySettingChanged);
    markersLayout->addRow("Tamanho fonte:", fontSizeSpinBox);
    
    settingsLayout->addWidget(markersGroup);
    
    // Grupo: Opções de Visualização
    QGroupBox *displayGroup = new QGroupBox("👁️ Opções de Visualização");
    QVBoxLayout *displayLayout = new QVBoxLayout(displayGroup);
    
    showNumbersCheck = new QCheckBox("Mostrar números das minúcias");
    showNumbersCheck->setChecked(true);
    connect(showNumbersCheck, &QCheckBox::toggled, this, &FragmentExportDialog::onAnySettingChanged);
    displayLayout->addWidget(showNumbersCheck);
    
    showTypesCheck = new QCheckBox("Mostrar tipos das minúcias");
    showTypesCheck->setChecked(true);
    connect(showTypesCheck, &QCheckBox::toggled, this, &FragmentExportDialog::onAnySettingChanged);
    displayLayout->addWidget(showTypesCheck);
    
    showDirectionsCheck = new QCheckBox("Mostrar direções");
    showDirectionsCheck->setChecked(true);
    connect(showDirectionsCheck, &QCheckBox::toggled, this, &FragmentExportDialog::onAnySettingChanged);
    displayLayout->addWidget(showDirectionsCheck);
    
    // Cores
    QHBoxLayout *colorsLayout = new QHBoxLayout();
    markerColorButton = new QPushButton("🎨 Cor da Marcação");
    markerColorButton->setStyleSheet(QString("background-color: rgb(0, 255, 0); color: black;"));
    connect(markerColorButton, &QPushButton::clicked, this, &FragmentExportDialog::onMarkerColorClicked);
    colorsLayout->addWidget(markerColorButton);
    
    textColorButton = new QPushButton("🎨 Cor do Texto");
    textColorButton->setStyleSheet(QString("background-color: rgb(255, 255, 0); color: black;"));
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
    // Dimensões originais
    widthSpinBox->setValue(originalWidth);
    heightSpinBox->setValue(originalHeight);
    
    // Caminho padrão
    QString suggestedName = QString("fragmento_%1_marcado.png")
        .arg(fragment->id.left(8));
    QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    filePathEdit->setText(QDir(defaultPath).filePath(suggestedName));
    
    // Calcular tamanho das marcações baseado na escala
    if (scale > 0) {
        int markerSize = static_cast<int>(4.0 * scale);  // 4mm
        markerSizeSpinBox->setValue(std::max(20, std::min(100, markerSize)));
        
        int fontSize = static_cast<int>(scale * 2.5);  // Aproximadamente 2.5mm
        fontSizeSpinBox->setValue(std::max(12, std::min(48, fontSize)));
    }
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
        int markerRadius = markerSizeSpinBox->value() * scaleFactor / 2;
        double fontSize = fontSizeSpinBox->value() * scaleFactor / 30.0;
        int fontThickness = std::max(1, (int)(markerRadius / 8.0));
        
        cv::Scalar markerColorCV(markerColor.blue(), markerColor.green(), markerColor.red());
        cv::Scalar textColorCV(textColor.blue(), textColor.green(), textColor.red());
        
        for (int i = 0; i < fragment->minutiae.size(); ++i) {
            const auto& m = fragment->minutiae[i];
            cv::Point center(
                (int)(m.position.x() * scaleFactor),
                (int)(m.position.y() * scaleFactor)
            );
            
            // Desenhar círculo
            cv::circle(result, center, markerRadius, markerColorCV, 2, cv::LINE_AA);
            
            // Desenhar linha de direção
            if (showDirectionsCheck->isChecked()) {
                double angle = m.angle * M_PI / 180.0;
                int lineLength = markerRadius + markerRadius / 2;
                cv::Point endPoint(
                    center.x + (int)(lineLength * std::cos(angle)),
                    center.y + (int)(lineLength * std::sin(angle))
                );
                cv::line(result, center, endPoint, markerColorCV, 2, cv::LINE_AA);
            }
            
            // Desenhar rótulo
            QString label;
            if (showNumbersCheck->isChecked() && showTypesCheck->isChecked()) {
                label = QString("%1-%2").arg(i + 1).arg(m.getTypeAbbreviation());
            } else if (showNumbersCheck->isChecked()) {
                label = QString::number(i + 1);
            } else if (showTypesCheck->isChecked()) {
                label = m.getTypeAbbreviation();
            }
            
            if (!label.isEmpty()) {
                int textOffsetY = markerRadius + (int)(fontSize * 35);
                cv::Point textPos(center.x - markerRadius / 2, center.y - textOffsetY);
                
                // Fundo do texto
                cv::Size textSize = cv::getTextSize(
                    label.toStdString(),
                    cv::FONT_HERSHEY_SIMPLEX,
                    fontSize,
                    fontThickness,
                    nullptr
                );
                
                cv::Rect textBg(
                    std::max(0, textPos.x - 2),
                    std::max(0, textPos.y - textSize.height - 2),
                    std::min(textSize.width + 4, result.cols - textPos.x + 2),
                    textSize.height + 4
                );
                
                if (textBg.x + textBg.width <= result.cols && 
                    textBg.y + textBg.height <= result.rows &&
                    textBg.width > 0 && textBg.height > 0) {
                    cv::Mat roi = result(textBg);
                    cv::Mat bgColor(textBg.size(), result.type(), cv::Scalar(0, 0, 0));
                    cv::addWeighted(roi, 0.6, bgColor, 0.4, 0, roi);
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

void FragmentExportDialog::onResetToDefaults()
{
    loadDefaultSettings();
    markerColor = QColor(0, 255, 0);
    textColor = QColor(255, 255, 0);
    markerColorButton->setStyleSheet("background-color: rgb(0, 255, 0); color: black;");
    textColorButton->setStyleSheet("background-color: rgb(255, 255, 0); color: black;");
    
    includeMinutiaeCheck->setChecked(true);
    showNumbersCheck->setChecked(true);
    showTypesCheck->setChecked(true);
    showDirectionsCheck->setChecked(true);
    maintainAspectCheck->setChecked(true);
    
    formatComboBox->setCurrentIndex(0);  // PNG
    qualitySlider->setValue(95);
    
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
