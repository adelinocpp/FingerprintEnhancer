#include <QtWidgets/QApplication>
#include <QtCore/QDir>
#include <QtCore/QStandardPaths>
#include <QtCore/QLoggingCategory>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStyleFactory>
#include <QtGui/QPalette>
#include <iostream>

#include "gui/MainWindow.h"
#include "core/TranslationManager_Simple.h"

// Configurar logging do Qt
Q_LOGGING_CATEGORY(fpenhancer, "fpenhancer")

// Handler customizado para garantir que todos os logs apareçam no stderr
void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "[DEBUG] %s\n", localMsg.constData());
        fflush(stderr);
        break;
    case QtInfoMsg:
        fprintf(stderr, "[INFO] %s\n", localMsg.constData());
        fflush(stderr);
        break;
    case QtWarningMsg:
        fprintf(stderr, "[WARNING] %s\n", localMsg.constData());
        fflush(stderr);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "[CRITICAL] %s\n", localMsg.constData());
        fflush(stderr);
        break;
    case QtFatalMsg:
        fprintf(stderr, "[FATAL] %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
        fflush(stderr);
        abort();
    }
}

/**
 * @brief Configurar estilo da aplicação
 */
void setupApplicationStyle(QApplication &app) {
    // Usar estilo Fusion para melhor aparência multiplataforma
    app.setStyle(QStyleFactory::create("Fusion"));
    
    // Configurar paleta de cores profissional
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::WindowText, Qt::white);
    darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
    darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
    darkPalette.setColor(QPalette::ToolTipText, Qt::white);
    darkPalette.setColor(QPalette::Text, Qt::white);
    darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
    darkPalette.setColor(QPalette::ButtonText, Qt::white);
    darkPalette.setColor(QPalette::BrightText, Qt::red);
    darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    darkPalette.setColor(QPalette::HighlightedText, Qt::black);
    
    // Aplicar paleta apenas se não estivermos no modo claro do sistema
    // (pode ser configurável no futuro)
    // app.setPalette(darkPalette);
}

/**
 * @brief Configurar diretórios da aplicação
 */
void setupApplicationDirectories() {
    // Criar diretórios necessários se não existirem
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir appDataDir(appDataPath);
    
    if (!appDataDir.exists()) {
        appDataDir.mkpath(".");
        qCInfo(fpenhancer) << "Created application data directory:" << appDataPath;
    }
    
    // Criar subdiretórios
    QStringList subDirs = {"projects", "temp", "exports", "logs"};
    for (const QString &subDir : subDirs) {
        QString subDirPath = appDataDir.absoluteFilePath(subDir);
        QDir dir(subDirPath);
        if (!dir.exists()) {
            dir.mkpath(".");
            qCInfo(fpenhancer) << "Created subdirectory:" << subDirPath;
        }
    }
}

/**
 * @brief Verificar dependências do sistema
 */
bool checkSystemDependencies() {
    // Verificar se OpenCV está disponível
    try {
        cv::Mat testImage = cv::Mat::zeros(100, 100, CV_8UC1);
        if (testImage.empty()) {
            return false;
        }
    } catch (const cv::Exception &e) {
        qCCritical(fpenhancer) << "OpenCV error:" << e.what();
        return false;
    }
    
    qCInfo(fpenhancer) << "OpenCV version:" << CV_VERSION;
    qCInfo(fpenhancer) << "Qt version:" << QT_VERSION_STR;
    
    return true;
}

/**
 * @brief Função principal da aplicação
 */
