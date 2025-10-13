#include "ScaleConfigDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <cmath>

ScaleConfigDialog::ScaleConfigDialog(QWidget *parent)
    : QDialog(parent),
      currentMode(MODE_RIDGE_SPACING),
      updatingValues(false)
{
    setWindowTitle("⚙️ Configuração de Escala");
    setupUI();
    
    // Valores padrão
    setRidgeSpacing(0.4545);  // Padrão AFIS
}

ScaleConfigDialog::~ScaleConfigDialog()
{
}

void ScaleConfigDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Informação no topo
    infoLabel = new QLabel(
        "Configure os parâmetros de escala para calibração de impressões digitais.\n"
        "Modifique qualquer valor - os demais serão calculados automaticamente."
    );
    infoLabel->setWordWrap(true);
    infoLabel->setStyleSheet("QLabel { background-color: #E3F2FD; padding: 10px; border-radius: 5px; }");
    mainLayout->addWidget(infoLabel);
    
    // Grupo: Parâmetros Biológicos
    QGroupBox *bioGroup = new QGroupBox("📏 Parâmetros Biológicos (Cristas)");
    QFormLayout *bioLayout = new QFormLayout(bioGroup);
    
    // Distância entre cristas
    ridgeSpacingSpinBox = new QDoubleSpinBox();
    ridgeSpacingSpinBox->setRange(0.1, 2.0);
    ridgeSpacingSpinBox->setDecimals(4);
    ridgeSpacingSpinBox->setSingleStep(0.0001);
    ridgeSpacingSpinBox->setSuffix(" mm");
    ridgeSpacingSpinBox->setToolTip("Distância média entre cristas papilares adjacentes");
    bioLayout->addRow("Distância entre cristas:", ridgeSpacingSpinBox);
    
    // Cristas por 10mm
    ridgesPer10mmSpinBox = new QDoubleSpinBox();
    ridgesPer10mmSpinBox->setRange(5.0, 100.0);
    ridgesPer10mmSpinBox->setDecimals(2);
    ridgesPer10mmSpinBox->setSingleStep(0.1);
    ridgesPer10mmSpinBox->setSuffix(" cristas/10mm");
    ridgesPer10mmSpinBox->setToolTip("Número de cristas em um segmento de 10mm");
    bioLayout->addRow("Cristas por 10mm:", ridgesPer10mmSpinBox);
    
    mainLayout->addWidget(bioGroup);
    
    // Grupo: Resolução de Imagem
    QGroupBox *resGroup = new QGroupBox("🖼️ Resolução de Imagem");
    QFormLayout *resLayout = new QFormLayout(resGroup);
    
    // Pixels por mm
    pixelsPerMmSpinBox = new QDoubleSpinBox();
    pixelsPerMmSpinBox->setRange(1.0, 1000.0);
    pixelsPerMmSpinBox->setDecimals(2);
    pixelsPerMmSpinBox->setSingleStep(1.0);
    pixelsPerMmSpinBox->setSuffix(" px/mm");
    pixelsPerMmSpinBox->setToolTip("Quantidade de pixels por milímetro");
    resLayout->addRow("Pixels por mm:", pixelsPerMmSpinBox);
    
    // DPI
    dpiSpinBox = new QDoubleSpinBox();
    dpiSpinBox->setRange(10.0, 10000.0);
    dpiSpinBox->setDecimals(0);
    dpiSpinBox->setSingleStep(10.0);
    dpiSpinBox->setSuffix(" DPI");
    dpiSpinBox->setToolTip("Dots Per Inch - resolução em pontos por polegada");
    resLayout->addRow("DPI (resolução):", dpiSpinBox);
    
    mainLayout->addWidget(resGroup);
    
    // Conectar sinais
    connect(ridgeSpacingSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ScaleConfigDialog::onRidgeSpacingChanged);
    connect(ridgesPer10mmSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ScaleConfigDialog::onRidgesPer10mmChanged);
    connect(pixelsPerMmSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ScaleConfigDialog::onPixelsPerMmChanged);
    connect(dpiSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ScaleConfigDialog::onDPIChanged);
    
    // Botões
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    
    resetButton = new QPushButton("🔄 Restaurar Padrão");
    resetButton->setToolTip("Restaurar valores padrão AFIS (0.4545mm)");
    connect(resetButton, &QPushButton::clicked, this, &ScaleConfigDialog::resetToDefaults);
    
    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel
    );
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    buttonLayout->addWidget(resetButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(buttonBox);
    
    mainLayout->addLayout(buttonLayout);
    
    setMinimumWidth(450);
}

