# 🎯 Sistema de Estados do Programa

## 📋 Visão Geral

Implementado um **sistema robusto de gerenciamento de estados** que controla o comportamento do programa baseado no contexto atual (imagem, fragmento ou edição de minúcia).

---

## 🔄 Estados Disponíveis

```cpp
enum ProgramState {
    STATE_NONE,              // Nenhuma seleção
    STATE_IMAGE,             // Imagem selecionada
    STATE_FRAGMENT,          // Fragmento selecionado
    STATE_MINUTIA_EDITING    // Minúcia em edição interativa
};
```

### **STATE_NONE** - Sem Seleção
- ✅ Nenhuma entidade selecionada
- ✅ Overlay passa eventos (transparente)
- ✅ ImageViewer inativo
- 📊 Status: "Pronto"

### **STATE_IMAGE** - Imagem Selecionada
- ✅ Imagem completa carregada
- ✅ Overlay passa eventos
- ✅ Operações disponíveis:
  - Zoom (scroll do mouse)
  - Pan (arrastar com mouse)
  - Crop (seleção de fragmentos)
  - Realces (processamento)
  - Rotação, Espelhamento
- 📊 Status: "Imagem selecionada - Operações: zoom, pan, crop, realces"

### **STATE_FRAGMENT** - Fragmento Selecionado
- ✅ Fragmento carregado
- ✅ Overlay passa eventos
- ✅ Operações disponíveis:
  - **Tudo de STATE_IMAGE** +
  - Adicionar minúcias (clique manual)
  - Extrair minúcias (automático)
- 📊 Status: "Fragmento selecionado - Adicione minúcias ou edite imagem"

### **STATE_MINUTIA_EDITING** - Edição de Minúcia
- ✅ Minúcia selecionada e em edição
- ⚠️ Overlay **CAPTURA** eventos (não passa para ImageViewer)
- ❌ Zoom com scroll do mouse DESABILITADO
- ❌ Pan com arrastar DESABILITADO
- ✅ Scroll bars continuam funcionando
- ✅ Operações disponíveis:
  - **Arrastar = Mover posição** (modo azul)
  - **Arrastar = Rotacionar ângulo** (modo laranja)
  - **Clique = Alternar** entre mover/rotacionar
  - **Botão direito = Menu** com opções
- 📊 Status: "🎯 MODO DE EDIÇÃO DE MINÚCIA ATIVO - Arraste para mover/rotacionar"

---

## 🎮 Transições de Estado

```
┌─────────────────────────────────────────────────────────┐
│                                                          │
│  Abrir Projeto → STATE_NONE                            │
│                                                          │
│  Selecionar Imagem → STATE_IMAGE                       │
│                                                          │
│  Selecionar Fragmento → STATE_FRAGMENT                 │
│                                                          │
│  Ctrl+I (com minúcia selecionada) → STATE_MINUTIA_EDITING │
│                                                          │
│  Ctrl+I novamente → Volta ao estado anterior           │
│                                                          │
└─────────────────────────────────────────────────────────┘
```

### Diagrama de Transições

```
        ┌─────────────┐
        │ STATE_NONE  │
        └──────┬──────┘
               │
    ┌──────────┼──────────┐
    │                     │
    v                     v
┌─────────┐         ┌──────────────┐
│ STATE   │         │ STATE_       │
│ IMAGE   │         │ FRAGMENT     │
└────┬────┘         └───────┬──────┘
     │                      │
     │      Ctrl+I          │
     │   (c/ minúcia)       │
     └──────────┬───────────┘
                │
                v
       ┌─────────────────────┐
       │ STATE_MINUTIA_      │
       │ EDITING             │
       └─────────────────────┘
                │
                │ Ctrl+I
                v
         (volta ao anterior)
```

---

## 🔧 Implementação Técnica

### Funções Principais

