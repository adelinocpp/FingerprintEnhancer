# 🐛 DEBUG - Problema ao Criar Novo Projeto

## 📋 Situação

O usuário reporta que ao fechar um projeto sem salvar e tentar criar outro, aparece a mensagem:
"Não foi possível criar o projeto"

## 🔍 Logs de Debug Adicionados

Adicionei logs detalhados em `MainWindow::newProject()` para identificar o problema.

## 🧪 Como Testar e Enviar Logs

### 1. Execute a aplicação pelo terminal:

```bash
cd /media/DRAGONSTONE/MEGAsync/Papiloscopia/FingerprintEnhancer
./build/bin/FingerprintEnhancer 2>&1 | tee debug_log.txt
```

### 2. Reproduza o problema:

1. Menu → Arquivo → Novo Projeto
   - Nome: "Teste 1"
   - OK

2. Menu → Arquivo → Novo Projeto (SEM adicionar nada ao projeto anterior)
   - Nome: "Teste 2"
   - Escolha "Não" quando perguntar sobre salvar
   - OK

3. **Se aparecer erro**, copie TODO o log do terminal

### 3. Logs Esperados

Os logs devem mostrar algo como:

```
[DEBUG] newProject() iniciado
[DEBUG] hasOpenProject(): true/false
[DEBUG] Há projeto aberto, iniciando processo de fechamento
[DEBUG] Projeto não está modificado, fechando sem perguntar
[DEBUG] Limpando visualizações...
[DEBUG] Fechando projeto anterior...
[DEBUG] Projeto fechado. hasOpenProject(): false
[DEBUG] Abrindo diálogo de novo projeto...
[DEBUG] Tentando criar projeto: Teste 2
[DEBUG] hasOpenProject() antes de criar: false
[DEBUG] Projeto criado com sucesso!
```

### 4. Se der erro, o log mostrará:

```
[DEBUG] ERRO: Falha ao criar projeto!
[DEBUG] hasOpenProject() após falha: true  <-- Indica que não fechou corretamente
```

## 📊 O Que Pode Estar Acontecendo

### Hipótese 1: closeProject() não está funcionando
- `hasOpenProject()` ainda retorna `true` após `closeProject()`
- Indica problema no `ProjectManager::closeProject()`

### Hipótese 2: createNewProject() verifica projeto aberto internamente
- `createNewProject()` pode estar verificando se já há projeto aberto
- E recusando criar se houver

### Hipótese 3: Problema de sincronização
- Estado do `ProjectManager` não está sendo atualizado corretamente

## 🔧 Próximos Passos

Após analisar os logs, posso:

1. Corrigir `closeProject()` se não estiver limpando o estado
2. Adicionar `closeProject()` dentro de `createNewProject()` automaticamente
3. Verificar se há locks ou mutexes travando o estado
