# Scripts PowerShell para FingerprintEnhancer

## 🚀 Vantagens dos Scripts PowerShell

- ✅ **Não precisa** do "Developer Command Prompt for VS 2022"
- ✅ Roda diretamente no **PowerShell normal**
- ✅ Configura **automaticamente** o ambiente do Visual Studio
- ✅ Detecção **automática** de Qt6 e OpenCV
- ✅ Sintaxe mais **moderna e legível**

---

## 📋 Pré-requisitos

1. **Visual Studio 2022** com C++ Desktop Development
2. **Qt6** com MSVC 2022 64-bit (testado com 6.9.3)
3. **OpenCV** compilado com MSVC (testado com 4.12.0)
4. **CMake** (geralmente vem com o Qt)

---

## 🎯 Uso Rápido

### Opção 1: All-in-One (Recomendado)

```powershell
# Compilar, fazer deploy e executar tudo de uma vez (Debug)
.\build-and-run.ps1

# Compilar Release
.\build-and-run.ps1 -BuildType release

# Limpar e recompilar
.\build-and-run.ps1 -Clean

# Compilar e fazer deploy, mas não executar
.\build-and-run.ps1 -NoRun
```

### Opção 2: Passo a Passo

```powershell
# 1. Compilar
.\scripts\build.ps1

# 2. Copiar DLLs
.\scripts\deploy.ps1

# 3. Executar
.\scripts\run.ps1
```

---

## 📚 Detalhes dos Scripts

### 1️⃣ build.ps1

Compila o projeto.

**Uso:**
```powershell
.\scripts\build.ps1 [-BuildType debug|release] [-Clean]
```

**Exemplos:**
```powershell
# Compilar Debug
.\scripts\build.ps1

# Compilar Release
.\scripts\build.ps1 -BuildType release

# Limpar e compilar
.\scripts\build.ps1 -Clean
```

**O que faz:**
- ✅ Detecta e configura Visual Studio automaticamente
- ✅ Detecta Qt6 MSVC e OpenCV
- ✅ Configura CMake
- ✅ Compila o projeto

---

### 2️⃣ deploy.ps1

Copia DLLs necessárias para o diretório do executável.

**Uso:**
```powershell
.\scripts\deploy.ps1 [-BuildType debug|release]
```

**Exemplos:**
```powershell
# Deploy Debug
.\scripts\deploy.ps1

# Deploy Release
.\scripts\deploy.ps1 -BuildType release
```

**O que faz:**
- ✅ Copia DLLs do Qt6 (com sufixo 'd' para Debug)
- ✅ Copia plugins do Qt6 (platforms, imageformats, etc)
- ✅ Copia DLLs do OpenCV
- ✅ Verifica se todas as dependências foram copiadas

---

### 3️⃣ run.ps1

Executa a aplicação.

**Uso:**
```powershell
.\scripts\run.ps1 [-BuildType debug|release]
```

**Exemplos:**
```powershell
# Executar Debug
.\scripts\run.ps1

# Executar Release
.\scripts\run.ps1 -BuildType release
```

---

### 4️⃣ build-and-run.ps1

Script all-in-one que faz tudo.

**Uso:**
```powershell
.\build-and-run.ps1 [-BuildType debug|release] [-Clean] [-NoDeploy] [-NoRun]
```

**Exemplos:**
```powershell
# Compilar, deploy e executar (Debug)
.\build-and-run.ps1

# Compilar e executar Release
.\build-and-run.ps1 -BuildType release

# Limpar, compilar, deploy e executar
.\build-and-run.ps1 -Clean

# Só compilar e fazer deploy (não executar)
.\build-and-run.ps1 -NoRun

# Só compilar (sem deploy e sem executar)
.\build-and-run.ps1 -NoDeploy -NoRun
```

---

## 🔧 Configuração de Variáveis de Ambiente (Opcional)

Se Qt6 ou OpenCV estiverem em locais não-padrão, defina:

