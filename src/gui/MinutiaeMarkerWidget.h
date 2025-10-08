#ifndef MINUTIAEMARKERWIDGET_H
#define MINUTIAEMARKERWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QVector>
#include <QMenu>
#include <opencv2/opencv.hpp>
#include "../core/MinutiaeTypes.h"

/**
 * @brief Widget para marcação manual de minúcias em imagens
 *
 * Permite ao usuário:
 * - Adicionar minúcias clicando na imagem
 * - Selecionar tipo de minúcia (56 tipos)
 * - Editar/remover minúcias
 * - Visualizar minúcias numeradas
 * - Exportar imagem com minúcias numeradas
 */
class MinutiaeMarkerWidget : public QWidget {
    Q_OBJECT

public:
    enum class Mode {
        View,           // Apenas visualização
        AddMinutia,     // Adicionar minúcia
        EditMinutia,    // Editar minúcia
        RemoveMinutia,  // Remover minúcia
        ShowNumbers     // Mostrar números
    };

    explicit MinutiaeMarkerWidget(QWidget *parent = nullptr);
    ~MinutiaeMarkerWidget();

    // Configuração de imagem
    void setImage(const cv::Mat &image);
    cv::Mat getCurrentImage() const { return baseImage; }

    // Gerenciamento de minúcias
    void addMinutia(const MinutiaeData &minutia);
    void removeMinutia(int id);
    void updateMinutia(int id, const MinutiaeData &minutia);
    void clearMinutiae();
    QVector<MinutiaeData> getMinutiae() const { return minutiae; }
    void setMinutiae(const QVector<MinutiaeData> &newMinutiae);

    // Modo de operação
    void setMode(Mode mode);
    Mode getMode() const { return currentMode; }

    // Tipo de minúcia para adicionar
    void setCurrentMinutiaeType(MinutiaeType type);
    MinutiaeType getCurrentMinutiaeType() const { return currentType; }

    // Visualização
    void setShowNumbers(bool show);
    bool isShowingNumbers() const { return showNumbers; }
    void setShowAngles(bool show);
    bool isShowingAngles() const { return showAngles; }
    void setShowLabels(bool show);
    bool isShowingLabels() const { return showLabels; }

    // Exportação
    cv::Mat renderMinutiaeImage(bool includeNumbers = true,
                                bool includeAngles = true,
                                bool includeLabels = true);

    // Zoom
    void setZoomFactor(double factor);
    double getZoomFactor() const { return zoomFactor; }

signals:
    void minutiaAdded(MinutiaeData minutia);
    void minutiaRemoved(int id);
    void minutiaUpdated(int id, MinutiaeData minutia);
    void minutiaeChanged();
    void modeChanged(Mode mode);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    // Estado
    cv::Mat baseImage;
    QVector<MinutiaeData> minutiae;
    Mode currentMode;
    MinutiaeType currentType;
    int nextId;

    // Visualização
    bool showNumbers;
    bool showAngles;
    bool showLabels;
    double zoomFactor;

    // Interação
    int selectedMinutiaId;
    QPoint lastMousePos;
    bool draggingMinutia;

    // Configurações visuais
    int minutiaRadius;
    int lineLength;
    QFont labelFont;
    QFont numberFont;

    // Métodos auxiliares
    void drawMinutiae(QPainter &painter);
    void drawMinutia(QPainter &painter, const MinutiaeData &minutia, int index);
    void drawMinutiaNumber(QPainter &painter, const MinutiaeData &minutia, int number);
    void drawMinutiaAngle(QPainter &painter, const MinutiaeData &minutia);
    void drawMinutiaLabel(QPainter &painter, const MinutiaeData &minutia);

    int findMinutiaAt(const QPoint &pos, int tolerance = 10);
    QPoint imageToWidget(const cv::Point2f &imagePoint) const;
    cv::Point2f widgetToImage(const QPoint &widgetPoint) const;

    void showContextMenu(const QPoint &pos);
    QColor getMinutiaColor(MinutiaeType type) const;
};

#endif // MINUTIAEMARKERWIDGET_H
