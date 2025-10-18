#include "MinutiaeDisplayDialog.h"
#include "../core/UserSettings.h"
#include <QGridLayout>
#include <QSlider>
#include <QPainter>
#include <QPixmap>
#include <QMessageBox>

namespace FingerprintEnhancer {

MinutiaeDisplayDialog::MinutiaeDisplayDialog(const MinutiaeDisplaySettings& currentSettings, QWidget *parent)
    : QDialog(parent)
    , settings(currentSettings)
    , accepted(false)
{
    setWindowTitle("Configurações de Visualização de Minúcias");
    setMinimumWidth(500);
    setupUI();
}

void MinutiaeDisplayDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Grupo: Símbolo da Marcação
    QGroupBox* symbolGroup = new QGroupBox("Símbolo da Marcação");
    QFormLayout* symbolLayout = new QFormLayout(symbolGroup);

    symbolCombo = new QComboBox();
    symbolCombo->addItem("Círculo Simples", static_cast<int>(MinutiaeSymbol::CIRCLE));
    symbolCombo->addItem("Círculo com X", static_cast<int>(MinutiaeSymbol::CIRCLE_X));
    symbolCombo->addItem("Círculo com Seta (Ângulo)", static_cast<int>(MinutiaeSymbol::CIRCLE_ARROW));
    symbolCombo->addItem("Círculo com Cruz", static_cast<int>(MinutiaeSymbol::CIRCLE_CROSS));
    symbolCombo->addItem("Triângulo", static_cast<int>(MinutiaeSymbol::TRIANGLE));
    symbolCombo->addItem("Quadrado", static_cast<int>(MinutiaeSymbol::SQUARE));
    symbolCombo->addItem("Losango", static_cast<int>(MinutiaeSymbol::DIAMOND));

    // Definir símbolo atual
    int currentIndex = symbolCombo->findData(static_cast<int>(settings.symbol));
    if (currentIndex >= 0) {
        symbolCombo->setCurrentIndex(currentIndex);
    }

    symbolLayout->addRow("Tipo de Símbolo:", symbolCombo);
    mainLayout->addWidget(symbolGroup);

    // Grupo: Tamanhos
    QGroupBox* sizeGroup = new QGroupBox("Tamanhos");
    QFormLayout* sizeLayout = new QFormLayout(sizeGroup);

    markerSizeSpinBox = new QSpinBox();
    markerSizeSpinBox->setRange(10, 100);
    markerSizeSpinBox->setValue(settings.markerSize);
    markerSizeSpinBox->setSuffix(" px");
    sizeLayout->addRow("Tamanho da Marcação:", markerSizeSpinBox);

    labelFontSizeSpinBox = new QSpinBox();
    labelFontSizeSpinBox->setRange(6, 24);
    labelFontSizeSpinBox->setValue(settings.labelFontSize);
    labelFontSizeSpinBox->setSuffix(" pt");
    sizeLayout->addRow("Tamanho da Fonte:", labelFontSizeSpinBox);

    labelPositionCombo = new QComboBox();
    labelPositionCombo->addItem("À Direita (Padrão)", static_cast<int>(FingerprintEnhancer::MinutiaLabelPosition::RIGHT));
    labelPositionCombo->addItem("À Esquerda", static_cast<int>(FingerprintEnhancer::MinutiaLabelPosition::LEFT));
    labelPositionCombo->addItem("Acima", static_cast<int>(FingerprintEnhancer::MinutiaLabelPosition::ABOVE));
    labelPositionCombo->addItem("Abaixo", static_cast<int>(FingerprintEnhancer::MinutiaLabelPosition::BELOW));
    labelPositionCombo->addItem("Oculto", static_cast<int>(FingerprintEnhancer::MinutiaLabelPosition::HIDDEN));
    int labelPosIndex = labelPositionCombo->findData(static_cast<int>(settings.defaultLabelPosition));
    if (labelPosIndex >= 0) labelPositionCombo->setCurrentIndex(labelPosIndex);
    sizeLayout->addRow("Posição do Rótulo:", labelPositionCombo);

