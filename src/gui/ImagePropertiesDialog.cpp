#include "ImagePropertiesDialog.h"
#include "../core/ProjectManager.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QMessageBox>

namespace FingerprintEnhancer {

ImagePropertiesDialog::ImagePropertiesDialog(FingerprintImage* image, QWidget *parent)
    : QDialog(parent)
    , image(image)
{
    setWindowTitle("Propriedades da Imagem");
    setMinimumWidth(500);
    setMinimumHeight(400);
    
    setupUI();
    loadData();
}

void ImagePropertiesDialog::setupUI() {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // Grupo de identificação (editável)
    QGroupBox *idGroup = new QGroupBox("Identificação");
    QFormLayout *idLayout = new QFormLayout(idGroup);
    
    QHBoxLayout *numberLayout = new QHBoxLayout();
    QLabel *numberPrefixLabel = new QLabel("Imagem");
    numberSpinBox = new QSpinBox();
    numberSpinBox->setRange(1, 99);
    numberSpinBox->setToolTip("Número de identificação (01-99)");
    connect(numberSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ImagePropertiesDialog::validateNumber);
    numberLayout->addWidget(numberPrefixLabel);
    numberLayout->addWidget(numberSpinBox);
    numberLayout->addStretch();
    idLayout->addRow("Número:", numberLayout);
    
    nameEdit = new QLineEdit();
    nameEdit->setPlaceholderText("Nome da imagem...");
    idLayout->addRow("Nome:", nameEdit);
    
    uuidLabel = new QLabel();
    uuidLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    uuidLabel->setStyleSheet("QLabel { font-family: monospace; font-size: 9pt; color: gray; }");
    idLayout->addRow("UUID:", uuidLabel);
    
    mainLayout->addWidget(idGroup);
    
    // Grupo de informações
    QGroupBox *infoGroup = new QGroupBox("Informações do Arquivo");
    QFormLayout *infoLayout = new QFormLayout(infoGroup);
    
    filePathLabel = new QLabel();
    filePathLabel->setWordWrap(true);
    filePathLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    infoLayout->addRow("Arquivo:", filePathLabel);
    
    imageSizeLabel = new QLabel();
    infoLayout->addRow("Dimensões:", imageSizeLabel);
    
    fragmentCountLabel = new QLabel();
    infoLayout->addRow("Fragmentos:", fragmentCountLabel);
    
    minutiaeCountLabel = new QLabel();
    infoLayout->addRow("Minúcias:", minutiaeCountLabel);
    
    createdAtLabel = new QLabel();
    infoLayout->addRow("Criado em:", createdAtLabel);
    
    modifiedAtLabel = new QLabel();
    infoLayout->addRow("Modificado em:", modifiedAtLabel);
    
    hashLabel = new QLabel();
    hashLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    hashLabel->setStyleSheet("QLabel { font-family: monospace; font-size: 9pt; }");
    infoLayout->addRow("Hash MD5:", hashLabel);
    
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
    
    // Grupo de comentários
    QGroupBox *commentsGroup = new QGroupBox("Comentários");
    QVBoxLayout *commentsLayout = new QVBoxLayout(commentsGroup);
    
    commentsEdit = new QTextEdit();
    commentsEdit->setPlaceholderText("Digite comentários ou observações sobre esta imagem...");
    commentsLayout->addWidget(commentsEdit);
    
    mainLayout->addWidget(commentsGroup);
    
    // Botões
    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
}

void ImagePropertiesDialog::loadData() {
    if (!image) return;
    
    // Identificação
    numberSpinBox->setValue(image->displayNumber);
    nameEdit->setText(image->displayName);
    uuidLabel->setText(image->id);
    
    // Informações do arquivo
    QFileInfo fileInfo(image->originalFilePath);
    filePathLabel->setText(fileInfo.fileName());
    filePathLabel->setToolTip(image->originalFilePath);
    
    // Dimensões da imagem
    if (!image->originalImage.empty()) {
        imageSizeLabel->setText(QString("%1 × %2 pixels")
                               .arg(image->originalImage.cols)
                               .arg(image->originalImage.rows));
    } else {
        imageSizeLabel->setText("N/A");
    }
    
    // Contadores
    fragmentCountLabel->setText(QString::number(image->getFragmentCount()));
    minutiaeCountLabel->setText(QString::number(image->getTotalMinutiaeCount()));
    
    // Datas
    createdAtLabel->setText(image->createdAt.toString("dd/MM/yyyy HH:mm:ss"));
    modifiedAtLabel->setText(image->modifiedAt.toString("dd/MM/yyyy HH:mm:ss"));
    
    // Hash
    hashLabel->setText(image->originalHash);
    
    // Transformações
    rotationLabel->setText(QString("%1°").arg(image->currentRotationAngle, 0, 'f', 1));
    
    // Histórico de transformações geométricas
    int geomCount = image->transformationHistory.size();
    int procCount = image->processingHistory.size();
    int totalCount = geomCount + procCount;
    
    if (totalCount > 0) {
        transformsLabel->setText(QString("%1 geométrica(s), %2 processamento(s)").arg(geomCount).arg(procCount));
    } else {
        transformsLabel->setText("Nenhuma operação aplicada");
    }
    
    // Comentários
    commentsEdit->setPlainText(image->notes);
    nameEdit->setFocus();
}

void ImagePropertiesDialog::validateNumber() {
    if (!image) return;
    
    int newNumber = numberSpinBox->value();
    
    // Se não mudou, não precisa validar
    if (newNumber == image->displayNumber) {
        return;
    }
    
    // Reajustar números de outras imagens se houver conflito
    using PM = FingerprintEnhancer::ProjectManager;
    if (PM::instance().getCurrentProject()) {
        for (auto& img : PM::instance().getCurrentProject()->images) {
            if (img.id != image->id && img.displayNumber == newNumber) {
                // Encontrar próximo número disponível
                int nextAvailable = 1;
                bool found = false;
                while (!found && nextAvailable <= 99) {
                    found = true;
                    for (const auto& checkImg : PM::instance().getCurrentProject()->images) {
                        if (checkImg.displayNumber == nextAvailable) {
                            found = false;
                            nextAvailable++;
                            break;
                        }
                    }
                }
                if (nextAvailable <= 99) {
                    img.displayNumber = nextAvailable;
                    fprintf(stderr, "[IMAGE] Número reajustado: Imagem %s movida para %02d\n",
                            img.id.toStdString().c_str(), nextAvailable);
                }
            }
        }
    }
}

int ImagePropertiesDialog::getDisplayNumber() const {
    return numberSpinBox->value();
}

QString ImagePropertiesDialog::getDisplayName() const {
    return nameEdit->text().trimmed();
}

} // namespace FingerprintEnhancer
