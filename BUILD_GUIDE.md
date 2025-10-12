# 🔨 Guia de Compilação Cross-Platform - FingerprintEnhancer

Este documento contém instruções detalhadas para compilar o FingerprintEnhancer em diferentes plataformas.

---

## 📋 Índice

1. [Windows](#-windows)
2. [Linux (Ubuntu/Mint)](#-linux-ubuntumint)
3. [macOS](#-macos)
4. [Requisitos Gerais](#-requisitos-gerais)
5. [Estrutura do Projeto](#-estrutura-do-projeto)
6. [Troubleshooting](#-troubleshooting)

---

## 🪟 Windows

### Pré-requisitos

1. **Visual Studio 2022** (Community Edition é suficiente)
   - Instalar workload: "Desktop development with C++"
   - Download: https://visualstudio.microsoft.com/

2. **Qt 6.9.3** (ou superior) com MSVC 2022 64-bit
   - Download: https://www.qt.io/download-qt-installer
   - Componentes necessários:
     - Qt 6.9.3 > MSVC 2022 64-bit
     - Qt 6.9.3 > Additional Libraries > Qt Multimedia

3. **OpenCV 4.12.0** (ou superior) pré-compilado para MSVC
   - Download: https://opencv.org/releases/
   - Extrair em `C:\opencv\`

4. **CMake 3.16+**
   - Geralmente instalado com Qt ou Visual Studio
   - Download: https://cmake.org/download/

### Opção 1: Scripts PowerShell (Recomendado)

```powershell
# Abrir PowerShell NORMAL (não precisa Developer Command Prompt!)
cd Z:\FingerprintEnhancer

# Compilar, fazer deploy e executar Debug
.\build-and-run.ps1

# Compilar Release
.\build-and-run.ps1 -BuildType release -Clean
```

### Opção 2: Scripts Batch (Requer Developer Command Prompt)

```cmd
# Abrir "Developer Command Prompt for VS 2022"
cd Z:\FingerprintEnhancer

# Compilar Debug
scripts\build_win.bat debug

# Copiar DLLs
scripts\deploy_win.bat debug

# Executar
scripts\run_win.bat debug
```

### Opção 3: CMake Direto

```powershell
# Configurar
cmake -B build -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_PREFIX_PATH="C:\Qt\6.9.3\msvc2022_64" `
    -DOpenCV_DIR="C:\opencv\build"

# Compilar
cmake --build build --config Debug

# Executável em: build\bin\Debug\FingerprintEnhancer.exe
```

### Variáveis de Ambiente (Opcional)

```powershell
$env:QT_DIR = "C:\Qt\6.9.3"
$env:OPENCV_DIR = "C:\opencv"
```

---

## 🐧 Linux (Ubuntu/Mint)

### Pré-requisitos

```bash
# Atualizar repositórios
sudo apt update

# Instalar ferramentas de build
sudo apt install build-essential cmake git

# Instalar Qt 6
sudo apt install qt6-base-dev qt6-base-dev-tools libqt6core6 libqt6gui6 libqt6widgets6

# Instalar OpenCV
sudo apt install libopencv-dev

# Verificar versões
cmake --version      # >= 3.16
qmake6 --version     # Qt 6.x
pkg-config --modversion opencv4  # >= 4.0
```

### Compilação

#### Opção 1: Script Shell (Recomendado)

```bash
# Dar permissão de execução
chmod +x scripts/build.sh

# Verificar dependências
./scripts/build.sh deps

# Compilar Debug
./scripts/build.sh debug

# Compilar Release
./scripts/build.sh release

# Executar
./build/bin/FingerprintEnhancer
```

#### Opção 2: Makefile

```bash
# Compilar Debug
make

# Compilar Release
make release

# Executar Debug
make run

# Executar Release  
make run-release

# Limpar builds
make clean-all

# Instalar no sistema
make install
```

#### Opção 3: CMake Direto

```bash
# Configurar
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Compilar (usa todos os cores)
cmake --build build --parallel $(nproc)

# Executar
./build/bin/FingerprintEnhancer
```

### Instalação no Sistema

```bash
# Compilar Release e instalar
./scripts/build.sh release
./scripts/build.sh install

# Ou com Makefile
make install

# Executável instalado em: /usr/local/bin/FingerprintEnhancer
```

---

## 🍎 macOS

### Pré-requisitos

```bash
# Instalar Homebrew (se não tiver)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Instalar Xcode Command Line Tools
xcode-select --install

# Instalar dependências
brew install cmake qt@6 opencv

# Linkar Qt6
brew link qt@6 --force
```

### Compilação

```bash
# Configurar
cmake -B build -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6"

# Compilar
cmake --build build --parallel $(sysctl -n hw.ncpu)

# Executar
./build/bin/FingerprintEnhancer
```

---

## 🔧 Requisitos Gerais

### Versões Mínimas

| Componente | Versão Mínima | Versão Testada |
|------------|---------------|----------------|
| CMake      | 3.16          | 3.30           |
| C++        | C++17         | C++17          |
| Qt         | 6.0           | 6.9.3          |
| OpenCV     | 4.0           | 4.12.0         |

### Compiladores Suportados

- **Windows**: MSVC 2022 (v143), MinGW 11.0+
- **Linux**: GCC 9.0+, Clang 10.0+
- **macOS**: Clang 12.0+ (Xcode 12+)

---

## 📁 Estrutura do Projeto

```
FingerprintEnhancer/
├── CMakeLists.txt              # Configuração CMake principal
├── Makefile                    # Wrapper Makefile para Linux/macOS
├── src/
│   ├── main.cpp
│   ├── core/                   # Lógica de processamento
│   ├── gui/                    # Interface Qt
│   └── afis/                   # Sistema AFIS
├── scripts/
│   ├── build.ps1               # Build PowerShell (Windows)
│   ├── deploy.ps1              # Deploy DLLs (Windows)
│   ├── run.ps1                 # Executar (Windows)
│   ├── build.sh                # Build Shell (Linux/macOS)
│   ├── build_win.bat           # Build Batch (Windows legado)
│   ├── deploy_win.bat          # Deploy Batch (Windows legado)
│   └── run_win.bat             # Executar Batch (Windows legado)
├── build/                      # Build Debug (gerado)
├── build-release/              # Build Release (gerado)
└── BUILD_GUIDE.md              # Este arquivo
```

---

## ❓ Troubleshooting

### Windows

#### Erro: "Qt6 não encontrado"
**Solução:**
```powershell
$env:QT_DIR = "C:\Qt\6.9.3"
# Ou adicionar ao CMAKE_PREFIX_PATH
```

#### Erro: "OpenCV não encontrado"
**Solução:**
```powershell
$env:OPENCV_DIR = "C:\opencv"
# Ou especificar -DOpenCV_DIR no CMake
```

#### Erro: "DLLs Debug/Release misturadas"
**Solução:**
```powershell
# Limpar e recompilar
.\scripts\clean.ps1 -BuildType all
.\build-and-run.ps1 -BuildType release -Clean
```

#### Erro: "execution policy"
**Solução:**
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

---

### Linux

#### Erro: "Qt6 não encontrado"
**Solução:**
```bash
# Ubuntu/Debian
sudo apt install qt6-base-dev qt6-base-dev-tools

# Fedora
sudo dnf install qt6-qtbase-devel

# Arch
sudo pacman -S qt6-base
```

#### Erro: "OpenCV não encontrado"
**Solução:**
```bash
# Ubuntu/Debian
sudo apt install libopencv-dev

# Fedora
sudo dnf install opencv-devel

# Arch
sudo pacman -S opencv
```

#### Erro: "pkg-config não encontra Qt6"
**Solução:**
```bash
export PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig:$PKG_CONFIG_PATH
# Ou especificar CMAKE_PREFIX_PATH
export CMAKE_PREFIX_PATH=/usr/lib/x86_64-linux-gnu/cmake
```

---

### macOS

#### Erro: "Qt6 não encontrado"
**Solução:**
```bash
brew install qt@6
brew link qt@6 --force
export CMAKE_PREFIX_PATH="/opt/homebrew/opt/qt@6"
```

#### Erro: "OpenCV não encontrado"
**Solução:**
```bash
brew install opencv
```

---

## 🎯 Detecção Automática de Plataforma

O CMakeLists.txt detecta automaticamente:

1. **Sistema Operacional**: Windows, Linux, macOS
2. **Compilador**: MSVC, GCC, Clang
3. **Arquitetura**: x64, ARM64
4. **Build Type**: Debug, Release

Exemplo de output durante configuração:

```
-- Plataforma: Windows
-- Compilador: MSVC 19.44.35217.0
-- Build type: Release
-- Qt version: 6.9.3
-- OpenCV version: 4.12.0
```

---

## 🚀 Build Types

### Debug
- **Otimização**: Desabilitada
- **Símbolos**: Habilitados
- **DLLs**: Com sufixo "d" (Qt6Cored.dll, opencv_world4120d.dll)
- **Uso**: Desenvolvimento e debugging

### Release
- **Otimização**: Máxima
- **Símbolos**: Desabilitados
- **DLLs**: Sem sufixo (Qt6Core.dll, opencv_world4120.dll)
- **Uso**: Produção e distribuição

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
#       FingerprintEnhancer-1.0.0-Linux.rpm
```

### macOS (DMG)
```bash
cmake --build build-release --target package
# Gera: FingerprintEnhancer-1.0.0-Darwin.dmg
```

---

## 📞 Suporte

Para problemas de compilação:
1. Verifique as versões das dependências
2. Consulte a seção [Troubleshooting](#-troubleshooting)
3. Abra uma issue no GitHub com:
   - Sistema operacional e versão
   - Versões de Qt, OpenCV, CMake
   - Log completo do erro

---

**Última atualização**: 2025-01-12
