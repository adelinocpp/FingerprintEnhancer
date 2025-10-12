# 📍 Sistema de Posicionamento de Rótulos de Minúcias

## 🎯 Visão Geral

Implementado sistema completo para controlar a posição dos rótulos (número + iniciais do tipo) das minúcias em relação à marcação visual.

---

## 📋 Funcionalidades Implementadas

### **1. Posições Disponíveis**

```cpp
enum class MinutiaLabelPosition {
    RIGHT,      // À direita (padrão)
    LEFT,       // À esquerda
    ABOVE,      // Acima
    BELOW,      // Abaixo
    HIDDEN      // Oculto (não exibe)
};
```

### **2. Controle Individual por Minúcia**

Cada minúcia agora tem seu próprio campo `labelPosition`:

```cpp
class Minutia {
    ...
    MinutiaLabelPosition labelPosition = MinutiaLabelPosition::RIGHT;
    ...
};
```

### **3. Três Formas de Configurar**

#### **A) Janela de Edição de Minúcia**
- Duplo clique na minúcia OU
- Menu → Ferramentas → Minúcias → Editar Minúcia
- Nova seção "**Aparência**" com combo de posição do rótulo

#### **B) Menu de Contexto (Botão Direito)**
- No modo de edição de minúcia (Ctrl+I)
- Botão direito na minúcia
- Submenu **"📍 Posição do Rótulo"**:
  ```
  → À Direita
  ← À Esquerda
  ↑ Acima
  ↓ Abaixo
  ⊗ Oculto
  ```

#### **C) Configuração Global**
- Menu → **Visualizar → Configurar Visualização de Minúcias**
- Campo: **"Posição do Rótulo"**
- Define posição padrão para novas minúcias
- Opção de aplicar a todas as minúcias do fragmento atual

---

## 🎨 Exemplos Visuais

### Posicionamento dos Rótulos

```
┌─────────────────────────────────────────┐
│                                         │
│         ABOVE ↑                         │
│          1 BE                           │
│   LEFT ← ● → RIGHT (padrão)             │
│    1 BE      1 BE                       │
│          1 BE                           │
│         BELOW ↓                         │
│                                         │
│         HIDDEN = nada aparece           │
└─────────────────────────────────────────┘
```

**Legenda:**
- `●` = Marcação da minúcia
- `1` = Número da minúcia
- `BE` = Iniciais do tipo (ex: Bifurcation Ending)

---

## 🔧 Como Usar

### **Workflow 1: Editar Minúcia Individual**

```
1. Abrir fragmento com minúcias
2. Duplo clique na minúcia
3. Na janela de edição:
   └─ Grupo "Aparência"
      └─ Posição do Rótulo: [Escolher]
4. OK
5. Rótulo muda de posição imediatamente
```

### **Workflow 2: Menu de Contexto (Modo de Edição)**

```
1. Abrir fragmento com minúcias
2. Ctrl+I (ativar modo de edição)
3. Botão direito na minúcia
4. "📍 Posição do Rótulo" → Escolher
5. Rótulo muda de posição imediatamente
```

### **Workflow 3: Configuração Global**

```
1. Menu → Visualizar → Configurar Visualização de Minúcias
2. Alterar "Posição do Rótulo"
3. OK
4. Pergunta: "Aplicar a TODAS as minúcias?"
   ├─ Sim → Aplica ao fragmento atual
   └─ Não → Só para novas minúcias
```

---

## 📐 Cálculo de Posições

### Implementação

O algoritmo calcula a posição do texto baseado na posição central da minúcia:

```cpp
switch (labelPos) {
    case RIGHT:  // À direita (padrão)
        textPos = QPoint(pos.x() + size/2 + margin, pos.y() - size/2);
        break;
        
    case LEFT:   // À esquerda
        textPos = QPoint(pos.x() - size/2 - margin - textWidth, pos.y() - size/2);
        break;
        
    case ABOVE:  // Acima
        textPos = QPoint(pos.x() - textWidth/2, pos.y() - size/2 - margin - textHeight);
        break;
        
    case BELOW:  // Abaixo
        textPos = QPoint(pos.x() - textWidth/2, pos.y() + size/2 + margin + textHeight);
        break;
        
    case HIDDEN: // Oculto
        return; // Não desenha nada
}
```

