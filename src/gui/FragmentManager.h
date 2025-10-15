#ifndef FRAGMENTMANAGER_H
#define FRAGMENTMANAGER_H

#include <QWidget>
#include <QListWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QSplitter>
#include <QComboBox>
#include <QKeyEvent>
#include "../core/ProjectModel.h"

namespace FingerprintEnhancer {

/**
 * Widget para gerenciar hierarquia do projeto:
 * Projeto → Imagens → Fragmentos → Minúcias
 */
class FragmentManager : public QWidget {
    Q_OBJECT

public:
    explicit FragmentManager(QWidget *parent = nullptr);

    void setProject(Project* project);
    void updateView();

    // Seleção atual
    QString getSelectedImageId() const;
    QString getSelectedFragmentId() const;
    QString getSelectedMinutiaId() const;
    
    // Seleção programática
    void selectImage(const QString& imageId);
    void selectFragment(const QString& fragmentId);
    void selectMinutia(const QString& minutiaId);
    void selectPreviousItem();

signals:
    void imageSelected(const QString& imageId);
    void fragmentSelected(const QString& fragmentId);
    void minutiaSelected(const QString& minutiaId);

    void createFragmentRequested(const QString& imageId);
    void editMinutiaRequested(const QString& minutiaId);
    void deleteImageRequested(const QString& imageId);
    void deleteFragmentRequested(const QString& fragmentId);
    void deleteMinutiaRequested(const QString& minutiaId);

    void viewOriginalRequested(const QString& imageId, bool isFragment);
    void resetWorkingImageRequested(const QString& id, bool isFragment);

    void makeCurrentRequested(const QString& entityId, bool isFragment);
    void sendToLeftPanelRequested(const QString& entityId, bool isFragment);
    void sendToRightPanelRequested(const QString& entityId, bool isFragment);
    
    void duplicateFragmentRequested(const QString& fragmentId);
    void exportImageRequested(const QString& imageId);
    void exportFragmentRequested(const QString& fragmentId);
    
    void editImagePropertiesRequested(const QString& imageId);
    void editFragmentPropertiesRequested(const QString& fragmentId);

private slots:
    void onTreeItemSelectionChanged();
    void onTreeContextMenu(const QPoint& pos);
    void onCreateFragment();
    void onDeleteSelected();
    void onShowMinutiaInfo();
    void onViewOriginal();
    void onResetWorkingImage();
    void onDuplicateFragment();
    void onExportSelected();
    void onFilterChanged();

private:
    Project* currentProject;

    // UI Components
    QTreeWidget* hierarchyTree;
    QLabel* infoLabel;
    QPushButton* createFragmentBtn;
    QPushButton* deleteBtn;
    QPushButton* infoBtn;
    QPushButton* viewOriginalBtn;
    QPushButton* resetWorkingBtn;
    QPushButton* duplicateBtn;
    QPushButton* exportBtn;
    
    // Filtros
    QComboBox* filterCombo;
    enum FilterType { FILTER_ALL, FILTER_WITH_FRAGMENTS, FILTER_WITH_MINUTIAE, FILTER_WITHOUT_MINUTIAE };
    FilterType currentFilter;

    void setupUI();
    void updateProjectInfo();
    void buildHierarchyTree();

    // Helper para criar itens da árvore
    QTreeWidgetItem* createImageItem(const FingerprintImage& image);
    QTreeWidgetItem* createFragmentItem(const Fragment& fragment, int index);
    QTreeWidgetItem* createMinutiaItem(const Minutia& minutia, int index);
    
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
};

} // namespace FingerprintEnhancer

#endif // FRAGMENTMANAGER_H
