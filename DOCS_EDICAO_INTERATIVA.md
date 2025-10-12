# 📚 Documentação Técnica - Sistema de Edição Interativa de Minúcias

## 🏗️ Arquitetura

### Visão Geral

O sistema de edição interativa permite manipular minúcias diretamente na imagem através de drag and drop, com três modos distintos de operação.

```
┌─────────────────────────────────────────────────┐
│           MinutiaeOverlay (Widget Qt)           │
│  ┌───────────────────────────────────────────┐  │
│  │  Estado: MinutiaEditState                 │  │
│  │  - IDLE                                   │  │
│  │  - SELECTED                               │  │
│  │  - EDITING_POSITION                       │  │
│  │  - EDITING_ANGLE                          │  │
│  └───────────────────────────────────────────┘  │
│                                                  │
│  ┌───────────────────────────────────────────┐  │
│  │  Event Handlers                           │  │
│  │  - mousePressEvent()                      │  │
│  │  - mouseMoveEvent()                       │  │
│  │  - mouseReleaseEvent()                    │  │
│  │  - paintEvent()                           │  │
│  └───────────────────────────────────────────┘  │
│                                                  │
│  ┌───────────────────────────────────────────┐  │
│  │  Signals Emitidos                         │  │
│  │  - positionChanged(id, QPoint)            │  │
│  │  - angleChanged(id, float)                │  │
│  │  - editStateChanged(state)                │  │
│  └───────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
                       ↓
┌─────────────────────────────────────────────────┐
│              MainWindow (Receptor)              │
│  ┌───────────────────────────────────────────┐  │
│  │  Slots Conectados                         │  │
│  │  - onMinutiaPositionChanged()             │  │
│  │  - onMinutiaAngleChanged()                │  │
│  └───────────────────────────────────────────┘  │
│                                                  │
│  ┌───────────────────────────────────────────┐  │
│  │  Atualiza Modelo de Dados                │  │
│  │  - ProjectManager::updateMinutiaPosition()│  │
│  │  - Minutia::angle = newAngle              │  │
│  │  - Project::setModified()                 │  │
│  └───────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
```

## 📋 Enumerações

### MinutiaEditState

```cpp
enum class MinutiaEditState {
    IDLE,              // Nenhuma minúcia selecionada
    SELECTED,          // Minúcia selecionada (pronta para edição)
    EDITING_POSITION,  // Arrastando para mover posição
    EDITING_ANGLE      // Arrastando para rotacionar ângulo
};
```

**Transições de Estado:**
```
IDLE → SELECTED (clique em minúcia)
SELECTED → EDITING_POSITION (clique na mesma minúcia)
EDITING_POSITION → EDITING_ANGLE (clique na mesma minúcia)
EDITING_ANGLE → SELECTED (clique na mesma minúcia)
SELECTED → IDLE (clique fora ou clearSelection())
```

## 🔧 Classes Modificadas

### MinutiaeOverlay

**Novos Atributos:**
```cpp
private:
    MinutiaEditState editState;  // Estado atual de edição
    bool isDragging;              // Flag de arrasto ativo
    QPoint dragStartPos;          // Posição inicial do drag
    QPoint lastDragPos;           // Última posição do drag
    float initialAngle;           // Ângulo inicial ao começar rotação
```

**Novos Métodos Públicos:**
```cpp
void setEditMode(bool enabled);
MinutiaEditState getEditState() const;
```

**Novos Sinais:**
```cpp
signals:
    void angleChanged(const QString& minutiaId, float newAngle);
    void editStateChanged(MinutiaEditState newState);
```

**Novos Métodos Privados:**
```cpp
void drawMinutiaWithArrow(QPainter& painter, const QPoint& pos, 
                         float angle, const QColor& color, bool isSelected);
void drawEditStateIndicator(QPainter& painter, const QPoint& pos);
float calculateAngleFromDrag(const QPoint& center, const QPoint& dragPos) const;
```

### MainWindow

**Novo Slot:**
```cpp
void onMinutiaAngleChanged(const QString& minutiaId, float newAngle);
```

**Nova Ação de Menu:**
```cpp
QAction* editModeAction = minutiaeMenu->addAction("🎯 Modo de Edição Interativa");
editModeAction->setCheckable(true);
editModeAction->setShortcut(QKeySequence("Ctrl+I"));
```

## 🎨 Sistema de Renderização

### drawMinutiaWithArrow()

Desenha a minúcia no modo de edição com círculo e seta.

**Parâmetros:**
- `painter`: QPainter para desenhar
- `pos`: Posição na tela (já escalada)
- `angle`: Ângulo da minúcia (0-360°)
- `color`: Cor do marcador
- `isSelected`: Se está selecionada

**Implementação:**
```cpp
1. Desenha círculo ao redor da posição
2. Calcula posição final da seta usando trigonometria
3. Desenha linha da seta
4. Desenha pontas da seta (2 linhas)
5. Se selecionada, adiciona círculo tracejado externo
```

