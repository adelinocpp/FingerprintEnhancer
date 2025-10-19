#include "CorrespondenceVisualizationDialog.h"
#include "../core/UserSettings.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QColorDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QFormLayout>
#include <QPainter>
#include <QPen>
#include <QSettings>
#include <opencv2/opencv.hpp>

CorrespondenceVisualizationDialog::CorrespondenceVisualizationDialog(QWidget *parent)
    : QDialog(parent),
      markerColor(145, 65, 172),
      textColor(145, 65, 172),
      labelBgColor(255, 255, 255),
      labelOpacity(100)
{
    setWindowTitle("Visualização de Correspondências");
    resize(1400, 700);
    
    setupUI();
    loadSettings();
}

void CorrespondenceVisualizationDialog::setupUI() {
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    
    // === COLUNA ESQUERDA: Configurações ===
    QVBoxLayout* settingsLayout = new QVBoxLayout();
    settingsLayout->setSpacing(10);
    
    QGroupBox* exportGroup = new QGroupBox("⚙️ Configurações de Exportação");
    QFormLayout* exportLayout = new QFormLayout(exportGroup);
    
    // Tamanho da marcação
    markerSizeSpinBox = new QSpinBox();
    markerSizeSpinBox->setRange(5, 50);
    markerSizeSpinBox->setValue(8);
    markerSizeSpinBox->setSuffix(" px");
    connect(markerSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CorrespondenceVisualizationDialog::onSettingsChanged);
    exportLayout->addRow("Tamanho marcação:", markerSizeSpinBox);
    
    // Tamanho da fonte
    fontSizeSpinBox = new QSpinBox();
    fontSizeSpinBox->setRange(6, 20);
    fontSizeSpinBox->setValue(8);
    fontSizeSpinBox->setSuffix(" pt");
    connect(fontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CorrespondenceVisualizationDialog::onSettingsChanged);
    exportLayout->addRow("Tamanho fonte:", fontSizeSpinBox);
    
    // Largura da linha
    lineWidthSpinBox = new QSpinBox();
    lineWidthSpinBox->setRange(1, 10);
    lineWidthSpinBox->setValue(2);
    lineWidthSpinBox->setSuffix(" px");
    connect(lineWidthSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &CorrespondenceVisualizationDialog::onSettingsChanged);
    exportLayout->addRow("Largura linha:", lineWidthSpinBox);
    
    // Tipo de símbolo
    symbolComboBox = new QComboBox();
    symbolComboBox->addItem("Círculo Simples");
    symbolComboBox->addItem("Círculo com X");
    symbolComboBox->addItem("Círculo com Seta");
    connect(symbolComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CorrespondenceVisualizationDialog::onSettingsChanged);
    exportLayout->addRow("Tipo símbolo:", symbolComboBox);
    
    // Posição rótulo
    labelPositionComboBox = new QComboBox();
    labelPositionComboBox->addItem("Acima");
    labelPositionComboBox->addItem("Abaixo");
    labelPositionComboBox->addItem("Esquerda");
    labelPositionComboBox->addItem("Direita");
    labelPositionComboBox->addItem("Ocultar");
    connect(labelPositionComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CorrespondenceVisualizationDialog::onSettingsChanged);
    exportLayout->addRow("Posição rótulo:", labelPositionComboBox);
    
    // Checkboxes de visualização
    showNumbersCheck = new QCheckBox("Mostrar números das minúcias");
    showNumbersCheck->setChecked(true);
    connect(showNumbersCheck, &QCheckBox::toggled,
            this, &CorrespondenceVisualizationDialog::onSettingsChanged);
    exportLayout->addRow("", showNumbersCheck);
    
    showLabelTypeCheck = new QCheckBox("Mostrar tipos das minúcias");
    showLabelTypeCheck->setChecked(true);
    connect(showLabelTypeCheck, &QCheckBox::toggled,
            this, &CorrespondenceVisualizationDialog::onSettingsChanged);
    exportLayout->addRow("", showLabelTypeCheck);
    
    showAnglesCheck = new QCheckBox("Mostrar direções");
    showAnglesCheck->setChecked(true);
    connect(showAnglesCheck, &QCheckBox::toggled,
            this, &CorrespondenceVisualizationDialog::onSettingsChanged);
    exportLayout->addRow("", showAnglesCheck);
    
    // Fundo branco
    whiteBackgroundCheck = new QCheckBox("Fundo branco (padrão: cinza)");
    whiteBackgroundCheck->setChecked(false);
    connect(whiteBackgroundCheck, &QCheckBox::toggled,
            this, &CorrespondenceVisualizationDialog::onSettingsChanged);
    exportLayout->addRow("", whiteBackgroundCheck);
    
    // Cores
    QHBoxLayout* colorsLayout = new QHBoxLayout();
    markerColorButton = new QPushButton("🎨 Marcação");
    markerColorButton->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
        .arg(markerColor.red()).arg(markerColor.green()).arg(markerColor.blue()));
    connect(markerColorButton, &QPushButton::clicked,
            this, &CorrespondenceVisualizationDialog::onMarkerColorClicked);
    colorsLayout->addWidget(markerColorButton);
    
    textColorButton = new QPushButton("🎨 Texto");
    textColorButton->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
        .arg(textColor.red()).arg(textColor.green()).arg(textColor.blue()));
    connect(textColorButton, &QPushButton::clicked,
            this, &CorrespondenceVisualizationDialog::onTextColorClicked);
    colorsLayout->addWidget(textColorButton);
    
    labelBgColorButton = new QPushButton("🎨 Fundo");
    labelBgColorButton->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
        .arg(labelBgColor.red()).arg(labelBgColor.green()).arg(labelBgColor.blue()));
    connect(labelBgColorButton, &QPushButton::clicked,
            this, &CorrespondenceVisualizationDialog::onLabelBgColorClicked);
    colorsLayout->addWidget(labelBgColorButton);
    exportLayout->addRow("Cores:", colorsLayout);
    
    // Opacidade rótulo
    QHBoxLayout* opacityLayout = new QHBoxLayout();
    labelOpacitySlider = new QSlider(Qt::Horizontal);
    labelOpacitySlider->setRange(0, 100);
    labelOpacitySlider->setValue(100);
    connect(labelOpacitySlider, &QSlider::valueChanged,
            this, &CorrespondenceVisualizationDialog::onLabelOpacityChanged);
    opacityLayout->addWidget(labelOpacitySlider);
    labelOpacityLabel = new QLabel("100%");
    labelOpacityLabel->setMinimumWidth(45);
    opacityLayout->addWidget(labelOpacityLabel);
    exportLayout->addRow("Opacidade:", opacityLayout);
    
    // Formato
    formatComboBox = new QComboBox();
    formatComboBox->addItems({"PNG (*.png)", "JPEG (*.jpg)", "TIFF (*.tif)", "BMP (*.bmp)"});
    exportLayout->addRow("Formato:", formatComboBox);
    
    settingsLayout->addWidget(exportGroup);
    
    // Botão aplicar padrões
    applySettingsButton = new QPushButton("✅ Aplicar como Padrão");
    applySettingsButton->setToolTip("Salvar estas configurações como padrão");
    connect(applySettingsButton, &QPushButton::clicked,
            this, &CorrespondenceVisualizationDialog::onApplySettingsClicked);
    settingsLayout->addWidget(applySettingsButton);
    
    settingsLayout->addStretch();
    
    mainLayout->addLayout(settingsLayout);
    
    // === COLUNA DIREITA: Preview ===
    QVBoxLayout* previewLayout = new QVBoxLayout();
    
    // Label para exibir imagem
    imageLabel = new QLabel();
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet("QLabel { background-color: #2b2b2b; }");
    previewLayout->addWidget(imageLabel, 1);
    
    // Botões de ação
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    saveButton = new QPushButton("💾 Salvar Imagem");
    closeButton = new QPushButton("Fechar");
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(closeButton);
    previewLayout->addLayout(buttonLayout);
    
    mainLayout->addLayout(previewLayout, 1);
    
    connect(saveButton, &QPushButton::clicked, this, &CorrespondenceVisualizationDialog::onSaveClicked);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
}

