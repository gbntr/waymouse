# 03 — Basic Mouse Config

## Brief

Configurações funcionais fundamentais de mouse: velocidade do ponteiro (sensitivity/accel speed) e perfil de aceleração (linear, adaptativa, flat). Aplicadas via backend do compositor (Mango IPC, Niri IPC) ou libinput direto. Essa é a base funcional do waymouse.

## Prioridade

**Alta** — Core do projeto. Sem isso, waymouse é apenas uma GUI vazia.

## Motivação

Velocidade do mouse e aceleração são as configs mais acessadas por usuários. No Wayland, cada compositor expõe isso de forma diferente (ou não expõe). O waymouse unifica a experiência.

## Escopo

### Incluído
- Aceleração/sensibilidade (range libinput: -1.0 a 1.0).
- Perfil de aceleração: `default`, `flat`, `adaptive` (e futuramente `custom` curva).
- Natural scroll (inverter direção).
- Left-handed (trocar botões).
- Aplicar por device (cada mouse pode ter config diferente).
- Persistência por device.

### Excluído
- Curvas de aceleração custom (per-pixel, polynomial) — pode ser adicionado depois.
- Configuração de DPI do hardware (requer software vendor).
- Taxa de polling (requer root/kernel).

## Backend

### O que precisa fazer

1. **MangoBackend**:
   - Mango é baseado em dwl + wlroots. dwl configura input via `config.h` (estático) ou `config.mk` (não, é build).
   - A configuração de mouse em dwl é feita em tempo de compilação no `config.h`. Isso é um problema: **dwl tradicional não suporta config dinâmica de input**.
   - No entanto, Mango pode ter adicionado IPC ou protocolo custom. Precisamos verificar o código do Mango.
   - Se Mango não tiver IPC, o backend deve usar `libinput` direto no device node (via `libinput_device_config_accel_set_speed()` e `libinput_device_config_accel_set_profile()`). Isso funciona porque o compositor também usa libinput, mas pode haver conflito de quem "manda".
   - **lwlr/dwl**: o compositor abre o device via `libinput_path_add_device`. Se o waymouse também abrir o mesmo device, pode funcionar (libinput permite múltiplos contextos), mas o compositor é quem processa os eventos. Alterar a config no contexto do waymouse **não afeta** o contexto do compositor. A config é por-`libinput` contexto.
   - **Conclusão**: para Mango, a única forma confiável é se o Mango expuser um IPC ou protocolo. Caso contrário, precisamos documentar: "Mango não suporta config dinâmica. Considere usar LibinputFallback com udev rules."
   - **LibinputFallback para Mango**: criar regras udev (`/etc/udev/rules.d/99-waymouse.rules`) que setam `LIBINPUT_ATTR_*` ou usar `libinput` CLI (não existe, mas existe `libinput measure` e `libinput list-devices`). A forma de fallback é usar `udev` para setar propriedades que o libinput do kernel lê. Isso é persistente mas requer root.
   - **Possível**: `libinput` device config pode ser aplicada via `libinput` se o compositor expuser o `libinput_device` (não expõe). Então **não é possível** alterar libinput de outro processo e afetar o compositor.

   **Decisão**: Para Mango, o backend deve tentar IPC/protocolo. Se não existir, reportar `false` e a GUI instruir o usuário a usar udev rules ou reiniciar o compositor. O fallback de udev pode ser um helper script (não MVP).

2. **NiriBackend**:
   - Niri tem IPC maduro. `niri msg` (ou socket direto) aceita comandos.
   - Verificar se Niri aceita `input.mouse` config via IPC. Se sim, implementar payload JSON.
   - Se não, mesmo problema do Mango.

