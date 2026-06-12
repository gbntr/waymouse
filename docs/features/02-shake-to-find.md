# 02 — Shake to Find

## Brief

Quando o usuário "sacode" o mouse rapidamente (movimentos bruscos e rápidos), o cursor é momentaneamente **ampliado** (aumenta de tamanho) ou **destacado** (círculo ao redor) para facilitar a localização na tela. Isso resolve o problema clássico de "perder o cursor" em telas grandes, múltiplos monitores ou após AFK.

## Prioridade

**Alta** — *Esta é a feature que motivou a criação do waymouse. Deve ser a referência de qualidade do projeto.*

## Motivação

Em setups com múltiplos monitores de alta resolução, é muito comum perder o cursor visualmente. O "Shake to Find" do macOS é uma das funcionalidades mais elogiadas de UX. Não existe um equivalente nativo e acessível no Linux/Wayland. O waymouse deve preencher essa lacuna.

## Escopo

### Incluído
- Detecção de padrão de "shake" (movimentos rápidos, mudanças abruptas de direção, velocidade acima de threshold).
- Ampliação do cursor por N segundos após o shake.
- Configurações: velocidade mínima do shake, tempo de ampliação, fator de escala, ativar/desativar.
- Persistência no TOML.
- Funciona em todos os backends (Mango, Niri, Libinput).

### Excluído
- Efeitos sonoros (nice-to-have).
- Animações complexas (bounce, fade, particle) — manter simples: scale + highlight.
- Tecla de atalho manual (pode vir depois).
- Touchpad shake (foco em mouse).

## Backend

### O que precisa fazer

1. **Captura de movimento**:
   - O backend não captura movimento diretamente. Precisamos de um **listener de input** ou **hook** no sistema.
   - **Opção A**: `libinput` events — abrir o device do mouse, ler `LIBINPUT_EVENT_POINTER_MOTION`, calcular velocidade delta.
   - **Opção B**: Wayland protocolo — não existe um protocolo padrão para "cursor watcher". O cliente waymouse não é quem controla o cursor; o compositor é.
   - **Opção C (recomendada)**: `evdev` — abrir o event node do mouse (`/dev/input/eventX`), ler `EV_REL` events, calcular velocidade e aceleração em userspace. Isso é independente do compositor.
   - **Opção D (mais integrada)**: Compositor plugin. Mango e Niri teriam que suportar isso nativamente. Não é realista para o waymouse.

   **Decisão arquitetural**: O `waymouse` deve rodar um **daemon/thread** que escuta o event node do mouse via `evdev` ou `libinput`. Quando detecta shake, envia um **comando** ao compositor para alterar o cursor size/temp overlay. Ou, se não houver como alterar o cursor runtime, o waymouse pode usar um **overlay próprio** (janela transparente, sempre no topo, posição do cursor) para desenhar o círculo/ampliado.

2. **Overlay próprio**:
   - Criar uma janela Wayland transparente (`wl_shell` ou `xdg_wm_base`), sempre on top, posicionada no cursor.
   - Desenhar um círculo ou cursor ampliado via `QPainter`/`QOpenGLWidget`.
   - Sincronizar posição com o cursor real.
   - Esconder após o tempo configurado.
   - **Desafio**: wayland não permite que um cliente leia a posição absoluta do cursor. O cliente recebe apenas deltas relativos. Precisamos calcular a posição absoluta nós mesmos (soma dos deltas) + lidar com bordas de tela.
   - Alternativa: usar `zwp_relative_pointer_manager_v1` para capturar deltas sem confinamento, e `zwp_locked_pointer_v1` para não precisar de cursor visível (não é o caso).
   - **Solução viável**: `libinput` com `LIBINPUT_DEVICE_CAP_POINTER` + captura de deltas. Acumular posição absoluta. Overlay Qt com `Qt::WindowStaysOnTopHint` + `Qt::FramelessWindowHint` + `Qt::WindowDoesNotAcceptFocus`. Fundo transparente (`Qt::WA_TranslucentBackground`).

3. **Configurações**:
   ```toml
   [shake_to_find]
   enabled = true
   shake_threshold = 800       # pixels/segundo
   scale_factor = 2.0          # cursor fica 2x
   duration_ms = 1000          # 1 segundo
   cooldown_ms = 500         # anti-spam
   ```

4. **Fallbacks**:
   - Se não conseguir abrir event device (permissões), mostrar notificação de erro na GUI.
   - Se não conseguir overlay (compositor não suporta), mostrar notificação: "Shake to Find requer suporte de overlay do compositor".

### Stubs existentes
- `device_manager.cpp` já enumera dispositivos via udev. Podemos reusar para encontrar o event node.
- `backend.hpp` não é usado para esta feature; é um módulo independente (`src/core/shake_detector.cpp`?).

## Frontend

### O que precisa fazer

- **Aba/Seção "Shake to Find"** na GUI.
- **Toggle on/off**: `QCheckBox` master.
- **Slider "Shake sensitivity"**: mapeia para `shake_threshold`. Invertido: slider maior = threshold menor = mais sensível.
- **Slider "Scale"**: 1.5x a 4.0x.
- **Slider "Duration"**: 500ms a 3000ms.
- **Slider "Cooldown"**: 0ms a 2000ms.
- **Preview/Test**: botão "Test" que simula o visual do overlay (mostra uma animação na GUI).
- **Status**: indicador se o detector está ativo (rodando) ou parado (erro de permissão).

