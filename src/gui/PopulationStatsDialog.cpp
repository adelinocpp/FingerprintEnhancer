#include "PopulationStatsDialog.h"
#include <QHeaderView>
#include <QFileInfo>
#include <QApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QPrinter>
#include <QPainter>
#include <QTextDocument>

PopulationStatsDialog::PopulationStatsDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    loadStatistics();
}

void PopulationStatsDialog::setupUI() {
    setWindowTitle("Estatísticas Populacionais de Minúcias");
    setMinimumSize(900, 700);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Label de cabeçalho
    QLabel* headerLabel = new QLabel("<h2>Estatísticas Populacionais de Minúcias</h2>"
                                     "<p>Baseado em: Gomes et al. (2024) - População Brasileira (n=600)</p>");
    mainLayout->addWidget(headerLabel);
    
    // TabWidget
    tabWidget = new QTabWidget();
    createOverviewTab();
    createClassificationTab();
    createFrequencyTab();
    createChartsTab();
    mainLayout->addWidget(tabWidget);
    
    // Botões
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    closeButton = new QPushButton("Fechar");
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);
    
    mainLayout->addLayout(buttonLayout);
}

void PopulationStatsDialog::createOverviewTab() {
    overviewText = new QTextEdit();
    overviewText->setReadOnly(true);
    tabWidget->addTab(overviewText, "Visão Geral");
}

void PopulationStatsDialog::createClassificationTab() {
    classificationTable = new QTableWidget();
    classificationTable->setColumnCount(4);
    classificationTable->setHorizontalHeaderLabels({"Classificação", "Quantidade", "% do Total", "Faixa de Frequência"});
    classificationTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    classificationTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tabWidget->addTab(classificationTable, "Por Classificação");
}

void PopulationStatsDialog::createFrequencyTab() {
    frequencyTable = new QTableWidget();
    frequencyTable->setColumnCount(6);
    frequencyTable->setHorizontalHeaderLabels({"ID", "Nome", "Classificação", "Frequência (%)", "Média", "Desvio Padrão"});
    frequencyTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    frequencyTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    frequencyTable->setSortingEnabled(true);
    tabWidget->addTab(frequencyTable, "Frequências Detalhadas");
}

void PopulationStatsDialog::createChartsTab() {
    chartText = new QTextEdit();
    chartText->setReadOnly(true);
    tabWidget->addTab(chartText, "Gráficos");
}

void PopulationStatsDialog::loadStatistics() {
    // Carregar catálogo JSON
    QString catalogPath = QApplication::applicationDirPath() + "/../src/knolegment/minutiae_catalog.json";
    
    if (!QFileInfo::exists(catalogPath)) {
        catalogPath = "src/knolegment/minutiae_catalog.json";
    }
    if (!QFileInfo::exists(catalogPath)) {
        catalogPath = "./knolegment/minutiae_catalog.json";
    }
    
    if (!catalog.loadFromJson(catalogPath)) {
        QMessageBox::warning(this, "Aviso", 
            "Não foi possível carregar o catálogo de minúcias.");
        return;
    }
    
    populateOverview();
    populateClassificationTable();
    populateFrequencyTable();
    createFrequencyChart();
}

