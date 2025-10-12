# Guia de Testes - Windows 11

Este documento descreve como testar a compilação e execução do FingerprintEnhancer no Windows 11.

## ✅ Checklist de Validação

### 1. Verificação de Dependências

```powershell
# Execute o script de verificação
.\scripts\install_deps_win.bat check
```

**Resultado esperado:**
```
[OK] CMake encontrado: 3.xx.x
[OK] Qt6 encontrado em: C:\Qt\6.9.3\mingw_64
[OK] OpenCV encontrado em: C:\opencv (MinGW build)
[OK] MinGW-w64 g++ encontrado: 15.x.x
[OK] Todas as dependencias estao instaladas!
```

### 2. Compilação Debug

```powershell
# Limpar builds anteriores
.\scripts\build_win.bat clean

# Compilar versão debug
.\scripts\build_win.bat debug
```

**Resultado esperado:**
- Configuração CMake sem erros
- Compilação concluída em 100%
- Mensagem: `[OK] Build concluido com sucesso!`
- Executável criado em: `build\bin\FingerprintEnhancer.exe`

**Verificar tamanho do executável:**
```powershell
dir build\bin\FingerprintEnhancer.exe
```
Tamanho esperado: ~25-35 MB (versão debug)

### 3. Deploy de DLLs

```powershell
# Copiar todas as DLLs necessárias
.\scripts\deploy_win.bat debug
```

**Resultado esperado:**
```
[OK] Qt6Core.dll
[OK] Qt6Gui.dll
[OK] Qt6Widgets.dll
[OK] Plugin platforms
[OK] libopencv_core4120.dll
[OK] libopencv_imgproc4120.dll
...
[OK] Deploy concluido com sucesso!
```

**Verificar DLLs copiadas:**
```powershell
# DLLs Qt
dir build\bin\Qt6*.dll

# DLLs OpenCV
dir build\bin\*opencv*.dll

# Plugin de plataforma (ESSENCIAL!)
dir build\bin\platforms\qwindows.dll
```

### 4. Teste de Execução

```powershell
# Executar aplicação
.\scripts\run_win.bat debug
```

**Resultado esperado:**
- Aplicação inicia sem erros de DLL
- Janela principal aparece
- Interface Qt carrega corretamente
- Menu e barras de ferramentas visíveis

### 5. Testes Funcionais Básicos

Com a aplicação aberta, teste:

#### Teste 1: Criar Novo Projeto
1. Menu → Arquivo → Novo Projeto
2. Preencher nome do projeto
3. Salvar

**Esperado**: Projeto criado sem erros

#### Teste 2: Adicionar Imagem
1. Menu → Arquivo → Adicionar Imagem ao Projeto
2. Selecionar uma imagem qualquer (PNG, JPG)
3. Confirmar

**Esperado**: Imagem carregada e exibida no viewer

#### Teste 3: Aplicar Filtro
1. Com imagem carregada, Menu → Realce → CLAHE
2. Aguardar processamento

**Esperado**: Filtro aplicado, imagem atualizada

#### Teste 4: Rotação
1. Menu → Ferramentas → Rotação de Imagem → Rotação 90° Horário

**Esperado**: Imagem rotacionada corretamente

## 🧪 Testes Avançados

### Teste de Performance

```powershell
# Compilar versão Release
.\scripts\build_win.bat release
.\scripts\deploy_win.bat release
.\scripts\run_win.bat release
```

**Verificar:**
- Inicialização mais rápida que Debug
- Processamento de imagens mais rápido
- Tamanho do executável menor (~15-20 MB)

### Teste de Compatibilidade OpenCV

Verifique se todas as funções OpenCV estão linkadas corretamente:

```powershell
# No PowerShell, após compilar
dumpbin /IMPORTS build\bin\FingerprintEnhancer.exe | Select-String "opencv"
```

**Esperado**: Lista de funções OpenCV importadas

### Teste de Integridade de Dependências

```powershell
# Listar DLLs carregadas dinamicamente
dumpbin /DEPENDENTS build\bin\FingerprintEnhancer.exe
```

**Esperado**:
- Qt6Core.dll
- Qt6Gui.dll
- Qt6Widgets.dll
- libopencv_*.dll
- KERNEL32.dll
- USER32.dll

## 🐛 Diagnóstico de Problemas

### Aplicação não inicia - "Faltando DLL"

```powershell
# Verificar quais DLLs estão faltando
.\scripts\deploy_win.bat debug

# Listar DLLs no diretório bin
dir build\bin\*.dll
```

### Aplicação inicia mas crasheia imediatamente

```powershell
# Executar com output de debug
build\bin\FingerprintEnhancer.exe > output.log 2>&1
type output.log
```

### Plugin de plataforma não encontrado

**Erro**: "This application failed to start because no Qt platform plugin could be initialized"

```powershell
# Verificar se qwindows.dll existe
dir build\bin\platforms\qwindows.dll

# Se não existir, executar deploy novamente
.\scripts\deploy_win.bat debug
```

### Símbolos não resolvidos ao linkar

**Erro durante compilação**: `undefined reference to cv::...`

**Causa**: Incompatibilidade MinGW vs MSVC

**Solução**: Recompilar OpenCV com MinGW ou usar MSVC para tudo

## 📊 Relatório de Teste

Use esta checklist para documentar seus testes:

```
[ ] Dependências verificadas com sucesso
[ ] Compilação Debug sem erros
[ ] Compilação Release sem erros
[ ] Deploy de DLLs completo
[ ] Aplicação inicia corretamente
[ ] Interface gráfica carrega
[ ] Criar projeto funciona
[ ] Adicionar imagem funciona
[ ] Filtros funcionam (CLAHE, FFT, etc.)
[ ] Rotação/Transformações funcionam
[ ] Marcação de minúcias funciona
[ ] Salvar/Carregar projeto funciona
[ ] Performance aceitável
```

## 🎯 Critérios de Sucesso

Para considerar a compilação bem-sucedida:

1. ✅ Todos os scripts executam sem erros
2. ✅ Executável criado (~25-35 MB debug, ~15-20 MB release)
3. ✅ Todas as DLLs necessárias copiadas
4. ✅ Aplicação inicia e interface carrega
5. ✅ Funções básicas (abrir imagem, aplicar filtro) funcionam
6. ✅ Sem crashes durante uso normal

## 📝 Log de Testes

Documente seus resultados:

```
Data: __/__/____
Sistema: Windows 11 (versão: _____)
Qt6: 6.x.x
OpenCV: 4.x.x
Compilador: [ ] MinGW  [ ] MSVC

Resultados:
- Compilação: [ ] OK  [ ] FALHOU
- Deploy: [ ] OK  [ ] FALHOU
- Execução: [ ] OK  [ ] FALHOU
- Funcionalidades: [ ] OK  [ ] PARCIAL  [ ] FALHOU

Notas:
_________________________________
_________________________________
```

## 🔄 Teste de Regressão

Após cada modificação no código, execute:

```powershell
# Teste rápido
.\scripts\build_win.bat rebuild
.\scripts\deploy_win.bat debug
.\scripts\run_win.bat debug

# Teste completo
.\scripts\setup_all_win.bat debug
```

---

**Dúvidas?** Consulte [INSTALL_WINDOWS.md](INSTALL_WINDOWS.md) ou abra uma issue.
