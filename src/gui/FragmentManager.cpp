#include "FragmentManager.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QFileInfo>
#include <QMenu>

namespace FingerprintEnhancer {

FragmentManager::FragmentManager(QWidget *parent)
    : QWidget(parent), currentProject(nullptr)
{
    setupUI();
}

void FragmentManager::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Label de informações do projeto
    infoLabel = new QLabel("Nenhum projeto aberto", this);
    infoLabel->setStyleSheet("QLabel { background-color: #e0e0e0; padding: 8px; border-radius: 4px; font-weight: bold; }");
    mainLayout->addWidget(infoLabel);

    // Árvore hierárquica
    hierarchyTree = new QTreeWidget(this);
    hierarchyTree->setHeaderLabels({"Nome", "Detalhes"});
    hierarchyTree->setColumnWidth(0, 250);
    hierarchyTree->setAlternatingRowColors(true);
    hierarchyTree->setSelectionMode(QAbstractItemView::SingleSelection);
    hierarchyTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(hierarchyTree, &QTreeWidget::itemSelectionChanged, this, &FragmentManager::onTreeItemSelectionChanged);
    connect(hierarchyTree, &QTreeWidget::customContextMenuRequested, this, &FragmentManager::onTreeContextMenu);
    mainLayout->addWidget(hierarchyTree);

    // Botões de ação
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    createFragmentBtn = new QPushButton("Criar Fragmento", this);
    createFragmentBtn->setEnabled(false);
    connect(createFragmentBtn, &QPushButton::clicked, this, &FragmentManager::onCreateFragment);
    buttonLayout->addWidget(createFragmentBtn);

    deleteBtn = new QPushButton("Excluir Selecionado", this);
    deleteBtn->setEnabled(false);
    connect(deleteBtn, &QPushButton::clicked, this, &FragmentManager::onDeleteSelected);
    buttonLayout->addWidget(deleteBtn);

    infoBtn = new QPushButton("Informações", this);
    infoBtn->setEnabled(false);
    connect(infoBtn, &QPushButton::clicked, this, &FragmentManager::onShowMinutiaInfo);
    buttonLayout->addWidget(infoBtn);

    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // Segunda linha de botões para gerenciar imagens
    QHBoxLayout* imageButtonLayout = new QHBoxLayout();

    viewOriginalBtn = new QPushButton("Ver Original", this);
    viewOriginalBtn->setEnabled(false);
    connect(viewOriginalBtn, &QPushButton::clicked, this, &FragmentManager::onViewOriginal);
    imageButtonLayout->addWidget(viewOriginalBtn);

    resetWorkingBtn = new QPushButton("Resetar Realces", this);
    resetWorkingBtn->setEnabled(false);
    connect(resetWorkingBtn, &QPushButton::clicked, this, &FragmentManager::onResetWorkingImage);
    imageButtonLayout->addWidget(resetWorkingBtn);

    imageButtonLayout->addStretch();
    mainLayout->addLayout(imageButtonLayout);

    setLayout(mainLayout);
}

void FragmentManager::setProject(Project* project) {
    currentProject = project;
    updateView();
}

void FragmentManager::updateView() {
    hierarchyTree->clear();

    if (!currentProject) {
        infoLabel->setText("Nenhum projeto aberto");
        createFragmentBtn->setEnabled(false);
        deleteBtn->setEnabled(false);
        infoBtn->setEnabled(false);
        return;
    }

    updateProjectInfo();
    buildHierarchyTree();
}

void FragmentManager::updateProjectInfo() {
    if (!currentProject) return;

    QString info = currentProject->getSummary();
    infoLabel->setText(info);
}

void FragmentManager::buildHierarchyTree() {
    if (!currentProject) return;

    for (const auto& image : currentProject->images) {
        QTreeWidgetItem* imageItem = createImageItem(image);
        hierarchyTree->addTopLevelItem(imageItem);

        int fragIndex = 1;
        for (const auto& fragment : image.fragments) {
            QTreeWidgetItem* fragmentItem = createFragmentItem(fragment, fragIndex);
            imageItem->addChild(fragmentItem);

            int minIndex = 1;
            for (const auto& minutia : fragment.minutiae) {
                QTreeWidgetItem* minutiaItem = createMinutiaItem(minutia, minIndex);
                fragmentItem->addChild(minutiaItem);
                minIndex++;
            }

            fragIndex++;
        }

        imageItem->setExpanded(true);
    }
}

QTreeWidgetItem* FragmentManager::createImageItem(const FingerprintImage& image) {
    QTreeWidgetItem* item = new QTreeWidgetItem();

    QFileInfo fileInfo(image.originalFilePath);
    QString fileName = fileInfo.fileName();

    item->setText(0, QString("📷 %1").arg(fileName));
    item->setText(1, QString("%1 fragmentos | %2 minúcias")
        .arg(image.getFragmentCount())
        .arg(image.getTotalMinutiaeCount()));

    item->setData(0, Qt::UserRole, "IMAGE");
    item->setData(0, Qt::UserRole + 1, image.id);

    QFont font = item->font(0);
    font.setBold(true);
    item->setFont(0, font);

    return item;
}

