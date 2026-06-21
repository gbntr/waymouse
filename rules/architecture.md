# architecture.md

## Visão geral

waymouse é um utilitário de configuração de mouse para compositores Wayland.
Ele detecta o compositor em runtime, seleciona o backend apropriado e expõe
as configurações via GUI Qt6.

## Camadas

```
┌─────────────────────────────────────────────────────┐
│                    GUI (Qt6)                        │
│  main_window, device_panel, pointer_panel,          │
│  shake_panel, shake_overlay                         │
├─────────────────────────────────────────────────────┤
│                     Core                            │
│  config_manager, device_mgr, compositor_detector,   │
│  pointer_manager, theme_detector, shake_manager,    │
│  shake_session_runtime, shake_detector,             │
│  raw_input_monitor, config_watcher,                 │
│  runtime_status, runtime_lock, mango_ipc_client     │
├─────────────────────────────────────────────────────┤
│          Backends (Device + Cursor)                  │
│  Mango │ Niri │ Libinput │ EnvCursor │ WlrCursor    │
├─────────────────────────────────────────────────────┤
│              Wayland Layer                          │
│  layer_shell_surface (wlr-layer-shell-v1)           │
├─────────────────────────────────────────────────────┤
│      Sistema (Linux)                                │
│  libinput / libudev / wayland-client / /dev/input   │
└─────────────────────────────────────────────────────┘
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
2. Detector retorna `BackendType` → fábrica cria backend concreto e cursor backend.
3. `DeviceManager` enumera dispositivos.
4. `ConfigManager` carrega config persistida.
5. `PointerManager` é instanciado com `ThemeDetector` + `CursorBackend`.
6. Se o processo for `waymouse --shake-runtime`, o app entra em modo headless de sessão.
7. `ShakeSessionRuntime` adquire lock de instância, carrega `[shake]`, inicializa overlay, input monitor e publicação de status.
8. Se o compositor for Mango, `MangoIpcClient` consulta layout de monitores via `mmsg`.
9. `ConfigWatcher` observa alterações no TOML e reaplica `[shake]` sem restart.
10. No modo GUI, `MainWindow` é exibida com três abas: Dispositivo, Aparência, Shake.
11. A GUI lê o estado autoritativo do runtime de sessão em vez de assumir sucesso local.
12. Usuário altera config → `ConfigManager` salva TOML; runtime aplica via watcher.

## Shake to Find Flow

1. O runtime de sessão é iniciado no Mango via `exec-once=waymouse --shake-runtime`.
2. `RawInputMonitor` tenta abrir `/dev/input/event*` para `ID_INPUT_MOUSE=true` e continua em retry/rescan se falhar.
3. Eventos `REL_X`/`REL_Y` alimentam `ShakeDetector` e atualizam posição estimada do cursor.
4. Em Mango, `MangoIpcClient` fornece layout de monitores para clamp/posicionamento.
5. Ao detectar shake, o runtime solicita exibição do overlay.
6. No Mango, `ShakeOverlay` prioriza `zwlr_layer_shell_v1`; se falhar, cai para `QWindow` fallback com estado degradado explícito.
7. `RuntimeStatusPublisher` grava o estado real da feature em `$XDG_RUNTIME_DIR/waymouse/shake-status.json`.
8. A GUI lê esse estado para mostrar ativo, degradado, parado ou erro de permissão.
