# FingerprintEnhancer

**Open Source Fingerprint Enhancement Software**

Uma alternativa de código aberto e multiplataforma ao descontinuado "ImaQuest Fingerprint Enhancement Software Suite", desenvolvida em C++ com Qt Framework e OpenCV.

## 🎯 Objetivo

O FingerprintEnhancer foi criado para preencher a lacuna deixada pelo ImaQuest, oferecendo ferramentas profissionais de análise forense de impressões digitais com as seguintes características:

- **Código Aberto**: Transparência total nos algoritmos utilizados
- **Multiplataforma**: Funciona em Windows, Linux e macOS
- **Cadeia de Custódia Digital**: Rastreabilidade completa de todas as operações
- **Interface Profissional**: Design intuitivo baseado no padrão da indústria

## ✨ Funcionalidades Principais

### Processamento de Imagem (Enhancement)
- **FFT (Fast Fourier Transform)** para remoção de ruído periódico
- **Subtração de Fundo** para imagens com iluminação irregular
- **Filtros Avançados**: Gaussian, Sharpen, CLAHE, Equalização de Histograma
- **Operações Morfológicas**: Binarização, Esqueletização
- **Calibração de Escala** para medições precisas

### Análise de Minúcias
- **Extração Automática** usando algoritmo Crossing Number
- **Marcação Manual** com editor visual interativo
- **Comparação Lado a Lado** de impressões conhecidas vs. latentes
- **Geração de Gráficos** (Charting) para apresentação em tribunal
- **Filtragem de Minúcias Falsas** com algoritmos inteligentes

### Gestão de Projetos
- **Cadeia de Custódia Digital** com trilha de auditoria completa
- **Formato de Projeto Seguro** com verificação de integridade
- **Exportação de Relatórios** em múltiplos formatos
- **Histórico Completo** de todas as operações realizadas

## 🚀 Início Rápido

### Pré-requisitos

- **Sistema Operacional**: Linux (Ubuntu/Debian), Windows 10+, ou macOS 10.15+
- **Compilador**: GCC 9+ ou Clang 10+ com suporte a C++17
- **CMake**: Versão 3.16 ou superior
- **Qt6**: Framework de interface gráfica
- **OpenCV**: Biblioteca de processamento de imagem

### Instalação Automática (Linux)

```bash
# Clone o repositório
git clone https://github.com/seu-usuario/FingerprintEnhancer.git
cd FingerprintEnhancer

# Execute o script de configuração (instala todas as dependências)
./scripts/setup_dev_env.sh

# Compile o projeto
./scripts/build.sh release

# Execute o programa
./build-release/bin/FingerprintEnhancer
```

### Instalação Manual

Consulte o arquivo [docs/README.md](docs/README.md) para instruções detalhadas de instalação manual das dependências.

## 🛠️ Desenvolvimento

### Estrutura do Projeto

```
FingerprintEnhancer/
├── src/
│   ├── core/                 # Núcleo C++ (algoritmos de processamento)
│   │   ├── ImageProcessor.h/cpp    # Processamento de imagem
│   │   ├── MinutiaeExtractor.h/cpp # Extração de minúcias
│   │   └── ProjectManager.h/cpp    # Gerenciamento de projetos
│   ├── gui/                  # Interface Qt
│   │   ├── MainWindow.h/cpp        # Janela principal
│   │   ├── ImageViewer.h/cpp       # Visualizador de imagem
│   │   └── MinutiaeEditor.h/cpp    # Editor de minúcias
│   └── main.cpp             # Ponto de entrada
├── scripts/                 # Scripts de build e configuração
├── docs/                    # Documentação
├── assets/                  # Recursos (ícones, imagens de exemplo)
├── .vscode/                 # Configurações do Visual Studio Code
└── CMakeLists.txt          # Configuração do CMake
```

### Compilação

```bash
# Build debug (para desenvolvimento)
./scripts/build.sh debug

# Build release (otimizado)
./scripts/build.sh release

# Limpar builds
./scripts/build.sh clean

# Ver todas as opções
./scripts/build.sh help
```

### Visual Studio Code

O projeto está pré-configurado para desenvolvimento no VSCode:

1. Abra o projeto: `code .`
2. Instale as extensões recomendadas (será sugerido automaticamente)
3. Use `Ctrl+Shift+P` → "CMake: Configure" para configurar
4. Use `Ctrl+Shift+P` → "CMake: Build" para compilar
5. Use `F5` para executar com debug

## 📋 Comandos Úteis

```bash
# Verificar dependências
./scripts/build.sh deps

# Informações do sistema
./scripts/build.sh info

# Executar testes
./scripts/build.sh test

# Instalar no sistema
./scripts/build.sh install
```

## 🏗️ Arquitetura

O FingerprintEnhancer segue uma arquitetura **Núcleo + Interface**:

- **Núcleo (Core)**: Biblioteca C++ com todos os algoritmos de processamento
- **Interface (GUI)**: Camada Qt que utiliza o núcleo para operações

Esta separação permite:
- Máximo desempenho para operações críticas
- Facilidade de manutenção e testes
- Possibilidade de criar interfaces alternativas (CLI, web, etc.)
- Reutilização do núcleo em outros projetos

## 🔬 Algoritmos Implementados

### Processamento de Imagem
- **FFT 2D** com máscara interativa para remoção de ruído periódico
- **Subtração de fundo** com normalização automática
- **Filtros de convolução** otimizados (Gaussian, Laplacian, Sharpen)
- **CLAHE** (Contrast Limited Adaptive Histogram Equalization)
- **Esqueletização** usando operações morfológicas iterativas

### Extração de Minúcias
- **Crossing Number** para identificação de terminações e bifurcações
- **Filtragem de ruído** baseada em comprimento de crista
- **Cálculo de orientação** usando gradientes locais
- **Avaliação de qualidade** baseada em contraste local

## 🤝 Contribuindo

Contribuições são muito bem-vindas! Por favor:

1. Faça um fork do projeto
2. Crie uma branch para sua feature (`git checkout -b feature/AmazingFeature`)
3. Commit suas mudanças (`git commit -m 'Add some AmazingFeature'`)
4. Push para a branch (`git push origin feature/AmazingFeature`)
5. Abra um Pull Request

### Diretrizes de Contribuição

- Siga o padrão de código existente
- Adicione testes para novas funcionalidades
- Atualize a documentação quando necessário
- Use commits descritivos em inglês

## 📝 Licença

Este projeto está licenciado sob a Licença MIT - veja o arquivo [LICENSE](LICENSE) para detalhes.

## 🙏 Agradecimentos

- Comunidade OpenCV pelos algoritmos de processamento de imagem
- Projeto Qt pela excelente framework de interface
- Pesquisadores da área de biometria pelas publicações científicas
- Comunidade forense digital pelo feedback e sugestões

## 📞 Suporte

- **Email**: adelinocpp@yahoo.com

## 🗺️ Roadmap

### Versão 1.1
- [ ] Suporte a mais formatos de imagem
- [ ] Algoritmos de enhancement adicionais
- [ ] Interface de plugins

### Versão 1.2
- [ ] Análise automatizada de qualidade
- [ ] Exportação para AFIS
- [ ] Suporte a impressões palmares

### Versão 2.0
- [ ] Interface web
- [ ] API REST
- [ ] Processamento em lote
- [ ] Machine Learning para enhancement

---

**FingerprintEnhancer** - Democratizando a análise forense de impressões digitais através do código aberto.

