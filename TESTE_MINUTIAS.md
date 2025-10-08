# Guia de Teste - Sistema de Minúcias

## 📝 Fluxo Completo de Teste

### Passo 1: Criar Projeto
1. Abrir aplicação
2. Menu `Arquivo` → `Novo Projeto`
3. Informar nome do projeto: "TesteMinucias"
4. Salvar projeto: `Arquivo` → `Salvar Projeto`

✅ **Resultado esperado**: Projeto criado e salvo

---

### Passo 2: Adicionar Imagem
1. Menu `Arquivo` → `Adicionar Imagem ao Projeto`
2. Selecionar uma imagem de impressão digital (PNG, JPG, etc.)

✅ **Resultado esperado**:
- Imagem aparece no painel direito (aba "Projeto")
- Imagem é exibida na área central

---

### Passo 3: Selecionar Imagem como Corrente
1. No painel direito (aba "Projeto"), **clicar UMA VEZ** na imagem adicionada

✅ **Resultado esperado**:
- Barra de status mostra: `🖼️ Imagem Corrente: nome_do_arquivo.png`
- Imagem aparece na visualização central

---

### Passo 4: Criar Fragmento (Recorte)
1. Menu `Ferramentas` → `Recorte de Imagem` → `Ativar Ferramenta de Recorte` (ou `Ctrl+Shift+R`)
2. **Clicar e arrastar** na imagem para desenhar um retângulo de seleção
3. **Clicar com botão DIREITO** dentro da seleção
4. Escolher `✓ Aplicar Recorte`

✅ **Resultado esperado**:
- Fragmento criado aparece na árvore do projeto, sob a imagem pai
- Visualização mostra o fragmento recortado
- Barra de status mostra: `📐 Fragmento Corrente: 800x600 pixels (0 minúcia(s))`

**Problema conhecido**: Se não funcionar na primeira imagem, adicione outra imagem e tente novamente.

---

### Passo 5: Garantir que Fragmento está Selecionado
1. No painel direito (aba "Projeto"), verificar se o fragmento está selecionado (destacado)
2. Se não estiver, **clicar UMA VEZ** no fragmento

✅ **Resultado esperado**:
- Barra de status continua mostrando: `📐 Fragmento Corrente: ...`
- `currentEntityType` internamente é `ENTITY_FRAGMENT`

---

### Passo 6: Ativar Modo de Adicionar Minúcia
**Opção A - Via Menu:**
1. Menu `Ferramentas` → `Minúcias` → `Adicionar Minúcia Manual` (ou `Ctrl+M`)

**Opção B - Via Botão Direito Direto:**
1. Pular para o Passo 7

✅ **Resultado esperado** (Opção A):
- Barra de status mostra: `✚ Modo: Adicionar Minúcia ATIVO. Clique com BOTÃO DIREITO...`
- `currentToolMode` internamente é `TOOL_ADD_MINUTIA`

---

### Passo 7: Adicionar Minúcia
1. **Clicar com BOTÃO DIREITO** na posição desejada na imagem do fragmento
2. No menu de contexto, escolher `➕ Adicionar Minúcia Aqui`

✅ **Resultado esperado**:
- Diálogo `MinutiaEditDialog` abre
- Diálogo mostra:
  - Posição (X, Y)
  - ComboBox com **56 tipos de minúcias**
  - Campo de ângulo
  - Campo de qualidade
  - Campo de notas

---

### Passo 8: Configurar Minúcia
1. No diálogo, escolher tipo (ex: "Ridge Ending B")
2. Ajustar ângulo se necessário
3. Ajustar qualidade (0.0 a 1.0)
4. Adicionar notas se desejar
5. Clicar `OK`

✅ **Resultado esperado**:
- Minúcia é adicionada ao fragmento
- Barra de status mostra: `Minúcia Ridge Ending B adicionada em (123, 456)`
- **Minúcia aparece visível no overlay sobre o fragmento** (círculo + linha de direção)
- **Minúcia aparece na lista do painel "Projeto"** sob o fragmento

---

### Passo 9: Verificar Minúcia na Lista
1. No painel direito (aba "Projeto"), expandir o fragmento
2. Verificar se a minúcia aparece listada

✅ **Resultado esperado**:
- Minúcia listada com tipo e posição
- Exemplo: `Minúcia #1 - Ridge Ending B at (123, 456)`

---

### Passo 10: Adicionar Mais Minúcias
1. Repetir passos 7-8 para adicionar mais minúcias
2. Cada minúcia pode ter tipo diferente dos 56 disponíveis

