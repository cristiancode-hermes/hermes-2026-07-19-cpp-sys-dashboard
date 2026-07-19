# SysDashboard — Monitor de Sistema Linux en TUI

Dashboard interactivo de recursos del sistema Linux que lee estadísticas directamente de `/proc/` usando **C++23** con interfaz **FTXUI** en terminal.

## Características

- **CPU**: Uso por núcleo en tiempo real con barras de progreso coloreadas
- **Memoria**: Total, usada, disponible, swap con porcentajes
- **Disco**: IOPS y throughput de lectura/escritura por dispositivo
- **Red**: Throughput RX/TX y paquetes por segundo por interfaz
- **Sistema**: Uptime, load average (1/5/15 min), número de núcleos
- **Modo JSON** (`--json`): Salida estructurada para integración con scripts
- Arquitectura RAII con manejo de errores robusto
- Actualización cada segundo mediante hilo en background con PostEvent

## Requisitos

- Compilador: GCC 14+ o Clang 18+
- CMake 3.28+
- Sistema Linux con `/proc` (lectura de `/proc/stat`, `/proc/meminfo`, `/proc/diskstats`, `/proc/net/dev`, `/proc/loadavg`, `/proc/uptime`)

## Compilar y ejecutar

```bash
# Debug
cmake -G "Unix Makefiles" -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug

# Ejecutar TUI
./build/debug/sys-dashboard

# Modo JSON
./build/debug/sys-dashboard --json

# Release
cmake -G "Unix Makefiles" -S . -B build/release -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

## Tests

```bash
cmake -G "Unix Makefiles" -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build build/debug
cd build/debug && ctest --output-on-failure
```

## Tecnologías

| Tecnología | Versión | Propósito |
|-----------|---------|-----------|
| C++23 | GCC 14 | Estándar del lenguaje |
| FTXUI | 5.0.0 | Terminal UI interactiva |
| nlohmann_json | 3.11.3 | Serialización JSON |
| spdlog | 1.15.1 | Logging estructurado |
| Catch2 | 3.8.0 | Tests unitarios y de integración |

## Estructura del proyecto

```
├── CMakeLists.txt
├── CMakePresets.json
├── include/sysdashboard/
│   ├── cpu_stats.hpp
│   ├── mem_stats.hpp
│   ├── disk_stats.hpp
│   ├── net_stats.hpp
│   └── dashboard_app.hpp
├── src/
│   ├── main.cpp
│   ├── cpu_stats.cpp
│   ├── mem_stats.cpp
│   ├── disk_stats.cpp
│   ├── net_stats.cpp
│   └── dashboard_app.cpp
└── tests/
    ├── CMakeLists.txt
    └── test_main.cpp
```
