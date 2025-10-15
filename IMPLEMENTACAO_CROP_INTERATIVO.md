# Plano de Implementação - Recorte Interativo e Funcionalidades

## 1. Ferramenta de Recorte Interativa

### 1.1 Losangos nas Bordas
- [ ] Adicionar método `drawEdgeHandles()` em CropTool
- [ ] Desenhar losangos (diamantes) nos centros das 4 bordas
- [ ] Detectar clique nos losangos para redimensionar

### 1.2 Movimentação da Seleção
- [ ] Clicar e arrastar fora dos losangos = mover toda seleção
- [ ] Menu botão direito: "Mover Seleção" = modo movimento só com mouse
- [ ] ESC cancela modo "Mover Seleção"

### 1.3 Modificações no CropTool.h
```cpp
// Adicionar:
- State::MovingWithMouse  // Modo movimento apenas com mouse
- void drawEdgeHandles()  // Desenhar losangos nas bordas
- bool isPointInEdgeHandle(QPoint, EdgeHandle)  // Detectar clique em losango
- enum EdgeHandle { Top, Bottom, Left, Right }
```

## 2. Armazenar Posição Original do Fragmento

### 2.1 Campos no Fragment (ProjectModel.h)
```cpp
struct Fragment {
    // ... campos existentes ...
    
    // NOVOS CAMPOS:
    QRect originalCropRect;     // Posição na imagem original
    double originalRotation;    // Ângulo de rotação quando destacado
    QString sourceImageId;      // ID da imagem de onde veio
};
```

### 2.2 Salvar ao destacar fragmento
- Capturar rect do crop
- Capturar rotação atual da imagem (se houver)
- Salvar ID da imagem original

## 3. Indicar Fragmentos na Imagem Original

### 3.1 Menu no FragmentManager
- [ ] Botão direito na imagem → "Mostrar Fragmentos Destacados"
- [ ] Desenhar retângulos na posição original
- [ ] Desenhar rótulos (estilo minúcia) com número do fragmento

### 3.2 Overlay de Fragmentos
- [ ] Criar `FragmentIndicatorOverlay` (similar a MinutiaeOverlay)
- [ ] Desenhar retângulos semi-transparentes
- [ ] Rótulos com fundo colorido

## 4. Fechar Projeto ao Criar Novo

### 4.1 MainWindow::newProject()
```cpp
void MainWindow::newProject() {
    if (ProjectManager::instance().hasCurrentProject()) {
        if (ProjectManager::instance().getCurrentProject()->isModified()) {
            int result = QMessageBox::question(
                this, "Salvar Projeto?",
                "Deseja salvar o projeto atual antes de criar um novo?",
                QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel
            );
            
            if (result == QMessageBox::Cancel) return;
            if (result == QMessageBox::Yes) {
                if (!saveProject()) return;
            }
        }
        closeProject();  // Novo método
    }
    
    // ... criar novo projeto ...
}
```

## Ordem de Implementação

1. ✅ Adicionar campos ao Fragment (originalCropRect, originalRotation, sourceImageId)
2. ✅ Modificar CropTool para losangos nas bordas
3. ✅ Implementar movimentação interativa
4. ✅ Menu "Mover Seleção"
5. ✅ Salvar posição ao destacar fragmento
6. ✅ Menu "Mostrar Fragmentos Destacados"
7. ✅ Fechar projeto ao criar novo