    mainLayout->addWidget(sizeGroup);

    // Grupo: Cores
    QGroupBox* colorGroup = new QGroupBox("Cores");
    QFormLayout* colorLayout = new QFormLayout(colorGroup);

    // Cor da marcação
    markerColorButton = new QPushButton("Escolher Cor");
    markerColorButton->setFixedSize(150, 30);
    updateMarkerColorButton();
    colorLayout->addRow("Cor da Marcação:", markerColorButton);

    // Cor do texto
    textColorButton = new QPushButton("Escolher Cor");
    textColorButton->setFixedSize(150, 30);
    updateTextColorButton();
    colorLayout->addRow("Cor do Texto:", textColorButton);

    // Cor de fundo do rótulo
    bgColorButton = new QPushButton("Escolher Cor");
    bgColorButton->setFixedSize(150, 30);
    updateBgColorButton();
    colorLayout->addRow("Cor de Fundo Rótulo:", bgColorButton);

    mainLayout->addWidget(colorGroup);

    // Grupo: Aparência dos Rótulos
    QGroupBox* labelGroup = new QGroupBox("Aparência dos Rótulos");
    QVBoxLayout* labelLayout = new QVBoxLayout(labelGroup);

    QHBoxLayout* opacityLayout = new QHBoxLayout();
    opacityLayout->addWidget(new QLabel("Opacidade:"));
    opacitySlider = new QSlider(Qt::Horizontal);
    opacitySlider->setRange(0, 255);
    opacitySlider->setValue(settings.labelBackgroundOpacity);
    opacityLayout->addWidget(opacitySlider);
    opacityLabel = new QLabel(QString("%1%").arg(int(settings.labelBackgroundOpacity * 100.0 / 255.0)));
    opacityLayout->addWidget(opacityLabel);
    labelLayout->addLayout(opacityLayout);
    
    // Checkboxes para controlar exibição
    showLabelTypeCheckBox = new QCheckBox("Exibir tipo no rótulo (se desmarcado, mostra apenas o número)");
    showLabelTypeCheckBox->setChecked(settings.showLabelType);
    labelLayout->addWidget(showLabelTypeCheckBox);
    
    showAnglesCheckBox = new QCheckBox("Exibir ângulo da minúcia");
    showAnglesCheckBox->setChecked(settings.showAngles);
    labelLayout->addWidget(showAnglesCheckBox);

    mainLayout->addWidget(labelGroup);

    // Preview
    QGroupBox* previewGroup = new QGroupBox("Visualização");
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);
    previewLabel = new QLabel();
    previewLabel->setMinimumHeight(200);
    previewLabel->setMinimumWidth(470);
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLayout->addWidget(previewLabel);
    mainLayout->addWidget(previewGroup);

    // Botões
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Cancel);
    buttonBox->button(QDialogButtonBox::Apply)->setText("Aplicar");
    mainLayout->addWidget(buttonBox);

    // Conectar sinais
    connect(symbolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MinutiaeDisplayDialog::onSymbolChanged);
    connect(markerSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MinutiaeDisplayDialog::onMarkerSizeChanged);
    connect(labelFontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MinutiaeDisplayDialog::onLabelFontSizeChanged);
    connect(labelPositionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MinutiaeDisplayDialog::onLabelPositionChanged);
    connect(markerColorButton, &QPushButton::clicked, this, &MinutiaeDisplayDialog::onChooseMarkerColor);
    connect(textColorButton, &QPushButton::clicked, this, &MinutiaeDisplayDialog::onChooseTextColor);
    connect(bgColorButton, &QPushButton::clicked, this, &MinutiaeDisplayDialog::onChooseBackgroundColor);
    connect(opacitySlider, &QSlider::valueChanged, this, &MinutiaeDisplayDialog::onOpacityChanged);
    
    // CRÍTICO: Conectar checkboxes para atualização em tempo real
    connect(showLabelTypeCheckBox, &QCheckBox::stateChanged, this, &MinutiaeDisplayDialog::onShowLabelTypeChanged);
    connect(showAnglesCheckBox, &QCheckBox::stateChanged, this, &MinutiaeDisplayDialog::onShowAnglesChanged);
    
    connect(buttonBox, &QDialogButtonBox::accepted, this, &MinutiaeDisplayDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &MinutiaeDisplayDialog::onRejected);
    connect(buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, 
            this, &MinutiaeDisplayDialog::onApplyClicked);

    // Atualizar preview inicial
    updatePreview();
}

