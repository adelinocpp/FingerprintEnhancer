#!/bin/bash
# Wrapper para manter compatibilidade - chama o script em scripts/
exec "$(dirname "$0")/scripts/run_debug.sh" "$@"
