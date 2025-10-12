cd Z:\FingerprintEnhancer

REM Deletar build anterior
rmdir /s /q build

REM Definir caminhos CORRETOS
set QT_DIR=C:\Qt\6.9.3
set OPENCV_DIR=C:\opencv
set GENERATOR=Visual Studio 17 2022

REM IMPORTANTE: Adicionar Qt MSVC ao CMAKE_PREFIX_PATH
set CMAKE_PREFIX_PATH=C:\Qt\6.9.3\msvc2022_64\lib\cmake;C:\opencv\build

REM Compilar
scripts\build_win.bat debug