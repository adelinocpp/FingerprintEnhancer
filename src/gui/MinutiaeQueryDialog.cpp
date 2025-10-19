#include "MinutiaeQueryDialog.h"
#include <QHeaderView>
#include <QFileInfo>
#include <QPixmap>
#include <QApplication>
#include <QMessageBox>
#include <QDir>

MinutiaeQueryDialog::MinutiaeQueryDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    loadCatalogData();
}

void MinutiaeQueryDialog::setupUI() {
    setWindowTitle("Consultar Tipos de Minúcias");
    setMinimumSize(1000, 700);
    
    QHBoxLayout* mainLayout = new QHBoxLayout(this);
    
    // Painel esquerdo - Filtros e tabela
    QWidget* leftPanel = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftPanel);
    
    // Grupo de filtros
    QGroupBox* filterGroup = new QGroupBox("Filtros de Busca");
    QVBoxLayout* filterLayout = new QVBoxLayout(filterGroup);
    
    // Campo de busca
    filterLayout->addWidget(new QLabel("Buscar:"));
    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText("Digite o nome ou código da minúcia...");
    connect(searchEdit, &QLineEdit::textChanged, this, &MinutiaeQueryDialog::onSearchTextChanged);
    filterLayout->addWidget(searchEdit);
    
    // Filtro por categoria
    filterLayout->addWidget(new QLabel("Categoria:"));
    categoryCombo = new QComboBox();
    categoryCombo->addItem("Todas as Categorias");
    categoryCombo->addItem("Término de Linha");
    categoryCombo->addItem("Bifurcação");
    categoryCombo->addItem("Convergência");
    categoryCombo->addItem("Fragmento");
    categoryCombo->addItem("Fechamento");
    categoryCombo->addItem("Interrupção");
    categoryCombo->addItem("Sobreposição");
    categoryCombo->addItem("Barra Transversal");
    categoryCombo->addItem("Ponte");
    categoryCombo->addItem("Trifurcação");
    categoryCombo->addItem("Tripé");
    categoryCombo->addItem("Outras");
    connect(categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MinutiaeQueryDialog::onCategoryFilterChanged);
    filterLayout->addWidget(categoryCombo);
    
    // Filtro por classificação
    filterLayout->addWidget(new QLabel("Classificação:"));
    classificationCombo = new QComboBox();
    classificationCombo->addItem("Todas");
    classificationCombo->addItem("Fundamental");
    classificationCombo->addItem("Comum");
    classificationCombo->addItem("Rara");
    connect(classificationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MinutiaeQueryDialog::onClassificationFilterChanged);
    filterLayout->addWidget(classificationCombo);
    
    leftLayout->addWidget(filterGroup);
    
    // Tabela de minúcias
    minutiaeTable = new QTableWidget();
    minutiaeTable->setColumnCount(5);
    minutiaeTable->setHorizontalHeaderLabels({"ID", "Nome", "Código", "Categoria", "Frequência (%)"});
    minutiaeTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    minutiaeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    minutiaeTable->setSelectionMode(QAbstractItemView::SingleSelection);
    minutiaeTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(minutiaeTable, &QTableWidget::cellClicked, this, &MinutiaeQueryDialog::onMinutiaeSelected);
    leftLayout->addWidget(minutiaeTable);
    
    mainLayout->addWidget(leftPanel, 2);
    
    // Painel direito - Detalhes
    QWidget* detailsPanel = new QWidget();
    QVBoxLayout* detailsLayout = new QVBoxLayout(detailsPanel);
    
    QLabel* detailsLabel = new QLabel("<b>Detalhes da Minúcia</b>");
    detailsLayout->addWidget(detailsLabel);
    
    // Imagem da minúcia (196x196 pixels)
    imageLabel = new QLabel();
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setFixedSize(196, 196);
    imageLabel->setScaledContents(false);
    imageLabel->setStyleSheet("border: 1px solid #ccc; background-color: white;");
    QHBoxLayout* imageLayout = new QHBoxLayout();
    imageLayout->addStretch();
    imageLayout->addWidget(imageLabel);
    imageLayout->addStretch();
    detailsLayout->addLayout(imageLayout);
    
    // Texto de detalhes
    detailsText = new QTextEdit();
    detailsText->setReadOnly(true);
    detailsLayout->addWidget(detailsText, 1);
    
    // Botão fechar
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    closeButton = new QPushButton("Fechar");
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);
    detailsLayout->addLayout(buttonLayout);
    
    mainLayout->addWidget(detailsPanel, 1);
}