int main(int argc, char *argv[]) {
    // Configurar atributos da aplicação antes de criar QApplication
    // Qt6 habilita High DPI automaticamente, não precisa mais destes atributos:
    // QApplication::setAttribute(Qt::AA_EnableHighDpiScaling); // Depreciado
    // QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);    // Depreciado
    
    // Criar aplicação
    QApplication app(argc, argv);
    
    // Instalar handler customizado de mensagens ANTES de qualquer log
    qInstallMessageHandler(customMessageHandler);
    
    // Teste imediato de logging
    fprintf(stderr, "==========================================\n");
    fprintf(stderr, "TESTE: Message handler instalado!\n");
    fprintf(stderr, "==========================================\n");
    fflush(stderr);
    
    qDebug() << "🧪 TESTE: qDebug() funcionando!";
    qInfo() << "🧪 TESTE: qInfo() funcionando!";
    
    // Configurar informações da aplicação
    app.setApplicationName("FingerprintEnhancer");
    app.setApplicationVersion("1.0.0");
    app.setApplicationDisplayName("Fingerprint Enhancement Software");
    app.setOrganizationName("FingerprintEnhancer");
    app.setOrganizationDomain("fingerprintenhancer.org");
    
    // Configurar logging - HABILITAR TODOS OS NÍVEIS
    // Formato: categoria.nível=true/false
    // * = todas as categorias
    QLoggingCategory::setFilterRules(
        "mainwindow.debug=true\n"  // Habilitar DEBUG do MainWindow
        "overlay.debug=true\n"     // Habilitar DEBUG do Overlay
        "fpenhancer.debug=true\n"  // Habilitar DEBUG geral
        "*.debug=true\n"           // Habilitar DEBUG de todas as categorias
        "*.info=true\n"            // Habilitar INFO de todas as categorias
        "*.warning=true\n"         // Habilitar WARNING
        "*.critical=true\n"        // Habilitar CRITICAL
        "qt.*.debug=false\n"       // Desabilitar debug interno do Qt (muito verboso)
        "qt.*.info=false\n"        // Desabilitar info interno do Qt
        "qt.qpa.*.debug=false"     // Desabilitar debug de QPA
    );
    
    // Forçar modo debug para garantir
    QLoggingCategory::defaultCategory()->setEnabled(QtDebugMsg, true);
    
    qDebug() << "🧪 TESTE FINAL: qDebug sem categoria funcionando?";
    qCDebug(fpenhancer) << "🧪 TESTE: qCDebug(fpenhancer) funcionando!";
    
    qCInfo(fpenhancer) << "Starting FingerprintEnhancer v" << app.applicationVersion();
    qCInfo(fpenhancer) << "Built with Qt" << QT_VERSION_STR << "and OpenCV" << CV_VERSION;
    
    // Verificar dependências
    if (!checkSystemDependencies()) {
        QMessageBox::critical(nullptr, "System Error", 
            "Required system dependencies are not available.\n"
            "Please ensure OpenCV is properly installed.");
        return 1;
    }
    
    // Configurar diretórios
    setupApplicationDirectories();
    
    // Configurar estilo
    setupApplicationStyle(app);
    
    // Processar argumentos da linha de comando
    QStringList arguments = app.arguments();
    QString projectFile;
    QString imageFile;
    
    for (int i = 1; i < arguments.size(); ++i) {
        const QString &arg = arguments.at(i);
        if (arg == "--help" || arg == "-h") {
            std::cout << "FingerprintEnhancer - Open Source Fingerprint Enhancement Software\n"
                      << "Usage: " << argv[0] << " [options] [project_file]\n"
                      << "\nOptions:\n"
                      << "  --help, -h          Show this help message\n"
                      << "  --version, -v       Show version information\n"
                      << "  --image <file>      Open image file directly\n"
                      << "\nArguments:\n"
                      << "  project_file        Open project file\n"
                      << std::endl;
            return 0;
        } else if (arg == "--version" || arg == "-v") {
            std::cout << "FingerprintEnhancer version " << app.applicationVersion().toStdString() << "\n"
                      << "Built with Qt " << QT_VERSION_STR << " and OpenCV " << CV_VERSION << "\n"
                      << "Copyright (C) 2024 FingerprintEnhancer Project\n"
                      << "This is free software; see the source for copying conditions.\n"
                      << std::endl;
            return 0;
        } else if (arg == "--image" && i + 1 < arguments.size()) {
            imageFile = arguments.at(++i);
        } else if (!arg.startsWith("--")) {
            projectFile = arg;
        }
    }
    
    try {
        // Inicializar sistema de tradução simplificado
        TranslationManager& translationManager = TranslationManager::instance();

        // Definir idioma padrão como Português
        translationManager.setLanguage(TranslationManager::PORTUGUESE_BR);

        qCInfo(fpenhancer) << "Translation system initialized with language:"
                          << translationManager.getLanguageCode(TranslationManager::PORTUGUESE_BR).c_str();
        
        // Criar e mostrar janela principal
        MainWindow window;
        window.show();
        
        // Carregar arquivo de projeto se especificado
        if (!projectFile.isEmpty()) {
            qCInfo(fpenhancer) << "Loading project file:" << projectFile;
            // TODO: Implementar carregamento de projeto via linha de comando
        }
        
        // Carregar imagem se especificada
        if (!imageFile.isEmpty()) {
            qCInfo(fpenhancer) << "Loading image file:" << imageFile;
            // TODO: Implementar carregamento de imagem via linha de comando
        }
        
        qCInfo(fpenhancer) << "Application started successfully";
        
        // Executar loop principal da aplicação
        int result = app.exec();
        
        qCInfo(fpenhancer) << "Application finished with code:" << result;
        return result;
        
    } catch (const std::exception &e) {
        qCCritical(fpenhancer) << "Unhandled exception:" << e.what();
        QMessageBox::critical(nullptr, "Fatal Error", 
            QString("An unhandled exception occurred:\n%1\n\n"
                   "The application will now exit.").arg(e.what()));
        return 1;
    } catch (...) {
        qCCritical(fpenhancer) << "Unknown exception occurred";
        QMessageBox::critical(nullptr, "Fatal Error", 
            "An unknown error occurred.\n\n"
            "The application will now exit.");
        return 1;
    }
}

