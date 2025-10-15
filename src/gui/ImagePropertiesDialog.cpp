#include "ImagePropertiesDialog.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QDateTime>

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
    
    // Grupo de informações
    QGroupBox *infoGroup = new QGroupBox("Informações");
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
    
    // Comentários
    commentsEdit->setPlainText(image->notes);
    commentsEdit->setFocus();
}

} // namespace FingerprintEnhancer
