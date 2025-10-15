#include "ProjectSaverWorker.h"
#include "../core/ProjectManager.h"
#include <QDebug>
#include <QFileInfo>

namespace FingerprintEnhancer {

ProjectSaverWorker::ProjectSaverWorker(QObject *parent)
    : QThread(parent), cancelled(false), timeoutMs(300000) { // 5 minutos padrão
    
    // Timer será criado na thread correta
    timeoutTimer = nullptr;
}

ProjectSaverWorker::~ProjectSaverWorker() {
    cancel();
    wait();
    if (timeoutTimer) {
        timeoutTimer->stop();
        delete timeoutTimer;
    }
}

void ProjectSaverWorker::setSaveParameters(const QString& projectPath) {
    QMutexLocker locker(&mutex);
    this->projectPath = projectPath;
}

void ProjectSaverWorker::run() {
    // Criar timer na thread de execução
    timeoutTimer = new QTimer();
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, this, &ProjectSaverWorker::onTimeout);
    timeoutTimer->start(timeoutMs);

    emit progressUpdated("Iniciando salvamento do projeto...");

    // Verificar cancelamento
    {
        QMutexLocker locker(&mutex);
        if (cancelled) {
            timeoutTimer->stop();
            emit saveCompleted(false, "Salvamento cancelado pelo usuário");
            emit finished();
            return;
        }
    }

    try {
        emit progressUpdated("Serializando dados do projeto...");
        
        // Obter ProjectManager singleton
        ProjectManager& pm = ProjectManager::instance();
        
        if (!pm.hasOpenProject()) {
            timeoutTimer->stop();
            emit saveCompleted(false, "Nenhum projeto aberto para salvar");
            emit finished();
            return;
        }

        emit progressUpdated("Salvando imagens e fragmentos...");
        
        // Salvar projeto (usa o caminho já configurado no projeto ou saveProjectAs)
        bool success = false;
        if (projectPath.isEmpty()) {
            // Salvar no caminho atual do projeto
            success = pm.saveProject();
        } else {
            // Salvar em novo caminho
            success = pm.saveProjectAs(projectPath);
        }

        timeoutTimer->stop();

        if (success) {
            QFileInfo fileInfo(pm.getCurrentProjectPath());
            QString message = QString("Projeto salvo com sucesso!\nArquivo: %1")
                             .arg(fileInfo.fileName());
            emit saveCompleted(true, message);
        } else {
            emit saveCompleted(false, "Falha ao salvar o projeto. Verifique permissões de escrita.");
        }

    } catch (const std::exception& e) {
        timeoutTimer->stop();
        QString errorMsg = QString("Erro durante salvamento: %1").arg(e.what());
        emit saveCompleted(false, errorMsg);
    } catch (...) {
        timeoutTimer->stop();
        emit saveCompleted(false, "Erro desconhecido durante o salvamento");
    }

    emit finished();
}

void ProjectSaverWorker::cancel() {
    QMutexLocker locker(&mutex);
    cancelled = true;
}

void ProjectSaverWorker::onTimeout() {
    cancel();
    emit saveTimeout();
    emit saveCompleted(false, "Timeout: O salvamento demorou mais de 5 minutos e foi cancelado");
    emit finished();
}

} // namespace FingerprintEnhancer
