# 🔍 Como Testar o Modo de Edição Interativa de Minúcias

## ✅ Sistema de Logging Implementado

Todo o sistema de debug agora usa `fprintf(stderr, ...)` diretamente, **garantindo que TODOS os logs apareçam**.

Os logs são prefixados com:
- `[MAINWINDOW]` - Logs do MainWindow
- `[OVERLAY]` - Logs do MinutiaeOverlay
- `[INFO]` - Logs gerais do Qt

---

## 🚀 Como Executar

### Método 1: Script Automático (RECOMENDADO)
```bash
./run_debug.sh
```

### Método 2: Manual
```bash
./build/bin/FingerprintEnhancer 2>&1 | tee debug_log.txt
```

---

## 📋 Passos para Testar

### 1. Executar o Programa
```bash
./run_debug.sh
```

### 2. Abrir Projeto com Fragmento
- Menu → Arquivo → Abrir Projeto
- OU Carregar um projeto que tenha fragmentos com minúcias

### 3. Selecionar Fragmento
- Clicar em um fragmento na árvore de projetos (painel esquerdo)
- **Importante**: O fragmento DEVE TER minúcias marcadas

### 4. Ativar Modo de Edição Interativa

**Opção A - Menu:**
- Menu → Minúcias → 🎯 Modo de Edição Interativa
- OU pressionar `Ctrl+I`

**Opção B - Toolbar:**
- Clicar no botão `✏️ Editar Minúcia` na barra de ferramentas

**✅ Você DEVE ver no console:**
```
[MAINWINDOW] 🎯 toggleInteractiveEditMode: true
[MAINWINDOW]   📊 Estado do overlay antes:
[MAINWINDOW]      - editMode atual: false
[MAINWINDOW]      - tem fragmento: true
[MAINWINDOW]   ✅ Modo de edição interativa ATIVADO no overlay
[MAINWINDOW]   📊 Estado do overlay depois:
[MAINWINDOW]      - editMode agora: true
[MAINWINDOW]      - fragmento: true
```

### 5. Clicar com Botão Direito em Minúcia
- Localizar uma minúcia visualmente na imagem (círculo vermelho)
- Clicar com **botão DIREITO** em cima da minúcia

**✅ Você DEVE ver no console:**
```
[OVERLAY] 📌 MinutiaeOverlay::contextMenuEvent chamado!
[OVERLAY]    - Posição do clique: (X, Y)
[OVERLAY]    - currentFragment: true
[OVERLAY]    - editMode: true
[OVERLAY]    - scaleFactor: 1.000000
[OVERLAY]    - scrollOffset: (0, 0)
[OVERLAY]    - imageOffset: (X, Y)
[OVERLAY]    - Número de minúcias no fragmento: N

[OVERLAY] findMinutiaAt: pos=(X, Y) clickRadius=30 minutiae count=N
[OVERLAY]   Minutia at (X1, Y1) distance=D1 id=XXXXXXXX
[OVERLAY]   Minutia at (X2, Y2) distance=D2 id=XXXXXXXX
[OVERLAY]   -> FOUND! Returning minutia XXXXXXXX
[OVERLAY]   ✅ Minúcia encontrada: {UUID} - Mostrando menu de contexto
```

**✅ Menu de contexto DEVE aparecer com opções:**
- ↔️ Mover Minúcia
- 🔄 Rotacionar Minúcia
- 🗑 Remover Minúcia
- 📍 Posição do Rótulo ▶
- ✏️ Editar Propriedades...

---

## 🐛 Diagnóstico de Problemas

### Problema 1: Não vejo `[MAINWINDOW] 🎯 toggleInteractiveEditMode`
**Causa**: O modo de edição não está sendo ativado
**Solução**: 
- Verificar se clicou em Ctrl+I ou no botão da toolbar
- Verificar se o botão está marcado (checkado)