### UX

- Feedback imediato: ao ligar, o detector inicia. Se erro, banner vermelho na GUI.
- Shake detectado → overlay aparece na tela real (não na GUI). O usuário não precisa fazer nada.
- Configurações aplicam imediatamente sem restart.

## Critérios de Aceite (QA)

- [ ] Shake detector inicia corretamente ao ligar o toggle.
- [ ] Shake real do mouse ativa o overlay (círculo ou cursor ampliado).
- [ ] Overlay some após `duration_ms`.
- [ ] Configurações salvas no TOML.
- [ ] Overlay funciona em Mango.
- [ ] Overlay funciona em Niri.
- [ ] Se dispositivo não tiver permissão de leitura, GUI mostra erro claro.
- [ ] Anti-spam cooldown funciona (shake contínuo não gera overlay infinito).
- [ ] Posição do overlay segue o cursor com < 50ms de lag.

## Dependências

- **DeviceManager** (já existe) — para localizar o event node.
- **ConfigManager** (já existe) — para persistir.
- Nenhuma outra feature.

## Notas para Harvey

- **Pesquisa crítica #1**: `evdev` vs `libinput` para leitura contínua. `libinput` é mais "correta" (abstrai event nodes, normaliza). `evdev` é mais crua (menos overhead). Para cálculo de velocidade, `evdev` é suficiente. Mas `libinput` já está linkada no projeto.
- **Pesquisa crítica #2**: Overlay Qt transparente + always-on-top + no-focus no Wayland. `Qt::WindowStaysOnTopHint` é suportado no Wayland? Não diretamente. Precisamos usar `xdg_wm_base` + `set_xdg_toplevel` com hints? Ou `wl_shell`? Ou protocolo `wlr_layer_shell_unstable_v1` para surfaces de overlay? **Layer Shell** é o protocolo correto para desenhar sobre tudo. Mango (wlroots) suporta. Niri também suporta. Isso é a chave.
- **Decisão arquitetural**: Usar `libwayland-client` + `wlr_layer_shell_unstable_v1` (layer=overlay) para criar a surface de highlight. Ou usar `QWindow` com `Qt::WindowStaysOnTopHint` e torcer que o compositor respeite. **Recomendação**: implementar layer shell nativo para garantia, mas começar com `Qt::WindowStaysOnTopHint` como MVP. Se não funcionar, escalar para layer shell.
- **Pesquisa crítica #3**: Como obter posição absoluta do cursor? Wayland não expõe. Soluções:
  a) Acumular deltas relativos do próprio device (drift ao longo do tempo).
  b) Usar `wl_fixed` do `wl_pointer` (se o app receber focus) — mas o app não tem foco quando o usuário está em outro lugar.
  c) Usar `zwlr_input_inhibit_manager` (não ajuda na posição).
  d) **Não precisamos da posição absoluta do sistema**. O overlay pode ser um **cursor custom** que o próprio waymouse desenha e move de acordo com os deltas que ele mesmo lê. Ou seja: o waymouse se torna o "cursor de shake". Mas o cursor real do sistema continua embaixo. O usuário vê dois cursores? Não, se o real for escondido. Mas esconder o cursor real é trabalho do compositor.
  
  **Alternativa brilhante**: Não precisamos de overlay. Podemos usar o **protocolo de cursor** do Wayland. O cliente pode setar a surface do cursor para o `wl_pointer`. Se o compositor permitir, o waymouse pode oferecer uma **surface de cursor temporária e maior** quando o shake é detectado. Mas o waymouse precisa ser o "cliente focado" para ter o `wl_pointer`. Não é o caso.
  
  **Conclusão**: A solução mais robusta é **layer shell overlay** posicionada onde o waymouse *calcula* que o cursor está (acumulando deltas). Para o cálculo de posição, usar os mesmos deltas que o compositor recebe. Como o mouse envia os mesmos eventos para todos os clientes que escutam, o waymouse acumula a mesma posição. Inicializar a posição no centro da tela (ou detectar via `wlr_output_manager` + cursor pos se algum protocolo permitir). Na prática, o drift pode ser aceitável para um overlay de 1-2 segundos.
  
  **Ou mais simples**: O overlay não precisa seguir o cursor com precisão milimétrica. Ele pode ser um **círculo grande no centro da tela** ou próximo à última posição conhecida. Ou simplesmente **trocar o cursor theme para um maior** via `XCURSOR_SIZE` (se o compositor reagir). Mas isso não é "shake to find", é "config de cursor".
  
  **Recomendação de Harvey**: decidir entre:
  1. **Overlay layer shell com tracking aproximado** (MVP funcional, drift aceitável).
  2. **Aumento temporário do cursor theme** (se o compositor suportar hot-reload). Não é shake-specific, mas resolve o problema.
  3. **Integração com compositor** (requer mudança no Mango/Niri). Não é viável.
  
  A **opção 1** é a que realmente entrega "shake to find". Mike pode começar com tracking de deltas via `evdev`/`libinput` e overlay Qt/LayerShell.

- **Decisão de threshold**: shake é definido por velocidade instantânea (mm/s) ou por padrão de movimento (zig-zag)? macOS usa zig-zag. Zig-zag é mais complexo de detectar. Velocidade bruta é mais simples e pode ser suficiente. Propor começar com velocidade bruta (threshold de pixels/segundo) e evoluir para padrão.
