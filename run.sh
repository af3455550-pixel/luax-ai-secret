#!/bin/bash
# Fireline Wildfire Command - Runner

set -e

echo "=== FIRELINE: WILDFIRE COMMAND ==="
echo "Compilando se necessário..."

if [ ! -f ./fireline ]; then
    echo "Compilando..."
    g++ -std=c++17 -I src src/main.cpp -o fireline -lpthread -O3
fi

echo "Executável: $(du -h fireline | cut -f1)"
echo ""
echo "Opções:"
echo "  ./run.sh              - Jogo interativo"
echo "  ./run.sh credits      - Créditos 3D cinematográficos"
echo "  ./run.sh demo         - Demo simulação fogo"
echo "  ./run.sh test         - Testes"
echo ""

MODE=${1:-game}

case $MODE in
    credits)
        ./fireline --credits
        ;;
    demo)
        ./fireline --demo
        ;;
    test)
        ./fireline --test
        ;;
    game|*)
        ./fireline
        ;;
esac