#### **1. setProgramState(ProgramState newState)**
```cpp
void MainWindow::setProgramState(ProgramState newState) {
    qDebug() << "🔄 setProgramState:" << currentProgramState << "→" << newState;
    
    if (currentProgramState == newState) {
        return; // Evita transições desnecessárias
    }
    
    currentProgramState = newState;
    updateUIForCurrentState(); // Atualiza interface
}
```

#### **2. updateUIForCurrentState()**
```cpp
void MainWindow::updateUIForCurrentState() {
    // Obtém viewer e overlay ativos
    ImageViewer* activeViewer = getActiveViewer();
    MinutiaeOverlay* activeOverlay = getActiveOverlay();
    
    switch (currentProgramState) {
        case STATE_IMAGE:
            // Overlay transparente (passa eventos)
            activeOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            activeOverlay->setEditMode(false);
            break;
            
        case STATE_MINUTIA_EDITING:
            // Overlay captura eventos (não passa)
            activeOverlay->setAttribute(Qt::WA_TransparentForMouseEvents, false);
            activeOverlay->setEditMode(true);
            break;
    }
}
```

#### **3. enableMinutiaEditingMode(bool enable)**
```cpp
void MainWindow::enableMinutiaEditingMode(bool enable) {
    if (enable) {
        // Verifica se há minúcia selecionada
        if (overlay && !overlay->getSelectedMinutiaId().isEmpty()) {
            setProgramState(STATE_MINUTIA_EDITING);
        } else {
            QMessageBox::information(this, "Edição de Minúcia", 
                "Selecione uma minúcia primeiro");
        }
    } else {
        // Volta ao estado anterior
        if (currentEntityType == ENTITY_FRAGMENT) {
            setProgramState(STATE_FRAGMENT);
        } else {
            setProgramState(STATE_IMAGE);
        }
    }
}
```

### Integração Automática

O estado muda **automaticamente** quando você seleciona algo:

```cpp
void MainWindow::setCurrentEntity(const QString& entityId, CurrentEntityType type) {
    // ... código de carregamento ...
    
    // Atualizar estado automaticamente
    if (type == ENTITY_IMAGE) {
        setProgramState(STATE_IMAGE);
    } else if (type == ENTITY_FRAGMENT) {
        setProgramState(STATE_FRAGMENT);
    }
}
```

---

## 🖱️ Controle de Eventos do Mouse

### Fluxo Normal (STATE_IMAGE / STATE_FRAGMENT)

```
Mouse Event
    ↓
MinutiaeOverlay (WA_TransparentForMouseEvents = true)
    ↓ (passa através)
ImageViewer
    ↓
Processa: zoom, pan, crop
```

### Modo de Edição (STATE_MINUTIA_EDITING)

```
Mouse Event
    ↓
MinutiaeOverlay (WA_TransparentForMouseEvents = false)
    ↓ (CAPTURA!)
Processa: mover minúcia, rotacionar ângulo
    ↓
ImageViewer NÃO recebe o evento
```

---

## 📊 Logs de Debug

Todos os estados emitem logs detalhados:

```bash
🔄 setProgramState: 0 → 2
  📄 STATE_FRAGMENT - Fragmento selecionado
  ✅ UI atualizada para estado: 2

🎯 enableMinutiaEditingMode: true
  ✅ Modo de edição de minúcia ATIVADO
🔄 setProgramState: 2 → 3
  ✏️  STATE_MINUTIA_EDITING - Editando minúcia
  ✅ UI atualizada para estado: 3
```

---

## 🎯 Como Usar

### 1️⃣ Workflow com Imagem

```
1. Abrir projeto
2. Selecionar imagem na árvore
   → Estado: STATE_IMAGE
3. Zoom com scroll funciona ✅
4. Pan com arrastar funciona ✅
5. Crop para criar fragmento ✅
```

### 2️⃣ Workflow com Fragmento

```
1. Selecionar fragmento na árvore
   → Estado: STATE_FRAGMENT
2. Zoom/Pan funcionam ✅
3. Clique para adicionar minúcia ✅
4. Minúcias aparecem desenhadas ✅
```

### 3️⃣ Workflow com Edição de Minúcia

