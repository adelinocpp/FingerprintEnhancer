/**
 * @file MainWindow_AsyncOperations.cpp
 * @brief Implementações de operações assíncronas para MainWindow
 * 
 * Este arquivo contém as implementações das funcionalidades de:
 * - Carregamento múltiplo de imagens com threading
 * - Salvamento de projeto com threading e timeout
 * - Obtenção do diretório do projeto para exportação
 * 
 * INSTRUÇÕES PARA INTEGRAÇÃO:
 * Adicione estas implementações ao arquivo MainWindow.cpp
 */

// ============================================================================
// HELPER: Obter diretório do projeto
// ============================================================================

QString MainWindow::getProjectDirectory() const {
    using namespace FingerprintEnhancer;
    ProjectManager& pm = ProjectManager::instance();
    
    if (!pm.hasOpenProject()) {
        return QDir::homePath();
    }
    
    QString projectPath = pm.getCurrentProjectPath();
    if (projectPath.isEmpty()) {
        return QDir::homePath();
    }
    
    QFileInfo fileInfo(projectPath);
    return fileInfo.absolutePath();
}

// ============================================================================
// CARREGAMENTO MÚLTIPLO DE IMAGENS COM THREADING
// ============================================================================

void MainWindow::openImage() {
    using namespace FingerprintEnhancer;
    
    // Verificar se há projeto aberto
    ProjectManager& pm = ProjectManager::instance();
    if (!pm.hasOpenProject()) {
        QMessageBox::warning(this, "Projeto Necessário",
                           "Por favor, crie ou abra um projeto antes de adicionar imagens.");
        return;
    }

    // Verificar se já está carregando imagens
    if (isLoadingImages) {
        QMessageBox::information(this, "Carregamento em Progresso",
                               "Aguarde o carregamento das imagens anteriores terminar.");
        return;
    }

    // Obter diretório do projeto como padrão
    QString defaultDir = getProjectDirectory();

    // Permitir seleção múltipla de arquivos
    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        "Selecionar Imagens para Adicionar ao Projeto",
        defaultDir,
        "Imagens (*.png *.jpg *.jpeg *.bmp *.tiff *.tif);;Todos os Arquivos (*.*)"
    );

    if (filePaths.isEmpty()) {
        return;
    }

    // Criar worker para carregar imagens em thread separada
    imageLoaderWorker = new ImageLoaderWorker(filePaths, this);
    
    // Conectar signals
    connect(imageLoaderWorker, &ImageLoaderWorker::progressUpdated,
            this, &MainWindow::onImageLoadProgress);
    connect(imageLoaderWorker, &ImageLoaderWorker::imageLoaded,
            this, &MainWindow::onImageLoaded);
    connect(imageLoaderWorker, &ImageLoaderWorker::loadingFailed,
            this, &MainWindow::onImageLoadingFailed);
    connect(imageLoaderWorker, &ImageLoaderWorker::allImagesLoaded,
            this, &MainWindow::onAllImagesLoaded);
    connect(imageLoaderWorker, &ImageLoaderWorker::finished,
            imageLoaderWorker, &QObject::deleteLater);

    // Configurar interface para loading
    isLoadingImages = true;
    progressBar->setVisible(true);
    progressBar->setMaximum(filePaths.size());
    progressBar->setValue(0);
    statusLabel->setText(QString("Carregando %1 imagem(ns)...").arg(filePaths.size()));

    // Iniciar carregamento
    imageLoaderWorker->start();
}

void MainWindow::onImageLoadProgress(int current, int total, const QString& currentFile) {
    progressBar->setValue(current);
    statusLabel->setText(QString("Carregando %1/%2: %3").arg(current).arg(total).arg(currentFile));
    
    // Forçar atualização da interface
    QApplication::processEvents();
}

