# 📋 Instruções de Integração - Melhorias Implementadas

## 🎯 Resumo das Melhorias

1. ✅ **Carregamento múltiplo de imagens** com threading e preservação de cores originais
2. ✅ **Salvamento assíncrono de projetos** com timeout de 5 minutos
3. ✅ **Caminho do projeto como padrão** para exportação de fragmentos

---

## 📁 Novos Arquivos Criados

### Workers Assíncronos
- `src/gui/ImageLoaderWorker.h` - Header do worker de carregamento
- `src/gui/ImageLoaderWorker.cpp` - Implementação do worker de carregamento
- `src/gui/ProjectSaverWorker.h` - Header do worker de salvamento
- `src/gui/ProjectSaverWorker.cpp` - Implementação do worker de salvamento

### Implementações para MainWindow
- `src/gui/MainWindow_AsyncOperations.cpp` - **ARQUIVO TEMPORÁRIO** contendo as implementações

---

## 🔧 Modificações Realizadas

### 1. ProjectManager (✅ Completo)
- **Arquivo**: `src/core/ProjectManager.h` e `src/core/ProjectManager.cpp`
- **Mudanças**:
  - `addImageToProject()` agora carrega imagens em cores originais (IMREAD_UNCHANGED)
  - Novo método `addMultipleImagesToProject(const QStringList&)` adicionado

### 2. FragmentExportDialog (✅ Completo)
- **Arquivos**: `src/gui/FragmentExportDialog.h` e `src/gui/FragmentExportDialog.cpp`
- **Mudanças**:
  - Construtor aceita parâmetro `defaultDirectory`
  - Novo método `setDefaultExportPath()` 
  - `onBrowseClicked()` usa diretório do projeto como padrão

### 3. MainWindow.h (✅ Completo)
- **Arquivo**: `src/gui/MainWindow.h`
- **Mudanças**:
  - Includes adicionados: `ImageLoaderWorker.h`, `ProjectSaverWorker.h`
  - Novos slots para loading: `onImageLoadProgress()`, `onImageLoaded()`, etc.
  - Novos slots para saving: `onSaveProgress()`, `onSaveCompleted()`, etc.
  - Novos membros privados: `imageLoaderWorker`, `projectSaverWorker`, flags de estado
  - Novo método helper: `getProjectDirectory()`

### 4. CMakeLists.txt (✅ Completo)
- **Arquivo**: `CMakeLists.txt`
- **Mudanças**:
  - Adicionados workers em `GUI_SOURCES` e `GUI_HEADERS`

---

## 🚀 PRÓXIMO PASSO: Integração no MainWindow.cpp

O arquivo `MainWindow.cpp` é muito grande (49154 linhas) para editar diretamente.
As implementações necessárias estão no arquivo **`MainWindow_AsyncOperations.cpp`**.

### Como Integrar:

#### Opção 1: Copiar e Colar Manualmente
1. Abra `src/gui/MainWindow_AsyncOperations.cpp`
2. Copie as implementações dos métodos
3. Cole no final de `src/gui/MainWindow.cpp` antes do último `}`

#### Opção 2: Usar Comando (Recomendado)
```bash
cd /home/adelino/NUVEM/MEGAsync/Papiloscopia/FingerprintEnhancer

# Fazer backup
cp src/gui/MainWindow.cpp src/gui/MainWindow.cpp.backup

# Extrair apenas as implementações (remover comentários de instrução)
grep -v "^/\*\|^ \*\|^INSTRUÇÕES" src/gui/MainWindow_AsyncOperations.cpp | \
grep -v "^// ====" > /tmp/implementations.cpp

# Adicionar as implementações antes do último }
head -n -1 src/gui/MainWindow.cpp > /tmp/mainwindow_temp.cpp
cat /tmp/implementations.cpp >> /tmp/mainwindow_temp.cpp
echo "}" >> /tmp/mainwindow_temp.cpp
mv /tmp/mainwindow_temp.cpp src/gui/MainWindow.cpp
```

### Métodos a Adicionar

Os seguintes métodos precisam ser adicionados ao `MainWindow.cpp`:

1. `QString MainWindow::getProjectDirectory() const`
2. `void MainWindow::openImage()` - **SUBSTITUIR** o método existente
3. `void MainWindow::onImageLoadProgress(int, int, const QString&)`
4. `void MainWindow::onImageLoaded(const QString&, const cv::Mat&)`
5. `void MainWindow::onImageLoadingFailed(const QString&, const QString&)`
6. `void MainWindow::onAllImagesLoaded(int, int)`
7. `void MainWindow::saveProject()` - **SUBSTITUIR** o método existente
8. `void MainWindow::saveProjectAs()` - **SUBSTITUIR** o método existente
9. `void MainWindow::onSaveProgress(const QString&)`
10. `void MainWindow::onSaveCompleted(bool, const QString&)`
11. `void MainWindow::onSaveTimeout()`

---

## 🔨 Inicialização no Construtor

Adicione no construtor `MainWindow::MainWindow()`:

