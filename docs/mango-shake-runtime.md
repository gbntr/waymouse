# Mango Shake Runtime

## Objetivo

Fazer o Shake to Find iniciar junto com sua sessão Mango, sem precisar abrir a GUI do `waymouse`.

## Setup recomendado

Adicione ao `~/.config/mango/config.conf`:

```conf
exec-once=waymouse --shake-runtime
```

## O que isso faz

- sobe o runtime do Shake to Find no login do Mango
- mantém a feature viva sem abrir a GUI
- permite que a GUI apenas configure e observe o estado

## Estados esperados

- `running`: input + overlay funcionando
- `degraded`: runtime ativo, mas com fallback de overlay ou sem IPC Mango
- `permission_denied`: sem acesso a `/dev/input/event*`
- `stopped`: runtime não está ativo

## Single instance

- O runtime usa `shake-runtime.lock` em `$XDG_RUNTIME_DIR/waymouse/`
- Se já houver outra instância na mesma sessão, a nova sai com erro limpo

## Troubleshooting

### Runtime não inicia
- verifique se `waymouse --shake-runtime` funciona manualmente num terminal do Mango
- confirme que o binário `waymouse` está no `PATH` da sessão

### Runtime inicia mas não detecta shake
- verifique estado de permissão para `/dev/input/event*`
- confira o status publicado em `$XDG_RUNTIME_DIR/waymouse/shake-status.json`

### Overlay cai para fallback
- o Mango/wlroots deve suportar `wlr-layer-shell`
- se o status mostrar `qwindow-fallback`, a inicialização nativa do layer-shell falhou

## Nota operacional

O MVP não altera permissões do sistema automaticamente. Se houver bloqueio de acesso ao input, o runtime informa o erro e depende de correção explícita do ambiente.
