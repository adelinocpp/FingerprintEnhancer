# TODO: Calcular Posições dos Fragmentos

## Problema

Quando a imagem tem rotação arbitrária (ex: 13°), os retângulos de destaque dos fragmentos aparecem em **posições incorretas** sobre a imagem rotacionada.

Para ângulos múltiplos de 90° (0°, 90°, 180°, 270°) funciona corretamente.

## Estrutura dos Objetos

### Imagem
- **`currentImage->originalImage`** : `cv::Mat` - Imagem ORIGINAL (sem rotações, ex: 4000x3000)
- **`currentImage->workingImage`** : `cv::Mat` - Imagem ATUAL (rotacionada, ex: 4572x3822 para 13°)
- **`currentImage->currentRotationAngle`** : `double` - Ângulo acumulado (ex: 13.0)

### Fragmento
- **`fragment->sourceRect`** : `QRect` - Coordenadas na imagem ORIGINAL (nunca muda)
  - Exemplo: `[2236, 1664, 974, 1027]` = x=2236, y=1664, largura=974, altura=1027

### Overlay
- **`FragmentRegionsOverlay`** : `QWidget` - Camada transparente SOBRE o ImageViewer
  - Desenha os retângulos/polígonos de destaque dos fragmentos

## Fluxo Atual

### 1. Rotação da Imagem
**Arquivo:** `src/gui/RotationDialog.cpp`  
**Função:** `RotationDialog::rotateImage()`

```cpp
// Centro da imagem
cv::Point2f center(image.cols / 2.0f, image.rows / 2.0f);

// Matriz de rotação
cv::Mat rotMatrix = cv::getRotationMatrix2D(center, -angle, 1.0);

// Calcular novo tamanho (para incluir borda branca)
double abs_cos = fabs(rotMatrix.at<double>(0, 0));
double abs_sin = fabs(rotMatrix.at<double>(0, 1));
int new_w = int(image.rows * abs_sin + image.cols * abs_cos);
int new_h = int(image.rows * abs_cos + image.cols * abs_sin);

// Ajustar translação
rotMatrix.at<double>(0, 2) += (new_w / 2.0) - center.x;
rotMatrix.at<double>(1, 2) += (new_h / 2.0) - center.y;

// Rotacionar
cv::warpAffine(image, rotated, rotMatrix, cv::Size(new_w, new_h));
```

### 2. Transformação dos Retângulos
**Arquivo:** `src/gui/FragmentRegionsOverlay.cpp`  
**Função:** `FragmentRegionsOverlay::calculateRotatedPolygon()`

Tenta replicar **exatamente** a mesma transformação acima:

```cpp
// Mesma matriz de rotação
cv::Point2f center(originalSize.width() / 2.0f, originalSize.height() / 2.0f);
cv::Mat rotMatrix = cv::getRotationMatrix2D(center, -angleDeg, 1.0);

// Mesmo cálculo de novo tamanho
double abs_cos = fabs(rotMatrix.at<double>(0, 0));
double abs_sin = fabs(rotMatrix.at<double>(0, 1));
int new_w = int(originalSize.height() * abs_sin + originalSize.width() * abs_cos);
int new_h = int(originalSize.height() * abs_cos + originalSize.width() * abs_sin);

// Mesmo ajuste de translação
rotMatrix.at<double>(0, 2) += (new_w / 2.0) - center.x;
rotMatrix.at<double>(1, 2) += (new_h / 2.0) - center.y;

// Transformar os 4 cantos do retângulo
std::vector<cv::Point2f> srcPoints = { /* 4 cantos */ };
std::vector<cv::Point2f> dstPoints;
cv::transform(srcPoints, dstPoints, rotMatrix);
```

### 3. Desenho na Tela
**Arquivo:** `src/gui/FragmentRegionsOverlay.cpp`  
**Função:** `FragmentRegionsOverlay::drawFragmentRegion()`

```cpp
// Pega pontos transformados
QPolygonF rotatedPoly = calculateRotatedPolygon(...);

// Aplica zoom e scroll
for (const QPointF& pt : rotatedPoly) {
    scaledPoly << QPointF(
        pt.x() * scaleFactor + imageOffset.x() - scrollOffset.x(),
        pt.y() * scaleFactor + imageOffset.y() - scrollOffset.y()
    );
}

// Desenha
painter.drawPolygon(scaledPoly);
```

## O Que Verificar

### Hipótese 1: Diferença na matriz de rotação
- Comparar a matriz gerada em `RotationDialog::rotateImage()` com a de `calculateRotatedPolygon()`
- Verificar se os valores de `rotMatrix.at<double>(0,2)` e `rotMatrix.at<double>(1,2)` são idênticos

### Hipótese 2: Tamanho da imagem rotacionada
- Verificar se `new_w` e `new_h` calculados são iguais
- Comparar com o tamanho real de `workingImage` (cols x rows)

### Hipótese 3: Transformação invertida
- A imagem pode estar sendo rotacionada em um sentido e os pontos no outro
- Verificar se precisa usar `+angle` em vez de `-angle` (ou vice-versa)

### Hipótese 4: Coordenadas de origem
- Verificar se os cantos do retângulo estão sendo corretamente definidos
- `originalRect.left()`, `right()`, `top()`, `bottom()` podem ter convenções diferentes

## Como Depurar

1. **Adicionar logs na rotação da imagem:**
   ```cpp
   fprintf(stderr, "[DEBUG] Rotacionando: center=(%.1f,%.1f), new_size=%dx%d\n",
           center.x, center.y, new_w, new_h);
   fprintf(stderr, "[DEBUG] Matriz: [%.4f %.4f %.1f; %.4f %.4f %.1f]\n",
           rotMatrix.at<double>(0,0), rotMatrix.at<double>(0,1), rotMatrix.at<double>(0,2),
           rotMatrix.at<double>(1,0), rotMatrix.at<double>(1,1), rotMatrix.at<double>(1,2));
   ```

2. **Adicionar logs na transformação dos retângulos:**
   Os logs já existem em `calculateRotatedPolygon()`

3. **Comparar valores lado a lado:**
   - Rotacionar imagem com 13°
   - Verificar matriz e tamanhos nos logs
   - Verificar se os valores são EXATAMENTE iguais

4. **Teste visual:**
   - Criar fragmento em posição conhecida (ex: centro da imagem)
   - Rotacionar 13°
   - Ver onde o retângulo aparece vs. onde deveria estar

## Arquivos Modificados

- `src/gui/FragmentRegionsOverlay.cpp` - Comentários TODO adicionados
- `src/gui/RotationDialog.cpp` - Comentários TODO adicionados
- `TODO_FRAGMENTOS.md` - Este arquivo

## Referências no Código

Buscar por: `TODO: calcular posições dos fragmentos`

Locais marcados:
1. `FragmentRegionsOverlay.cpp` - Cabeçalho do arquivo
2. `FragmentRegionsOverlay.cpp` - Função `drawFragmentRegion()`
3. `FragmentRegionsOverlay.cpp` - Função `calculateRotatedPolygon()`
4. `RotationDialog.cpp` - Função `rotateImage()`
