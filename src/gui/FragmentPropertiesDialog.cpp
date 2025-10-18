#include "FragmentPropertiesDialog.h"
#include "../core/ProjectManager.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QMessageBox>
#include <QFileInfo>

namespace FingerprintEnhancer {

FragmentPropertiesDialog::FragmentPropertiesDialog(Fragment* fragment, QWidget *parent)
    : QDialog(parent)
    , fragment(fragment)
{
    setWindowTitle("Propriedades do Fragmento");
    setMinimumWidth(500);
    setMinimumHeight(350);
    
    setupUI();
    loadData();
}

void FragmentPropertiesDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Grupo de identificação (editável)
    QGroupBox *idGroup = new QGroupBox("Identificação");
    QFormLayout *idLayout = new QFormLayout(idGroup);
    
    QHBoxLayout *numberLayout = new QHBoxLayout();
    QLabel *numberPrefixLabel = new QLabel("Fragmento");
    numberSpinBox = new QSpinBox();
    numberSpinBox->setRange(1, 99);
    numberSpinBox->setToolTip("Número do fragmento (apenas a segunda parte, ex: 02 em 01-02)");
    connect(numberSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &FragmentPropertiesDialog::validateNumber);
    numberLayout->addWidget(numberPrefixLabel);
    numberLayout->addWidget(numberSpinBox);
    numberLayout->addStretch();
    idLayout->addRow("Número:", numberLayout);
    
    nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText("Nome do fragmento...");
    idLayout->addRow("Nome:", nameEdit);
    
    uuidLabel = new QLabel();
    uuidLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    uuidLabel->setStyleSheet("QLabel { font-family: monospace; font-size: 9pt; color: gray; }");
    idLayout->addRow("UUID:", uuidLabel);
    
    mainLayout->addWidget(idGroup);
    
    // Grupo de informações
    QGroupBox *infoGroup = new QGroupBox("Informações do Fragmento");
    QFormLayout *infoLayout = new QFormLayout(infoGroup);
    
    parentImageLabel = new QLabel();
    infoLayout->addRow("Imagem de Origem:", parentImageLabel);
    
    fragmentIdLabel = new QLabel();
    fragmentIdLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    fragmentIdLabel->setStyleSheet("QLabel { font-family: monospace; font-size: 9pt; }");
    infoLayout->addRow("Número Completo:", fragmentIdLabel);
    
    sourceRectLabel = new QLabel();
    infoLayout->addRow("Região de Origem:", sourceRectLabel);
    
    imageSizeLabel = new QLabel();
    infoLayout->addRow("Dimensões:", imageSizeLabel);
    
    minutiaeCountLabel = new QLabel();
    infoLayout->addRow("Minúcias:", minutiaeCountLabel);
    
    createdAtLabel = new QLabel();
    infoLayout->addRow("Criado em:", createdAtLabel);
    
    modifiedAtLabel = new QLabel();
    infoLayout->addRow("Modificado em:", modifiedAtLabel);
    
    mainLayout->addWidget(infoGroup);
    
    // Grupo de operações aplicadas
    QGroupBox *transformGroup = new QGroupBox("Operações Aplicadas");
    QFormLayout *transformLayout = new QFormLayout(transformGroup);
    
    rotationLabel = new QLabel();
    transformLayout->addRow("Rotação:", rotationLabel);
    
    transformsLabel = new QLabel();
    transformsLabel->setWordWrap(true);
    transformsLabel->setStyleSheet("QLabel { color: gray; font-size: 9pt; }");
    transformLayout->addRow("Histórico:", transformsLabel);
    
    mainLayout->addWidget(transformGroup);
    
    // Grupo de escala
    QGroupBox *scaleGroup = new QGroupBox("Escala (Calibração)");
    QFormLayout *scaleLayout = new QFormLayout(scaleGroup);
    
    scaleSpinBox = new QDoubleSpinBox();
    scaleSpinBox->setRange(0.0, 1000.0);
    scaleSpinBox->setDecimals(2);
    scaleSpinBox->setSingleStep(0.1);
    scaleSpinBox->setSuffix(" px/mm");
    scaleSpinBox->setSpecialValueText("Não definida");
    scaleSpinBox->setToolTip("Defina a escala medindo uma distância conhecida na imagem.\n"
                             "Ex: Se 10mm = 78.5 pixels, então escala = 78.5/10 = 7.85 px/mm");
    scaleLayout->addRow("Pixels por Milímetro:", scaleSpinBox);
    