3. **LibinputBackend**:
   - Abrir o device via `libinput_path_create_context()` + `libinput_path_add_device()`.
   - Aplicar `libinput_device_config_accel_set_speed()`, `libinput_device_config_accel_set_profile()`, etc.
   - **Problema**: como dito, isso não afeta o compositor. O LibinputBackend no waymouse é útil se o usuário quiser que **o waymouse** controle o mouse (não faz sentido).
   - **Correção**: o `LibinputBackend` deve ser **udev rules** — escrever regras udev que o kernel/libinput lê na próxima vez que o device é conectado. Ou usar `libinput` via `systemd`? Não.
   - **Revisão**: A forma correta de "fallback genérico" é modificar o `libinput` config do **compositor**, não criar outro contexto. Como não podemos, o fallback é:
     - A) `evdev` direct (muda os eventos do kernel? Não, é read-only).
     - B) **Interception Tools** (`udevmon`, `intercept`, `intercept`)? Usa `libevdev` para interceptar e modificar eventos. Isso é complexo.
     - C) **Permitir que o usuário configure o compositor manualmente** e o waymouse apenas gere o arquivo de config (TOML) e instrua o usuário. Não é o ideal.
     
   **Revisão profunda**: O que realmente funciona no Wayland genérico?
   - GNOME: `gsettings`.
   - KDE: `kwriteconfig5`.
   - Sway: `swaymsg` (comando IPC).
   - Hyprland: `hyprctl`.
   - Niri: `niri msg`.
   - Mango/dwl: ???
   - wlroots genérico: não existe IPC padrão.
   - **libinput**: a config é setada pelo compositor via `libinput_device_config_*`. O cliente (waymouse) não tem acesso ao `libinput_device` do compositor.
   
   **Portanto**: O `LibinputBackend` como "fallback universal" é um **mito**. Não funciona. O que funciona é:
   - IPC do compositor (Sway, Hyprland, Niri, etc.).
   - Ou udev rules (que afetam o kernel/libinput antes do compositor abrir o device).
   
   Para o MVP, o `LibinputBackend` deve ser **reformulado** como `UdevBackend` (escrever regras udev) ou `IpcBackend` (genérico). Ou simplesmente ter backends específicos por compositor.

   **Reformulação proposta**: A interface `Backend` continua, mas `LibinputBackend` vira `UdevBackend` que escreve `/etc/udev/rules.d/99-waymouse-<device>.rules` e roda `udevadm trigger`. Isso é "quasi-universal" (qualquer compositor que use libinput vai ler as udev properties). É root-only, mas é o que existe.

   **Alternativa**: Não ter `LibinputBackend` como aplicação runtime. Ter apenas `MangoBackend` e `NiriBackend` e, se não forem suportados, salvar no TOML e mostrar: "Seu compositor não suporta aplicação dinâmica. Configurações serão aplicadas na próxima sessão ou via config do compositor." A GUI gera um snippet de config para o usuário copiar.

   **Decisão para Harvey**: decidir se vale a pena manter `UdevBackend` ou se o escopo é apenas Mango + Niri. Eu recomendo: **Mango + Niri + SnippetGenerator**. SnippetGenerator é um "backend" que não aplica, mas gera texto de config (ex: para o usuário colar no `config.h` do dwl ou no `config.toml` do Niri). Isso é mais útil que udev rules.

### Dados (TOML)

```toml
[device."Logitech G102"]
accel_speed = 0.0
accel_profile = "flat"      # default, flat, adaptive
natural_scroll = false
left_handed = false
```

### Stubs existentes
- `mango_backend.cpp` (stub)
- `niri_backend.cpp` (stub)
- `libinput_backend.cpp` (stub, precisa ser reformulado)

## Frontend

### O que precisa fazer

- **Device list**: lista de dispositivos detectados (já existe no `MainWindow`).
- **Por-device panel**: ao clicar no device, mostra configurações específicas.
- **Slider Acel Speed**: -1.0 a 1.0, com label mostrando valor.
- **Combo Acel Profile**: `default`, `flat`, `adaptive`.
- **Checkboxes**: Natural Scroll, Left Handed.
- **Botão Apply**: aplica imediatamente via backend.
- **Indicador de status**: "Aplicado" ou "Erro: compositor não suporta".

### UX

- Se backend retornar `false`, banner amarelo com mensagem explicativa.
- Se dispositivo não tiver config salva, mostrar valores "default" e hint "default".
- Ao alterar qualquer campo, botão Apply fica ativo (unsaved changes).

## Critérios de Aceite (QA)

- [ ] Velocidade do mouse muda imediatamente no Mango (via IPC ou outro meio).
- [ ] Velocidade do mouse muda imediatamente no Niri (via IPC).
- [ ] Configuração persistida no TOML.
- [ ] Natural scroll inverte direção.
- [ ] Left handed troca botões.
- [ ] GUI mostra lista de dispositivos e painel por device.
- [ ] GUI trata erro de backend não suportado com mensagem clara.

## Dependências

- **Pointer Manager** (01) e **Shake to Find** (02) não são dependências diretas, mas a GUI estrutural pode ser aproveitada.
- **Device Separation** (04) é dependência parcial (esta feature já precisa separar touchpad de mouse no TOML, mas pode começar com mouse apenas).

## Notas para Harvey

- **Pesquisa crítica #1**: Como Mango (dwl) expõe configuração dinâmica de mouse? Verificar o repositório do Mango para IPC, signals, ou protocolo Wayland custom.
- **Pesquisa crítica #2**: Payload exato do Niri para `input.mouse` settings. `niri msg` documentação ou código fonte.
- **Decisão arquitetural**: O que fazer quando o backend não consegue aplicar runtime? Opções:
  A. Salvar no TOML e mostrar "configuração salva, mas não aplicada".
  B. Gerar snippet de config do compositor para o usuário copiar.
  C. Implementar `UdevBackend` (root) como fallback.
  D. Documentar que não é suportado e usar o `LibinputBackend` como o user roda o app com root.
  
  A **Opção A** é o mínimo viável. A **Opção B** é ótima para UX. A **Opção C** é complexa. A **Opção D** é anti-padrão (GUI app não deve rodar como root).
  
  **Recomendação**: A + B. Implementar um `ConfigExporter` (não-backend) que gera o trecho de config específico do compositor para o usuário copiar. Isso pode ser uma aba "Export" na GUI.
- **libinput**: confirmar que `libinput_device_config_*` não afeta o compositor quando chamado de outro processo. Testar com `libinput` CLI se existir.