void ScaleConfigDialog::blockSignalsTemporarily(bool block)
{
    ridgeSpacingSpinBox->blockSignals(block);
    ridgesPer10mmSpinBox->blockSignals(block);
    pixelsPerMmSpinBox->blockSignals(block);
    dpiSpinBox->blockSignals(block);
}

void ScaleConfigDialog::onRidgeSpacingChanged(double value)
{
    if (updatingValues) return;
    updatingValues = true;
    currentMode = MODE_RIDGE_SPACING;
    
    blockSignalsTemporarily(true);
    
    // Calcular cristas por 10mm: 10mm / espaçamento
    double ridgesPer10mm = 10.0 / value;
    ridgesPer10mmSpinBox->setValue(ridgesPer10mm);
    
    blockSignalsTemporarily(false);
    updatingValues = false;
}

void ScaleConfigDialog::onRidgesPer10mmChanged(double value)
{
    if (updatingValues) return;
    updatingValues = true;
    currentMode = MODE_RIDGES_PER_10MM;
    
    blockSignalsTemporarily(true);
    
    // Calcular espaçamento: 10mm / número de cristas
    double ridgeSpacing = 10.0 / value;
    ridgeSpacingSpinBox->setValue(ridgeSpacing);
    
    blockSignalsTemporarily(false);
    updatingValues = false;
}

void ScaleConfigDialog::onPixelsPerMmChanged(double value)
{
    if (updatingValues) return;
    updatingValues = true;
    currentMode = MODE_PIXELS_PER_MM;
    
    blockSignalsTemporarily(true);
    
    // Calcular DPI: pixels/mm * 25.4 mm/inch
    double dpi = value * 25.4;
    dpiSpinBox->setValue(dpi);
    
    blockSignalsTemporarily(false);
    updatingValues = false;
}

void ScaleConfigDialog::onDPIChanged(double value)
{
    if (updatingValues) return;
    updatingValues = true;
    currentMode = MODE_DPI;
    
    blockSignalsTemporarily(true);
    
    // Calcular pixels/mm: DPI / 25.4
    double pixelsPerMm = value / 25.4;
    pixelsPerMmSpinBox->setValue(pixelsPerMm);
    
    blockSignalsTemporarily(false);
    updatingValues = false;
}

void ScaleConfigDialog::resetToDefaults()
{
    blockSignalsTemporarily(true);
    
    // Padrão AFIS: 0.4545mm entre cristas
    ridgeSpacingSpinBox->setValue(0.4545);
    ridgesPer10mmSpinBox->setValue(22.0);
    
    // Deixar resolução como está (depende da imagem)
    
    blockSignalsTemporarily(false);
}

// Getters
double ScaleConfigDialog::getRidgeSpacing() const
{
    return ridgeSpacingSpinBox->value();
}

double ScaleConfigDialog::getRidgesPer10mm() const
{
    return ridgesPer10mmSpinBox->value();
}

double ScaleConfigDialog::getPixelsPerMm() const
{
    return pixelsPerMmSpinBox->value();
}

double ScaleConfigDialog::getDPI() const
{
    return dpiSpinBox->value();
}

// Setters
void ScaleConfigDialog::setRidgeSpacing(double mm)
{
    updatingValues = true;
    blockSignalsTemporarily(true);
    
    ridgeSpacingSpinBox->setValue(mm);
    ridgesPer10mmSpinBox->setValue(10.0 / mm);
    
    blockSignalsTemporarily(false);
    updatingValues = false;
}

void ScaleConfigDialog::setRidgesPer10mm(double count)
{
    updatingValues = true;
    blockSignalsTemporarily(true);
    
    ridgesPer10mmSpinBox->setValue(count);
    ridgeSpacingSpinBox->setValue(10.0 / count);
    
    blockSignalsTemporarily(false);
    updatingValues = false;
}

void ScaleConfigDialog::setPixelsPerMm(double ppm)
{
    updatingValues = true;
    blockSignalsTemporarily(true);
    
    pixelsPerMmSpinBox->setValue(ppm);
    dpiSpinBox->setValue(ppm * 25.4);
    
    blockSignalsTemporarily(false);
    updatingValues = false;
}

void ScaleConfigDialog::setDPI(double dpi)
{
    updatingValues = true;
    blockSignalsTemporarily(true);
    
    dpiSpinBox->setValue(dpi);
    pixelsPerMmSpinBox->setValue(dpi / 25.4);
    
    blockSignalsTemporarily(false);
    updatingValues = false;
}

void ScaleConfigDialog::setInputMode(InputMode mode)
{
    currentMode = mode;
}