QTreeWidgetItem* FragmentManager::createFragmentItem(const Fragment& fragment, int index) {
    QTreeWidgetItem* item = new QTreeWidgetItem();

    item->setText(0, QString("🔍 Fragmento #%1").arg(index));
    item->setText(1, QString("%1×%2 | %3 minúcias")
        .arg(fragment.sourceRect.width())
        .arg(fragment.sourceRect.height())
        .arg(fragment.getMinutiaeCount()));

    item->setData(0, Qt::UserRole, "FRAGMENT");
    item->setData(0, Qt::UserRole + 1, fragment.id);
    item->setData(0, Qt::UserRole + 2, fragment.parentImageId);

    return item;
}

QTreeWidgetItem* FragmentManager::createMinutiaItem(const Minutia& minutia, int index) {
    QTreeWidgetItem* item = new QTreeWidgetItem();

    QString typeName = minutia.getTypeName();
    if (typeName.length() > 30) {
        typeName = minutia.getTypeAbbreviation();
    }

    item->setText(0, QString("📍 Minúcia #%1").arg(index));
    item->setText(1, QString("(%1, %2) | %3")
        .arg(minutia.position.x())
        .arg(minutia.position.y())
        .arg(typeName));

    item->setData(0, Qt::UserRole, "MINUTIA");
    item->setData(0, Qt::UserRole + 1, minutia.id);

    return item;
}

void FragmentManager::onTreeItemSelectionChanged() {
    QTreeWidgetItem* item = hierarchyTree->currentItem();

    if (!item) {
        createFragmentBtn->setEnabled(false);
        deleteBtn->setEnabled(false);
        infoBtn->setEnabled(false);
        viewOriginalBtn->setEnabled(false);
        resetWorkingBtn->setEnabled(false);
        return;
    }

    QString itemType = item->data(0, Qt::UserRole).toString();

    if (itemType == "IMAGE") {
        QString imageId = item->data(0, Qt::UserRole + 1).toString();
        createFragmentBtn->setEnabled(true);
        deleteBtn->setEnabled(false);
        infoBtn->setEnabled(false);
        viewOriginalBtn->setEnabled(true);
        resetWorkingBtn->setEnabled(true);
        emit imageSelected(imageId);

    } else if (itemType == "FRAGMENT") {
        QString fragmentId = item->data(0, Qt::UserRole + 1).toString();
        createFragmentBtn->setEnabled(false);
        deleteBtn->setEnabled(true);
        infoBtn->setEnabled(false);
        viewOriginalBtn->setEnabled(true);
        resetWorkingBtn->setEnabled(true);
        emit fragmentSelected(fragmentId);

    } else if (itemType == "MINUTIA") {
        QString minutiaId = item->data(0, Qt::UserRole + 1).toString();
        createFragmentBtn->setEnabled(false);
        deleteBtn->setEnabled(true);
        infoBtn->setEnabled(true);
        viewOriginalBtn->setEnabled(false);
        resetWorkingBtn->setEnabled(false);
        emit minutiaSelected(minutiaId);
    }
}

void FragmentManager::onCreateFragment() {
    QTreeWidgetItem* item = hierarchyTree->currentItem();
    if (!item) return;

    QString itemType = item->data(0, Qt::UserRole).toString();
    if (itemType == "IMAGE") {
        QString imageId = item->data(0, Qt::UserRole + 1).toString();
        emit createFragmentRequested(imageId);
    }
}

void FragmentManager::onDeleteSelected() {
    QTreeWidgetItem* item = hierarchyTree->currentItem();
    if (!item) return;

    QString itemType = item->data(0, Qt::UserRole).toString();

    if (itemType == "FRAGMENT") {
        QString fragmentId = item->data(0, Qt::UserRole + 1).toString();

        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Confirmar Exclusão",
            "Deseja realmente excluir este fragmento e todas as suas minúcias?",
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            emit deleteFragmentRequested(fragmentId);
        }

    } else if (itemType == "MINUTIA") {
        QString minutiaId = item->data(0, Qt::UserRole + 1).toString();

        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            "Confirmar Exclusão",
            "Deseja realmente excluir esta minúcia?",
            QMessageBox::Yes | QMessageBox::No
        );

        if (reply == QMessageBox::Yes) {
            emit deleteMinutiaRequested(minutiaId);
        }
    }
}