```powershell
# Temporário (só para sessão atual)
$env:QT_DIR = "C:\Caminho\Para\Qt\6.9.3"
$env:OPENCV_DIR = "C:\Caminho\Para\opencv"

# Permanente (via PowerShell Admin)
[System.Environment]::SetEnvironmentVariable("QT_DIR", "C:\Caminho\Para\Qt\6.9.3", "User")
[System.Environment]::SetEnvironmentVariable("OPENCV_DIR", "C:\Caminho\Para\opencv", "User")
```

---

## 🛡️ Política de Execução do PowerShell

Se aparecer erro de "execution policy", execute como **Administrador**:

```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

Ou execute o script diretamente:

```powershell
powershell -ExecutionPolicy Bypass -File .\build-and-run.ps1
```

---

## 📁 Estrutura de Diretórios

```
FingerprintEnhancer/
├── build/                          # Build Debug
│   └── bin/Debug/
│       ├── FingerprintEnhancer.exe
│       ├── Qt6Cored.dll            # DLLs Debug (sufixo 'd')
│       ├── opencv_world4120d.dll
│       └── platforms/              # Plugins Qt6
├── build-release/                  # Build Release
│   └── bin/Release/
│       ├── FingerprintEnhancer.exe
│       ├── Qt6Core.dll             # DLLs Release (sem sufixo)
│       └── opencv_world4120.dll
├── scripts/
│   ├── build.ps1                   # Compilar
│   ├── deploy.ps1                  # Deploy DLLs
│   ├── run.ps1                     # Executar
│   ├── build_win.bat               # Scripts batch (legado)
│   ├── deploy_win.bat
│   └── run_win.bat
└── build-and-run.ps1               # All-in-one
```

---

## ❓ Troubleshooting

### Erro: "Visual Studio não encontrado"

**Solução:** Instale Visual Studio 2022 com "Desktop development with C++"

### Erro: "Qt6 MSVC não encontrado"

**Soluções:**
1. Instale Qt6 com componente MSVC 2022 64-bit
2. Ou defina: `$env:QT_DIR = "C:\Qt\6.9.3"`

### Erro: "OpenCV MSVC não encontrado"

**Soluções:**
1. Baixe binários pré-compilados do OpenCV
2. Ou defina: `$env:OPENCV_DIR = "C:\opencv"`

### Erro: "execution policy"

**Solução:**
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

---

## 🎯 Comparação: Batch vs PowerShell

| Característica | Batch (.bat) | PowerShell (.ps1) |
|----------------|--------------|-------------------|
| Requer Developer Prompt | ✅ Sim | ❌ Não |
| Configura VS automaticamente | ❌ Não | ✅ Sim |
| Sintaxe moderna | ❌ Não | ✅ Sim |
| Mensagens coloridas | ⚠️ Limitado | ✅ Completo |
| Parâmetros nomeados | ❌ Não | ✅ Sim |
| Validação de entrada | ❌ Manual | ✅ Automática |

---

## 💡 Dicas

### Criar Alias Permanente

```powershell
# Adicionar ao seu perfil PowerShell
notepad $PROFILE

# Adicionar estas linhas:
function Build-FP { Set-Location "Z:\FingerprintEnhancer"; .\build-and-run.ps1 @args }
Set-Alias fp Build-FP

# Depois, de qualquer lugar:
fp              # Compila e executa Debug
fp release      # Compila e executa Release
fp -Clean       # Limpa e recompila
```

### Atalho no Desktop

Crie um arquivo `FingerprintEnhancer.lnk` com:
- **Destino:** `powershell.exe -ExecutionPolicy Bypass -File "Z:\FingerprintEnhancer\build-and-run.ps1"`
- **Iniciar em:** `Z:\FingerprintEnhancer`

---

## 📞 Suporte

Os scripts batch originais (`build_win.bat`, `deploy_win.bat`, `run_win.bat`) ainda funcionam normalmente, mas **requerem** o Developer Command Prompt.

Use os scripts PowerShell para maior conveniência! 🚀