void PopulationStatsDialog::populateOverview() {
    auto allMinutiae = catalog.getAllMinutiae();
    auto fundamental = catalog.getMinutiaeByClassification(FingerprintAnalysis::MinutiaeClassification::Fundamental);
    auto common = catalog.getMinutiaeByClassification(FingerprintAnalysis::MinutiaeClassification::Common);
    auto rare = catalog.getMinutiaeByClassification(FingerprintAnalysis::MinutiaeClassification::Rare);
    
    // Calcular média de frequências
    double totalFreq = 0.0;
    for (const auto& m : allMinutiae) {
        totalFreq += m.frequencyGeneral;
    }
    double avgFreq = allMinutiae.isEmpty() ? 0.0 : totalFreq / allMinutiae.size();
    
    QString overview = QString(
        "<h3>Resumo Estatístico</h3>"
        "<hr>"
        "<h4>Dados Gerais</h4>"
        "<ul>"
        "<li><b>Total de tipos de minúcias catalogados:</b> %1</li>"
        "<li><b>Tamanho da amostra populacional:</b> 600 impressões digitais</li>"
        "<li><b>Origem dos dados:</b> População Brasileira</li>"
        "<li><b>Referência:</b> Gomes et al. (2024), Forensic Science International</li>"
        "<li><b>Total de minúcias analisadas:</b> 49.369</li>"
        "</ul>"
        "<hr>"
        "<h4>Distribuição por Classificação</h4>"
        "<ul>"
        "<li><b>Minúcias Fundamentais:</b> %2 tipos (≥99%% de ocorrência)</li>"
        "<li><b>Minúcias Comuns:</b> %3 tipos (10-99%% de ocorrência)</li>"
        "<li><b>Minúcias Raras:</b> %4 tipos (<10%% de ocorrência)</li>"
        "</ul>"
        "<hr>"
        "<h4>Estatísticas de Frequência</h4>"
        "<ul>"
        "<li><b>Frequência média geral:</b> %5%%</li>"
        "<li><b>Média de minúcias por impressão:</b> 82.28</li>"
        "<li><b>Média em impressões masculinas:</b> 86.62</li>"
        "<li><b>Média em impressões femininas:</b> 77.94</li>"
        "</ul>"
        "<hr>"
        "<h4>Minúcias Fundamentais</h4>"
        "<p>As quatro minúcias fundamentais representam aproximadamente 75%% de todas as minúcias encontradas:</p>"
        "<ul>"
        "<li><b>Ridge ending-B (Início):</b> 100%% - média de 18.5 por impressão</li>"
        "<li><b>Ridge ending-C (Fim):</b> 99.83%% - média de 17.7 por impressão</li>"
        "<li><b>Bifurcation:</b> 100%% - média de 14.5 por impressão</li>"
        "<li><b>Convergence:</b> 99.67%% - média de 11.3 por impressão</li>"
        "</ul>"
        "<hr>"
        "<h4>Distribuição por Padrão Geral (Henry)</h4>"
        "<ul>"
        "<li><b>Whorl (Verticilo):</b> 31.33%% (n=188)</li>"
        "<li><b>Left Loop:</b> 31.33%% (n=188)</li>"
        "<li><b>Right Loop:</b> 30.00%% (n=180)</li>"
        "<li><b>Arch (Arco):</b> 7.33%% (n=44)</li>"
        "</ul>"
        "<hr>"
        "<h4>Características Adicionais</h4>"
        "<ul>"
        "<li><b>Poros:</b> 65.83%% (74%% masculino, 57.67%% feminino)</li>"
        "<li><b>Cicatrizes:</b> 35.83%% (32.33%% masculino, 39.33%% feminino)</li>"
        "<li><b>Cristas Incipientes:</b> 20.5%% (15%% masculino, 26%% feminino)</li>"
        "</ul>"
    ).arg(allMinutiae.size())
     .arg(fundamental.size())
     .arg(common.size())
     .arg(rare.size())
     .arg(QString::number(avgFreq, 'f', 2));
    
    overviewText->setHtml(overview);
}

void PopulationStatsDialog::populateClassificationTable() {
    auto fundamental = catalog.getMinutiaeByClassification(FingerprintAnalysis::MinutiaeClassification::Fundamental);
    auto common = catalog.getMinutiaeByClassification(FingerprintAnalysis::MinutiaeClassification::Common);
    auto rare = catalog.getMinutiaeByClassification(FingerprintAnalysis::MinutiaeClassification::Rare);
    int total = catalog.getCount();
    
    classificationTable->setRowCount(3);
    
    // Fundamental
    classificationTable->setItem(0, 0, new QTableWidgetItem("Fundamental"));
    classificationTable->setItem(0, 1, new QTableWidgetItem(QString::number(fundamental.size())));
    classificationTable->setItem(0, 2, new QTableWidgetItem(QString::number(100.0 * fundamental.size() / total, 'f', 2) + "%"));
    classificationTable->setItem(0, 3, new QTableWidgetItem("≥ 99%"));
    
    // Comum
    classificationTable->setItem(1, 0, new QTableWidgetItem("Comum"));
    classificationTable->setItem(1, 1, new QTableWidgetItem(QString::number(common.size())));
    classificationTable->setItem(1, 2, new QTableWidgetItem(QString::number(100.0 * common.size() / total, 'f', 2) + "%"));
    classificationTable->setItem(1, 3, new QTableWidgetItem("10% - 99%"));
    
    // Rara
    classificationTable->setItem(2, 0, new QTableWidgetItem("Rara"));
    classificationTable->setItem(2, 1, new QTableWidgetItem(QString::number(rare.size())));
    classificationTable->setItem(2, 2, new QTableWidgetItem(QString::number(100.0 * rare.size() / total, 'f', 2) + "%"));
    classificationTable->setItem(2, 3, new QTableWidgetItem("< 10%"));
}

