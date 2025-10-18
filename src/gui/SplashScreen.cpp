#include "SplashScreen.h"
#include <QFont>
#include <QLinearGradient>

SplashScreen::SplashScreen()
    : QSplashScreen(createSplashPixmap())
{
    setWindowFlags(Qt::SplashScreen | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
}

void SplashScreen::showMessage(const QString& message, int alignment) {
    QSplashScreen::showMessage(message, alignment, Qt::white);
    QApplication::processEvents();
}

QPixmap SplashScreen::createSplashPixmap() {
    // Criar pixmap com dimensões adequadas
    QPixmap pixmap(600, 400);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // ==================== FUNDO COM GRADIENTE ====================
    QLinearGradient gradient(0, 0, 0, 400);
    gradient.setColorAt(0.0, QColor(25, 35, 75));      // Azul escuro
    gradient.setColorAt(0.5, QColor(35, 50, 100));     // Azul médio
    gradient.setColorAt(1.0, QColor(45, 65, 125));     // Azul claro
    
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(0, 0, 600, 400, 15, 15);
    
    // Borda sutil
    painter.setPen(QPen(QColor(100, 120, 180, 100), 2));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(1, 1, 598, 398, 15, 15);
    
    // ==================== ÍCONE PROVISÓRIO (PLACEHOLDER) ====================
    painter.setBrush(QColor(70, 90, 150));
    painter.setPen(QPen(Qt::white, 3));
    painter.drawEllipse(250, 80, 100, 100);
    
    // Desenhar "fingerprint" estilizado (círculos concêntricos)
    painter.setBrush(Qt::NoBrush);
    for (int i = 1; i <= 4; ++i) {
        painter.drawEllipse(250 + 15*i, 80 + 15*i, 100 - 30*i, 100 - 30*i);
    }
    
    // ==================== TÍTULO ====================
    painter.setPen(Qt::white);
    
    QFont titleFont("Arial", 28, QFont::Bold);
    painter.setFont(titleFont);
    painter.drawText(QRect(0, 200, 600, 50), Qt::AlignCenter, "[Nome do Programa]");
    
    // ==================== SUBTÍTULO ====================
    QFont subtitleFont("Arial", 12);
    painter.setFont(subtitleFont);
    painter.setPen(QColor(200, 210, 230));
    painter.drawText(QRect(0, 250, 600, 30), Qt::AlignCenter, 
        "Sistema de Processamento e Análise de Impressões Digitais");
    
    // ==================== VERSÃO ====================
    QFont versionFont("Arial", 10);
    painter.setFont(versionFont);
    painter.setPen(QColor(150, 170, 200));
    painter.drawText(QRect(0, 280, 600, 20), Qt::AlignCenter, 
        QString("Versão %1").arg(QApplication::applicationVersion()));
    
    // ==================== RODAPÉ ====================
    painter.setFont(QFont("Arial", 9));
    painter.setPen(QColor(120, 140, 180));
    painter.drawText(QRect(0, 360, 600, 20), Qt::AlignCenter, 
        "Inicializando...");
    
    return pixmap;
}
