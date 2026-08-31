@echo off
REM FIRELINE: WILDFIRE COMMAND - Runner Windows
echo === FIRELINE: WILDFIRE COMMAND ===

IF NOT EXIST fireline.exe (
    echo Compilando para Windows...
    g++ -std=c++17 -I src src/main.cpp -o fireline.exe -O3
    IF ERRORLEVEL 1 (
        echo ERRO: g++ nao encontrado. Instala MinGW-w64 ou usa WSL.
        echo Opcoes:
        echo 1. Instala https://winlibs.com/ e adiciona ao PATH
        echo 2. Ou usa WSL: wsl ./fireline
        echo 3. Ou usa Git Bash com g++
        pause
        exit /b 1
    )
)

echo Executavel: fireline.exe
echo.
echo Opcoes:
echo   run_windows.bat           - Jogo interativo
echo   run_windows.bat credits   - Creditos 3D
echo   run_windows.bat demo      - Demo fogo
echo   run_windows.bat test      - Testes
echo.

IF "%1"=="credits" (
    fireline.exe --credits
) ELSE IF "%1"=="demo" (
    fireline.exe --demo
) ELSE IF "%1"=="test" (
    fireline.exe --test
) ELSE IF "%1"=="help" (
    fireline.exe --help
) ELSE (
    fireline.exe
)
