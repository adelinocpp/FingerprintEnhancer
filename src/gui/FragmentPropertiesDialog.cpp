#include "FragmentPropertiesDialog.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QDateTime>

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
    
    // Grupo de informações
    QGroupBox *infoGroup = new QGroupBox("Informações");
    QFormLayout *infoLayout = new QFormLayout(infoGroup);
    
    fragmentIdLabel = new QLabel();
    fragmentIdLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    fragmentIdLabel->setStyleSheet("QLabel { font-family: monospace; font-size: 9pt; }");
    infoLayout->addRow("ID:", fragmentIdLabel);
    
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
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    
    mainLayout->addWidget(buttonBox);
}

void FragmentPropertiesDialog::loadData() {
    if (!fragment) return;
    
    // ID do fragmento (primeiros 8 caracteres)
    fragmentIdLabel->setText(fragment->id.left(8) + "...");
    fragmentIdLabel->setToolTip(fragment->id);
    
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
    
    // Comentários
    commentsEdit->setPlainText(fragment->notes);
    commentsEdit->setFocus();
}

} // namespace FingerprintEnhancer