✅ **Resultado esperado**:
- Múltiplas minúcias visíveis no overlay
- Todas listadas no painel Projeto
- Contador na barra de status aumenta: `📐 Fragmento Corrente: 800x600 pixels (3 minúcia(s))`

---

### Passo 11: Testar Transformações (Rotação/Espelhamento)
1. Com fragmento selecionado, aplicar rotação: `Ferramentas` → `Rotação` → `Rotacionar 90° à Direita`
2. Verificar se minúcias acompanham a rotação

✅ **Resultado esperado**:
- Fragmento rotaciona
- **Minúcias rotacionam junto** (overlay sincronizado)
- Posições das minúcias são transformadas corretamente

---

### Passo 12: Salvar e Recarregar Projeto
1. Salvar projeto: `Arquivo` → `Salvar Projeto`
2. Fechar aplicação
3. Abrir aplicação novamente
4. Menu `Arquivo` → `Abrir Projeto`
5. Selecionar o projeto salvo

✅ **Resultado esperado**:
- Projeto carrega completamente
- Imagens, fragmentos E minúcias são restaurados
- Minúcias aparecem no overlay
- Minúcias listadas no painel Projeto

---

## 🐛 Problemas Conhecidos e Soluções

### Problema 1: Menu de contexto não aparece
**Causa**: Fragmento não está selecionado como corrente

**Solução**:
1. Clicar no fragmento no painel Projeto
2. Verificar barra de status: deve mostrar `📐 Fragmento Corrente`
3. Tentar botão direito novamente

---

### Problema 2: Recorte não funciona na primeira imagem
**Workaround temporário**:
1. Adicionar uma segunda imagem
2. Tentar recorte novamente
3. Deve funcionar em todas as imagens após isso

---

### Problema 3: Minúcia não aparece visível
**Possíveis causas**:
1. Overlay não foi atualizado
2. Fragmento não está selecionado

**Solução**:
1. Clicar no fragmento novamente no painel Projeto
2. Verificar se método `minutiaeOverlay->update()` foi chamado
3. Verificar console para erros

---

## 📊 Checklist de Funcionalidades

- [ ] 1. Criar projeto
- [ ] 2. Adicionar imagem
- [ ] 3. Selecionar imagem (torna corrente)
- [ ] 4. Criar fragmento (recorte)
- [ ] 5. Fragmento aparece na árvore
- [ ] 6. Selecionar fragmento (torna corrente)
- [ ] 7. Ativar modo adicionar minúcia (Ctrl+M)
- [ ] 8. Menu contexto mostra "Adicionar Minúcia Aqui"
- [ ] 9. Diálogo abre com 56 tipos
- [ ] 10. Minúcia é adicionada
- [ ] 11. Minúcia aparece no overlay (visível)
- [ ] 12. Minúcia aparece na lista do painel
- [ ] 13. Adicionar múltiplas minúcias funciona
- [ ] 14. Rotação sincroniza minúcias
- [ ] 15. Salvar projeto preserva minúcias
- [ ] 16. Carregar projeto restaura minúcias

---

## 📝 Notas de Implementação

### Arquitetura do Sistema de Minúcias

```
MainWindow
├── currentEntityType (ENTITY_FRAGMENT quando fragmento selecionado)
├── currentEntityId (ID do fragmento)
├── currentToolMode (TOOL_ADD_MINUTIA quando ativo)
│
├── showContextMenu()
│   ├── Verifica currentEntityType
│   └── Mostra "Adicionar Minúcia Aqui" se ENTITY_FRAGMENT
│
├── addMinutiaAtPosition()
│   ├── Cria Minutia temporária
│   ├── Abre MinutiaEditDialog
│   ├── Adiciona ao ProjectManager
│   ├── Atualiza minutiaeOverlay
│   └── Atualiza fragmentManager
│
└── MinutiaeOverlay
    ├── setFragment() - vincula ao fragmento
    ├── Desenha minúcias sobre a imagem
    └── Sincroniza com transformações
```

### Fluxo de Dados

```
Usuário clica direito → showContextMenu()
                            ↓
                     "Adicionar Minúcia Aqui"
                            ↓
                  addMinutiaAtPosition()
                            ↓
                   MinutiaEditDialog (56 tipos)
                            ↓
              ProjectManager::addMinutiaToFragment()
                            ↓
         Fragment.minutiae.append(nova minúcia)
                            ↓
      minutiaeOverlay->setFragment(fragment)
                            ↓
           minutiaeOverlay->update()
                            ↓
        Minúcia visível no overlay!
```

---

**Versão**: FingerprintEnhancer v2.0.0
**Data**: 2025-10-08
**Status**: Sistema de minúcias implementado e em teste
