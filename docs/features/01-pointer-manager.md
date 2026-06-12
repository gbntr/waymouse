# 01 — Pointer Manager

## Brief

Gerenciador de aparência do cursor do mouse. Permite ao usuário ajustar o **tamanho** do ponteiro e selecionar o **tema** (cursor theme) diretamente pela GUI do waymouse. As configurações aplicam-se via protocolos Wayland/XDG standard e fallback de env vars.

## Prioridade

**Alta**

## Motivação

Aparência do cursor é uma configuração fundamental de UX. Atualmente, usuários de compositores Wayland precisam editar variáveis de ambiente ou arquivos de config do compositor manualmente. O waymouse deve centralizar isso.

## Escopo

### Incluído
- Seleção de tema de cursor (leitura de temas disponíveis em `/usr/share/icons` e `~/.icons`).
- Slider de tamanho (inteiro, múltiplos de 2, ex: 16, 18, 20, 24, 32, 48, 64).
- Aplicação imediata ao sistema (sem restart).
- Persistência no TOML.

### Excluído
- Preview animado do cursor (nice-to-have, Fase 2).
- Download de temas (fora do escopo do waymouse).
- Configuração de hotspot por cursor.

## Backend

### O que precisa fazer

1. **Detector de temas**: listar diretórios `cursors` dentro de `~/.icons/*` e `/usr/share/icons/*`. Validar que o tema contém cursores (pasta `cursors/` com arquivos `.cursor` ou `.png`/`xmc`).
2. **Aplicação de tema**:
   - Wayland não tem um protocolo único para cursor theme. A forma padrão é via `XCURSOR_THEME` e `XCURSOR_SIZE`.
   - Compositores como Mango (wlroots/dwl) e Niri leem essas variáveis no startup, mas nem todos reagem a runtime.
   - **Mango**: pode precisar de IPC ou enviar sinal para reiniciar o cursor context do wlroots. Pesquisar se Mango/dwl suporta hot-reload de cursor theme.
   - **Niri**: verificar se aceita reload via IPC ou se precisa de `niri msg`.
   - **Fallback**: escrever `~/.config/X11/xresources` ou `~/.Xresources` com `Xcursor.theme` e `Xcursor.size` + rodar `xrdb` (XWayland fallback). Ou mais simples: apenas persistir no env do usuário e explicar que logout/login pode ser necessário.
   - **wlroots genérico**: `zwp_cursor_shape_manager_v1` é para cursor shapes, não para tema. O tema de cursor é decidido pelo cliente (app) ou pelo compositor. O compositor é quem renderiza o cursor quando o app não está fornecendo surface custom. Então: a config pode precisar ser aplicada no **compositor**, não no cliente waymouse.
3. **Persistência TOML**:
   ```toml
   [pointer]
   theme = "Bibata-Modern-Ice"
   size = 24
   ```

### Stubs existentes
- Nenhum. Precisa criar novo módulo `src/core/pointer_manager.cpp`.

## Frontend

### O que precisa fazer

- **ComboBox de tema**: populado com lista de temas detectados. Mostra preview do nome. Filtro de busca (nice-to-have).
- **Slider de tamanho**: `QSlider` com `QSpinBox` lado a lado. Range: 16 a 64, step 2. Label mostrando valor atual.
- **Seção dedicada**: Aba ou grupo na GUI chamado "Aparência" ou "Cursor".
- **Preview**: mostrar um ícone grande do cursor atual (usar `QCursor::pixmap()` ou carregar do tema).

### UX

- Mudança no slider/combo aplica imediatamente (live preview) se o compositor permitir.
- Se não permitir runtime, mostrar badge "Requer logout/login".
- Botão "Restaurar padrão".

## Critérios de Aceite (QA)

- [ ] waymouse detecta todos os temas de cursor disponíveis no sistema.
- [ ] Tema selecionado é salvo no TOML corretamente.
- [ ] Tamanho é salvo e restaurado ao reabrir.
- [ ] Aplicação no compositor Mango funciona (ou documenta limitação).
- [ ] Aplicação no compositor Niri funciona (ou documenta limitação).
- [ ] GUI mostra seção de cursor com tema e tamanho.

## Dependências

- Nenhuma (pode ser a primeira feature).

## Notas para Harvey

- **Pesquisa crítica**: como Mango/dwl (wlroots) gerencia cursor themes? É via `XCURSOR_THEME` no startup do compositor ou existe algum protocolo/sinal para reload? O mesmo para Niri.
- **Decisão**: se nenhum compositor suportar hot-reload de tema, a feature vira "persistência + notificação ao usuário de reiniciar sessão". Isso é aceitável?
- **wlroots**: investigar se `wlr_cursor` lê `XCURSOR_THEME` apenas no startup. Se sim, o backend de aplicação pode ser um "no-op" com mensagem ao usuário.
