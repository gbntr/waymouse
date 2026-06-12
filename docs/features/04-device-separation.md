# 04 — Device Separation

## Brief

Distinguir automaticamente entre **mouse** e **touchpad** na enumeração de dispositivos. Aplicar configurações específicas para cada tipo (ex: touchpad tem natural scroll, tap-to-click, palm detection; mouse tem polling, DPI). A GUI deve mostrar abas ou filtros por tipo.

## Prioridade

**Média** — Melhora UX, mas não bloqueia o core.

## Motivação

Touchpad e mouse são dispositivos de input completamente diferentes. Configurar um como o outro não faz sentido. Um usuário pode querer natural scroll no touchpad mas não no mouse. A separação clara é esperada em qualquer app de config de input.

## Escopo

### Incluído
- Detecção de tipo: `mouse` vs `touchpad` via udev properties (`ID_INPUT_MOUSE`, `ID_INPUT_TOUCHPAD`, `ID_INPUT_TRACKBALL`, etc.).
- Aba/filtro na GUI por tipo.
- Configurações específicas de touchpad:
  - Tap-to-click (on/off).
  - Palm detection (on/off).
  - Scroll method (edge, two-finger).
  - Disable while typing.
- Configurações específicas de mouse:
  - Polling rate (read-only ou hint, não aplicável diretamente).
  - Botões extras (side buttons mapping — futuro).
- Persistência por tipo no TOML.

### Excluído
- Trackpoint (pode ser adicionado depois).
- Tablet/Wacom (outro escopo).
- Configuração de gestos multi-touch (complexo, depende de libinput versão).

## Backend

### O que precisa fazer

1. **DeviceManager**:
   - Atualizar `enumerate()` para classificar o device.
   - Usar `udev_device_get_property_value(dev, "ID_INPUT_TOUCHPAD")` para detectar touchpad.
   - Adicionar campo `type` ao struct `Device` (enum: Mouse, Touchpad, Trackpoint, Unknown).

2. **ConfigManager**:
   - Suportar chaves por tipo no TOML:
     ```toml
     [device."SynPS/2 Synaptics TouchPad"]
     type = "touchpad"
     tap_to_click = true
     natural_scroll = true
     scroll_method = "two-finger"
     disable_while_typing = true
     
     [device."Logitech G102"]
     type = "mouse"
     accel_speed = -0.2
     accel_profile = "flat"
     ```
   - Ou: `[touchpad."nome"]` e `[mouse."nome"]` (mais organizado).

3. **Backends**:
   - Touchpad configs via IPC do compositor (Mango/Niri) ou libinput (mas mesmo problema do Basic Mouse Config).
   - Propriedades libinput de touchpad: `libinput_device_config_tap_set_enabled()`, `libinput_device_config_dwt_set_enabled()`, etc.
   - Se backend não suportar, salvar no TOML e exportar snippet.

### Stubs existentes
- `DeviceManager` já enumera via udev.
- `ConfigManager` já lê/salva TOML.

## Frontend

### O que precisa fazer

- **Filtro/Abas**: "All", "Mouse", "Touchpad" na GUI.
- **Icones**: ícone de mouse vs touchpad na lista.
- **Touchpad Panel**:
  - Toggle: Tap to Click, Palm Detection, Disable While Typing.
  - Combo: Scroll Method (edge, two-finger, none).
- **Mouse Panel**: (já existe no Basic Mouse Config).
- **Aplicação condicional**: se o device selecionado é touchpad, mostrar painel de touchpad. Se mouse, mostrar painel de mouse.

### UX

- Se nenhum touchpad detectado, aba "Touchpad" desabilitada ou com mensagem "No touchpad detected".
- Configurações de tipo incompatível (ex: natural scroll em mouse) podem ser mostradas ou ocultas dependendo do tipo.

## Critérios de Aceite (QA)

- [ ] DeviceManager classifica corretamente mouse vs touchpad.
- [ ] GUI mostra lista filtrada por tipo.
- [ ] Touchpad configs aparecem apenas para touchpads.
- [ ] Mouse configs aparecem apenas para mouses.
- [ ] TOML salva `type` e configs específicas.
- [ ] Aplicação de touchpad config funciona no Mango/Niri (ou fallback apropriado).

## Dependências

- **Basic Mouse Config** (03) — a GUI de device list é base.

## Notas para Harvey

- **Pesquisa**: udev properties `ID_INPUT_TOUCHPAD`, `ID_INPUT_TRACKPOINT`, `ID_INPUT_TABLET`, `ID_INPUT_JOYSTICK`. Verificar quais são confiáveis.
- **Decisão**: TOML estrutura por tipo. `type = "mouse"` dentro do device é redundante (podemos inferir do udev). Mas para portabilidade (arquivo copiado para outra máquina), pode ser útil. Ou simplesmente usar `[mouse."nome"]` e `[touchpad."nome"]` como namespaces.
- **Decisão**: touchpad configs dependem de libinput features. Verificar se `libinput` em todos os sistemas suporta tap, dwt, etc. Sim, mas com `LIBINPUT_CONFIG_TAP_*` e `LIBINPUT_CONFIG_SCROLL_*`.