void MinutiaeDisplayDialog::onSymbolChanged(int index) {
    settings.symbol = static_cast<MinutiaeSymbol>(symbolCombo->itemData(index).toInt());
    updatePreview();
}

void MinutiaeDisplayDialog::onMarkerSizeChanged(int value) {
    settings.markerSize = value;
    updatePreview();
}

void MinutiaeDisplayDialog::onLabelFontSizeChanged(int value) {
    settings.labelFontSize = value;
    updatePreview();
}

void MinutiaeDisplayDialog::onLabelPositionChanged(int index) {
    settings.defaultLabelPosition = static_cast<FingerprintEnhancer::MinutiaLabelPosition>(labelPositionCombo->itemData(index).toInt());
    updatePreview();
}

void MinutiaeDisplayDialog::onChooseMarkerColor() {
    QColor color = QColorDialog::getColor(settings.markerColor, this, "Escolher Cor da Marcação");
    if (color.isValid()) {
        settings.markerColor = color;
        updateMarkerColorButton();
        updatePreview();
    }
}

void MinutiaeDisplayDialog::onChooseTextColor() {
    QColor color = QColorDialog::getColor(settings.textColor, this, "Escolher Cor do Texto");
    if (color.isValid()) {
        settings.textColor = color;
        updateTextColorButton();
        updatePreview();
    }
}

void MinutiaeDisplayDialog::onChooseBackgroundColor() {
    QColor color = QColorDialog::getColor(settings.labelBackgroundColor, this, "Escolher Cor de Fundo do Rótulo");
    if (color.isValid()) {
        settings.labelBackgroundColor = color;
        settings.labelBackgroundColor.setAlpha(settings.labelBackgroundOpacity);
        updateBgColorButton();
        updatePreview();
    }
}

void MinutiaeDisplayDialog::onOpacityChanged(int value) {
    settings.labelBackgroundOpacity = value;
    settings.labelBackgroundColor.setAlpha(value);
    opacityLabel->setText(QString("%1%").arg(int(value * 100.0 / 255.0)));
    updateBgColorButton();
    updatePreview();
}

void MinutiaeDisplayDialog::onAccepted() {
    // Salvar ao aceitar (OK)
    saveToGlobalSettings();
    accepted = true;
    accept();
}

void MinutiaeDisplayDialog::onRejected() {
    accepted = false;
    reject();
}

void MinutiaeDisplayDialog::onApplyClicked() {
    // Salvar sem fechar o diálogo
    saveToGlobalSettings();
    QMessageBox::information(this, "Configurações Aplicadas", 
                            "As configurações foram salvas como padrão global.");
}

