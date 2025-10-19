# Diretório de Recursos (Resources)

Este diretório contém todos os recursos visuais e de mídia do projeto.

## Estrutura

```
resources/
├── icons/          # Ícones da aplicação
│   └── app_icon.svg    # Ícone principal (provisório)
│
├── images/         # Imagens e splash screens
│   └── (futuro)
│
└── README.md       # Esta documentação
```

## Ícones

### app_icon.svg
- **Tamanho**: 128x128 pixels
- **Formato**: SVG (escalável)
- **Descrição**: Ícone provisório com impressão digital estilizada
- **Status**: PROVISÓRIO - substituir por design final

**TODO**: Criar versões em múltiplas resoluções:
- 16x16 (taskbar)
- 32x32 (janelas)
- 48x48 (desktop)
- 128x128 (About dialog)
- 256x256 (alta resolução)
- 512x512 (retina displays)

## Imagens

### Splash Screen
- **Status**: Gerado programaticamente em `SplashScreen.cpp`
- **TODO**: Criar imagem PNG/SVG customizada

### Backgrounds
- (Futuro) Backgrounds para dialogs especiais

## Guidelines

### Ícones
- Usar SVG sempre que possível (escalabilidade)
- Manter paleta de cores consistente:
  - Azul primário: #1a2550 - #3a5080
  - Azul claro: #42d4f4
  - Branco/Cinza: para contraste
- Estilo: Minimalista, profissional, científico

### Nomenclatura
- Use snake_case: `app_icon.svg`, `splash_background.png`
- Seja descritivo: `minutia_bifurcation_icon.svg`
- Inclua tamanho se relevante: `logo_128.png`

## Como Adicionar Recursos

1. **Adicionar arquivo** neste diretório (subpasta apropriada)
2. **Atualizar CMakeLists.txt** se necessário (para resources Qt)
3. **Documentar** neste README
4. **Commitar** com mensagem descritiva

## Licença

Todos os recursos neste diretório seguem a mesma licença do projeto principal.
Recursos de terceiros devem incluir atribuição apropriada.

---

**Última atualização**: 2024