**Fórmulas Trigonométricas:**
```cpp
radians = angle * π / 180.0
endX = centerX + arrowLength * cos(radians)
endY = centerY - arrowLength * sin(radians)  // Y invertido
```

### drawEditStateIndicator()

Desenha indicador de instrução acima da minúcia.

**Estados e Cores:**
- SELECTED: Verde (0, 255, 0)
- EDITING_POSITION: Azul claro (0, 200, 255)
- EDITING_ANGLE: Laranja (255, 165, 0)

**Layout:**
```
┌────────────────────────────────┐
│ Texto de instrução             │ ← Fundo colorido semi-transparente
└────────────────────────────────┘
              ┊
              ┊ ← Linha tracejada
              ↓
             ⭕ ← Minúcia
```

## 🧮 Cálculo de Ângulo

### calculateAngleFromDrag()

Calcula o ângulo entre o centro da minúcia e a posição do mouse.

**Algoritmo:**
```cpp
1. Calcular diferenças de coordenadas
   dx = dragX - centerX
   dy = centerY - dragY  // Invertido pois Y cresce para baixo
   
2. Calcular ângulo em radianos
   radians = atan2(dy, dx)  // Retorna -π a π
   
3. Converter para graus
   degrees = radians * 180.0 / π
   
4. Normalizar para 0-360
   if (degrees < 0)
       degrees += 360.0
```

**Exemplo:**
```
Mouse à direita: atan2(0, 1) = 0° 
Mouse acima: atan2(1, 0) = 90°
Mouse à esquerda: atan2(0, -1) = 180°
Mouse abaixo: atan2(-1, 0) = 270°
```

## 🖱️ Tratamento de Eventos

### mousePressEvent()

**Fluxo:**
```cpp
1. Verificar se está no modo de edição
2. Encontrar minúcia na posição do clique
3. Se encontrou minúcia:
   a. Se é a mesma já selecionada:
      - Avançar ciclo de estados
   b. Se é outra minúcia:
      - Selecionar nova minúcia
      - Estado = SELECTED
4. Se não encontrou:
   - Desselecionar (clearSelection)
5. Emitir sinais apropriados
```

### mouseMoveEvent()

**Fluxo:**
```cpp
1. Verificar se está arrastando e no modo de edição
2. Se no estado EDITING_POSITION:
   a. Descalar posição do mouse
   b. Emitir positionChanged
3. Se no estado EDITING_ANGLE:
   a. Encontrar minúcia selecionada
   b. Calcular novo ângulo
   c. Emitir angleChanged
4. Atualizar overlay
```

### mouseReleaseEvent()

```cpp
1. Marcar isDragging = false
2. Manter estado atual (não muda)
```

## 📊 Fluxo de Dados

### Atualização de Posição

```
1. MinutiaeOverlay::mouseMoveEvent()
   ↓ emite
2. Signal: positionChanged(minutiaId, newPos)
   ↓ conectado a
3. MainWindow::onMinutiaPositionChanged()
   ↓ chama
4. ProjectManager::updateMinutiaPosition()
   ↓ atualiza
5. Minutia::position = newPos
   ↓ marca
6. Project::setModified()
```

### Atualização de Ângulo

```
1. MinutiaeOverlay::mouseMoveEvent()
   ↓ calcula novo ângulo
2. MinutiaeOverlay::calculateAngleFromDrag()
   ↓ emite
3. Signal: angleChanged(minutiaId, newAngle)
   ↓ conectado a
4. MainWindow::onMinutiaAngleChanged()
   ↓ atualiza
5. Minutia::angle = newAngle
   ↓ marca
6. Project::setModified()
   ↓ atualiza
7. MinutiaeOverlay::update() (ambos painéis)
```

## 🎯 Coordenadas e Escalas

### Sistema de Coordenadas

**Três sistemas coexistem:**

1. **Coordenadas da Imagem** (armazenadas no modelo)
   - Pixels originais da imagem
   - Invariantes ao zoom

2. **Coordenadas da Tela** (widget)
   - Posição na janela
   - Afetadas por zoom e scroll

3. **Coordenadas Escaladas** (para desenho)
   - Coordenadas da imagem × scaleFactor
   - + imageOffset (centralização)
   - - scrollOffset (barra de rolagem)

### Funções de Conversão

**scalePoint()**: Imagem → Tela
```cpp
screenX = imageX * scaleFactor + imageOffset.x - scrollOffset.x
screenY = imageY * scaleFactor + imageOffset.y - scrollOffset.y
```

**unscalePoint()**: Tela → Imagem
```cpp
imageX = (screenX + scrollOffset.x - imageOffset.x) / scaleFactor
imageY = (screenY + scrollOffset.y - imageOffset.y) / scaleFactor
```

## 🔗 Conexões de Sinais

### Em MainWindow::setupConnections()