    dpiSpinBox = new QDoubleSpinBox();
    dpiSpinBox->setRange(0.0, 25400.0);
    dpiSpinBox->setDecimals(1);
    dpiSpinBox->setSingleStep(1.0);
    dpiSpinBox->setSuffix(" DPI");
    dpiSpinBox->setSpecialValueText("Não definida");
    dpiSpinBox->setToolTip("Resolução em Dots Per Inch (pontos por polegada).\n"
                           "Converte automaticamente de/para px/mm (1 polegada = 25.4 mm)");
    scaleLayout->addRow("Resolução (DPI):", dpiSpinBox);
    
    // Conectar os spinboxes para sincronização
    connect(scaleSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &FragmentPropertiesDialog::onScaleChanged);
    connect(dpiSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &FragmentPropertiesDialog::onDpiChanged);
    
    QLabel *scaleHintLabel = new QLabel("💡 Necessária para comparação 1:1");
    scaleHintLabel->setStyleSheet("QLabel { color: #666; font-size: 9pt; }");
    scaleLayout->addRow("", scaleHintLabel);
    
    mainLayout->addWidget(scaleGroup);
    
    // Grupo de comentários
    QGroupBox *commentsGroup = new QGroupBox("Comentários");
    QVBoxLayout *commentsLayout = new QVBoxLayout(commentsGroup);
    
    commentsEdit = new QTextEdit();
    commentsEdit->setPlaceholderText("Digite comentários ou observações sobre este fragmento...\n"
                                    "Por exemplo: identificação do dedo, posição, qualidade, etc.");
    commentsLayout->addWidget(commentsEdit);
    
    mainLayout->addWidget(commentsGroup);
    
    // Botões
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &FragmentPropertiesDialog::saveData);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
}

void FragmentPropertiesDialog::loadData() {
    if (!fragment) return;
    
    using PM = FingerprintEnhancer::ProjectManager;
    
    // Identificação
    numberSpinBox->setValue(fragment->displayNumber);
    nameEdit->setText(fragment->displayName);
    uuidLabel->setText(fragment->id);
    
    // Número completo formatado
    QString fullNumber = PM::instance().getFragmentDisplayNumber(fragment->id);
    fragmentIdLabel->setText(fullNumber);
    fragmentIdLabel->setToolTip(fragment->id);
    
    // Imagem de origem
    FingerprintImage* parentImage = PM::instance().getCurrentProject()->findImage(fragment->parentImageId);
    if (parentImage) {
        QFileInfo fileInfo(parentImage->originalFilePath);
        QString imgNumber = PM::instance().getImageDisplayNumber(fragment->parentImageId);
        parentImageLabel->setText(QString("Imagem %1: %2").arg(imgNumber).arg(parentImage->displayName));
    } else {
        parentImageLabel->setText("N/A");
    }
    
    // Região de origem
    sourceRectLabel->setText(QString("(%1, %2) - %3×%4 pixels")
                            .arg(fragment->sourceRect.x())
                            .arg(fragment->sourceRect.y())
                            .arg(fragment->sourceRect.width())
                            .arg(fragment->sourceRect.height()));
    
    // Dimensões do fragmento
    if (!fragment->originalImage.empty()) {
        imageSizeLabel->setText(QString("%1 × %2 pixels")
                               .arg(fragment->originalImage.cols)
                               .arg(fragment->originalImage.rows));
    } else {
        imageSizeLabel->setText("N/A");
    }
    
    // Contador de minúcias
    minutiaeCountLabel->setText(QString::number(fragment->getMinutiaeCount()));
    
    // Datas
    createdAtLabel->setText(fragment->createdAt.toString("dd/MM/yyyy HH:mm:ss"));
    modifiedAtLabel->setText(fragment->modifiedAt.toString("dd/MM/yyyy HH:mm:ss"));
    
    // Transformações
    rotationLabel->setText(QString("%1°").arg(fragment->currentRotationAngle, 0, 'f', 1));
    
    // Histórico de transformações geométricas e processamento
    int geomCount = fragment->geometricTransforms.size();
    int procCount = fragment->processingHistory.size();
    int totalCount = geomCount + procCount;
    
    if (totalCount > 0) {
        transformsLabel->setText(QString("%1 geométrica(s), %2 processamento(s)").arg(geomCount).arg(procCount));
    } else {
        transformsLabel->setText("Nenhuma operação aplicada");
    }
    
    // Escala (bloquear sinais para evitar loop)
    scaleSpinBox->blockSignals(true);
    dpiSpinBox->blockSignals(true);
    
    scaleSpinBox->setValue(fragment->pixelsPerMM);
    updateDpiFromScale();
    
    scaleSpinBox->blockSignals(false);
    dpiSpinBox->blockSignals(false);
    
    // Comentários
    commentsEdit->setPlainText(fragment->notes);
    nameEdit->setFocus();
}