void FragmentManager::onShowMinutiaInfo() {
    QTreeWidgetItem* item = hierarchyTree->currentItem();
    if (!item) return;

    QString itemType = item->data(0, Qt::UserRole).toString();
    if (itemType != "MINUTIA") return;

    QString minutiaId = item->data(0, Qt::UserRole + 1).toString();

    if (currentProject) {
        Minutia* minutia = currentProject->findMinutia(minutiaId);
        if (minutia) {
            QString info = QString(
                "ID: %1\n"
                "Posição: (%2, %3)\n"
                "Tipo: %4\n"
                "Ângulo: %5°\n"
                "Qualidade: %6\n"
                "Criada: %7\n"
                "Modificada: %8\n"
                "Notas: %9"
            )
            .arg(minutia->id)
            .arg(minutia->position.x())
            .arg(minutia->position.y())
            .arg(minutia->getTypeName())
            .arg(minutia->angle, 0, 'f', 1)
            .arg(minutia->quality, 0, 'f', 2)
            .arg(minutia->createdAt.toString("dd/MM/yyyy hh:mm:ss"))
            .arg(minutia->modifiedAt.toString("dd/MM/yyyy hh:mm:ss"))
            .arg(minutia->notes.isEmpty() ? "Nenhuma" : minutia->notes);

            QMessageBox::information(this, "Informações da Minúcia", info);
        }
    }
}

QString FragmentManager::getSelectedImageId() const {
    QTreeWidgetItem* item = hierarchyTree->currentItem();
    if (!item) return QString();

    QString itemType = item->data(0, Qt::UserRole).toString();
    if (itemType == "IMAGE") {
        return item->data(0, Qt::UserRole + 1).toString();
    }
    return QString();
}

QString FragmentManager::getSelectedFragmentId() const {
    QTreeWidgetItem* item = hierarchyTree->currentItem();
    if (!item) return QString();

    QString itemType = item->data(0, Qt::UserRole).toString();
    if (itemType == "FRAGMENT") {
        return item->data(0, Qt::UserRole + 1).toString();
    }
    return QString();
}

QString FragmentManager::getSelectedMinutiaId() const {
    QTreeWidgetItem* item = hierarchyTree->currentItem();
    if (!item) return QString();

    QString itemType = item->data(0, Qt::UserRole).toString();
    if (itemType == "MINUTIA") {
        return item->data(0, Qt::UserRole + 1).toString();
    }
    return QString();
}

void FragmentManager::onViewOriginal() {
    QTreeWidgetItem* item = hierarchyTree->currentItem();
    if (!item) return;

    QString itemType = item->data(0, Qt::UserRole).toString();

    if (itemType == "IMAGE") {
        QString imageId = item->data(0, Qt::UserRole + 1).toString();
        emit viewOriginalRequested(imageId, false);
    } else if (itemType == "FRAGMENT") {
        QString fragmentId = item->data(0, Qt::UserRole + 1).toString();
        emit viewOriginalRequested(fragmentId, true);
    }
}

void FragmentManager::onResetWorkingImage() {
    QTreeWidgetItem* item = hierarchyTree->currentItem();
    if (!item) return;

    QString itemType = item->data(0, Qt::UserRole).toString();

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Confirmar Reset",
        "Deseja realmente resetar a imagem de trabalho para a original?\n"
        "Todos os realces aplicados serão perdidos.",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        if (itemType == "IMAGE") {
            QString imageId = item->data(0, Qt::UserRole + 1).toString();
            emit resetWorkingImageRequested(imageId, false);
        } else if (itemType == "FRAGMENT") {
            QString fragmentId = item->data(0, Qt::UserRole + 1).toString();
            emit resetWorkingImageRequested(fragmentId, true);
        }
    }
}

void FragmentManager::onTreeContextMenu(const QPoint& pos) {
    QTreeWidgetItem* item = hierarchyTree->itemAt(pos);
    if (!item) return;

    QString itemType = item->data(0, Qt::UserRole).toString();
    QString entityId = item->data(0, Qt::UserRole + 1).toString();

    QMenu menu(this);

    if (itemType == "IMAGE" || itemType == "FRAGMENT") {
        menu.addAction("🎯 Tornar Corrente (Selecionar)", [this, entityId, itemType]() {
            bool isFragment = (itemType == "FRAGMENT");
            emit makeCurrentRequested(entityId, isFragment);
        });

        menu.addSeparator();

        menu.addAction("👁 Ver Original", [this, entityId, itemType]() {
            bool isFragment = (itemType == "FRAGMENT");
            emit viewOriginalRequested(entityId, isFragment);
        });

        menu.addAction("🔄 Resetar Realces", [this, entityId, itemType]() {
            bool isFragment = (itemType == "FRAGMENT");
            emit resetWorkingImageRequested(entityId, isFragment);
        });

        if (itemType == "FRAGMENT") {
            menu.addSeparator();
            menu.addAction("🗑 Excluir Fragmento", [this, entityId]() {
                emit deleteFragmentRequested(entityId);
            });
        }
    } else if (itemType == "MINUTIA") {
        menu.addAction("ℹ Informações", this, &FragmentManager::onShowMinutiaInfo);
        menu.addAction("✏️ Editar Minúcia", [this, entityId]() {
            emit editMinutiaRequested(entityId);
        });
        menu.addAction("🗑 Excluir Minúcia", [this, entityId]() {
            emit deleteMinutiaRequested(entityId);
        });
    }

    menu.exec(hierarchyTree->mapToGlobal(pos));
}

} // namespace FingerprintEnhancer