void MinutiaeQueryDialog::loadCatalogData() {
    // Carregar catálogo JSON
    QString catalogPath = QApplication::applicationDirPath() + "/../src/knolegment/minutiae_catalog.json";
    
    // Tentar caminhos alternativos
    if (!QFileInfo::exists(catalogPath)) {
        catalogPath = "src/knolegment/minutiae_catalog.json";
    }
    if (!QFileInfo::exists(catalogPath)) {
        catalogPath = "./knolegment/minutiae_catalog.json";
    }
    
    if (!catalog.loadFromJson(catalogPath)) {
        QMessageBox::warning(this, "Aviso", 
            "Não foi possível carregar o catálogo de minúcias.\n"
            "Arquivo: " + catalogPath);
        return;
    }
    
    // Carregar todas as minúcias inicialmente
    currentMinutiae = catalog.getAllMinutiae();
    populateTable(currentMinutiae);
}

void MinutiaeQueryDialog::populateTable(const QVector<FingerprintAnalysis::MinutiaeData>& minutiae) {
    minutiaeTable->setRowCount(0);
    
    for (const auto& m : minutiae) {
        int row = minutiaeTable->rowCount();
        minutiaeTable->insertRow(row);
        
        minutiaeTable->setItem(row, 0, new QTableWidgetItem(QString::number(m.id)));
        minutiaeTable->setItem(row, 1, new QTableWidgetItem(m.namePortuguese));
        
        // Extrair código do nome em inglês se disponível
        QString code = m.nameEnglish;
        if (m.nameEnglish.contains("-")) {
            QStringList parts = m.nameEnglish.split("-");
            if (parts.size() > 1) {
                code = parts.last().trimmed();
            }
        }
        minutiaeTable->setItem(row, 2, new QTableWidgetItem(code));
        minutiaeTable->setItem(row, 3, new QTableWidgetItem(m.categoryToString()));
        minutiaeTable->setItem(row, 4, new QTableWidgetItem(QString::number(m.frequencyGeneral, 'f', 2)));
    }
}

void MinutiaeQueryDialog::onSearchTextChanged(const QString& text) {
    Q_UNUSED(text);
    applyFilters();
}

void MinutiaeQueryDialog::onCategoryFilterChanged(int index) {
    Q_UNUSED(index);
    applyFilters();
}

void MinutiaeQueryDialog::onClassificationFilterChanged(int index) {
    Q_UNUSED(index);
    applyFilters();
}

