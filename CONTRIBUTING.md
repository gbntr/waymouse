# Contributing to waymouse

Thanks for your interest! This guide explains how to contribute, from picking a task to opening a PR.

## Getting Started

### Prerequisites

- C++20 compiler (GCC 12+, Clang 15+, or MSVC 2022+)
- CMake 3.22+
- Qt6 (Core, Widgets)
- `pkg-config`, `libwayland-client`, `libudev`, `libinput`, `wayland-protocols`

### Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

### Run Tests

```bash
cd build && ctest --output-on-failure
```

## Finding Work

See [`docs/roadmap.md`](docs/roadmap.md) for the current feature list and their status. Pick something marked `not started`.

**Good first issues:**
- UI widgets and panels
- Theme detection logic
- Config file handling
- Test coverage

**Advanced issues:**
- Wayland protocol implementations
- Compositor IPC backends (Mango, Niri)
- `evdev` / `libinput` event capture

## Workflow

1. **Fork & branch:** Create a feature branch from `main`.
2. **Code:** Follow `rules/coderules.md` (C++ style, Qt6 patterns, commit format).
3. **Test:** Add or update tests in `tests/`. All PRs must pass CI.
4. **Document:** Update `README.md` or `docs/` if your change affects user-facing behavior.
5. **PR:** Open a pull request with a clear description.

## Code Standards

- **C++20**, Qt6 Widgets.
- **Header guards:** Use `#pragma once`.
- **Naming:** `CamelCase` for classes, `snake_case` for functions/variables, `m_` prefix for private members.
- **Commits:** `[area] verb description` — e.g., `[gui] add theme combo box`, `[niri] implement mouse speed IPC`.
- **No `using namespace std;`** at global scope.
- **Exceptions:** Do not use exceptions for control flow.
- **Backend interface:** Every new compositor backend must inherit from `src/backends/backend.hpp`.

## Adding a New Compositor Backend

See [`rules/backend-guide.md`](rules/backend-guide.md) for the full guide. In short:

1. Implement `Backend` interface in `src/backends/<compositor>_backend.{hpp,cpp}`.
2. Register in `CompositorDetector` (`src/core/compositor_detector.cpp`).
3. Add tests in `tests/`.
4. Update `README.md` and `docs/roadmap.md`.

## Questions?

- Open a **GitHub Discussion** for architectural questions.
- Open an **Issue** for bugs or feature requests.
- Read `rules/architecture.md` for system design context.

## License

By contributing, you agree that your contributions will be licensed under the same license as the project (see `LICENSE` file).