**Onde:**
- `pos` = Centro da minúcia
- `size` = Tamanho do marcador (displaySettings.markerSize)
- `margin` = Espaçamento (5 pixels)
- `textWidth` / `textHeight` = Dimensões calculadas do texto

---

## 🎯 Casos de Uso

### **1. Minúcias Próximas - Evitar Sobreposição**

```
Problema: Rótulos se sobrepõem
Solução: Alternar posições

    2 ↑         ↑ 3
      ●    →    ●
    1 ↓         ↓ 4
```

### **2. Bordas da Imagem**

```
Borda direita: Use LEFT
Borda esquerda: Use RIGHT
Borda superior: Use BELOW
Borda inferior: Use ABOVE
```

### **3. Exportação Limpa**

```
Para exportar sem rótulos:
→ Ocultar todos: HIDDEN
```

### **4. Apresentações**

```
Organizar rótulos para melhor visualização:
- Core: ABOVE
- Deltas: BELOW
- Extremidades: Alternar LEFT/RIGHT
```

---

## 💾 Persistência

### Salvar

O campo `labelPosition` é salvo automaticamente com o projeto:

```cpp
// Ao salvar projeto
for (auto& minutia : fragment.minutiae) {
    // labelPosition é serializado junto
}
```

### Carregar

Ao abrir projeto, a posição é restaurada:

```cpp
// Ao carregar
minutia.labelPosition = saved_position;
```

---

## 🔄 Integração com Sistema Existente

### **Arquivos Modificados**

| Arquivo | Mudanças |
|---------|----------|
| **ProjectModel.h** | + enum MinutiaLabelPosition<br>+ campo labelPosition na classe Minutia |
| **MinutiaeDisplayDialog.h/cpp** | + combo labelPositionCombo<br>+ campo defaultLabelPosition<br>+ slot onLabelPositionChanged |
| **MinutiaEditDialog.h/cpp** | + combo labelPositionComboBox<br>+ getter getLabelPosition()<br>+ carregar/salvar posição |
| **MinutiaeOverlay.h/cpp** | + parâmetro labelPos em drawMinutiaLabel<br>+ submenu em contextMenuEvent<br>+ lógica de posicionamento |
| **MainWindow.cpp** | + pergunta para aplicar globalmente |

### **Compatibilidade**

- ✅ **Projetos antigos**: Padrão = RIGHT (compatível)
- ✅ **Projetos novos**: Escolha livre de posição
- ✅ **Migração**: Automática (campo tem valor padrão)

---

## 🎨 Menu de Contexto Completo

```
┌────────────────────────────────────────┐
│  ↔️ Mover Minúcia                     │ ← Modo de edição
│  🔄 Rotacionar Minúcia                │
├────────────────────────────────────────┤
│  📍 Posição do Rótulo              ▶  │ ← NOVO!
│     ├─ → À Direita             ☑      │
│     ├─ ← À Esquerda                   │
│     ├─ ↑ Acima                        │
│     ├─ ↓ Abaixo                       │
│     └─ ⊗ Oculto                       │
├────────────────────────────────────────┤
│  ✏️ Editar Propriedades...            │
│  🗑 Excluir Minúcia                   │
└────────────────────────────────────────┘
```

---

## 📊 Janela de Edição Atualizada