void CorrespondenceVisualizationDialog::loadSettings() {
    QSettings settings("FingerprintEnhancer", "FingerprintEnhancer");
    // Carregar configurações salvas
    markerSizeSpinBox->setValue(settings.value("CorrespondenceDialog/markerSize", 8).toInt());
    fontSizeSpinBox->setValue(settings.value("CorrespondenceDialog/fontSize", 8).toInt());
    lineWidthSpinBox->setValue(settings.value("CorrespondenceDialog/lineWidth", 2).toInt());
    showNumbersCheck->setChecked(settings.value("CorrespondenceDialog/showNumbers", true).toBool());
    showLabelTypeCheck->setChecked(settings.value("CorrespondenceDialog/showTypes", true).toBool());
    showAnglesCheck->setChecked(settings.value("CorrespondenceDialog/showAngles", true).toBool());
    whiteBackgroundCheck->setChecked(settings.value("CorrespondenceDialog/whiteBackground", false).toBool());
    formatComboBox->setCurrentIndex(settings.value("CorrespondenceDialog/format", 0).toInt());
    symbolComboBox->setCurrentIndex(settings.value("CorrespondenceDialog/symbol", 0).toInt());
    labelPositionComboBox->setCurrentIndex(settings.value("CorrespondenceDialog/labelPosition", 0).toInt());
    labelOpacitySlider->setValue(settings.value("CorrespondenceDialog/labelOpacity", 100).toInt());
    
    // Cores
    markerColor = settings.value("CorrespondenceDialog/markerColor", QColor(145, 65, 172)).value<QColor>();
    textColor = settings.value("CorrespondenceDialog/textColor", QColor(145, 65, 172)).value<QColor>();
    labelBgColor = settings.value("CorrespondenceDialog/labelBgColor", QColor(255, 255, 255)).value<QColor>();
    
    // Atualizar botões de cor
    markerColorButton->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
        .arg(markerColor.red()).arg(markerColor.green()).arg(markerColor.blue()));
    textColorButton->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
        .arg(textColor.red()).arg(textColor.green()).arg(textColor.blue()));
    labelBgColorButton->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
        .arg(labelBgColor.red()).arg(labelBgColor.green()).arg(labelBgColor.blue()));
}

