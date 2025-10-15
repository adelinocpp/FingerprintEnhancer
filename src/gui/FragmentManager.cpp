#include "FragmentManager.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QFileInfo>
#include <QMenu>
#include <QEvent>

namespace FingerprintEnhancer {

FragmentManager::FragmentManager(QWidget *parent)
    : QWidget(parent), currentProject(nullptr), currentFilter(FILTER_ALL)
{
    setupUI();
}

void FragmentManager::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Label de informações do projeto
    infoLabel = new QLabel("Nenhum projeto aberto", this);
    infoLabel->setStyleSheet("QLabel { background-color: #e0e0e0; padding: 8px; border-radius: 4px; font-weight: bold; }");
    mainLayout->addWidget(infoLabel);

    // Filtro
    QHBoxLayout* filterLayout = new QHBoxLayout();
    QLabel* filterLabel = new QLabel("Filtrar:", this);
    filterLayout->addWidget(filterLabel);
    
    filterCombo = new QComboBox(this);
    filterCombo->addItem("🔍 Todos os itens", FILTER_ALL);
    filterCombo->addItem("📁 Com fragmentos", FILTER_WITH_FRAGMENTS);
    filterCombo->addItem("📍 Com minúcias", FILTER_WITH_MINUTIAE);
    filterCombo->addItem("⚪ Sem minúcias", FILTER_WITHOUT_MINUTIAE);
    connect(filterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &FragmentManager::onFilterChanged);
    filterLayout->addWidget(filterCombo, 1);
    
    mainLayout->addLayout(filterLayout);

    // Árvore hierárquica
    hierarchyTree = new QTreeWidget(this);
    hierarchyTree->setHeaderLabels({"Nome", "Detalhes"});
    hierarchyTree->setColumnWidth(0, 250);
    hierarchyTree->setAlternatingRowColors(true);
    hierarchyTree->setSelectionMode(QAbstractItemView::SingleSelection);
    hierarchyTree->setContextMenuPolicy(Qt::CustomContextMenu);
    hierarchyTree->installEventFilter(this);  // Para capturar teclas
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
    
    // Terceira linha - Duplicar e Exportar
    QHBoxLayout* utilityButtonLayout = new QHBoxLayout();
    
    duplicateBtn = new QPushButton("📋 Duplicar", this);
    duplicateBtn->setEnabled(false);
    duplicateBtn->setToolTip("Duplicar fragmento selecionado (Ctrl+D)");
    connect(duplicateBtn, &QPushButton::clicked, this, &FragmentManager::onDuplicateFragment);
    utilityButtonLayout->addWidget(duplicateBtn);
    
    exportBtn = new QPushButton("💾 Exportar", this);
    exportBtn->setEnabled(false);
    exportBtn->setToolTip("Exportar imagem ou fragmento selecionado (Ctrl+E)");
    connect(exportBtn, &QPushButton::clicked, this, &FragmentManager::onExportSelected);
    utilityButtonLayout->addWidget(exportBtn);
    
    utilityButtonLayout->addStretch();
    mainLayout->addLayout(utilityButtonLayout);

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
        // Aplicar filtros
        bool showImage = true;
        
        if (currentFilter == FILTER_WITH_FRAGMENTS && image.fragments.empty()) {
            showImage = false;
        } else if (currentFilter == FILTER_WITH_MINUTIAE && image.getTotalMinutiaeCount() == 0) {
            showImage = false;
        } else if (currentFilter == FILTER_WITHOUT_MINUTIAE && image.getTotalMinutiaeCount() > 0) {
            showImage = false;
        }
        
        if (!showImage) continue;
        
        QTreeWidgetItem* imageItem = createImageItem(image);
        hierarchyTree->addTopLevelItem(imageItem);

