#include "MinutiaeDisplayDialog.h"
#include <QGridLayout>
#include <QSlider>
#include <QPainter>
#include <QPixmap>

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

    mainLayout->addWidget(sizeGroup);

    // Grupo: Aparência dos Rótulos
    QGroupBox* labelGroup = new QGroupBox("Aparência dos Rótulos");
    QVBoxLayout* labelLayout = new QVBoxLayout(labelGroup);

    QHBoxLayout* colorLayout = new QHBoxLayout();
    colorLayout->addWidget(new QLabel("Cor de Fundo:"));
    colorButton = new QPushButton();
    colorButton->setFixedSize(100, 30);
    updateColorButton();
    colorLayout->addWidget(colorButton);
    colorLayout->addStretch();
    labelLayout->addLayout(colorLayout);

    QHBoxLayout* opacityLayout = new QHBoxLayout();
    opacityLayout->addWidget(new QLabel("Opacidade:"));
    opacitySlider = new QSlider(Qt::Horizontal);
    opacitySlider->setRange(0, 255);
    opacitySlider->setValue(settings.labelBackgroundOpacity);
    opacityLayout->addWidget(opacitySlider);
    opacityLabel = new QLabel(QString("%1%").arg(int(settings.labelBackgroundOpacity * 100.0 / 255.0)));
    opacityLayout->addWidget(opacityLabel);
    labelLayout->addLayout(opacityLayout);

    mainLayout->addWidget(labelGroup);

    // Preview
    QGroupBox* previewGroup = new QGroupBox("Visualização");
    QVBoxLayout* previewLayout = new QVBoxLayout(previewGroup);
    previewLabel = new QLabel();
    previewLabel->setMinimumHeight(120);
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLayout->addWidget(previewLabel);
    mainLayout->addWidget(previewGroup);

    // Botões
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    mainLayout->addWidget(buttonBox);

    // Conectar sinais
    connect(symbolCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MinutiaeDisplayDialog::onSymbolChanged);
    connect(markerSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MinutiaeDisplayDialog::onMarkerSizeChanged);
    connect(labelFontSizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MinutiaeDisplayDialog::onLabelFontSizeChanged);
    connect(colorButton, &QPushButton::clicked, this, &MinutiaeDisplayDialog::onChooseBackgroundColor);
    connect(opacitySlider, &QSlider::valueChanged, this, &MinutiaeDisplayDialog::onOpacityChanged);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &MinutiaeDisplayDialog::onAccepted);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &MinutiaeDisplayDialog::onRejected);

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

void MinutiaeDisplayDialog::onChooseBackgroundColor() {
    QColor color = QColorDialog::getColor(settings.labelBackgroundColor, this, "Escolher Cor de Fundo");
    if (color.isValid()) {
        settings.labelBackgroundColor = color;
        settings.labelBackgroundColor.setAlpha(settings.labelBackgroundOpacity);
        updateColorButton();
        updatePreview();
    }
}

void MinutiaeDisplayDialog::onOpacityChanged(int value) {
    settings.labelBackgroundOpacity = value;
    settings.labelBackgroundColor.setAlpha(value);
    opacityLabel->setText(QString("%1%").arg(int(value * 100.0 / 255.0)));
    updateColorButton();
    updatePreview();
}

void MinutiaeDisplayDialog::onAccepted() {
    accepted = true;
    accept();
}

void MinutiaeDisplayDialog::onRejected() {
    accepted = false;
    reject();
}

void MinutiaeDisplayDialog::updateColorButton() {
    QString colorStyle = QString("background-color: rgba(%1, %2, %3, %4);")
        .arg(settings.labelBackgroundColor.red())
        .arg(settings.labelBackgroundColor.green())
        .arg(settings.labelBackgroundColor.blue())
        .arg(settings.labelBackgroundOpacity);
    colorButton->setStyleSheet(colorStyle);
}

void MinutiaeDisplayDialog::updatePreview() {
    // Criar pixmap para preview
    QPixmap pixmap(400, 100);
    pixmap.fill(QColor(240, 240, 240));

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Desenhar exemplo de minúcia
    QPoint center(80, 50);
    int size = settings.markerSize;
    double angle = 45.0; // Ângulo de exemplo

    // Desenhar símbolo
    painter.setPen(QPen(QColor(0, 0, 255), 2));
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

    // Desenhar rótulos de exemplo
    QFont font;
    font.setPointSize(settings.labelFontSize);
    painter.setFont(font);

    // Rótulo de índice
    QString indexText = " 1 ";
    QRect indexRect = painter.fontMetrics().boundingRect(indexText);
    indexRect.moveCenter(QPoint(center.x(), center.y() - size/2 - 15));
    indexRect.adjust(-2, -2, 2, 2);

    painter.fillRect(indexRect, settings.labelBackgroundColor);
    painter.setPen(Qt::black);
    painter.drawText(indexRect, Qt::AlignCenter, indexText);

    // Rótulo de tipo
    QString typeText = " Bifurcação ";
    QRect typeRect = painter.fontMetrics().boundingRect(typeText);
    typeRect.moveCenter(QPoint(center.x(), center.y() + size/2 + 15));
    typeRect.adjust(-2, -2, 2, 2);

    painter.fillRect(typeRect, settings.labelBackgroundColor);
    painter.setPen(Qt::black);
    painter.drawText(typeRect, Qt::AlignCenter, typeText);

    // Adicionar texto descritivo
    painter.setPen(Qt::darkGray);
    painter.drawText(QRect(200, 10, 190, 80), Qt::AlignLeft | Qt::TextWordWrap,
                     QString("Símbolo: %1\n\nTamanho: %2 px\nFonte: %3 pt\nOpacidade: %4%")
                     .arg(settings.getSymbolName())
                     .arg(settings.markerSize)
                     .arg(settings.labelFontSize)
                     .arg(int(settings.labelBackgroundOpacity * 100.0 / 255.0)));

    painter.end();

    previewLabel->setPixmap(pixmap);
}

} // namespace FingerprintEnhancer