void MainWindow::onImageLoaded(const QString& filePath, const cv::Mat& image) {
    using namespace FingerprintEnhancer;
    
    // Adicionar imagem ao projeto (já na thread principal via signal)
    ProjectManager& pm = ProjectManager::instance();
    FingerprintImage* img = pm.addImageToProject(filePath);
    
    if (!img) {
        qWarning() << "Falha ao adicionar imagem ao projeto:" << filePath;
    }
}

void MainWindow::onImageLoadingFailed(const QString& filePath, const QString& error) {
    QFileInfo fileInfo(filePath);
    qWarning() << "Falha ao carregar" << fileInfo.fileName() << ":" << error;
    
    // Mostrar na barra de status brevemente
    statusLabel->setText(QString("Erro: %1").arg(fileInfo.fileName()));
}

void MainWindow::onAllImagesLoaded(int successCount, int failCount) {
    isLoadingImages = false;
    progressBar->setVisible(false);
    
    QString message;
    if (failCount == 0) {
        message = QString("✓ %1 imagem(ns) carregada(s) com sucesso!").arg(successCount);
        statusLabel->setText(message);
        QMessageBox::information(this, "Carregamento Concluído", message);
    } else {
        message = QString("Carregamento concluído: %1 sucesso, %2 falha(s)")
                    .arg(successCount).arg(failCount);
        statusLabel->setText(message);
        QMessageBox::warning(this, "Carregamento Concluído", message);
    }

    // Atualizar interface
    if (fragmentManager) {
        fragmentManager->refreshProjectTree();
    }
    updateStatusBar();
}

// ============================================================================
// SALVAMENTO DE PROJETO COM THREADING E TIMEOUT
// ============================================================================

void MainWindow::saveProject() {
    using namespace FingerprintEnhancer;
    
    // Verificar se há projeto aberto
    ProjectManager& pm = ProjectManager::instance();
    if (!pm.hasOpenProject()) {
        QMessageBox::warning(this, "Nenhum Projeto",
                           "Não há projeto aberto para salvar.");
        return;
    }

    // Verificar se já está salvando
    if (isSavingProject) {
        QMessageBox::information(this, "Salvamento em Progresso",
                               "Aguarde o salvamento atual terminar.");
        return;
    }

    // Verificar se projeto tem caminho (senão usar saveProjectAs)
    QString projectPath = pm.getCurrentProjectPath();
    if (projectPath.isEmpty()) {
        saveProjectAs();
        return;
    }

    // Criar worker para salvar em thread separada
    projectSaverWorker = new ProjectSaverWorker(this);
    projectSaverWorker->setSaveParameters(""); // Usa caminho atual do projeto
    projectSaverWorker->setTimeout(300000); // 5 minutos

    // Conectar signals
    connect(projectSaverWorker, &ProjectSaverWorker::progressUpdated,
            this, &MainWindow::onSaveProgress);
    connect(projectSaverWorker, &ProjectSaverWorker::saveCompleted,
            this, &MainWindow::onSaveCompleted);
    connect(projectSaverWorker, &ProjectSaverWorker::saveTimeout,
            this, &MainWindow::onSaveTimeout);
    connect(projectSaverWorker, &ProjectSaverWorker::finished,
            projectSaverWorker, &QObject::deleteLater);

    // Configurar interface para saving
    isSavingProject = true;
    progressBar->setVisible(true);
    progressBar->setMaximum(0); // Indeterminate progress
    
    // Desabilitar botões de salvar
    if (saveAction) saveAction->setEnabled(false);
    if (saveAsAction) saveAsAction->setEnabled(false);

    statusLabel->setText("Salvando projeto...");

    // Iniciar salvamento
    projectSaverWorker->start();
}

