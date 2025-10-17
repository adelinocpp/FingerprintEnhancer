#ifndef CORRESPONDENCEVISUALIZATIONDIALOG_H
#define CORRESPONDENCEVISUALIZATIONDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QPainter>
#include "../core/ProjectModel.h"

/**
 * @brief Diálogo para visualizar correspondências entre minúcias de dois fragmentos
 */
class CorrespondenceVisualizationDialog : public QDialog {
    Q_OBJECT

public:
    explicit CorrespondenceVisualizationDialog(QWidget *parent = nullptr);
    
    // Configurar dados para visualização
    void setData(const cv::Mat& img1, const cv::Mat& img2,
                 const QVector<FingerprintEnhancer::Minutia>& minutiae1,
                 const QVector<FingerprintEnhancer::Minutia>& minutiae2,
                 const QVector<QPair<int, int>>& correspondences);

private:
    void generateVisualization();
    void drawMinutiaMarker(QPainter& painter, const QPoint& pos, int number, const QColor& color);
    void drawConnectionLine(QPainter& painter, const QPoint& p1, const QPoint& p2, const QColor& color);

    QLabel* imageLabel;
    QPushButton* saveButton;
    QPushButton* closeButton;
    
    QPixmap visualizationPixmap;
    
    // Dados
    cv::Mat image1;
    cv::Mat image2;
    QVector<FingerprintEnhancer::Minutia> minutiae1;
    QVector<FingerprintEnhancer::Minutia> minutiae2;
    QVector<QPair<int, int>> correspondences;

private slots:
    void onSaveClicked();
};

#endif // CORRESPONDENCEVISUALIZATIONDIALOG_H
