# 🔍 FingerprintEnhancer

**Software Open Source de Análise Forense de Impressões Digitais**

Uma alternativa multiplataforma e de código aberto para análise profissional de impressões digitais, desenvolvida em C++ com Qt 6 e OpenCV.

[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blue)]()
[![C++](https://img.shields.io/badge/C%2B%2B-17-brightgreen)]()
[![Qt](https://img.shields.io/badge/Qt-6.9.3-green)]()
[![OpenCV](https://img.shields.io/badge/OpenCV-4.12.0-red)]()
[![License](https://img.shields.io/badge/license-Apache%202.0-blue)]()

---

## 📋 Índice

- [Sobre](#-sobre)
- [Funcionalidades](#-funcionalidades)
- [Compilação](#-compilação)
- [Início Rápido](#-início-rápido)
- [Estrutura do Projeto](#-estrutura-do-projeto)
- [Contribuindo](#-contribuindo)
- [Licença](#-licença)

---

## 📖 Sobre

O **FingerprintEnhancer** foi criado para fornecer ferramentas profissionais de análise forense de impressões digitais com:

- ✅ **Código Aberto**: Transparência total nos algoritmos
- ✅ **Multiplataforma**: Windows, Linux e macOS
- ✅ **Cadeia de Custódia Digital**: Rastreabilidade completa
- ✅ **Interface Profissional**: Design intuitivo e moderno
- ✅ **56 Tipos de Minúcias**: Classificação padrão internacional

---

## ✨ Funcionalidades

### 🖼️ Processamento de Imagem

- **FFT (Fast Fourier Transform)** - Remoção de ruído periódico
- **Subtração de Fundo** - Correção de iluminação irregular
- **Filtros Avançados** - Gaussian, Sharpen, CLAHE, Equalização
- **Operações Morfológicas** - Binarização, Esqueletização, Dilatação
- **Transformações Geométricas** - Rotação, Espelhamento, Recorte
- **Conversão de Espaços de Cor** - RGB, HSV, HSI, Lab, Grayscale
- **Calibração de Escala** - Medições precisas em mm

### 🔍 Análise de Minúcias

- **56 Tipos de Minúcias** conforme padrão internacional
- **Marcação Manual Avançada** com editor visual
- **Extração Automática** usando Crossing Number
- **Sistema Hierárquico** - Projeto → Imagens → Fragmentos → Minúcias
- **Overlay Sincronizado** com transformações
- **Comparação Lado a Lado** (conhecido vs latente)
- **Geração de Gráficos** para apresentação em tribunal

### 📁 Gestão de Projetos

- **Cadeia de Custódia Digital** com trilha de auditoria
- **Preservação de Originais** - Imagem original nunca modificada
- **Working Image** - Aplica realces sem destruir original
- **Formato Seguro** com verificação MD5
- **Exportação de Relatórios** em múltiplos formatos
- **Interface em Português** com suporte multilíngue

---

## 🔨 Compilação

### Requisitos

| Componente | Versão Mínima | Versão Testada |
|------------|---------------|----------------|
| CMake      | 3.16          | 3.30           |
| C++        | 17            | 17             |
| Qt         | 6.0           | 6.9.3          |
| OpenCV     | 4.0           | 4.12.0         |
| MSVC       | 2022          | 2022 (Windows) |
| GCC        | 9.0+          | 11.0+ (Linux)  |

---

### 🪟 Windows

#### Pré-requisitos
1. **Visual Studio 2022** com "Desktop development with C++"
2. **Qt 6.9.3** com MSVC 2022 64-bit
3. **OpenCV 4.12.0** (extrair em `C:\opencv\`)

#### Compilação Rápida

```powershell
# PowerShell normal (não precisa Developer Command Prompt!)

# Verificar dependências
.\scripts\check-platform.ps1

# Compilar e executar Debug
.\build-and-run.ps1

# Compilar e executar Release
.\build-and-run.ps1 -BuildType release -Clean
```

#### Compilação Passo a Passo

```powershell
# 1. Compilar
.\scripts\build.ps1 -BuildType release

# 2. Copiar DLLs necessárias
.\scripts\deploy.ps1 -BuildType release

# 3. Executar
.\scripts\run.ps1 -BuildType release
```

#### CMake Direto

```powershell
cmake -B build-release -G "Visual Studio 17 2022" -A x64 ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="C:\Qt\6.9.3\msvc2022_64" ^
    -DOpenCV_DIR="C:\opencv\build"

cmake --build build-release --config Release --parallel
```

---

### 🐧 Linux (Ubuntu/Mint/Debian)

#### Pré-requisitos

```bash
# Ferramentas de build
sudo apt update
sudo apt install build-essential cmake git

# Qt 6
sudo apt install qt6-base-dev qt6-base-dev-tools

# OpenCV
sudo apt install libopencv-dev

# Verificar versões
cmake --version             # >= 3.16
qmake6 --version            # Qt 6.x
pkg-config --modversion opencv4  # >= 4.0
```

#### Compilação Rápida

```bash
# Verificar dependências
chmod +x scripts/check-platform.sh
./scripts/check-platform.sh

# Compilar e executar
make run

# Ou compilar Release
make release
make run-release
```

#### Compilação Passo a Passo

```bash
# 1. Compilar
./scripts/build.sh release

# 2. Executar
./build-release/bin/FingerprintEnhancer

# 3. Instalar no sistema (opcional)
./scripts/build.sh install
```

#### CMake Direto

```bash
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel $(nproc)
./build-release/bin/FingerprintEnhancer
```

---

### 🍎 macOS

#### Pré-requisitos

```bash
# Homebrew
brew install cmake qt@6 opencv
brew link qt@6 --force
```

#### Compilação

```bash
cmake -B build-release -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6"

cmake --build build-release --parallel $(sysctl -n hw.ncpu)
./build-release/bin/FingerprintEnhancer
```

---

## 🚀 Início Rápido

### 1️⃣ Criar um Projeto

```
Arquivo → Novo Projeto → Salvar Projeto
```

### 2️⃣ Adicionar Imagem

```
Arquivo → Adicionar Imagem ao Projeto → Selecionar arquivo
```

### 3️⃣ Selecionar Entidade

- No painel direito (aba "Projeto"), clique em uma imagem
- Ou clique direito → "Tornar Corrente"
- Status aparece na barra inferior

### 4️⃣ Aplicar Realces

```
Realce → FFT / CLAHE / Gaussian / etc.
```

**Importante**: A imagem original é sempre preservada!

### 5️⃣ Criar Fragmentos

```
Ferramentas → Recorte de Imagem → Ativar Ferramenta
```

1. Desenhe retângulo de seleção
2. Clique direito → Aplicar Recorte
3. Fragmento aparece no painel do projeto

### 6️⃣ Adicionar Minúcias

1. Selecione um fragmento
2. Clique direito na imagem → "Adicionar Minúcia Aqui"
3. Escolha o tipo (56 tipos disponíveis)
4. Ajuste ângulo e posição

### 7️⃣ Calibrar Escala

```
Ferramentas → Calibração de Escala → Calibrar Escala
```

1. Informe distância conhecida (mm)
2. Meça distância em pixels

### 8️⃣ Transformações

- **Rotação**: `Ferramentas → Rotação de Imagem`
- **Espelhamento**: `Ferramentas → Espelhamento`
- **Cor**: `Ferramentas → Espaço de Cor`

---

## 📁 Estrutura do Projeto

```
FingerprintEnhancer/
├── CMakeLists.txt              # Configuração CMake cross-platform
├── Makefile                    # Wrapper para Linux/macOS
├── build-and-run.ps1           # Script all-in-one Windows
├── src/
│   ├── main.cpp
│   ├── core/                   # Lógica de processamento
│   │   ├── ImageProcessor.cpp
│   │   ├── MinutiaeExtractor.cpp
│   │   ├── ProjectManager.cpp
│   │   └── EnhancementAlgorithms.cpp
│   ├── gui/                    # Interface Qt
│   │   ├── MainWindow.cpp
│   │   ├── ImageViewer.cpp
│   │   ├── MinutiaeEditor.cpp
│   │   └── ProcessingWorker.cpp
│   └── afis/                   # Sistema AFIS
│       └── AFISMatcher.cpp
├── scripts/
│   ├── build.ps1               # Build PowerShell (Windows)
│   ├── deploy.ps1              # Deploy DLLs (Windows)
│   ├── run.ps1                 # Executar (Windows)
│   ├── clean.ps1               # Limpar builds
│   ├── check-platform.ps1      # Verificar dependências
│   ├── build.sh                # Build Shell (Linux/macOS)
│   └── check-platform.sh       # Verificar dependências
├── build/                      # Build Debug (gerado)
└── build-release/              # Build Release (gerado)
```

---

## 🎯 Build Types

### Debug
- **Otimização**: Desabilitada (`/Od` ou `-O0`)
- **Símbolos**: Habilitados para debugging
- **DLLs**: Com sufixo "d" (`Qt6Cored.dll`, `opencv_world4120d.dll`)
- **Uso**: Desenvolvimento e depuração

### Release
- **Otimização**: Máxima (`/O2` ou `-O3`)
- **Símbolos**: Desabilitados
- **DLLs**: Sem sufixo (`Qt6Core.dll`, `opencv_world4120.dll`)
- **Uso**: Produção e distribuição

---

## 🔧 Troubleshooting

### Windows

**Erro: "Qt6 não encontrado"**
```powershell
$env:QT_DIR = "C:\Qt\6.9.3"
```

**Erro: "OpenCV não encontrado"**
```powershell
$env:OPENCV_DIR = "C:\opencv"
```

**Erro: "execution policy"**
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

**Erro: "Access denied ao limpar build"**
- Feche a aplicação antes de limpar
- Ou use `.\scripts\clean.ps1 -BuildType release`

### Linux

**Erro: "Qt6 não encontrado"**
```bash
sudo apt install qt6-base-dev qt6-base-dev-tools
```

**Erro: "OpenCV não encontrado"**
```bash
sudo apt install libopencv-dev
```

---

## 📦 Empacotamento

### Windows (NSIS)
```powershell
cmake --build build-release --target package
# Gera: FingerprintEnhancer-1.0.0-win64.exe
```

### Linux (DEB/RPM)
```bash
cmake --build build-release --target package
# Gera: FingerprintEnhancer-1.0.0-Linux.deb
```

### macOS (DMG)
```bash
cmake --build build-release --target package
# Gera: FingerprintEnhancer-1.0.0-Darwin.dmg
```

---

## 🤝 Contribuindo

Contribuições são bem-vindas! Por favor:

1. Fork o repositório
2. Crie uma branch para sua feature (`git checkout -b feature/MinhaFeature`)
3. Commit suas mudanças (`git commit -m 'Adiciona MinhaFeature'`)
4. Push para a branch (`git push origin feature/MinhaFeature`)
5. Abra um Pull Request

### Diretrizes de Código

- **C++ Standard**: C++17
- **Style**: Google C++ Style Guide (adaptado)
- **Commits**: Mensagens claras e descritivas
- **Testes**: Adicione testes para novas funcionalidades

---

## 📄 Licença

Este projeto está licenciado sob a **Apache License 2.0**. Veja o arquivo [LICENSE](LICENSE) para mais detalhes.

---

## 📞 Suporte

- **Issues**: [GitHub Issues](https://github.com/adelinocpp/FingerprintEnhancer/issues)
- **Documentação**: Consulte a pasta `docs/`
- **Email**: adelinocpp@yahoo.com

---

## 🙏 Agradecimentos

- Comunidade **Qt** pelo excelente framework
- Projeto **OpenCV** pelas bibliotecas de visão computacional
- Comunidade **Open Source** pelo suporte e contribuições

---

**Desenvolvido com ❤️ para a Seção Técnica de Papiloscopia e Modelagem**

**Instituto de Criminalística de Minas Gerais**

*Última atualização: Outubro 2025*