### Problema 2: `editMode atual: false` ou `tem fragmento: false`
**Causa**: Estado incorreto do overlay
**Soluções**:
- Se `tem fragmento: false` → Selecionar um fragmento primeiro
- Se `editMode atual: true` após ativar → OK, está funcionando

### Problema 3: Não vejo `[OVERLAY] 📌 contextMenuEvent`
**Causa**: Evento de contexto não está chegando ao overlay
**Soluções**:
- Verificar se está usando botão DIREITO (não esquerdo)
- Verificar se está clicando dentro da área da imagem
- Verificar se `editMode: true` no log anterior

### Problema 4: `editMode: false` no contextMenuEvent
**Causa**: Modo de edição não está ativado no overlay
**Solução**: 
- Ativar o modo de edição primeiro (passo 4)
- Verificar logs do `toggleInteractiveEditMode`

### Problema 5: `currentFragment: false` no contextMenuEvent
**Causa**: Fragmento não foi carregado no overlay
**Solução**:
- Selecionar um fragmento na árvore
- Verificar se o fragmento tem minúcias

### Problema 6: `-> No minutia found at click position`
**Causa**: Coordenadas não estão alinhadas OU não clicou perto de minúcia
**Soluções**:
- Verificar valores de `scaleFactor`, `scrollOffset`, `imageOffset`
- Clicar mais perto do centro da minúcia (círculo vermelho)
- Tentar dar zoom na imagem antes
- Verificar distâncias no log (devem ser < 30 pixels)

---

## 📊 O Que Cada Log Significa

### `[MAINWINDOW] toggleInteractiveEditMode: true`
✅ Usuário ativou o modo de edição (Ctrl+I ou toolbar)

### `editMode atual: false` → `editMode agora: true`
✅ O overlay foi configurado para modo de edição

### `tem fragmento: true`
✅ O overlay tem um fragmento carregado (necessário para o menu)

### `[OVERLAY] contextMenuEvent chamado!`
✅ Usuário clicou com botão direito

### `editMode: true` E `currentFragment: true`
✅ Menu de contexto VAI aparecer (condições satisfeitas)

### `findMinutiaAt: ... minutiae count=N`
✅ Procurando minúcia na posição do clique entre N minúcias

### `Minutia at (X, Y) distance=D`
✅ Distância do clique até cada minúcia (deve ser ≤ 30 para encontrar)

### `-> FOUND! Returning minutia`
✅ Minúcia encontrada! Menu VAI aparecer!

### `-> No minutia found`
⚠️ Nenhuma minúcia perto o suficiente. Clicar mais perto ou aumentar zoom.

---

## 📝 Logs Completos

Todos os logs são salvos em `debug_log.txt` no diretório raiz.

Para ver apenas logs relevantes:
```bash
grep -E "\[MAIN|\[OVER" debug_log.txt
```

---

## 🎯 Checklist de Verificação

Use este checklist para confirmar que tudo está funcionando:

- [ ] Script `./run_debug.sh` executa sem erros
- [ ] Programa abre interface gráfica
- [ ] Projeto pode ser aberto
- [ ] Fragmento pode ser selecionado
- [ ] Ao pressionar Ctrl+I, aparece `[MAINWINDOW] toggleInteractiveEditMode: true`
- [ ] `editMode agora: true` aparece nos logs
- [ ] `tem fragmento: true` aparece nos logs
- [ ] Ao clicar botão direito, aparece `[OVERLAY] contextMenuEvent chamado!`
- [ ] `editMode: true` E `currentFragment: true` aparecem
- [ ] `findMinutiaAt` é chamado
- [ ] Minúcias são listadas com coordenadas e distâncias
- [ ] `-> FOUND! Returning minutia` aparece (se clicou perto)
- [ ] Menu de contexto aparece na tela

---

## ✅ Sucesso!

Se você vê o menu de contexto com as opções de mover/rotacionar, **ESTÁ FUNCIONANDO!** 🎉

Se não funcionar, **copie e cole os logs** (especialmente as partes com `[MAINWINDOW]` e `[OVERLAY]`) para análise.
