# Índice de Features

Este documento lista todas as features planejadas para o `waymouse`, em ordem de prioridade e desenvolvimento.

Cada feature possui um **Brief** inicial. Com base no Brief, **Harvey** elabora a `Spec` completa. Após aprovação da Spec, **Mike** implementa e **Dona** valida.

---

## Ordem de Desenvolvimento

### Fase 1 — Estética & UX

| # | Feature | Status | Prioridade |
|---|---------|--------|------------|
| 01 | [Pointer Manager](01-pointer-manager.md) | `pending` | **Alta** |
| 02 | [Shake to Find](02-shake-to-find.md) | `pending` | **Alta** |

### Fase 2 — Funcional Core

| # | Feature | Status | Prioridade |
|---|---------|--------|------------|
| 03 | [Basic Mouse Config](03-basic-mouse-config.md) | `pending` | **Alta** |
| 04 | [Device Separation](04-device-separation.md) | `pending` | **Média** |

---

## Legenda

- **Status**: `pending` → `spec-in-progress` → `spec-ready` → `in-progress` → `qa` → `done`
- **Prioridade**: `Alta` (bloqueante ou motivo principal do projeto) / `Média` (importante, não urgente) / `Baixa` (nice-to-have)

## Workflow

1. Leia o **Brief** da feature.
2. Harvey escreve a **Spec** detalhada em `specs/FEATURE-N.md` (não versionada, local).
3. Após aprovação do usuário, Mike implementa no `src/`.
4. Dona cria/executa testes de aceite na `tests/`.
5. Atualize este índice com o novo status.