```cpp
// Painel Esquerdo
connect(leftMinutiaeOverlay, &MinutiaeOverlay::positionChanged,
        this, &MainWindow::onMinutiaPositionChanged);
connect(leftMinutiaeOverlay, &MinutiaeOverlay::angleChanged,
        this, &MainWindow::onMinutiaAngleChanged);

// Painel Direito
connect(rightMinutiaeOverlay, &MinutiaeOverlay::positionChanged,
        this, &MainWindow::onMinutiaPositionChanged);
connect(rightMinutiaeOverlay, &MinutiaeOverlay::angleChanged,
        this, &MainWindow::onMinutiaAngleChanged);
```

## 🎨 Cores e Estilos

### Paleta de Cores

```cpp
QColor normalColor(255, 0, 0);      // Vermelho - Normal
QColor selectedColor(0, 255, 0);    // Verde - Selecionada
QColor positionColor(0, 200, 255);  // Azul claro - Movendo
QColor angleColor(255, 165, 0);     // Laranja - Rotacionando
```

### Espessura de Linhas

```cpp
int normalPenWidth = 2;     // Minúcia normal
int selectedPenWidth = 3;   // Minúcia selecionada
```

## 🧪 Casos de Teste

### Teste 1: Seleção
```
1. Modo de edição OFF → Clique em minúcia → Nada acontece ✓
2. Modo de edição ON → Clique em minúcia → Fica verde ✓
3. Clique em outra minúcia → Primeira desmarca, segunda marca ✓
4. Clique fora → Desmarca ✓
```

### Teste 2: Movimentação
```
1. Selecionar minúcia → Clique novamente → Fica azul ✓
2. Arrastar mouse → Minúcia acompanha ✓
3. Soltar botão → Posição fixada ✓
4. Verificar no modelo → Position atualizado ✓
```

### Teste 3: Rotação
```
1. Selecionar → Clique 2× → Fica laranja ✓
2. Arrastar em círculo → Seta gira ✓
3. Soltar botão → Ângulo fixado ✓
4. Verificar no modelo → Angle atualizado ✓
5. Status bar → Mostra ângulo ✓
```

### Teste 4: Ciclo
```
1. Clique 1 → SELECTED (verde) ✓
2. Clique 2 → EDITING_POSITION (azul) ✓
3. Clique 3 → EDITING_ANGLE (laranja) ✓
4. Clique 4 → SELECTED (verde novamente) ✓
```

## 📈 Performance

### Otimizações Implementadas

1. **Update Seletivo**: Apenas redesenha quando necessário
2. **Cálculo Eficiente**: atan2 é nativo do hardware
3. **Sem Alocações**: Reutiliza objetos QPainter
4. **Early Return**: Valida condições antes de processar

### Complexidade

- `mouseMoveEvent()`: O(n) onde n = número de minúcias
- `findMinutiaAt()`: O(n) busca linear
- `calculateAngleFromDrag()`: O(1) cálculo trigonométrico
- `paintEvent()`: O(n) desenha todas as minúcias

## 🐛 Problemas Conhecidos e Soluções

### Problema 1: Coordenadas Incorretas
**Causa**: Não considerar zoom/scroll
**Solução**: Usar scalePoint() e unscalePoint()

### Problema 2: Ângulo Invertido
**Causa**: Sistema de coordenadas Y invertido
**Solução**: `dy = centerY - dragY` (inverter Y)

### Problema 3: Estado Perdido ao Desativar
**Causa**: Não limpar estado ao sair do modo
**Solução**: `setEditMode(false)` chama `clearSelection()`

## 📝 Manutenção

### Adicionar Novo Estado

1. Adicionar no enum `MinutiaEditState`
2. Adicionar case em `mousePressEvent()` para transição
3. Adicionar case em `drawEditStateIndicator()` para visual
4. Adicionar case em `mouseMoveEvent()` para ação
5. Documentar comportamento

### Adicionar Nova Ação

1. Criar novo sinal em MinutiaeOverlay.h
2. Emitir sinal no local apropriado
3. Criar slot em MainWindow.h
4. Implementar slot em MainWindow.cpp
5. Conectar sinal→slot em setupConnections()

## 🔐 Segurança

- ✅ Validação de ponteiros (nullptr checks)
- ✅ Verificação de bounds (findMinutiaAt com raio)
- ✅ Estado consistente (clearSelection ao desabilitar)
- ✅ Sem memory leaks (Qt parent ownership)

## 📚 Referências

- [Qt Event System](https://doc.qt.io/qt-6/eventsandfilters.html)
- [QPainter API](https://doc.qt.io/qt-6/qpainter.html)
- [Signal/Slot Mechanism](https://doc.qt.io/qt-6/signalsandslots.html)
- [Mouse Events](https://doc.qt.io/qt-6/qmouseevent.html)

---

**Versão**: 1.0
**Autor**: Sistema de IA
**Data**: 2025-10-12