void MainWindow::saveProjectAs() {
    using namespace FingerprintEnhancer;
    
    // Verificar se há projeto aberto
    ProjectManager& pm = ProjectManager::instance();
    if (!pm.hasOpenProject()) {
        QMessageBox::warning(this, "Nenhum Projeto",
                           "Não há projeto aberto para salvar.");
        return;
    }

    // Verificar se já está salvando
    if (isSavingProject) {
        QMessageBox::information(this, "Salvamento em Progresso",
                               "Aguarde o salvamento atual terminar.");
        return;
    }

    // Obter diretório padrão
    QString defaultDir = getProjectDirectory();
    
    // Diálogo para escolher caminho
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Salvar Projeto Como",
        defaultDir + "/projeto.fpe",
        "Projetos FingerprintEnhancer (*.fpe);;Todos os Arquivos (*.*)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    // Garantir extensão .fpe
    if (!filePath.endsWith(".fpe", Qt::CaseInsensitive)) {
        filePath += ".fpe";
    }

    // Criar worker para salvar em thread separada
    projectSaverWorker = new ProjectSaverWorker(this);
    projectSaverWorker->setSaveParameters(filePath);
    projectSaverWorker->setTimeout(300000); // 5 minutos

    // Conectar signals
    connect(projectSaverWorker, &ProjectSaverWorker::progressUpdated,
            this, &MainWindow::onSaveProgress);
    connect(projectSaverWorker, &ProjectSaverWorker::saveCompleted,
            this, &MainWindow::onSaveCompleted);
    connect(projectSaverWorker, &ProjectSaverWorker::saveTimeout,
            this, &MainWindow::onSaveTimeout);
    connect(projectSaverWorker, &ProjectSaverWorker::finished,
            projectSaverWorker, &QObject::deleteLater);

    // Configurar interface para saving
    isSavingProject = true;
    progressBar->setVisible(true);
    progressBar->setMaximum(0); // Indeterminate progress
    
    // Desabilitar botões de salvar
    if (saveAction) saveAction->setEnabled(false);
    if (saveAsAction) saveAsAction->setEnabled(false);

    statusLabel->setText("Salvando projeto...");

    // Iniciar salvamento
    projectSaverWorker->start();
}

void MainWindow::onSaveProgress(const QString& message) {
    statusLabel->setText(message);
    QApplication::processEvents();
}

void MainWindow::onSaveCompleted(bool success, const QString& message) {
    isSavingProject = false;
    progressBar->setVisible(false);
    
    // Reabilitar botões de salvar
    if (saveAction) saveAction->setEnabled(true);
    if (saveAsAction) saveAsAction->setEnabled(true);

    if (success) {
        statusLabel->setText("✓ Projeto salvo com sucesso!");
        QMessageBox::information(this, "Salvamento Concluído", message);
        updateWindowTitle();
    } else {
        statusLabel->setText("✗ Falha ao salvar projeto");
        QMessageBox::critical(this, "Erro ao Salvar", message);
    }
}

void MainWindow::onSaveTimeout() {
    statusLabel->setText("✗ Timeout: Salvamento cancelado");
    QMessageBox::critical(this, "Timeout",
                         "O salvamento do projeto demorou mais de 5 minutos e foi cancelado.\n"
                         "Isso pode indicar um projeto muito grande ou problemas no disco.");
}

// ============================================================================
// INICIALIZAÇÃO NO CONSTRUTOR
// ============================================================================

/*
 * Adicione no construtor MainWindow::MainWindow():
 * 
 *     // Inicializar workers
 *     imageLoaderWorker = nullptr;
 *     projectSaverWorker = nullptr;
 *     isLoadingImages = false;
 *     isSavingProject = false;
 *     saveAction = nullptr;
 *     saveAsAction = nullptr;
 */

// ============================================================================
// ARMAZENAR ACTIONS NO createMenus()
// ============================================================================

/*
 * No método createMenus(), ao criar as ações de salvar:
 * 
 *     saveAction = fileMenu->addAction("&Salvar Projeto", this, &MainWindow::saveProject,
 *                                      QKeySequence::Save);
 *     saveAsAction = fileMenu->addAction("Salvar Projeto &Como...", this, &MainWindow::saveProjectAs,
 *                                        QKeySequence::SaveAs);
 */
