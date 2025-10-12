# 🎯 Guia Rápido - Edição Interativa de Minúcias

## 🚀 Como Usar

### 1. Ativar Modo de Edição Interativa

**Opção 1 - Atalho de Teclado:**
```
Ctrl+I
```

**Opção 2 - Menu:**
```
Ferramentas → Minúcias → 🎯 Modo de Edição Interativa
```

### 2. Selecionar Minúcia

Existem **duas formas** de selecionar uma minúcia:

**Forma 1 - Clique na Imagem:**
```
Clique esquerdo em uma minúcia → SELECIONADA (Verde)
```

**Forma 2 - Árvore de Projeto:**
```
Selecione a minúcia na árvore → SELECIONADA (Verde)
```

### 3. Ativar Modo de Edição

Com a minúcia selecionada, **clique com botão direito** e escolha:

```
┌────────────────────────────────┐
│  ↔️ Mover Minúcia             │ → Modo MOVER (Azul)
│  🔄 Rotacionar Minúcia        │ → Modo ROTACIONAR (Laranja)
├────────────────────────────────┤
│  ✏️ Editar Propriedades...    │
│  🗑 Excluir Minúcia           │
└────────────────────────────────┘
```

### 4. Editar

**No Modo MOVER (Azul):**
- **Arraste** com botão esquerdo pressionado = Move a posição
- **Clique** esquerdo simples = Alterna para modo ROTACIONAR

**No Modo ROTACIONAR (Laranja):**
- **Arraste** com botão esquerdo pressionado = Gira o ângulo
- **Clique** esquerdo simples = Alterna para modo MOVER

## 📐 Estados e Cores

| Estado | Cor | Ação | Descrição |
|--------|-----|------|-----------|
| **IDLE** | - | - | Nenhuma minúcia selecionada |
| **SELECTED** | 🟢 Verde | Clique | Minúcia selecionada, pronta para editar |
| **EDITING_POSITION** | 🔵 Azul | Arraste | Movendo posição da minúcia |
| **EDITING_ANGLE** | 🟠 Laranja | Arraste | Rotacionando ângulo da minúcia |

## 🖱️ Controles do Mouse

### Mover Posição
1. Clique na minúcia 2 vezes para entrar no modo MOVER
2. **Mantenha o botão esquerdo pressionado**
3. **Arraste** o mouse para mover a minúcia
4. Solte o botão para fixar a posição

### Rotacionar Ângulo
1. Clique na minúcia 3 vezes para entrar no modo ROTACIONAR
2. **Mantenha o botão esquerdo pressionado**
3. **Arraste** o mouse em círculo ao redor da minúcia
4. A seta gira acompanhando o movimento
5. Solte o botão para fixar o ângulo

## 💡 Dicas

✅ **Indicador visual**: Sempre mostra o que fazer em cada estado
✅ **Status bar**: Mostra o ângulo atual ao rotacionar
✅ **Desselecionar**: Clique fora da minúcia para desmarcar
✅ **Cancelar**: Pressione `Ctrl+I` novamente para sair do modo
✅ **Auto-save**: Mudanças são salvas automaticamente no projeto

## 🎨 Visualização

### Minúcia no Modo de Edição
```
    ┌─────────────────────────────────┐
    │ 🔄 ROTACIONAR - Arraste...     │ ← Indicador de estado
    └─────────────────────────────────┘
              ┊
              ┊ (linha pontilhada)
              ↓
              ↑ ← Seta de ângulo
             ⭕ ← Círculo da minúcia
            ⃝  ← Círculo tracejado (selecionada)
```

## ⌨️ Atalhos Úteis

| Atalho | Função |
|--------|--------|
| `Ctrl+I` | Ativar/Desativar modo de edição |
| `Ctrl+M` | Adicionar minúcia manual |
| `Ctrl+D` | Duplicar fragmento |
| `Ctrl+E` | Exportar fragmento |
| `Delete` | Excluir item selecionado |

## 🔧 Workflow Recomendado

1. **Abra um projeto** com fragmentos e minúcias
2. **Selecione um fragmento** na árvore
3. **Ative o modo de edição** (Ctrl+I)
4. **Selecione uma minúcia**:
   - Clique nela na imagem OU
   - Selecione na árvore
5. **Clique com botão direito** → "↔️ Mover Minúcia"
6. **Arraste com botão esquerdo** para ajustar a posição
7. **Clique (sem arrastar)** para alternar para modo rotacionar
8. **Arraste com botão esquerdo** para ajustar o ângulo
9. **Clique fora** para desselecionar
10. **Salve o projeto** (Ctrl+S)

## ⚠️ Importante

- ⚡ O modo de edição funciona **apenas em fragmentos**
- ⚡ É necessário ter **minúcias já criadas**
- ⚡ Funciona em **ambos os painéis** (esquerdo e direito)
- ⚡ Mudanças são **em tempo real**
- ⚡ O projeto é marcado como **modificado** automaticamente

## 🎯 Exemplo Prático

```
Passo a Passo:
1. Abrir projeto → testproject.fpproj
2. Selecionar fragmento na árvore
3. Ctrl+I (ativar modo de edição)
4. Clicar na minúcia na imagem → Verde (SELECIONADA)
5. Botão direito → "Mover Minúcia" → Azul (MOVER)
6. Arrastar com botão esquerdo → Minúcia move
7. Clique simples (sem arrastar) → Laranja (ROTACIONAR)
8. Arrastar com botão esquerdo → Seta gira
9. Clique simples → Volta para Azul (MOVER)
10. Ctrl+S (salvar)
```

## 🎮 Controles Detalhados

| Ação | Botão | Efeito |
|------|-------|--------|
| Selecionar minúcia | Esquerdo (clique) | Marca minúcia em verde |
| Menu de opções | Direito | Mostra menu com "Mover" e "Rotacionar" |
| Mover posição | Esquerdo (arrastar) | Move minúcia (modo azul) |
| Rotacionar ângulo | Esquerdo (arrastar) | Gira minúcia (modo laranja) |
| Alternar modo | Esquerdo (clique) | Mover ↔ Rotacionar |
| Desselecionar | Clique fora | Limpa seleção |

## 📊 Mensagens da Status Bar

| Mensagem | Significado |
|----------|-------------|
| "Modo de edição interativa ATIVADO" | Modo ativado com sucesso |
| "Modo de edição interativa DESATIVADO" | Modo desativado |
| "Ângulo da minúcia: 45°" | Ângulo atualizado em tempo real |

---

**Desenvolvido com** ❤️ **para análise papiloscópica profissional**
