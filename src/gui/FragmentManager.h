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

signals:
    void imageSelected(const QString& imageId);
    void fragmentSelected(const QString& fragmentId);
    void minutiaSelected(const QString& minutiaId);

    void createFragmentRequested(const QString& imageId);
    void editMinutiaRequested(const QString& minutiaId);
    void deleteFragmentRequested(const QString& fragmentId);
    void deleteMinutiaRequested(const QString& minutiaId);

    void viewOriginalRequested(const QString& imageId, bool isFragment);
    void resetWorkingImageRequested(const QString& id, bool isFragment);

    void makeCurrentRequested(const QString& entityId, bool isFragment);

private slots:
    void onTreeItemSelectionChanged();
    void onTreeContextMenu(const QPoint& pos);
    void onCreateFragment();
    void onDeleteSelected();
    void onShowMinutiaInfo();
    void onViewOriginal();
    void onResetWorkingImage();

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

    void setupUI();
    void updateProjectInfo();
    void buildHierarchyTree();

    // Helper para criar itens da árvore
    QTreeWidgetItem* createImageItem(const FingerprintImage& image);
    QTreeWidgetItem* createFragmentItem(const Fragment& fragment, int index);
    QTreeWidgetItem* createMinutiaItem(const Minutia& minutia, int index);
};

} // namespace FingerprintEnhancer

#endif // FRAGMENTMANAGER_H
