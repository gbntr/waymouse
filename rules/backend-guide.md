# backend-guide.md

Como adicionar suporte a um novo compositor Wayland.

## 1. Implementar a interface

Crie `src/backends/<compositor>_backend.hpp` e `.cpp`:

```cpp
class MyCompositorBackend : public Backend {
public:
    bool apply(const Device& device, const Config& cfg) override;
    bool supports(const Device& device) const override;
    std::string name() const override { return "mycompositor"; }
};
```

## 2. Registrar na fábrica

Atualize `src/core/compositor_detector.cpp` para mapear o novo `BackendType`.

## 3. Documentar

Atualize `README.md` (lista de suporte) e `rules/architecture.md` (seção do novo backend).

## 4. Testar

Crie mock em `tests/`.

## Protocolos Wayland

Se o compositor usar protocolo Wayland custom:
- Coloque o XML do protocolo em `src/backends/protocols/`
- Use `wayland-scanner` para gerar bindings C (CMake já deve chamar isso).
- Exemplo: Mango pode usar `wlr_input_inhibit_unstable_v1` ou protocolo privado.

## IPC

Se o compositor usar socket IPC:
- Abra socket Unix.
- Envie payload conforme documentação do compositor.
- Use `nlohmann/json` para serialização.

## Aplicação direta

Se usar `libinput` direto:
- Abra o device node via `libinput_path_add_device()`.
- Aplique `libinput_device_config_*`.
- **Atenção**: requer permissão no grupo `input` ou sudo.