void CorrespondenceVisualizationDialog::saveSettings() {
    QSettings settings("FingerprintEnhancer", "FingerprintEnhancer");
    settings.setValue("CorrespondenceDialog/markerSize", markerSizeSpinBox->value());
    settings.setValue("CorrespondenceDialog/fontSize", fontSizeSpinBox->value());
    settings.setValue("CorrespondenceDialog/lineWidth", lineWidthSpinBox->value());
    settings.setValue("CorrespondenceDialog/showNumbers", showNumbersCheck->isChecked());
    settings.setValue("CorrespondenceDialog/showTypes", showLabelTypeCheck->isChecked());
    settings.setValue("CorrespondenceDialog/showAngles", showAnglesCheck->isChecked());
    settings.setValue("CorrespondenceDialog/whiteBackground", whiteBackgroundCheck->isChecked());
    settings.setValue("CorrespondenceDialog/format", formatComboBox->currentIndex());
    settings.setValue("CorrespondenceDialog/symbol", symbolComboBox->currentIndex());
    settings.setValue("CorrespondenceDialog/labelPosition", labelPositionComboBox->currentIndex());
    settings.setValue("CorrespondenceDialog/labelOpacity", labelOpacitySlider->value());
    settings.setValue("CorrespondenceDialog/markerColor", markerColor);
    settings.setValue("CorrespondenceDialog/textColor", textColor);
    settings.setValue("CorrespondenceDialog/labelBgColor", labelBgColor);
}

void CorrespondenceVisualizationDialog::onApplySettingsClicked() {
    saveSettings();
    QMessageBox::information(this, "Configurações Salvas",
        "As configurações atuais foram salvas como padrão.");
}

void CorrespondenceVisualizationDialog::onSettingsChanged() {
    generateVisualization();
}

void CorrespondenceVisualizationDialog::onMarkerColorClicked() {
    QColor color = QColorDialog::getColor(markerColor, this, "Escolher Cor da Marcação");
    if (color.isValid()) {
        markerColor = color;
        markerColorButton->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
            .arg(color.red()).arg(color.green()).arg(color.blue()));
        generateVisualization();
    }
}

void CorrespondenceVisualizationDialog::onTextColorClicked() {
    QColor color = QColorDialog::getColor(textColor, this, "Escolher Cor do Texto");
    if (color.isValid()) {
        textColor = color;
        textColorButton->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
            .arg(color.red()).arg(color.green()).arg(color.blue()));
        generateVisualization();
    }
}

void CorrespondenceVisualizationDialog::onLabelBgColorClicked() {
    QColor color = QColorDialog::getColor(labelBgColor, this, "Escolher Cor de Fundo do Rótulo");
    if (color.isValid()) {
        labelBgColor = color;
        labelBgColorButton->setStyleSheet(QString("background-color: rgb(%1, %2, %3);")
            .arg(color.red()).arg(color.green()).arg(color.blue()));
        generateVisualization();
    }
}

void CorrespondenceVisualizationDialog::onLabelOpacityChanged(int value) {
    labelOpacity = value;
    labelOpacityLabel->setText(QString("%1%").arg(value));
    generateVisualization();
}

QColor CorrespondenceVisualizationDialog::getBackgroundColor() const {
    return whiteBackgroundCheck->isChecked() ? QColor(255, 255, 255) : QColor(43, 43, 43);
}