void MinutiaeQueryDialog::applyFilters() {
    // Começar com todas as minúcias
    QVector<FingerprintAnalysis::MinutiaeData> filtered = catalog.getAllMinutiae();
    
    // Filtro de busca por texto
    QString searchText = searchEdit->text();
    if (!searchText.isEmpty()) {
        QVector<FingerprintAnalysis::MinutiaeData> searchFiltered;
        QString searchLower = searchText.toLower();
        
        for (const auto& m : filtered) {
            if (m.namePortuguese.toLower().contains(searchLower) ||
                m.nameEnglish.toLower().contains(searchLower) ||
                m.descriptionPortuguese.toLower().contains(searchLower)) {
                searchFiltered.append(m);
            }
        }
        filtered = searchFiltered;
    }
    
    // Filtro por classificação
    int classificationIndex = classificationCombo->currentIndex();
    if (classificationIndex > 0) {
        QVector<FingerprintAnalysis::MinutiaeData> classFiltered;
        FingerprintAnalysis::MinutiaeClassification targetClass;
        
        if (classificationIndex == 1) {
            targetClass = FingerprintAnalysis::MinutiaeClassification::Fundamental;
        } else if (classificationIndex == 2) {
            targetClass = FingerprintAnalysis::MinutiaeClassification::Common;
        } else {
            targetClass = FingerprintAnalysis::MinutiaeClassification::Rare;
        }
        
        for (const auto& m : filtered) {
            if (m.classification == targetClass) {
                classFiltered.append(m);
            }
        }
        filtered = classFiltered;
    }
    
    // Filtro por categoria
    int categoryIndex = categoryCombo->currentIndex();
    if (categoryIndex > 0) {
        QVector<FingerprintAnalysis::MinutiaeData> categoryFiltered;
        QString categoryName = categoryCombo->currentText();
        
        for (const auto& m : filtered) {
            QString cat = m.categoryToString();
            if ((categoryName == "Término de Linha" && cat == "Ridge Ending") ||
                (categoryName == "Bifurcação" && cat == "Bifurcation") ||
                (categoryName == "Convergência" && cat == "Convergence") ||
                (categoryName == "Fragmento" && cat == "Fragment") ||
                (categoryName == "Fechamento" && cat == "Enclosure") ||
                (categoryName == "Interrupção" && cat == "Break") ||
                (categoryName == "Sobreposição" && cat == "Overlap") ||
                (categoryName == "Barra Transversal" && cat == "Crossbar") ||
                (categoryName == "Ponte" && cat == "Bridge") ||
                (categoryName == "Trifurcação" && cat == "Trifurcation") ||
                (categoryName == "Tripé" && cat == "Tripod")) {
                categoryFiltered.append(m);
            }
        }
        filtered = categoryFiltered;
    }
    
    currentMinutiae = filtered;
    populateTable(filtered);
}

void MinutiaeQueryDialog::onMinutiaeSelected(int row, int column) {
    Q_UNUSED(column);
    
    if (row < 0 || row >= minutiaeTable->rowCount())
        return;
    
    int id = minutiaeTable->item(row, 0)->text().toInt();
    const auto* minutiae = catalog.getMinutiaeById(id);
    
    if (minutiae) {
        updateMinutiaeDetails(*minutiae);
    }
}

void MinutiaeQueryDialog::updateMinutiaeDetails(const FingerprintAnalysis::MinutiaeData& minutiae) {
    // Atualizar texto de detalhes
    QString details = QString(
        "<h3>%1</h3>"
        "<p><b>Nome em Inglês:</b> %2</p>"
        "<p><b>Categoria:</b> %3</p>"
        "<p><b>Classificação:</b> %4</p>"
        "<p><b>Frequência Geral:</b> %5%</p>"
        "<p><b>Descrição:</b><br>%6</p>"
        "<hr>"
        "<p><b>Estatísticas:</b></p>"
        "<ul>"
        "<li>Média: %7</li>"
        "<li>Mediana: %8</li>"
        "<li>Desvio Padrão: %9</li>"
        "</ul>"
    ).arg(minutiae.namePortuguese)
     .arg(minutiae.nameEnglish)
     .arg(minutiae.categoryToString())
     .arg(minutiae.classificationToString())
     .arg(QString::number(minutiae.frequencyGeneral, 'f', 2))
     .arg(minutiae.descriptionPortuguese)
     .arg(QString::number(minutiae.statistics.mean, 'f', 2))
     .arg(QString::number(minutiae.statistics.median, 'f', 2))
     .arg(QString::number(minutiae.statistics.standardDeviation, 'f', 2));
    
    detailsText->setHtml(details);
    
    // Carregar e exibir imagem
    QString imagePath = QApplication::applicationDirPath() + "/../src/knolegment/" + minutiae.imagePath;
    if (!QFileInfo::exists(imagePath)) {
        imagePath = "src/knolegment/" + minutiae.imagePath;
    }
    if (!QFileInfo::exists(imagePath)) {
        imagePath = "./knolegment/" + minutiae.imagePath;
    }
    
    QPixmap pixmap(imagePath);
    if (!pixmap.isNull()) {
        imageLabel->setPixmap(pixmap);
    } else {
        imageLabel->setText("Imagem não disponível");
    }
}