```cpp
// Inicializar workers assíncronos
imageLoaderWorker = nullptr;
projectSaverWorker = nullptr;
isLoadingImages = false;
isSavingProject = false;
saveAction = nullptr;
saveAsAction = nullptr;
```

---

## 📋 Armazenar Actions no createMenus()

No método `createMenus()`, ao criar o menu File, **modifique** para armazenar as actions:

```cpp
// Em vez de:
// fileMenu->addAction("&Salvar Projeto", this, &MainWindow::saveProject, QKeySequence::Save);

// Use:
saveAction = fileMenu->addAction("&Salvar Projeto", this, &MainWindow::saveProject, 
                                 QKeySequence::Save);
saveAsAction = fileMenu->addAction("Salvar Projeto &Como...", this, &MainWindow::saveProjectAs,
                                   QKeySequence::SaveAs);
```

---

## 🎨 Atualizar Chamadas ao FragmentExportDialog

Sempre que `FragmentExportDialog` for instanciado, passe o diretório do projeto:

```cpp
// Em vez de:
// FragmentExportDialog dialog(fragment, scale, this);

// Use:
FragmentExportDialog dialog(fragment, scale, this, getProjectDirectory());
```

---

## ✅ Checklist de Integração

- [ ] Copiar implementações de `MainWindow_AsyncOperations.cpp` para `MainWindow.cpp`
- [ ] Adicionar inicializações no construtor
- [ ] Modificar `createMenus()` para armazenar actions
- [ ] Atualizar chamadas ao `FragmentExportDialog`
- [ ] Compilar o projeto: `make` ou `.\build-and-run.ps1`
- [ ] Testar carregamento múltiplo de imagens
- [ ] Testar salvamento assíncrono
- [ ] Testar exportação com caminho padrão

---

## 🧪 Como Testar

### Teste 1: Carregamento Múltiplo
1. Criar/abrir projeto
2. Menu → Arquivo → Adicionar Imagem ao Projeto
3. Selecionar **múltiplas** imagens (Ctrl+Click ou Shift+Click)
4. Verificar:
   - Barra de progresso aparece
   - Interface não trava
   - Imagens carregadas em cores originais
   - Mensagem de conclusão

### Teste 2: Salvamento Assíncrono
1. Fazer modificações no projeto
2. Menu → Arquivo → Salvar Projeto (Ctrl+S)
3. Verificar:
   - Botão "Salvar" desabilitado durante operação
   - Barra de progresso indeterminada aparece
   - Interface não trava
   - Mensagem de conclusão (ou timeout após 5 min)

### Teste 3: Caminho Padrão para Exportação
1. Salvar projeto em algum diretório (ex: `/home/user/projetos/caso123.fpe`)
2. Criar fragmento e adicionar minúcias
3. Exportar fragmento
4. Verificar:
   - Caminho padrão é `/home/user/projetos/fragmento_XXXXX_marcado.png`
   - Botão "Procurar" também usa esse diretório como inicial

---

## 📊 Funcionalidades Implementadas

### ImageLoaderWorker
- ✅ Carrega múltiplas imagens em thread separada
- ✅ Reporta progresso (current/total)
- ✅ Preserva cores originais (IMREAD_UNCHANGED)
- ✅ Converte grayscale para BGR automaticamente
- ✅ Emite signal para cada imagem carregada
- ✅ Tratamento de erros individual por imagem
- ✅ Pode ser cancelado

### ProjectSaverWorker
- ✅ Salva projeto em thread separada
- ✅ Timeout de 5 minutos configurável
- ✅ Reporta progresso durante salvamento
- ✅ Desabilita botões de salvar durante operação
- ✅ Mensagem de confirmação ou erro
- ✅ Pode ser cancelado

### Preservação de Cores
- ✅ Imagens coloridas permanecem coloridas
- ✅ Imagens grayscale são convertidas para BGR
- ✅ Uso de `cv::IMREAD_UNCHANGED` no ProjectManager

### Caminho Padrão
- ✅ `getProjectDirectory()` retorna diretório do projeto
- ✅ FragmentExportDialog usa diretório do projeto
- ✅ Fallback para `QDir::homePath()` se projeto não salvo

---

## 🐛 Troubleshooting

### Erro de compilação: "ImageLoaderWorker not found"
- Verifique que CMakeLists.txt foi atualizado
- Execute `cmake --build build --clean-first`

### Erro: "undefined reference to vtable"
- Os headers precisam estar corretos com Q_OBJECT
- Execute `make clean && make`

### Interface trava durante carregamento
- Verifique que está usando `imageLoaderWorker->start()`, não `run()`
- Verifique que signals estão conectados corretamente

### Timeout muito rápido
- Ajuste com `projectSaverWorker->setTimeout(milisegundos)`

---

## 📞 Suporte

Se encontrar problemas durante a integração:
1. Verifique os logs de compilação
2. Confirme que todos os arquivos foram atualizados
3. Faça backup antes de modificações grandes
4. Teste incrementalmente

---

**Data de Criação**: Outubro 2025  
**Versão**: 1.0.0
