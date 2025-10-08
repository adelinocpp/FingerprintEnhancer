#ifndef MINUTIAEEDITOR_H
#define MINUTIAEEDITOR_H

#include "ImageViewer.h"
#include "../core/MinutiaeExtractor.h"
#include <QtWidgets/QWidget>
#include <QtGui/QPainter>
#include <QtGui/QPen>
#include <QtCore/QPoint>
#include <vector>

/**
 * @brief Editor visual para marcação e edição de minúcias
 * 
 * Estende o ImageViewer para permitir marcação manual de minúcias,
 * edição de pontos existentes e visualização de comparações.
 */
class MinutiaeEditor : public ImageViewer {
    Q_OBJECT

public:
    enum EditMode {
        VIEW_MODE,          // Apenas visualização
        ADD_TERMINATION,    // Adicionar terminação
        ADD_BIFURCATION,    // Adicionar bifurcação
        EDIT_MODE,          // Editar minúcias existentes
        DELETE_MODE         // Deletar minúcias
    };

    explicit MinutiaeEditor(QWidget *parent = nullptr);
    ~MinutiaeEditor();
    
    // Controle de modo de edição
    void setEditMode(EditMode mode);
    EditMode getEditMode() const;
    
    // Gerenciamento de minúcias
    void setMinutiae(const std::vector<Minutia> &minutiae);
    std::vector<Minutia> getMinutiae() const;
    void clearMinutiae();
    
    // Visualização
    void setShowMinutiae(bool show);
    void setShowAngles(bool show);
    void setShowIds(bool show);
    void setMinutiaSize(int size);
    
    // Comparação lado a lado
    void setComparisonMinutiae(const std::vector<Minutia> &minutiae);
    void setShowComparison(bool show);
    void highlightMatchedMinutiae(const std::vector<std::pair<int, int>> &matches);

signals:
    void minutiaAdded(const Minutia &minutia);
    void minutiaModified(int id, const Minutia &minutia);
    void minutiaDeleted(int id);
    void minutiaSelected(int id);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onImageClicked(QPoint imagePosition);

private:
    // Estado do editor
    EditMode currentMode;
    std::vector<Minutia> currentMinutiae;
    std::vector<Minutia> comparisonMinutiae;
    std::vector<std::pair<int, int>> matchedPairs;
    
    // Configurações de visualização
    bool showMinutiae;
    bool showAngles;
    bool showIds;
    bool showComparison;
    int minutiaSize;
    
    // Estado de edição
    int selectedMinutiaId;
    bool draggingMinutia;
    QPoint dragStartPoint;
    
    // Cores e estilos
    QPen terminationPen;
    QPen bifurcationPen;
    QPen selectedPen;
    QPen comparisonPen;
    QPen matchedPen;
    QPen anglePen;
    
    // Métodos de desenho
    void drawMinutiae(QPainter &painter);
    void drawMinutia(QPainter &painter, const Minutia &minutia, bool isComparison = false);
    void drawMinutiaAngle(QPainter &painter, const Minutia &minutia);
    void drawMinutiaId(QPainter &painter, const Minutia &minutia);
    void drawMatchLines(QPainter &painter);
    
    // Métodos de interação
    int findMinutiaAt(const QPoint &position, int tolerance = 10);
    Minutia createMinutiaAt(const QPoint &position, int type);
    void selectMinutia(int id);
    void moveMinutia(int id, const QPoint &newPosition);
    void deleteMinutia(int id);
    
    // Métodos auxiliares
    QPoint minutiaToWidget(const Minutia &minutia) const;
    QPoint widgetToMinutia(const QPoint &widgetPoint) const;
    void updateMinutiaAngles();
    int getNextMinutiaId() const;
    
    // Configuração de estilos
    void setupDrawingStyles();
};

#endif // MINUTIAEEDITOR_H

