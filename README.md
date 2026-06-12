# waymouse

Aplicação open-source para gerenciamento de configurações de mouse em sistemas Wayland.

## Compositores suportados

- **Mango** (wlroots/dwl)
- **Niri**
- Fallback via libinput/udev

## Stack

- C++20
- Qt6 (Widgets)
- CMake
- TOML (configuração)
- libinput / libudev / wayland-client

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Uso

```bash
./waymouse
```

## Contribuindo

Veja [`docs/roadmap.md`](docs/roadmap.md) para a lista de features e próximos passos.  
Veja [`CONTRIBUTING.md`](CONTRIBUTING.md) para guias de como contribuir.  
Veja `rules/` para arquitetura e estilo de código.

## Equipe

- **Harvey** — Especificações e planejamento (`specs/` — local, não versionado)
- **Mike** — Implementação (`src/`)
- **Dona** — QA (`tests/`)