        int fragIndex = 1;
        for (const auto& fragment : image.fragments) {
            bool showFragment = true;
            
            if (currentFilter == FILTER_WITH_MINUTIAE && fragment.getMinutiaeCount() == 0) {
                showFragment = false;
            } else if (currentFilter == FILTER_WITHOUT_MINUTIAE && fragment.getMinutiaeCount() > 0) {
                showFragment = false;
            }
            
            if (!showFragment) continue;
            
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
        duplicateBtn->setEnabled(false);
        exportBtn->setEnabled(false);
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
        duplicateBtn->setEnabled(false);
        exportBtn->setEnabled(true);
        emit imageSelected(imageId);

    } else if (itemType == "FRAGMENT") {
        QString fragmentId = item->data(0, Qt::UserRole + 1).toString();
        createFragmentBtn->setEnabled(false);
        deleteBtn->setEnabled(true);
        infoBtn->setEnabled(false);
        viewOriginalBtn->setEnabled(true);
        resetWorkingBtn->setEnabled(true);
        duplicateBtn->setEnabled(true);
        exportBtn->setEnabled(true);
        emit fragmentSelected(fragmentId);

    } else if (itemType == "MINUTIA") {
        QString minutiaId = item->data(0, Qt::UserRole + 1).toString();
        createFragmentBtn->setEnabled(false);
        deleteBtn->setEnabled(true);
        infoBtn->setEnabled(true);
        viewOriginalBtn->setEnabled(false);
        resetWorkingBtn->setEnabled(false);
        duplicateBtn->setEnabled(false);
        exportBtn->setEnabled(false);
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
        
        menu.addAction("📝 Editar Propriedades...", [this, entityId, itemType]() {
            if (itemType == "IMAGE") {
                emit editImagePropertiesRequested(entityId);
            } else {
                emit editFragmentPropertiesRequested(entityId);
            }
        });

        menu.addSeparator();

        menu.addAction("◀ Enviar para Painel Esquerdo", [this, entityId, itemType]() {
            bool isFragment = (itemType == "FRAGMENT");
            emit sendToLeftPanelRequested(entityId, isFragment);
        });

        menu.addAction("▶ Enviar para Painel Direito", [this, entityId, itemType]() {
            bool isFragment = (itemType == "FRAGMENT");
            emit sendToRightPanelRequested(entityId, isFragment);
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

        menu.addSeparator();

        if (itemType == "IMAGE") {
            menu.addAction("🗑 Remover Imagem", [this, entityId]() {
                emit deleteImageRequested(entityId);
            });
        } else if (itemType == "FRAGMENT") {
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

    // Adicionar opções de duplicar e exportar
    if (itemType == "FRAGMENT") {
        menu.addSeparator();
        menu.addAction("📋 Duplicar Fragmento (Ctrl+D)", [this, entityId]() {
            emit duplicateFragmentRequested(entityId);
        });
        menu.addAction("💾 Exportar Fragmento (Ctrl+E)", [this, entityId]() {
            emit exportFragmentRequested(entityId);
        });
    } else if (itemType == "IMAGE") {
        menu.addSeparator();
        menu.addAction("💾 Exportar Imagem (Ctrl+E)", [this, entityId]() {
            emit exportImageRequested(entityId);
        });
    }

    menu.exec(hierarchyTree->mapToGlobal(pos));
}

void FragmentManager::onDuplicateFragment() {
    QTreeWidgetItem* item = hierarchyTree->currentItem();
    if (!item) return;

    QString itemType = item->data(0, Qt::UserRole).toString();
    if (itemType == "FRAGMENT") {
        QString fragmentId = item->data(0, Qt::UserRole + 1).toString();
        emit duplicateFragmentRequested(fragmentId);
    }
}

void FragmentManager::onExportSelected() {
    QTreeWidgetItem* item = hierarchyTree->currentItem();
    if (!item) return;

    QString itemType = item->data(0, Qt::UserRole).toString();
    QString entityId = item->data(0, Qt::UserRole + 1).toString();

    if (itemType == "IMAGE") {
        emit exportImageRequested(entityId);
    } else if (itemType == "FRAGMENT") {
        emit exportFragmentRequested(entityId);
    }
}

void FragmentManager::onFilterChanged() {
    int index = filterCombo->currentIndex();
    currentFilter = static_cast<FilterType>(filterCombo->itemData(index).toInt());
    updateView();
}

bool FragmentManager::eventFilter(QObject* obj, QEvent* event) {
    if (obj == hierarchyTree && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        
        // Ctrl+D - Duplicar
        if (keyEvent->modifiers() & Qt::ControlModifier && keyEvent->key() == Qt::Key_D) {
            onDuplicateFragment();
            return true;
        }
        
        // Ctrl+E - Exportar
        if (keyEvent->modifiers() & Qt::ControlModifier && keyEvent->key() == Qt::Key_E) {
            onExportSelected();
            return true;
        }
        
        // Delete - Excluir
        if (keyEvent->key() == Qt::Key_Delete) {
            onDeleteSelected();
            return true;
        }
        
        // Enter - Tornar corrente
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            QTreeWidgetItem* item = hierarchyTree->currentItem();
            if (item) {
                QString itemType = item->data(0, Qt::UserRole).toString();
                QString entityId = item->data(0, Qt::UserRole + 1).toString();
                
                if (itemType == "IMAGE") {
                    emit makeCurrentRequested(entityId, false);
                } else if (itemType == "FRAGMENT") {
                    emit makeCurrentRequested(entityId, true);
                }
            }
            return true;
        }
        
        // Setas Esquerda/Direita com Ctrl - Enviar para painéis
        if (keyEvent->modifiers() & Qt::ControlModifier) {
            QTreeWidgetItem* item = hierarchyTree->currentItem();
            if (item) {
                QString itemType = item->data(0, Qt::UserRole).toString();
                QString entityId = item->data(0, Qt::UserRole + 1).toString();
                
                if (itemType == "IMAGE" || itemType == "FRAGMENT") {
                    bool isFragment = (itemType == "FRAGMENT");
                    
                    if (keyEvent->key() == Qt::Key_Left) {
                        emit sendToLeftPanelRequested(entityId, isFragment);
                        return true;
                    } else if (keyEvent->key() == Qt::Key_Right) {
                        emit sendToRightPanelRequested(entityId, isFragment);
                        return true;
                    }
                }
            }
        }
    }
    
    return QWidget::eventFilter(obj, event);
}

// Métodos de seleção programática
void FragmentManager::selectImage(const QString& imageId) {
    if (imageId.isEmpty()) return;
    
    QTreeWidgetItemIterator it(hierarchyTree);
    while (*it) {
        QString itemType = (*it)->data(0, Qt::UserRole).toString();
        QString entityId = (*it)->data(0, Qt::UserRole + 1).toString();
        
        if (itemType == "IMAGE" && entityId == imageId) {
            hierarchyTree->setCurrentItem(*it);
            hierarchyTree->scrollToItem(*it);
            return;
        }
        ++it;
    }
}

void FragmentManager::selectFragment(const QString& fragmentId) {
    if (fragmentId.isEmpty()) return;
    
    QTreeWidgetItemIterator it(hierarchyTree);
    while (*it) {
        QString itemType = (*it)->data(0, Qt::UserRole).toString();
        QString entityId = (*it)->data(0, Qt::UserRole + 1).toString();
        
        if (itemType == "FRAGMENT" && entityId == fragmentId) {
            hierarchyTree->setCurrentItem(*it);
            hierarchyTree->scrollToItem(*it);
            (*it)->parent()->setExpanded(true);  // Expandir pai (imagem)
            return;
        }
        ++it;
    }
}

void FragmentManager::selectMinutia(const QString& minutiaId) {
    if (minutiaId.isEmpty()) return;
    
    QTreeWidgetItemIterator it(hierarchyTree);
    while (*it) {
        QString itemType = (*it)->data(0, Qt::UserRole).toString();
        QString entityId = (*it)->data(0, Qt::UserRole + 1).toString();
        
        if (itemType == "MINUTIA" && entityId == minutiaId) {
            hierarchyTree->setCurrentItem(*it);
            hierarchyTree->scrollToItem(*it);
            // Expandir ancestrais
            if ((*it)->parent() && (*it)->parent()->parent()) {
                (*it)->parent()->parent()->setExpanded(true);  // Imagem
                (*it)->parent()->setExpanded(true);  // Fragmento
            }
            return;
        }
        ++it;
    }
}

void FragmentManager::selectPreviousItem() {
    QTreeWidgetItem* currentItem = hierarchyTree->currentItem();
    if (!currentItem) return;
    
    // Obter índice do item atual no pai
    QTreeWidgetItem* parent = currentItem->parent();
    
    if (parent) {
        // Item tem pai (é fragmento ou minúcia)
        int index = parent->indexOfChild(currentItem);
        if (index > 0) {
            // Selecionar irmão anterior
            parent->child(index - 1)->setSelected(true);
            hierarchyTree->setCurrentItem(parent->child(index - 1));
        } else {
            // É o primeiro filho, selecionar o pai
            hierarchyTree->setCurrentItem(parent);
        }
    } else {
        // Item é de nível superior (imagem)
        int index = hierarchyTree->indexOfTopLevelItem(currentItem);
        if (index > 0) {
            // Selecionar imagem anterior
            hierarchyTree->setCurrentItem(hierarchyTree->topLevelItem(index - 1));
        }
    }
}

} // namespace FingerprintEnhancer
