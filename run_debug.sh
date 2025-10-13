#!/bin/bash

# Script para executar com logging detalhado
clear
echo "=========================================="
echo "FingerprintEnhancer - Modo Debug"
echo "=========================================="
echo ""
echo "📝 Todos os logs serão exibidos aqui E salvos em: debug_log.txt"
echo ""
echo "🎯 Para testar o modo de edição interativa:"
echo "   1. Abra um projeto"
echo "   2. Selecione um fragmento com minúcias"
echo "   3. Pressione Ctrl+I OU clique em '✏️ Editar Minúcia' na toolbar"
echo "   4. Clique com botão DIREITO em uma minúcia"
echo ""
echo "🔍 Procure por mensagens com [MAINWINDOW] e [OVERLAY]"
echo ""
echo "Pressione Ctrl+C para sair"
echo "=========================================="
echo ""

cd "$(dirname "$0")"
./build/bin/FingerprintEnhancer 2>&1 | tee debug_log.txt

echo ""
echo "=========================================="
echo "Programa encerrado."
echo "Logs completos salvos em: debug_log.txt"
echo "=========================================="
