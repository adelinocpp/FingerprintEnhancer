#include "CorrespondenceVisualizationDialog.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QHBoxLayout>
#include <QPainter>
#include <QPen>
#include <opencv2/opencv.hpp>

CorrespondenceVisualizationDialog::CorrespondenceVisualizationDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Visualização de Correspondências");
    resize(1200, 600);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Label para exibir imagem
    imageLabel = new QLabel();
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet("QLabel { background-color: #2b2b2b; }");
    mainLayout->addWidget(imageLabel, 1);
    
    // Botões
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    saveButton = new QPushButton("Salvar Imagem");
    closeButton = new QPushButton("Fechar");
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);
    
    connect(saveButton, &QPushButton::clicked, this, &CorrespondenceVisualizationDialog::onSaveClicked);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
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
    
    // Calcular dimensões
    int h1 = img1Color.rows;
    int h2 = img2Color.rows;
    int w1 = img1Color.cols;
    int w2 = img2Color.cols;
    int maxHeight = qMax(h1, h2);
    int totalWidth = w1 + w2 + 50;  // 50px de espaço entre imagens
    
    // Criar canvas combinado
    cv::Mat combined = cv::Mat::zeros(maxHeight, totalWidth, CV_8UC3);
    combined.setTo(cv::Scalar(43, 43, 43));  // Fundo cinza escuro
    
    // Copiar imagens
    img1Color.copyTo(combined(cv::Rect(0, (maxHeight - h1) / 2, w1, h1)));
    img2Color.copyTo(combined(cv::Rect(w1 + 50, (maxHeight - h2) / 2, w2, h2)));
    
    int offset1Y = (maxHeight - h1) / 2;
    int offset2X = w1 + 50;
    int offset2Y = (maxHeight - h2) / 2;
    
    // Converter para QImage para desenhar com QPainter
    QImage qImg(combined.data, combined.cols, combined.rows, combined.step, QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(qImg.rgbSwapped());
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Desenhar linhas de correspondência PRIMEIRO (atrás das minúcias)
    QPen connectionPen;
    connectionPen.setWidth(2);
    connectionPen.setStyle(Qt::DashLine);
    
    for (int i = 0; i < correspondences.size(); i++) {
        int idx1 = correspondences[i].first;
        int idx2 = correspondences[i].second;
        
        if (idx1 < 0 || idx1 >= minutiae1.size() || idx2 < 0 || idx2 >= minutiae2.size()) {
            continue;
        }
        
        QPoint p1(minutiae1[idx1].position.x(), minutiae1[idx1].position.y() + offset1Y);
        QPoint p2(minutiae2[idx2].position.x() + offset2X, minutiae2[idx2].position.y() + offset2Y);
        
        // Cor baseada no índice (rainbow)
        int hue = (i * 360 / qMax(1, correspondences.size())) % 360;
        QColor lineColor = QColor::fromHsv(hue, 200, 255, 180);
        connectionPen.setColor(lineColor);
        painter.setPen(connectionPen);
        painter.drawLine(p1, p2);
    }
    
    // Desenhar minúcias - FRAGMENTO 1 (esquerda)
    for (int i = 0; i < minutiae1.size(); i++) {
        QPoint pos(minutiae1[i].position.x(), minutiae1[i].position.y() + offset1Y);
        
        // Verificar se está em correspondência
        bool matched = false;
        for (const auto& corr : correspondences) {
            if (corr.first == i) {
                matched = true;
                break;
            }
        }
        
        QColor color = matched ? QColor(0, 255, 0) : QColor(128, 128, 128);
        drawMinutiaMarker(painter, pos, i + 1, color);
    }
    
    // Desenhar minúcias - FRAGMENTO 2 (direita)
    for (int i = 0; i < minutiae2.size(); i++) {
        QPoint pos(minutiae2[i].position.x() + offset2X, minutiae2[i].position.y() + offset2Y);
        
        // Verificar se está em correspondência
        bool matched = false;
        for (const auto& corr : correspondences) {
            if (corr.second == i) {
                matched = true;
                break;
            }
        }
        
        QColor color = matched ? QColor(0, 255, 0) : QColor(128, 128, 128);
        drawMinutiaMarker(painter, pos, i + 1, color);
    }
    
    // Adicionar legenda
    painter.setPen(QColor(255, 255, 255));
    QFont font = painter.font();
    font.setPointSize(12);
    font.setBold(true);
    painter.setFont(font);
    painter.drawText(10, 30, QString("Fragmento 1 (%1 minúcias)").arg(minutiae1.size()));
    painter.drawText(offset2X + 10, 30, QString("Fragmento 2 (%1 minúcias)").arg(minutiae2.size()));
    painter.drawText(10, maxHeight - 10, QString("%1 correspondências encontradas").arg(correspondences.size()));
    
    visualizationPixmap = pixmap;
    
    // Escalar para caber na janela se necessário
    QPixmap scaledPixmap = pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    imageLabel->setPixmap(scaledPixmap);
}

void CorrespondenceVisualizationDialog::drawMinutiaMarker(QPainter& painter, const QPoint& pos, int number, const QColor& color) {
    int radius = 8;
    
    // Círculo
    QPen circlePen(color, 2);
    painter.setPen(circlePen);
    painter.setBrush(Qt::NoBrush);
    painter.drawEllipse(pos, radius, radius);
    
    // Número
    QFont font = painter.font();
    font.setPointSize(8);
    font.setBold(true);
    painter.setFont(font);
    
    QString text = QString::number(number);
    QRect textRect = painter.fontMetrics().boundingRect(text);
    QPoint textPos(pos.x() + radius + 4, pos.y() + textRect.height() / 2 - 2);
    
    // Fundo do texto
    painter.fillRect(textRect.translated(textPos), QColor(0, 0, 0, 180));
    painter.setPen(color);
    painter.drawText(textPos, text);
}

void CorrespondenceVisualizationDialog::onSaveClicked() {
    QString fileName = QFileDialog::getSaveFileName(this,
        "Salvar Visualização de Correspondências",
        QString(),
        "Imagens PNG (*.png);;Imagens JPEG (*.jpg);;Todos os Arquivos (*)");
    
    if (fileName.isEmpty()) return;
    
    if (visualizationPixmap.save(fileName)) {
        QMessageBox::information(this, "Sucesso", 
            QString("Imagem salva com sucesso em:\n%1").arg(fileName));
    } else {
        QMessageBox::warning(this, "Erro", "Falha ao salvar imagem.");
    }
}