void PopulationStatsDialog::populateFrequencyTable() {
    auto allMinutiae = catalog.getAllMinutiae();
    frequencyTable->setRowCount(allMinutiae.size());
    
    for (int i = 0; i < allMinutiae.size(); ++i) {
        const auto& m = allMinutiae[i];
        
        frequencyTable->setItem(i, 0, new QTableWidgetItem(QString::number(m.id)));
        frequencyTable->setItem(i, 1, new QTableWidgetItem(m.namePortuguese));
        frequencyTable->setItem(i, 2, new QTableWidgetItem(m.classificationToString()));
        
        auto* freqItem = new QTableWidgetItem();
        freqItem->setData(Qt::DisplayRole, m.frequencyGeneral);
        frequencyTable->setItem(i, 3, freqItem);
        
        auto* meanItem = new QTableWidgetItem();
        meanItem->setData(Qt::DisplayRole, m.statistics.mean);
        frequencyTable->setItem(i, 4, meanItem);
        
        auto* stdItem = new QTableWidgetItem();
        stdItem->setData(Qt::DisplayRole, m.statistics.standardDeviation);
        frequencyTable->setItem(i, 5, stdItem);
    }
    
    // Ordenar por frequência decrescente
    frequencyTable->sortItems(3, Qt::DescendingOrder);
}

void PopulationStatsDialog::createFrequencyChart() {
    // Obter minúcias fundamentais para exibição textual
    auto fundamental = catalog.getFundamentalMinutiae();
    
    QString chartHtml = "<h3>Gráfico de Frequência de Minúcias Fundamentais</h3>";
    chartHtml += "<p>Representação visual das frequências:</p>";
    chartHtml += "<table style='width:100%; border-collapse: collapse;'>";
    chartHtml += "<tr style='background-color: #3498db; color: white;'>";
    chartHtml += "<th style='border: 1px solid #ddd; padding: 8px; text-align: left;'>Minúcia</th>";
    chartHtml += "<th style='border: 1px solid #ddd; padding: 8px; text-align: left;'>Frequência (%)</th>";
    chartHtml += "<th style='border: 1px solid #ddd; padding: 8px; text-align: left;'>Visualização</th>";
    chartHtml += "</tr>";
    
    for (const auto& m : fundamental) {
        chartHtml += "<tr>";
        chartHtml += "<td style='border: 1px solid #ddd; padding: 8px;'>" + m.namePortuguese + "</td>";
        chartHtml += "<td style='border: 1px solid #ddd; padding: 8px;'>" + QString::number(m.frequencyGeneral, 'f', 2) + "</td>";
        
        // Criar barra visual simples com caracteres
        int barLength = static_cast<int>(m.frequencyGeneral);
        QString bar = "<span style='color: #2ecc71;'>" + QString("█").repeated(barLength / 2) + "</span>";
        chartHtml += "<td style='border: 1px solid #ddd; padding: 8px;'>" + bar + "</td>";
        chartHtml += "</tr>";
    }
    
    chartHtml += "</table>";
    chartHtml += "<br><p><i>Nota: Cada bloco representa aproximadamente 2% de frequência</i></p>";
    
    chartText->setHtml(chartHtml);
}
