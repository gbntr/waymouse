# architecture.md

## Visão geral

waymouse é um utilitário de configuração de mouse para compositores Wayland.
Ele detecta o compositor em runtime, seleciona o backend apropriado e expõe
as configurações via GUI Qt6.

## Camadas

```
┌─────────────────────────────┐
│           GUI (Qt6)         │
│  main_window, device_panel  │
├─────────────────────────────┤
│           Core              │
│  config_manager, device_mgr,│
│  compositor_detector        │
├─────────────────────────────┤
│          Backends           │
│  Mango │ Niri │ Libinput    │
├─────────────────────────────┤
│      Sistema (Linux)        │
│  libinput / libudev / wl    │
└─────────────────────────────┘
```

## Core

### CompositorDetector
- Detecta o compositor em runtime via `XDG_CURRENT_DESKTOP`, socket Wayland e heurísticas.
- Retorna um `BackendType` que determina qual backend instanciar.
- Se Mango for detectado, verifica se o protocolo custom está disponível.
- Se Niri, verifica se o socket IPC existe (`$NIRI_SOCKET` ou `~/.local/share/niri/niri.sock.*`).
- Se nenhum, usa `LibinputBackend` como fallback.

### DeviceManager
- Enumera dispositivos apontadores via `libudev` (subsystem `input`, prop `ID_INPUT_MOUSE` ou `ID_INPUT_POINTER`).
- Fornece metadados: nome, sysfs path, vendor/product IDs.
- Não aplica configurações diretamente — delega ao backend.

### ConfigManager
- Persiste configurações em `~/.config/waymouse/config.toml`.
- Usa `toml11` para serialização.
- Estrutura:
  ```toml
  [device."Logitech G102"]
  accel_speed = 0.0
  accel_profile = "flat"
  natural_scroll = false
  left_handed = false
  ```
- O DeviceManager carrega o device a partir do nome udev (ou identificador estável).

## Backends

### Interface

```cpp
class Backend {
public:
    virtual ~Backend() = default;
    virtual bool apply(const Device& device, const Config& cfg) = 0;
    virtual bool supports(const Device& device) const = 0;
    virtual std::string name() const = 0;
};
```

### MangoBackend
- Comunica-se com o compositor Mango via protocolo Wayland custom.
- Mango é baseado em dwl + wlroots. Provavelmente expõe configuração de input via protocolo proprietário ou `zwp_pointer_constraints` analogia.
- **Status**: stub implementado. Protocolo exato a ser mapeado por Harvey/Mike.

### NiriBackend
- Comunica-se com Niri via socket Unix IPC.
- Niri aceita JSON commands. waymouse deve enviar comandos para alterar `input.mouse` settings.
- **Status**: stub implementado. Payload JSON exato a ser mapeado.

### LibinputBackend
- Fallback universal.
- Escreve regras udev ou usa `libinput` diretamente no device node (`/dev/input/event*`).
- Requer permissões (root ou grupo `input`).
- Aplica configurações via `libinput_device_config_*` APIs.
- **Status**: stub implementado.

## GUI

- `MainWindow`: janela principal, lista de dispositivos, painel de configuração.
- `DevicePanel`: widgets por propriedade (slider accel, checkbox natural scroll, etc.).
- A GUI não conhece backends diretamente — fala apenas com `Core` (config_manager + device_manager).

## Config Path

`$XDG_CONFIG_HOME/waymouse/config.toml` (fallback `~/.config/waymouse/config.toml`)

## Runtime Flow

1. `main` instancia `CompositorDetector`.
2. Detector retorna `BackendType` → fábrica cria backend concreto.
3. `DeviceManager` enumera dispositivos.
4. `ConfigManager` carrega config persistida.
5. `MainWindow` é exibida com lista de dispositivos.
6. Usuário altera config → `ConfigManager` salva TOML + `Backend::apply()` é chamado.
