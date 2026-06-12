# coderules.md

## Estilo C++

- **Padrão**: C++20
- **CamelCase** para classes e structs: `CompositorDetector`, `DeviceManager`
- **snake_case** para funções e variáveis: `apply_config`, `device_name`
- **m_** prefixo para membros privados: `m_backend`, `m_devices`
- **SCREAMING_SNAKE_CASE** para macros e constantes
- **Não usar** `using namespace std;` global. `using namespace` permitido apenas em escopo de função pequena.

## Qt6

- Preferir `QString` para UI, `std::string` para lógica core.
- Conectar sinais/slots com sintaxe moderna (pointer-to-member):
  ```cpp
  connect(slider, &QSlider::valueChanged, this, &DevicePanel::onSpeedChanged);
  ```
- Sempre usar `Q_OBJECT` em classes com sinais/slots.
- Widgets raw pointers em hierarquia Qt (parent-child). Smart pointers (`std::unique_ptr`) para objetos sem parent.

## Headers

- Ordem de includes:
  1. Header do próprio componente (se `.cpp`)
  2. Headers do projeto (`src/...`)
  3. Qt headers
  4. Headers de bibliotecas de terceiros
  5. Headers padrão
- Sempre usar `#pragma once` em vez de include guards.

## Ponteiros

- Preferir `std::unique_ptr` para ownership.
- `std::shared_ptr` apenas quando ownership compartilhado for realmente necessário.
- Raw pointers (`T*`) apenas para observação (non-owning).
- `QObject` parent-child já gerencia memória — não envolver em smart pointer.

## Erro

- Usar `std::expected` (C++23) quando disponível. Enquanto isso, retornar `bool` + `std::optional<std::string>` para mensagem de erro.
- Ou usar `Result<T>` simples (ver `src/core/result.hpp` futuro).
- **Não usar exceptions** para controle de fluxo. Exceptions apenas para erros fatais irrecuperáveis.

## Backend

- Todo novo backend deve herdar de `src/backends/backend.hpp`.
- Deve implementar `name()` e `apply()`.
- Deve ter teste mock em `tests/`.

## Commit

- Mensagem em inglês
- Formato: `[area] verbo descrição`
- Áreas: `core`, `gui`, `mango`, `niri`, `libinput`, `build`, `docs`, `test`
- Exemplo: `[gui] add acceleration slider widget`

## Formatação

- Use `clang-format` com estilo baseado em LLVM.
- 4 espaços de indentação.
- Linha máxima 100 caracteres.
- Chaves em nova linha (Allman style):
  ```cpp
  if (condition)
  {
      do_something();
  }
  ```
