# FingerprintEnhancer - Guia Rápido Windows 11

## 🚀 Início Rápido (3 comandos)

```powershell
# 1. Verificar dependências
.\scripts\install_deps_win.bat check

# 2. Compilar e fazer deploy
.\scripts\setup_all_win.bat debug

# 3. Executar
.\scripts\run_win.bat debug
```

## 📋 Pré-requisitos

Você precisa instalar manualmente:

1. **Qt6** - https://www.qt.io/download-qt-installer
   - Versão: 6.7.0 ou superior
   - Componente: MinGW 64-bit ou MSVC 2022 64-bit

2. **OpenCV** - https://opencv.org/releases/
   - Versão: 4.x
   - Extrair para `C:\opencv`

3. **CMake** - https://cmake.org/download/
   - Versão: 3.16 ou superior

4. **Compilador**:
   - **Opção A**: MinGW-w64 (recomendado) - `choco install mingw`
   - **Opção B**: Visual Studio 2022 Community (gratuito)

## 📦 Scripts Disponíveis

### Verificação de Dependências
```powershell
# Verificar o que está instalado
.\scripts\install_deps_win.bat check

# Instalar ferramentas básicas via Chocolatey (requer Admin)
.\scripts\install_deps_win.bat install
```

### Compilação
```powershell
# Compilar Debug
.\scripts\build_win.bat debug

# Compilar Release
.\scripts\build_win.bat release

# Limpar builds
.\scripts\build_win.bat clean

# Recompilar do zero
.\scripts\build_win.bat rebuild
```

### Deploy de DLLs
```powershell
# Copiar DLLs necessárias (Qt6, OpenCV, MinGW)
.\scripts\deploy_win.bat debug
.\scripts\deploy_win.bat release
```

### Execução
```powershell
# Executar Debug
.\scripts\run_win.bat debug

# Executar Release
.\scripts\run_win.bat release
```

### Setup Completo (Tudo em Um)
```powershell
# Verificar + Compilar + Deploy + Teste
.\scripts\setup_all_win.bat debug
```

## 🔧 Configuração Manual (Se Necessário)

Se os scripts não detectarem automaticamente Qt6 ou OpenCV:

```powershell
# Definir caminhos manualmente
$env:QT_DIR = "C:\Qt\6.9.3"
$env:OPENCV_DIR = "C:\opencv"

# Executar build
.\scripts\build_win.bat debug
```

## ⚠️ Problemas Comuns

### Erro: "Qt6 não encontrado"
**Solução**: Instale Qt6 em `C:\Qt\` ou defina `$env:QT_DIR`

### Erro: "OpenCV não encontrado"
**Solução**: Extraia OpenCV em `C:\opencv` ou defina `$env:OPENCV_DIR`

### Erro: "DLL não encontrada ao executar"
**Solução**: Execute `.\scripts\deploy_win.bat debug`

### Erro: Incompatibilidade MinGW vs MSVC
**Causa**: OpenCV compilado com MSVC, mas usando MinGW (ou vice-versa)
**Solução**: 
- Use MSVC para tudo (abra "Developer Command Prompt for VS 2022")
- OU recompile OpenCV com MinGW

### Erro: "Generator mismatch"
**Solução**: Limpe o cache
```powershell
.\scripts\build_win.bat clean
.\scripts\build_win.bat debug
```

## 📁 Estrutura Após Build

```
FingerprintEnhancer/
├── build/                      # Build Debug
│   └── bin/
│       ├── FingerprintEnhancer.exe
│       ├── Qt6Core.dll
│       ├── Qt6Gui.dll
│       ├── libopencv_*.dll
│       └── platforms/
│           └── qwindows.dll    # Plugin essencial!
└── build-release/              # Build Release
    └── ...
```

## 📖 Documentação Completa

- **Guia Detalhado**: [INSTALL_WINDOWS.md](INSTALL_WINDOWS.md)
- **README Geral**: [README.md](README.md)

## 🆘 Suporte

- **Email**: adelinocpp@yahoo.com
- **Issues**: GitHub Issues

---

## Resumo de Comandos

```powershell
# ========== WORKFLOW COMPLETO ==========
# 1. Verificar dependências
.\scripts\install_deps_win.bat check

# 2. Compilar
.\scripts\build_win.bat debug

# 3. Deploy de DLLs
.\scripts\deploy_win.bat debug

# 4. Executar
.\scripts\run_win.bat debug

# ========== OU TUDO DE UMA VEZ ==========
.\scripts\setup_all_win.bat debug
```

**Boa sorte com o FingerprintEnhancer!** 🎯
