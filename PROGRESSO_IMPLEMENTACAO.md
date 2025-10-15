# Progresso de Implementação - Funcionalidades Solicitadas

## ✅ 1. Fechar Projeto ao Criar Novo (CONCLUÍDO)

### Implementado:
- ✅ `newProject()`: Verifica se há projeto aberto antes de criar novo
- ✅ Pergunta se deseja salvar projeto modificado
- ✅ Botões: Sim | Não | Cancelar
- ✅ Fecha projeto anterior antes de criar novo
- ✅ `openProject()`: Mesma lógica ao abrir projeto existente

### Arquivos modificados:
- `src/gui/MainWindow.cpp` (funções `newProject()` e `openProject()`)

---

## 🚧 2. Ferramenta de Recorte Interativa (EM PROGRESSO)

### Necessário implementar:

#### 2.1 Losangos nas bordas do retângulo
```cpp
// CropTool.h
enum EdgeHandle { 
    EDGE_TOP, 
    EDGE_BOTTOM, 
    EDGE_LEFT, 
    EDGE_RIGHT 
};

void drawEdgeHandles(QPainter &painter);
EdgeHandle getEdgeHandleAt(QPoint pos);
bool isPointInEdgeHandle(QPoint pos, EdgeHandle handle);
```

#### 2.2 Movimentação da seleção
- Clicar e arrastar fora dos losangos = mover toda seleção
- Menu botão direito: "Mover Seleção" (modo só com mouse)
- ESC cancela modo "Mover Seleção"

```cpp
// CropTool.h - adicionar:
State::MovingWithMouseOnly;  // Modo movimento só com mouse
bool movingWithMouseMode;
void setMovingWithMouseMode(bool enable);
```

#### 2.3 Menu de contexto
```cpp
// MainWindow.cpp ou ImageViewer.cpp
void showCropContextMenu(QPoint pos) {
    QMenu menu;
    menu.addAction("Mover Seleção", [this]() {
        cropTool->setMovingWithMouseMode(true);
    });
    menu.exec(pos);
}
```

---

## 🚧 3. Armazenar Posição Original do Fragmento (PENDENTE)

### Campos a adicionar em ProjectModel.h:

```cpp
struct Fragment {
    // ... campos existentes ...
    
    // NOVOS CAMPOS:
    QRect originalCropRect;      // Posição (x,y,w,h) na imagem original
    double originalRotation;     // Ângulo quando destacado (0-360)
    QString sourceImageId;       // ID da imagem de origem
    cv::Size originalImageSize;  // Tamanho da imagem original
};
```

### Modificar função de destacar fragmento:

```cpp
// MainWindow::saveCroppedImage() - ao criar fragmento:
fragment->originalCropRect = cropTool->getSelectionRect();
fragment->originalRotation = getCurrentImageRotation();
fragment->sourceImageId = currentEntityId;
fragment->originalImageSize = getCurrentImage().size();
```

---

## 🚧 4. Indicar Fragmentos na Imagem Original (PENDENTE)

### Menu no FragmentManager:
- Botão direito na imagem → "Mostrar Fragmentos Destacados"

### Overlay de Fragmentos:
```cpp
// Criar novo arquivo: FragmentIndicatorOverlay.h/cpp
class FragmentIndicatorOverlay : public QWidget {
    void paintEvent(QPaintEvent *event) override;
    void drawFragmentRect(QPainter &painter, Fragment *frag);
    void drawFragmentLabel(QPainter &painter, QRect rect, int number);
};
```

### Desenho:
- Retângulo semi-transparente na posição original
- Rótulo com número do fragmento (estilo minúcia)
- Cor diferente para cada fragmento

---

## 📊 Status Geral

| Funcionalidade | Status | Progresso |
|----------------|--------|-----------|
| Fechar projeto ao criar novo | ✅ Completo | 100% |
| Losangos nas bordas do crop | 🚧 Pendente | 0% |
| Movimentação interativa | 🚧 Pendente | 0% |
| Menu "Mover Seleção" | 🚧 Pendente | 0% |
| Campos Fragment (posição original) | 🚧 Pendente | 0% |
| Salvar posição ao destacar | 🚧 Pendente | 0% |
| Menu "Mostrar Fragmentos" | 🚧 Pendente | 0% |
| Overlay de indicação | 🚧 Pendente | 0% |

---

## ⏭️ Próximos Passos

1. Adicionar campos ao Fragment (ProjectModel.h)
2. Modificar CropTool para desenhar losangos
3. Implementar detecção de clique nos losangos
4. Implementar movimentação da seleção
5. Adicionar menu de contexto
6. Criar FragmentIndicatorOverlay
7. Integrar tudo no MainWindow

---

## 🔧 Como Continuar

Para implementar as funcionalidades restantes, serão necessárias modificações em:

1. **src/core/ProjectModel.h** - Adicionar campos ao Fragment
2. **src/gui/CropTool.h/.cpp** - Losangos e movimentação
3. **src/gui/FragmentIndicatorOverlay.h/.cpp** - Novo arquivo para overlay
4. **src/gui/MainWindow.cpp** - Integrar funcionalidades
5. **src/gui/FragmentManager.cpp** - Menu de contexto

**Estimativa:** 4-6 horas de desenvolvimento para implementar completamente todas as funcionalidades.