```
1. Selecionar fragmento
   → Estado: STATE_FRAGMENT
2. Selecionar minúcia (clique na árvore ou duplo clique)
3. Pressionar Ctrl+I
   → Estado: STATE_MINUTIA_EDITING
4. Overlay captura eventos ✅
5. Zoom com scroll DESABILITADO ✅
6. Botão direito → Menu
7. Escolher "Mover Minúcia"
8. Arrastar = move em tempo real ✅
9. Clique = alterna para "Rotacionar"
10. Arrastar = gira em tempo real ✅
11. Ctrl+I novamente → Desativa modo
    → Estado: STATE_FRAGMENT
```

---

## ⚙️ Configurações Importantes

### Atributo Qt Chave

```cpp
Qt::WA_TransparentForMouseEvents
```

| Valor | Comportamento |
|-------|---------------|
| `true` | Overlay **passa** eventos para widget abaixo |
| `false` | Overlay **captura** eventos (não passa) |

### Estados e Transparência

| Estado | WA_TransparentForMouseEvents |
|--------|------------------------------|
| STATE_NONE | `true` (passa) |
| STATE_IMAGE | `true` (passa) |
| STATE_FRAGMENT | `true` (passa) |
| STATE_MINUTIA_EDITING | `false` (captura) |

---

## 🔍 Verificação de Funcionamento

### Teste 1: Zoom Funciona em STATE_IMAGE/FRAGMENT

```bash
# No terminal, ao dar zoom:
📊 (nenhum log do overlay - evento passa para ImageViewer)
```

### Teste 2: Zoom NÃO Funciona em STATE_MINUTIA_EDITING

```bash
# No terminal, ao dar scroll:
MinutiaeOverlay::mousePressEvent - button: ...
# Overlay captura! ImageViewer não recebe
```

### Teste 3: Transição de Estados

```bash
# Ao pressionar Ctrl+I:
🎯 Menu: Modo de Edição Interativa - ATIVADO
🎯 enableMinutiaEditingMode: true
  ✅ Modo de edição de minúcia ATIVADO
🔄 setProgramState: 2 → 3
  ✏️  STATE_MINUTIA_EDITING - Editando minúcia
  ✅ UI atualizada para estado: 3
```

---

## 📝 Checklist de Implementação

- [x] Enum `ProgramState` criado
- [x] Variável `currentProgramState` adicionada
- [x] Função `setProgramState()` implementada
- [x] Função `updateUIForCurrentState()` implementada
- [x] Função `enableMinutiaEditingMode()` implementada
- [x] Integração com `setCurrentEntity()`
- [x] Menu unificado (Ctrl+I)
- [x] Controle de `WA_TransparentForMouseEvents`
- [x] Logs de debug completos
- [x] Compilação sem erros

---

## 🎨 Status Bar por Estado

| Estado | Mensagem |
|--------|----------|
| NONE | "Pronto" |
| IMAGE | "Imagem selecionada - Operações: zoom, pan, crop, realces" |
| FRAGMENT | "Fragmento selecionado - Adicione minúcias ou edite imagem" |
| MINUTIA_EDITING | "🎯 MODO DE EDIÇÃO DE MINÚCIA ATIVO - Arraste para mover/rotacionar" |

---

## 🚀 Benefícios da Implementação

### ✅ Contexto Claro
- Usuário sempre sabe o que pode fazer
- Status bar mostra operações disponíveis
- Comportamento do mouse adaptado ao contexto

### ✅ Sem Conflitos
- Zoom/Pan só quando apropriado
- Edição de minúcia isolada
- Eventos roteados corretamente

### ✅ Extensível
- Fácil adicionar novos estados
- Logs facilitam debug
- Código organizado e modular

### ✅ Logs Detalhados
- Todas as transições registradas
- Fácil diagnosticar problemas
- Símbolos visuais (emojis) facilitam leitura

---

**Desenvolvido:** 2025-10-12  
**Versão:** 1.0  
**Status:** ✅ Implementado e Funcional