void FragmentPropertiesDialog::saveData() {
    if (!fragment) return;
    
    // Salvar escala (px/mm e DPI)
    fragment->pixelsPerMM = scaleSpinBox->value();
    fragment->dpi = dpiSpinBox->value();
    
    // Salvar comentários
    fragment->notes = commentsEdit->toPlainText();
    fragment->modifiedAt = QDateTime::currentDateTime();
    
    fprintf(stderr, "[FRAGMENT] Escala salva: %.2f px/mm (%.1f DPI)\n", 
            fragment->pixelsPerMM, fragment->dpi);
    
    accept();
}

double FragmentPropertiesDialog::getPixelsPerMM() const {
    return scaleSpinBox->value();
}

void FragmentPropertiesDialog::validateNumber() {
    if (!fragment) return;
    
    int newNumber = numberSpinBox->value();
    
    // Se não mudou, não precisa validar
    if (newNumber == fragment->displayNumber) {
        return;
    }
    
    // Reajustar números de outros fragmentos se houver conflito
    using PM = FingerprintEnhancer::ProjectManager;
    if (PM::instance().getCurrentProject()) {
        FingerprintImage* parentImage = PM::instance().getCurrentProject()->findImage(fragment->parentImageId);
        if (parentImage) {
            for (auto& frag : parentImage->fragments) {
                if (frag.id != fragment->id && frag.displayNumber == newNumber) {
                    // Encontrar próximo número disponível
                    int nextAvailable = 1;
                    bool found = false;
                    while (!found && nextAvailable <= 99) {
                        found = true;
                        for (const auto& checkFrag : parentImage->fragments) {
                            if (checkFrag.displayNumber == nextAvailable) {
                                found = false;
                                nextAvailable++;
                                break;
                            }
                        }
                    }
                    if (nextAvailable <= 99) {
                        frag.displayNumber = nextAvailable;
                        fprintf(stderr, "[FRAGMENT] Número reajustado: Fragmento %s movido para %02d\n",
                                frag.id.toStdString().c_str(), nextAvailable);
                    }
                }
            }
        }
    }
}

int FragmentPropertiesDialog::getDisplayNumber() const {
    return numberSpinBox->value();
}

QString FragmentPropertiesDialog::getDisplayName() const {
    return nameEdit->text().trimmed();
}

void FragmentPropertiesDialog::onScaleChanged(double value) {
    // Atualizar DPI quando px/mm muda
    dpiSpinBox->blockSignals(true);
    updateDpiFromScale();
    dpiSpinBox->blockSignals(false);
}

void FragmentPropertiesDialog::onDpiChanged(double value) {
    // Atualizar px/mm quando DPI muda
    scaleSpinBox->blockSignals(true);
    updateScaleFromDpi();
    scaleSpinBox->blockSignals(false);
}

void FragmentPropertiesDialog::updateDpiFromScale() {
    // Conversão: DPI = px/mm * 25.4 (1 inch = 25.4 mm)
    double pxPerMM = scaleSpinBox->value();
    double dpi = pxPerMM * 25.4;
    dpiSpinBox->setValue(dpi);
}

void FragmentPropertiesDialog::updateScaleFromDpi() {
    // Conversão: px/mm = DPI / 25.4
    double dpi = dpiSpinBox->value();
    double pxPerMM = dpi / 25.4;
    scaleSpinBox->setValue(pxPerMM);
}

} // namespace FingerprintEnhancer
