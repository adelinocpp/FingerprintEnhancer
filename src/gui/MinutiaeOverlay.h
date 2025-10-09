#ifndef MINUTIAEOVERLAY_H
#define MINUTIAEOVERLAY_H

#include <QWidget>
#include <QPainter>
#include <QVector>
#include <QPoint>
#include "../core/ProjectModel.h"
#include "MinutiaeDisplayDialog.h"

namespace FingerprintEnhancer {

/**
 * Widget de overlay para desenhar minúcias sobre a imagem
 * Não modifica a imagem original - desenha em uma camada separada
 */
class MinutiaeOverlay : public QWidget {
    Q_OBJECT

public:
    explicit MinutiaeOverlay(QWidget *parent = nullptr);

    void setFragment(Fragment* fragment);
    void setScaleFactor(double scale);
    void setScrollOffset(const QPoint& offset);
    void setImageOffset(const QPoint& offset);  // Offset de centralização da imagem
    void clearMinutiae();

    // Controle de seleção
    void setSelectedMinutia(const QString& minutiaId);
    QString getSelectedMinutiaId() const { return selectedMinutiaId; }
    void clearSelection();

    // Modo de edição
    void setEditMode(bool enabled) { editMode = enabled; update(); }
    bool isEditMode() const { return editMode; }

    // Configurações visuais
    void setShowLabels(bool show) { showLabels = show; update(); }
    void setShowAngles(bool show) { showAngles = show; update(); }
    void setMinutiaRadius(int radius) { minutiaRadius = radius; update(); }
    void setDisplaySettings(const MinutiaeDisplaySettings& settings) {
        displaySettings = settings;
        update();
    }

signals:
    void minutiaClicked(const QString& minutiaId, const QPoint& position);
    void minutiaDoubleClicked(const QString& minutiaId);
    void positionChanged(const QString& minutiaId, const QPoint& newPosition);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    Fragment* currentFragment;
    double scaleFactor;
    QPoint scrollOffset;
    QPoint imageOffset;  // Offset de centralização da imagem
    QString selectedMinutiaId;
    bool editMode;
    bool draggingMinutia;
    QPoint dragStartPos;

    // Configurações visuais
    bool showLabels;
    bool showAngles;
    int minutiaRadius;
    MinutiaeDisplaySettings displaySettings;

    // Cores
    QColor normalColor;
    QColor selectedColor;
    QColor hoverColor;

    // Helper functions
    void drawMinutia(QPainter& painter, const Minutia& minutia, bool isSelected);
    void drawMinutiaLabel(QPainter& painter, const QPoint& pos, int number, const QString& type, bool isSelected);

    Minutia* findMinutiaAt(const QPoint& pos);
    QPoint scalePoint(const QPoint& imagePoint) const;
    QPoint unscalePoint(const QPoint& widgetPoint) const;
};

} // namespace FingerprintEnhancer

#endif // MINUTIAEOVERLAY_H