void MinutiaeDisplayDialog::saveToGlobalSettings() {
    auto& userSettings = FingerprintEnhancer::UserSettings::instance();
    
    // Salvar TODAS as configurações de visualização
    userSettings.setViewMarkerColor(settings.markerColor);
    userSettings.setViewTextColor(settings.textColor);
    userSettings.setViewLabelBgColor(settings.labelBackgroundColor);
    userSettings.setViewMarkerSize(settings.markerSize);
    userSettings.setViewFontSize(settings.labelFontSize);
    userSettings.setViewLabelOpacity(settings.labelBackgroundOpacity * 100 / 255); // Converter 0-255 para 0-100
    userSettings.setViewShowNumbers(true); // Sempre true na visualização
    userSettings.setViewShowTypes(settings.showLabelType);
    userSettings.setViewShowAngles(settings.showAngles);
    userSettings.setViewLabelPosition(static_cast<int>(settings.defaultLabelPosition));
    userSettings.setViewSymbol(static_cast<int>(settings.symbol));
    
    // Gravar no arquivo .ini
    userSettings.save();
    
    fprintf(stderr, "[USER_SETTINGS] Configurações de visualização salvas:\n");
    fprintf(stderr, "  - Marcação: rgb(%d,%d,%d)\n", 
            settings.markerColor.red(), settings.markerColor.green(), settings.markerColor.blue());
    fprintf(stderr, "  - Texto: rgb(%d,%d,%d)\n",
            settings.textColor.red(), settings.textColor.green(), settings.textColor.blue());
    fprintf(stderr, "  - Tamanho: %d px, Fonte: %d pt\n",
            settings.markerSize, settings.labelFontSize);
}

void MinutiaeDisplayDialog::onShowLabelTypeChanged(int state) {
    settings.showLabelType = (state == Qt::Checked);
    updatePreview();
}

void MinutiaeDisplayDialog::onShowAnglesChanged(int state) {
    settings.showAngles = (state == Qt::Checked);
    updatePreview();
}

void MinutiaeDisplayDialog::updateMarkerColorButton() {
    QString colorStyle = QString("background-color: rgb(%1, %2, %3); color: %4;")
        .arg(settings.markerColor.red())
        .arg(settings.markerColor.green())
        .arg(settings.markerColor.blue())
        .arg(settings.markerColor.lightness() > 128 ? "black" : "white");
    markerColorButton->setStyleSheet(colorStyle);
}

void MinutiaeDisplayDialog::updateTextColorButton() {
    QString colorStyle = QString("background-color: rgb(%1, %2, %3); color: %4;")
        .arg(settings.textColor.red())
        .arg(settings.textColor.green())
        .arg(settings.textColor.blue())
        .arg(settings.textColor.lightness() > 128 ? "black" : "white");
    textColorButton->setStyleSheet(colorStyle);
}

void MinutiaeDisplayDialog::updateBgColorButton() {
    QString colorStyle = QString("background-color: rgba(%1, %2, %3, %4); color: %5;")
        .arg(settings.labelBackgroundColor.red())
        .arg(settings.labelBackgroundColor.green())
        .arg(settings.labelBackgroundColor.blue())
        .arg(settings.labelBackgroundOpacity)
        .arg(settings.labelBackgroundColor.lightness() > 128 ? "black" : "white");
    bgColorButton->setStyleSheet(colorStyle);
}

