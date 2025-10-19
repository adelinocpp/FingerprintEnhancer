#include "AboutDialog.h"
#include <QGroupBox>
#include <QApplication>
#include <QDesktopServices>
#include <QUrl>
#include <opencv2/opencv.hpp>

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Sobre - [Nome do Programa]");
    setMinimumSize(500, 400);
    setupUI();
}

void AboutDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    
    // ==================== CABEÇALHO ====================
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    // Ícone provisório (placeholder)
    QLabel* iconLabel = new QLabel();
    QPixmap iconPixmap(64, 64);
    iconPixmap.fill(Qt::darkBlue);
    iconLabel->setPixmap(iconPixmap);
    iconLabel->setFixedSize(64, 64);
    headerLayout->addWidget(iconLabel);
    
    // Nome e versão
    QVBoxLayout* titleLayout = new QVBoxLayout();
    
    QLabel* nameLabel = new QLabel("<b style='font-size: 18pt;'>[Nome do Programa]</b>");
    nameLabel->setTextFormat(Qt::RichText);
    titleLayout->addWidget(nameLabel);
    
    QLabel* versionLabel = new QLabel(QString("Versão %1").arg(QApplication::applicationVersion()));
    versionLabel->setStyleSheet("color: gray;");
    titleLayout->addWidget(versionLabel);
    
    titleLayout->addStretch();
    headerLayout->addLayout(titleLayout, 1);
    
    mainLayout->addLayout(headerLayout);
    
    // Linha separadora
    QFrame* line1 = new QFrame();
    line1->setFrameShape(QFrame::HLine);
    line1->setFrameShadow(QFrame::Sunken);
    mainLayout->addWidget(line1);
    
    // ==================== DESCRIÇÃO ====================
    QTextBrowser* aboutText = new QTextBrowser();
    aboutText->setOpenExternalLinks(true);
    aboutText->setHtml(getAboutText());
    aboutText->setMaximumHeight(200);
    mainLayout->addWidget(aboutText);
    
    // ==================== INFORMAÇÕES DE CONTATO ====================
    QGroupBox* contactGroup = new QGroupBox("Informações de Contato");
    QVBoxLayout* contactLayout = new QVBoxLayout(contactGroup);
    
    QLabel* contactLabel = new QLabel(
        "<b>Desenvolvedor Principal:</b><br>"
        "Nome: Dr. Adelino Pinheiro Silva<br>"
        "<br>"
        "<b>Instituição:</b><br>"
        "Instituto de Criminalística de Minas Gerais<br>"
        "<br>"
        "<b>E-mail:</b><br>"
        "<a href='mailto:adelinocpp@yahoo.com'>adelinocpp@yahoo.com</a><br>"
        "<br>"
        "<b>Website:</b><br>"
        "<a href='https://github.com/adelinocpp/FingerprintEnhancer'>https://github.com/adelinocpp/FingerprintEnhancer</a><br>"
    );
    contactLabel->setTextFormat(Qt::RichText);
    contactLabel->setOpenExternalLinks(true);
    contactLabel->setWordWrap(true);
    contactLayout->addWidget(contactLabel);
    
    mainLayout->addWidget(contactGroup);
    
    // ==================== BOTÕES ====================
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    QPushButton* closeButton = new QPushButton("Fechar");
    closeButton->setDefault(true);
    closeButton->setMinimumWidth(100);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);
    
    mainLayout->addLayout(buttonLayout);
}

QString AboutDialog::getAboutText() const {
    return QString(
        "<p><b>[Nome do Programa]</b> é um sistema avançado de processamento e análise de "
        "impressões digitais desenvolvido para aplicações forenses e de identificação biométrica.</p>"
        
        "<p><b>Principais Funcionalidades:</b></p>"
        "<ul>"
        "<li>Processamento avançado de imagens de impressões digitais</li>"
        "<li>Extração automática e manual de minúcias</li>"
        "<li>Análise AFIS (Automated Fingerprint Identification System)</li>"
        "<li>Cálculo de Likelihood Ratio baseado em Neumann et al. (2015)</li>"
        "<li>Comparação 1:1 de fragmentos com dados da população brasileira</li>"
        "<li>Suporte a múltiplos tipos de minúcias (54 tipos - Gomes et al. 2024)</li>"
        "</ul>"
        
        "<p><b>Tecnologias Utilizadas:</b></p>"
        "<ul>"
        "<li>Qt %1 - Interface gráfica</li>"
        "<li>OpenCV %2 - Processamento de imagens</li>"
        "<li>C++17 - Linguagem de programação</li>"
        "</ul>"
        
        "<p style='font-size: 9pt; color: gray;'>"
        "Este software é fornecido \"como está\", sem garantias de qualquer tipo. "
        "O uso em contexto forense deve seguir as diretrizes e validações apropriadas."
        "</p>"
    ).arg(qVersion()).arg(CV_VERSION);
}