void CorrespondenceVisualizationDialog::setData(
    const cv::Mat& img1, const cv::Mat& img2,
    const QVector<FingerprintEnhancer::Minutia>& min1,
    const QVector<FingerprintEnhancer::Minutia>& min2,
    const QVector<QPair<int, int>>& corr)
{
    image1 = img1.clone();
    image2 = img2.clone();
    minutiae1 = min1;
    minutiae2 = min2;
    correspondences = corr;
    
    generateVisualization();
}

void CorrespondenceVisualizationDialog::generateVisualization() {
    if (image1.empty() || image2.empty()) return;
    
    // Converter para RGB se necessário
    cv::Mat img1Color, img2Color;
    if (image1.channels() == 1) {
        cv::cvtColor(image1, img1Color, cv::COLOR_GRAY2RGB);
    } else {
        img1Color = image1.clone();
    }
    if (image2.channels() == 1) {
        cv::cvtColor(image2, img2Color, cv::COLOR_GRAY2RGB);
    } else {
        img2Color = image2.clone();
    }
    
    // Calcular dimensões - NORMALIZAR ALTURA
    int h1 = img1Color.rows;
    int h2 = img2Color.rows;
    int w1 = img1Color.cols;
    int w2 = img2Color.cols;
    
    // Determinar altura comum (maior altura)
    int targetHeight = qMax(h1, h2);
    
    // Redimensionar imagens para ter a mesma altura
    cv::Mat img1Resized, img2Resized;
    if (h1 != targetHeight) {
        double scale = static_cast<double>(targetHeight) / h1;
        cv::resize(img1Color, img1Resized, cv::Size(), scale, scale, cv::INTER_LINEAR);
    } else {
        img1Resized = img1Color;
    }
    
    if (h2 != targetHeight) {
        double scale = static_cast<double>(targetHeight) / h2;
        cv::resize(img2Color, img2Resized, cv::Size(), scale, scale, cv::INTER_LINEAR);
    } else {
        img2Resized = img2Color;
    }
    
    // Atualizar dimensões após resize
    int w1_new = img1Resized.cols;
    int w2_new = img2Resized.cols;
    double scale1 = static_cast<double>(targetHeight) / h1;
    double scale2 = static_cast<double>(targetHeight) / h2;
    
    int totalWidth = w1_new + w2_new + 50;  // 50px de espaço entre imagens
    
    // Criar canvas combinado com cor de fundo configurável
    cv::Mat combined = cv::Mat::zeros(targetHeight, totalWidth, CV_8UC3);
    QColor bgColor = getBackgroundColor();
    combined.setTo(cv::Scalar(bgColor.blue(), bgColor.green(), bgColor.red()));
    
    // Copiar imagens redimensionadas
    img1Resized.copyTo(combined(cv::Rect(0, 0, w1_new, targetHeight)));
    img2Resized.copyTo(combined(cv::Rect(w1_new + 50, 0, w2_new, targetHeight)));
    
    int offset1Y = 0;
    int offset2X = w1_new + 50;
    int offset2Y = 0;
    
    // Converter para QImage para desenhar com QPainter
    QImage qImg(combined.data, combined.cols, combined.rows, combined.step, QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(qImg.rgbSwapped());
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Obter configurações
    int lineWidth = lineWidthSpinBox->value();
    int markerRadius = markerSizeSpinBox->value();
    int fontSize = fontSizeSpinBox->value();
    bool showNumbers = showNumbersCheck->isChecked();
    
    // Desenhar linhas de correspondência PRIMEIRO (atrás das minúcias)
    QPen connectionPen;
    connectionPen.setWidth(lineWidth);
    connectionPen.setStyle(Qt::SolidLine);
    connectionPen.setColor(markerColor);
    
    for (int i = 0; i < correspondences.size(); i++) {
        int idx1 = correspondences[i].first;
        int idx2 = correspondences[i].second;
        
        if (idx1 < 0 || idx1 >= minutiae1.size() || idx2 < 0 || idx2 >= minutiae2.size()) {
            continue;
        }
        
        QPoint p1(minutiae1[idx1].position.x() * scale1, 
                  minutiae1[idx1].position.y() * scale1 + offset1Y);
        QPoint p2(minutiae2[idx2].position.x() * scale2 + offset2X, 
                  minutiae2[idx2].position.y() * scale2 + offset2Y);
        
        painter.setPen(connectionPen);
        painter.drawLine(p1, p2);
    }
    
    // Desenhar minúcias - FRAGMENTO 1 (esquerda) - aplicar escala
    for (int i = 0; i < minutiae1.size(); i++) {
        QPoint pos(minutiae1[i].position.x() * scale1, minutiae1[i].position.y() * scale1 + offset1Y);
        
        // Verificar se está em correspondência
        bool matched = false;
        for (const auto& corr : correspondences) {
            if (corr.first == i) {
                matched = true;
                break;
            }
        }
        
        QColor color = matched ? markerColor : QColor(128, 128, 128);
        if (showNumbers) {
            drawMinutiaMarker(painter, pos, i + 1, color, markerRadius, fontSize);
        } else {
            drawMinutiaMarker(painter, pos, -1, color, markerRadius, fontSize);
        }
    }
    
    // Desenhar minúcias - FRAGMENTO 2 (direita) - aplicar escala
    for (int i = 0; i < minutiae2.size(); i++) {
        QPoint pos(minutiae2[i].position.x() * scale2 + offset2X, 
                   minutiae2[i].position.y() * scale2 + offset2Y);
        
        // Verificar se está em correspondência
        bool matched = false;
        for (const auto& corr : correspondences) {
            if (corr.second == i) {
                matched = true;
                break;
            }
        }
        
        QColor color = matched ? markerColor : QColor(128, 128, 128);
        if (showNumbers) {
            drawMinutiaMarker(painter, pos, i + 1, color, markerRadius, fontSize);
        } else {
            drawMinutiaMarker(painter, pos, -1, color, markerRadius, fontSize);
        }
    }
    
    // Adicionar legenda - cor adaptada ao fundo
    QColor textColor = whiteBackgroundCheck->isChecked() ? QColor(0, 0, 0) : QColor(255, 255, 255);
    painter.setPen(textColor);
    QFont font = painter.font();
    font.setPointSize(12);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(10, 30, QString("Fragmento 1 (%1 minúcias)").arg(minutiae1.size()));
    painter.drawText(offset2X + 10, 30, QString("Fragmento 2 (%1 minúcias)").arg(minutiae2.size()));
    painter.drawText(10, targetHeight - 10, QString("%1 correspondências encontradas").arg(correspondences.size()));
    
    visualizationPixmap = pixmap;
    
    // Escalar para caber na janela se necessário
    QPixmap scaledPixmap = pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    imageLabel->setPixmap(scaledPixmap);
}

void CorrespondenceVisualizationDialog::drawMinutiaMarker(QPainter& painter, const QPoint& pos, int number, const QColor& color, int radius, int fontSize) {
    // Círculo
    QPen circlePen(color, lineWidthSpinBox->value());
    painter.setPen(circlePen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(pos, radius, radius);
    
    // Número (apenas se number > 0)
    if (number > 0) {
        QFont font = painter.font();
        font.setPointSize(fontSize);
        font.setBold(true);
        painter.setFont(font);
        
        QString text = QString::number(number);
        QRect textRect = painter.fontMetrics().boundingRect(text);
        QPoint textPos(pos.x() + radius + 4, pos.y() + textRect.height() / 2 - 2);
        
        // Fundo do texto (adaptar ao fundo)
        QColor bgColor = whiteBackgroundCheck->isChecked() ? QColor(255, 255, 255, 200) : QColor(0, 0, 0, 180);
        painter.fillRect(textRect.translated(textPos), bgColor);
        painter.setPen(color);
        painter.drawText(textPos, text);
    }
}

void CorrespondenceVisualizationDialog::onSaveClicked() {
    // Obter formato selecionado
    QString formatText = formatComboBox->currentText();
    QString filter;
    QString extension;
    
    if (formatText.contains("PNG")) {
        filter = "Imagens PNG (*.png)";
        extension = ".png";
    } else if (formatText.contains("JPEG")) {
        filter = "Imagens JPEG (*.jpg)";
        extension = ".jpg";
    } else if (formatText.contains("TIFF")) {
        filter = "Imagens TIFF (*.tif)";
        extension = ".tif";
    } else {
        filter = "Imagens BMP (*.bmp)";
        extension = ".bmp";
    }
    
    QString fileName = QFileDialog::getSaveFileName(this,
        "Salvar Visualização de Correspondências",
        QString(),
        filter + ";;Todos os Arquivos (*)");
    
    if (fileName.isEmpty()) return;
    
    // Adicionar extensão se não tiver
    if (!fileName.endsWith(extension, Qt::CaseInsensitive)) {
        fileName += extension;
    }
    
    if (visualizationPixmap.save(fileName)) {
        QMessageBox::information(this, "Sucesso", 
            QString("Imagem salva com sucesso em:\n%1").arg(fileName));
    } else {
        QMessageBox::warning(this, "Erro", "Falha ao salvar imagem.");
    }
}