```
┌─────────────────────────────────────────┐
│  Editar Minúcia                         │
├─────────────────────────────────────────┤
│  📍 Posição                             │
│     X: [___] px                         │
│     Y: [___] px                         │
│                                         │
│  🏷 Classificação                       │
│     Tipo: [Bifurcation ▼]              │
│     Ângulo: [___]°                      │
│     Qualidade: [___]                    │
│                                         │
│  🎨 Aparência              ← NOVO!      │
│     Posição do Rótulo:                  │
│     [À Direita (Padrão) ▼]             │
│                                         │
│  📝 Observações                         │
│     [____________________]              │
│                                         │
│              [Cancelar] [OK]            │
└─────────────────────────────────────────┘
```

---

## 🧪 Como Testar

### Teste 1: Edição Individual

```bash
1. Abrir fragmento com minúcias
2. Duplo clique em uma minúcia
3. Mudar "Posição do Rótulo" para "Acima"
4. OK
5. ✅ Verificar: Rótulo aparece acima da marcação
```

### Teste 2: Menu de Contexto

```bash
1. Abrir fragmento com minúcias
2. Ctrl+I (modo de edição)
3. Botão direito em minúcia
4. "Posição do Rótulo" → "À Esquerda"
5. ✅ Verificar: Rótulo aparece à esquerda
```

### Teste 3: Ocultar Rótulo

```bash
1. Botão direito em minúcia
2. "Posição do Rótulo" → "Oculto"
3. ✅ Verificar: Rótulo desaparece, só marcação visível
```

### Teste 4: Aplicação Global

```bash
1. Menu → Visualizar → Configurar Visualização
2. Mudar "Posição do Rótulo" para "Abaixo"
3. OK
4. Pergunta: "Aplicar a TODAS?"
5. Sim
6. ✅ Verificar: Todos os rótulos vão para baixo
```

### Teste 5: Persistência

```bash
1. Alterar posições de várias minúcias
2. Salvar projeto (Ctrl+S)
3. Fechar programa
4. Abrir projeto novamente
5. ✅ Verificar: Posições mantidas
```

---

## 📈 Estatísticas da Implementação

### Código Adicionado

- **~150 linhas** de código novo
- **5 arquivos** modificados
- **1 enum** novo (MinutiaLabelPosition)
- **1 campo** novo na classe Minutia
- **3 interfaces** atualizadas
- **1 algoritmo** de posicionamento

### Funcionalidades

- ✅ **5 posições** de rótulo
- ✅ **3 formas** de configurar
- ✅ **Configuração individual** por minúcia
- ✅ **Configuração global** opcional
- ✅ **Menu de contexto** intuitivo
- ✅ **Persistência** automática
- ✅ **Compatibilidade** com projetos antigos

---

## 🎉 Benefícios

### **1. Flexibilidade**
- Cada minúcia pode ter posição diferente
- Adapta-se a qualquer layout

### **2. Usabilidade**
- 3 formas diferentes de configurar
- Interface intuitiva
- Feedback visual imediato

### **3. Organização**
- Evita sobreposição de rótulos
- Melhor legibilidade
- Apresentações profissionais

### **4. Compatibilidade**
- Projetos antigos funcionam
- Migração automática
- Sem quebra de compatibilidade

---

## 🔮 Possíveis Melhorias Futuras

1. **Auto-posicionamento**: Detectar sobreposição e ajustar automaticamente
2. **Posições diagonais**: Superior-direita, inferior-esquerda, etc.
3. **Distância customizável**: Permitir ajustar margem
4. **Templates**: Salvar configurações de layout
5. **Exportação**: Opção "Ocultar todos" temporária para export

---

## 📝 Resumo

Implementação completa do sistema de posicionamento de rótulos de minúcias com:

- ✅ 5 posições (Direita, Esquerda, Acima, Abaixo, Oculto)
- ✅ Controle individual por minúcia
- ✅ Configuração global com aplicação em lote
- ✅ Interface em 3 locais (diálogo, menu contexto, configurações)
- ✅ Persistência automática
- ✅ Compatibilidade retroativa
- ✅ Compilação sem erros

**Status:** ✅ Pronto para uso!

---

**Implementado em:** 2025-10-12  
**Versão:** 1.0