void MinutiaeDisplayDialog::updatePreview() {
    // Criar pixmap para preview (maior para acomodar rótulos em todas as posições)
    QPixmap pixmap(450, 180);
    pixmap.fill(QColor(240, 240, 240));

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Desenhar exemplo de minúcia (centralizado verticalmente para dar espaço)
    QPoint center(100, 90);
    int size = settings.markerSize;
    double angle = 45.0; // Ângulo de exemplo

    // Desenhar símbolo com cor da marcação
    painter.setPen(QPen(settings.markerColor, 2));
    painter.setBrush(Qt::NoBrush);

    switch (settings.symbol) {
        case MinutiaeSymbol::CIRCLE:
            painter.drawEllipse(center, size/2, size/2);
            break;

        case MinutiaeSymbol::CIRCLE_X: {
            painter.drawEllipse(center, size/2, size/2);
            int offset = size / 4;
            painter.drawLine(center.x() - offset, center.y() - offset, center.x() + offset, center.y() + offset);
            painter.drawLine(center.x() - offset, center.y() + offset, center.x() + offset, center.y() - offset);
            break;
        }

        case MinutiaeSymbol::CIRCLE_ARROW: {
            painter.drawEllipse(center, size/2, size/2);
            // Desenhar seta baseada no ângulo
            double rad = angle * M_PI / 180.0;
            int arrowLen = size / 2;
            QPoint arrowEnd(center.x() + arrowLen * cos(rad), center.y() - arrowLen * sin(rad));
            painter.drawLine(center, arrowEnd);
            // Ponta da seta
            QPoint arrowTip1(arrowEnd.x() - 5 * cos(rad + M_PI/4), arrowEnd.y() + 5 * sin(rad + M_PI/4));
            QPoint arrowTip2(arrowEnd.x() - 5 * cos(rad - M_PI/4), arrowEnd.y() + 5 * sin(rad - M_PI/4));
            painter.drawLine(arrowEnd, arrowTip1);
            painter.drawLine(arrowEnd, arrowTip2);
            break;
        }

        case MinutiaeSymbol::CIRCLE_CROSS: {
            painter.drawEllipse(center, size/2, size/2);
            int offset = size / 3;
            painter.drawLine(center.x(), center.y() - offset, center.x(), center.y() + offset);
            painter.drawLine(center.x() - offset, center.y(), center.x() + offset, center.y());
            break;
        }

        case MinutiaeSymbol::TRIANGLE: {
            QPolygon triangle;
            int h = size * 0.866; // altura do triângulo equilátero
            triangle << QPoint(center.x(), center.y() - h/2)
                     << QPoint(center.x() - size/2, center.y() + h/2)
                     << QPoint(center.x() + size/2, center.y() + h/2);
            painter.drawPolygon(triangle);
            break;
        }

        case MinutiaeSymbol::SQUARE: {
            painter.drawRect(center.x() - size/2, center.y() - size/2, size, size);
            break;
        }

        case MinutiaeSymbol::DIAMOND: {
            QPolygon diamond;
            diamond << QPoint(center.x(), center.y() - size/2)
                    << QPoint(center.x() + size/2, center.y())
                    << QPoint(center.x(), center.y() + size/2)
                    << QPoint(center.x() - size/2, center.y());
            painter.drawPolygon(diamond);
            break;
        }
    }

    // Desenhar ângulo se habilitado (e não for CIRCLE_ARROW que já mostra)
    if (settings.showAngles && settings.symbol != MinutiaeSymbol::CIRCLE_ARROW) {
        int lineLength = size * 2;
        double rad = angle * M_PI / 180.0;
        int endX = center.x() + static_cast<int>(lineLength * cos(rad));
        int endY = center.y() - static_cast<int>(lineLength * sin(rad));
        painter.drawLine(center, QPoint(endX, endY));
    }

    // Desenhar rótulos de exemplo baseado na posição selecionada
    if (settings.defaultLabelPosition != FingerprintEnhancer::MinutiaLabelPosition::HIDDEN) {
        QFont font;
        font.setPointSize(settings.labelFontSize);
        painter.setFont(font);

        QString numberText = "( 1 )";
        QString typeText = " BE .";
        QRect numberRect = painter.fontMetrics().boundingRect(numberText);
        QRect typeRect = painter.fontMetrics().boundingRect(typeText);
        
        int margin = 5;
        
        // Calcular altura do tipo se for exibido
        int typeHeight = 0;
        if (settings.showLabelType) {
            typeHeight = typeRect.height() + 2; // +2 para espaçamento entre número e tipo
        }
        
        QPoint numberPos, typePos;
        
        // Calcular posições baseado em defaultLabelPosition
        // Se não há tipo, ajustar para ficar mais próximo da marcação
        switch (settings.defaultLabelPosition) {
            case FingerprintEnhancer::MinutiaLabelPosition::RIGHT: // À direita (padrão)
                numberPos = QPoint(center.x() + size/2 + margin, center.y() - size/2);
                break;
                
            case FingerprintEnhancer::MinutiaLabelPosition::LEFT: // À esquerda
                numberPos = QPoint(center.x() - size/2 - margin - numberRect.width(), center.y() - size/2);
                break;
                
            case FingerprintEnhancer::MinutiaLabelPosition::ABOVE: // Acima
                // Se tem tipo, o número fica mais longe; se não tem, fica mais perto-
                numberPos = QPoint(center.x() - numberRect.width()/2, center.y() - size/2 - margin - numberRect.height()); //  typeHeight
                break;
                
            case FingerprintEnhancer::MinutiaLabelPosition::BELOW: // Abaixo
                numberPos = QPoint(center.x() - numberRect.width()/2, center.y() + size/2 + margin + numberRect.height());
                break;
                
            default:
                numberPos = QPoint(center.x() + size/2 + margin, center.y() - size/2);
        }

        // Desenhar rótulo de número com cor do texto
        painter.fillRect(numberRect.translated(numberPos), settings.labelBackgroundColor);
        painter.setPen(settings.textColor);
        painter.drawText(numberPos, numberText);

        // Desenhar rótulo de tipo (se habilitado)
        if (settings.showLabelType) {
            // Tipo sempre em relação ao número
            switch (settings.defaultLabelPosition) {
                case FingerprintEnhancer::MinutiaLabelPosition::RIGHT:
                    // Tipo abaixo do número, alinhado à esquerda
                    typePos = QPoint(numberPos.x(), numberPos.y() + numberRect.height() + 2);
                    break;
                case FingerprintEnhancer::MinutiaLabelPosition::LEFT:
                    // Tipo abaixo do número, alinhado à direita
                    typePos = QPoint(numberPos.x() + numberRect.width() - typeRect.width(), numberPos.y() + numberRect.height() + 2);
                    break;
                case FingerprintEnhancer::MinutiaLabelPosition::ABOVE:
                    // Tipo acima do número, centralizado
                    typePos = QPoint(center.x() - typeRect.width()/2, numberPos.y() - typeRect.height() - 2);
                    break;
                case FingerprintEnhancer::MinutiaLabelPosition::BELOW:
                    // Tipo abaixo do número, centralizado
                    typePos = QPoint(center.x() - typeRect.width()/2, numberPos.y() + numberRect.height() + 2);
                    break;
                default:
                    typePos = QPoint(numberPos.x(), numberPos.y() + numberRect.height() + 2);
            }
            
            painter.fillRect(typeRect.translated(typePos), settings.labelBackgroundColor);
            painter.drawText(typePos, typeText);
        }
    }

    // Adicionar texto descritivo
    QString positionName;
    switch (settings.defaultLabelPosition) {
        case FingerprintEnhancer::MinutiaLabelPosition::RIGHT: positionName = "À Direita"; break;
        case FingerprintEnhancer::MinutiaLabelPosition::LEFT: positionName = "À Esquerda"; break;
        case FingerprintEnhancer::MinutiaLabelPosition::ABOVE: positionName = "Acima"; break;
        case FingerprintEnhancer::MinutiaLabelPosition::BELOW: positionName = "Abaixo"; break;
        case FingerprintEnhancer::MinutiaLabelPosition::HIDDEN: positionName = "Oculto"; break;
    }
    
    painter.setPen(Qt::darkGray);
    painter.drawText(QRect(220, 20, 220, 140), Qt::AlignLeft | Qt::TextWordWrap,
                     QString("Símbolo: %1\n\nTamanho: %2 px\nFonte: %3 pt\nOpacidade: %4%\nPosição: %5")
                     .arg(settings.getSymbolName())
                     .arg(settings.markerSize)
                     .arg(settings.labelFontSize)
                     .arg(int(settings.labelBackgroundOpacity * 100.0 / 255.0))
                     .arg(positionName));

    painter.end();

    previewLabel->setPixmap(pixmap);
}

} // namespace FingerprintEnhancer
