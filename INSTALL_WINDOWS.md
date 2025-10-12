# Guia de Instalação e Compilação - Windows 11

Este guia fornece instruções detalhadas para compilar e executar o **FingerprintEnhancer** no Windows 11.

## 📋 Índice

- [Pré-requisitos](#pré-requisitos)
- [Instalação Rápida](#instalação-rápida)
- [Instalação Manual](#instalação-manual)
- [Compilação](#compilação)
- [Solução de Problemas](#solução-de-problemas)
- [Perguntas Frequentes](#perguntas-frequentes)

---

## 🎯 Pré-requisitos

Para compilar o FingerprintEnhancer no Windows 11, você precisa de:

### Ferramentas Essenciais

1. **CMake 3.16+** - Sistema de build
2. **Qt6** - Framework de interface gráfica
3. **OpenCV 4.x** - Biblioteca de processamento de imagem
4. **Compilador C++17**:
   - **Opção 1**: MinGW-w64 (recomendado para código aberto)
   - **Opção 2**: Visual Studio 2022 Community (gratuito)

---

## ⚡ Instalação Rápida

### Usando Scripts Automatizados

Execute estes comandos no **PowerShell** (como Administrador):

```powershell
# 1. Verificar dependências instaladas
.\scripts\install_deps_win.bat check

# 2. Instalar ferramentas básicas (CMake, MinGW)
.\scripts\install_deps_win.bat install

# 3. Compilar o projeto
.\scripts\build_win.bat debug

# 4. Executar a aplicação
.\scripts\run_win.bat debug
```

> **Nota**: Qt6 e OpenCV precisam ser instalados manualmente (veja seção abaixo).

---

## 🔧 Instalação Manual

### 1. Instalar Chocolatey (Gerenciador de Pacotes)

Execute no **PowerShell (Administrador)**:

```powershell
Set-ExecutionPolicy Bypass -Scope Process -Force
[System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072
iex ((New-Object System.Net.WebClient).DownloadString('https://community.chocolatey.org/install.ps1'))
```

Feche e reabra o PowerShell após a instalação.

### 2. Instalar CMake

```powershell
choco install -y cmake
```

Ou baixe manualmente: https://cmake.org/download/

### 3. Instalar Compilador

#### Opção A: MinGW-w64 (Recomendado)

```powershell
choco install -y mingw
```

Ou baixe de: https://www.mingw-w64.org/

**Adicione ao PATH**:
- Caminho típico: `C:\ProgramData\mingw64\mingw64\bin`
- Painel de Controle → Sistema → Variáveis de Ambiente → PATH

#### Opção B: Visual Studio 2022 Community

Baixe e instale: https://visualstudio.microsoft.com/downloads/

Durante a instalação, selecione:
- ✅ **Desenvolvimento para Desktop com C++**
- ✅ **Ferramentas CMake para Windows**

### 4. Instalar Qt6

1. Baixe o instalador online: https://www.qt.io/download-qt-installer
2. Execute o instalador
3. Faça login (crie conta gratuita se necessário)
4. Selecione componentes:
   - ✅ **Qt 6.9.x** (ou versão mais recente)
   - ✅ **MinGW 64-bit** (se usar MinGW como compilador)
   - ✅ **MSVC 2022 64-bit** (se usar Visual Studio)
   - ✅ **Qt Creator** (opcional, IDE)

5. Caminho de instalação padrão: `C:\Qt\6.9.3`

### 5. Instalar OpenCV

#### Opção A: Binários Pré-compilados (Rápido)

1. Baixe: https://opencv.org/releases/
2. Extraia para `C:\opencv`
3. Estrutura esperada:
   ```
   C:\opencv\
   ├── build\
   │   └── x64\
   │       ├── vc16\      (binários MSVC)
   │       └── mingw\     (binários MinGW, se disponível)
   ```

> ⚠️ **IMPORTANTE**: Os binários oficiais do OpenCV são compilados com MSVC. Se usar MinGW, você precisará compilar o OpenCV manualmente (veja Opção B).

#### Opção B: Compilar OpenCV com MinGW (Para compatibilidade)

```powershell
# Clone o repositório
git clone https://github.com/opencv/opencv.git C:\opencv-src
cd C:\opencv-src

# Criar diretório de build
mkdir build-mingw
cd build-mingw

# Configurar com CMake
cmake -G "MinGW Makefiles" ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_INSTALL_PREFIX=C:\opencv ^
  -DBUILD_EXAMPLES=OFF ^
  -DBUILD_TESTS=OFF ^
  -DBUILD_PERF_TESTS=OFF ^
  ..

# Compilar (pode demorar 30-60 minutos)
cmake --build . --parallel 4

# Instalar
cmake --install .
```

---

## 🔨 Compilação

### Usando Scripts Automatizados (Recomendado)

```powershell
# Verificar se todas as dependências estão instaladas
.\scripts\install_deps_win.bat check

# Compilar versão Debug (para desenvolvimento)
.\scripts\build_win.bat debug

# Compilar versão Release (otimizada)
.\scripts\build_win.bat release

# Limpar builds
.\scripts\build_win.bat clean

# Recompilar do zero
.\scripts\build_win.bat rebuild
```

### Compilação Manual com CMake

Se os caminhos padrão não funcionarem, configure manualmente:

```powershell
# Definir caminhos das dependências
$env:QT_DIR = "C:\Qt\6.9.3"
$env:OPENCV_DIR = "C:\opencv"
$env:GENERATOR = "MinGW Makefiles"  # ou "Visual Studio 17 2022"

# Configurar
cmake -B build -S . -G "MinGW Makefiles" ^
  -DCMAKE_PREFIX_PATH="C:/Qt/6.9.3/mingw_64/lib/cmake;C:/opencv/build" ^
  -DCMAKE_BUILD_TYPE=Debug

# Compilar
cmake --build build --config Debug --parallel 4
```

### Deploy de DLLs

Após compilar, copie as DLLs necessárias:

```powershell
# Copiar DLLs automaticamente
.\scripts\deploy_win.bat debug

# Ou para release
.\scripts\deploy_win.bat release
```

Este script copia:
- DLLs do Qt6 (Qt6Core, Qt6Gui, Qt6Widgets, etc.)
- Plugins do Qt6 (platforms, imageformats, etc.)
- DLLs do OpenCV
- DLLs do MinGW runtime (se aplicável)

---

## ▶️ Executar a Aplicação

### Usando o Script

```powershell
# Executar versão Debug
.\scripts\run_win.bat debug

# Executar versão Release
.\scripts\run_win.bat release
```

### Executar Manualmente

```powershell
# Debug
.\build\bin\FingerprintEnhancer.exe

# Release
.\build-release\bin\FingerprintEnhancer.exe
```

---

## 🔍 Solução de Problemas

### Problema: "CMake não encontrado"

**Solução**:
```powershell
# Verificar instalação
cmake --version

# Se não funcionar, adicionar ao PATH
$env:PATH += ";C:\Program Files\CMake\bin"
```

### Problema: "Qt6 não encontrado"

**Erro**:
```
CMake Error: Could not find a package configuration file provided by "Qt6"
```

**Solução**:
```powershell
# Definir variável de ambiente
$env:QT_DIR = "C:\Qt\6.9.3"

# Ou especificar diretamente no CMake
cmake -DCMAKE_PREFIX_PATH="C:/Qt/6.9.3/mingw_64/lib/cmake" ...
```

### Problema: "OpenCV não encontrado"

**Erro**:
```
CMake Error: Could not find a package configuration file provided by "OpenCV"
```

**Solução**:
```powershell
# Verificar se OpenCV está instalado
dir C:\opencv\build

# Definir variável
$env:OPENCV_DIR = "C:\opencv"
```

### Problema: "DLL não encontrada ao executar"

**Erro**:
```
The code execution cannot proceed because Qt6Core.dll was not found
```

**Solução**:
```powershell
# Executar script de deploy
.\scripts\deploy_win.bat debug

# Verificar se DLLs foram copiadas
dir build\bin\*.dll
```

### Problema: Incompatibilidade MinGW vs MSVC

**Erro durante linking**:
```
undefined reference to ...
```

**Causa**: OpenCV compilado com MSVC, mas tentando usar MinGW (ou vice-versa).

**Solução**:

**Opção 1** - Usar MSVC para tudo:
```powershell
# Abrir "Developer Command Prompt for VS 2022"
$env:GENERATOR = "Visual Studio 17 2022"
.\scripts\build_win.bat debug
```

**Opção 2** - Recompilar OpenCV com MinGW (veja seção [Instalar OpenCV - Opção B](#opção-b-compilar-opencv-com-mingw-para-compatibilidade))

### Problema: "Generator mismatch"

**Erro**:
```
Error: generator : MinGW Makefiles
Does not match the generator used previously: NMake Makefiles
```

**Solução**:
```powershell
# Limpar cache do CMake
.\scripts\build_win.bat clean
# Recompilar
.\scripts\build_win.bat debug
```

### Problema: Faltam plugins do Qt

**Erro ao executar**:
```
This application failed to start because no Qt platform plugin could be initialized
```

**Solução**:
```powershell
# Verificar se plugin existe
dir build\bin\platforms\qwindows.dll

# Se não existir, executar deploy novamente
.\scripts\deploy_win.bat debug
```

---

## ❓ Perguntas Frequentes

### Q: Qual compilador devo usar: MinGW ou MSVC?

**A**: Recomendamos **MinGW-w64** para manter compatibilidade com código aberto. No entanto, se você já tem Visual Studio instalado e baixou OpenCV pré-compilado, **MSVC** é mais fácil.

### Q: Preciso recompilar o OpenCV?

**A**: Depende:
- **MSVC**: Não, use os binários pré-compilados do site oficial
- **MinGW**: Sim, compile manualmente para garantir compatibilidade

### Q: O projeto compila mas não executa

**A**: Execute o script de deploy:
```powershell
.\scripts\deploy_win.bat debug
```

### Q: Posso usar Qt5 ao invés de Qt6?

**A**: Não, o projeto requer Qt6. Modificar para Qt5 exigiria mudanças significativas no código.

### Q: Como atualizar as dependências?

**A**:
```powershell
# Atualizar via Chocolatey
choco upgrade cmake mingw

# Qt e OpenCV: baixar novas versões manualmente
```

### Q: Posso compilar versão de 32 bits?

**A**: O projeto é projetado para 64 bits. Compilar para 32 bits requer modificações no CMakeLists.txt e dependências x86.

### Q: Como criar um instalador?

**A**: Use CPack (incluído no CMakeLists.txt):
```powershell
# Após compilar release
cd build-release
cpack -G NSIS
```

Isso cria um instalador `.exe` usando NSIS.

---

## 📦 Estrutura de Diretórios Após Build

```
FingerprintEnhancer/
├── build/                          # Build Debug
│   ├── bin/                        # Executável e DLLs
│   │   ├── FingerprintEnhancer.exe
│   │   ├── Qt6Core.dll
│   │   ├── Qt6Gui.dll
│   │   ├── libopencv_core*.dll
│   │   ├── platforms/              # Plugins Qt
│   │   └── ...
│   └── lib/                        # Bibliotecas estáticas
├── build-release/                  # Build Release
│   └── ...
├── scripts/
│   ├── install_deps_win.bat        # Verificar/instalar dependências
│   ├── build_win.bat               # Compilar projeto
│   ├── deploy_win.bat              # Copiar DLLs
│   └── run_win.bat                 # Executar aplicação
└── src/                            # Código-fonte
```

---

## 🎓 Comandos de Referência Rápida

```powershell
# ===== SETUP INICIAL =====
.\scripts\install_deps_win.bat check    # Verificar dependências
.\scripts\install_deps_win.bat install  # Instalar via Chocolatey

# ===== COMPILAÇÃO =====
.\scripts\build_win.bat debug           # Compilar Debug
.\scripts\build_win.bat release         # Compilar Release
.\scripts\build_win.bat clean           # Limpar
.\scripts\build_win.bat rebuild         # Recompilar

# ===== DEPLOYMENT =====
.\scripts\deploy_win.bat debug          # Deploy DLLs (Debug)
.\scripts\deploy_win.bat release        # Deploy DLLs (Release)

# ===== EXECUÇÃO =====
.\scripts\run_win.bat debug             # Executar Debug
.\scripts\run_win.bat release           # Executar Release

# ===== VARIÁVEIS DE AMBIENTE =====
$env:QT_DIR = "C:\Qt\6.9.3"
$env:OPENCV_DIR = "C:\opencv"
$env:GENERATOR = "MinGW Makefiles"
```

---

## 📞 Suporte

Se você encontrar problemas não listados aqui:

1. Verifique o arquivo `build_log.txt` no diretório raiz
2. Abra uma issue no GitHub: [FingerprintEnhancer Issues](https://github.com/seu-usuario/FingerprintEnhancer/issues)
3. Email: adelinocpp@yahoo.com

---

## 📝 Changelog

### Versão 1.0.0
- ✅ Suporte completo para Windows 11
- ✅ Scripts automatizados de build e deploy
- ✅ Compatibilidade com MinGW e MSVC
- ✅ Auto-detecção de dependências

---

**FingerprintEnhancer** - Análise forense de impressões digitais de código aberto para Windows 11.
