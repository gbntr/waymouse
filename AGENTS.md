# AGENTS.md

## waymouse

Config manager de mouse agnóstico para Wayland.

### Comunicação com a equipe

#### Harvey (Specs)
- Trabalha em `specs/` — pasta local, **não versionada** (`.gitignore`).
- Entrega: documentos de requisitos, arquitetura de protocolo, fluxos de UX.
- Antes de iniciar qualquer feature, Harvey deve atualizar `rules/architecture.md`.

#### Mike (Implementação)
- Trabalha em `src/`.
- Segue estritamente `rules/coderules.md` e `rules/architecture.md`.
- Nunca implementa backend sem a interface `Backend` definida.
- Toda feature precisa de stub + teste mínimo antes de merge.

#### Dona (QA)
- Trabalha em `tests/`.
- Usa testes de unidade (mock de backends) e integração (compositores reais quando possível).
- CI roda em `ubuntu-latest` e `archlinux` (GitHub Actions).

### Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Testes

```bash
cd build && ctest --output-on-failure
```

### Convenções

- C++20
- Qt6 Widgets
- TOML via `toml11`
- Backend pattern: `src/backends/backend.hpp`
- Mensagens de commit em inglês, no padrão: `[area] verbo descrição`
  - Ex: `[gui] add device panel`, `[mango] implement pointer config protocol`
