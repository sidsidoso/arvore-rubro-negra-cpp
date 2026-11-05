# Árvore Rubro-Negra em C++

[![CI](https://github.com/sidsidoso/arvore-rubro-negra-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/sidsidoso/arvore-rubro-negra-cpp/actions/workflows/ci.yml)
![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
[![Licenca: MIT](https://img.shields.io/badge/licen%C3%A7a-MIT-green.svg)](LICENSE)

Implementação header-only de uma **árvore rubro-negra genérica** (`ArvoreRubroNegra<Chave, Valor, Comparador>`) em C++17, funcionando como um mapa ordenado chave → valor no estilo do `std::map`. Segue o Cormen (nó sentinela `NIL`, ponteiro para o pai, inserção com recoloração/rotações e remoção com *fix-up* completo), tem iterador em ordem compatível com `for` por intervalo, destruição iterativa sem vazamentos e uma função `validar()` que confere todos os invariantes nos testes. Projeto da disciplina de Estrutura de Dados.

## Sumário

- [Conceito](#conceito)
- [Inserção: os três casos](#inserção-os-três-casos)
- [Remoção: os quatro casos](#remoção-os-quatro-casos)
- [Complexidade](#complexidade)
- [Rubro-negra × AVL](#rubro-negra--avl)
- [Decisões de implementação e otimizações](#decisões-de-implementação-e-otimizações)
- [Estrutura de pastas](#estrutura-de-pastas)
- [Como compilar, executar e testar](#como-compilar-executar-e-testar)
- [Exemplo de saída](#exemplo-de-saída)
- [Referências](#referências)

## Conceito

Uma árvore rubro-negra é uma árvore binária de busca em que cada nó tem uma **cor** (vermelho ou preto) e valem cinco propriedades:

1. todo nó é vermelho ou preto;
2. a raiz é preta;
3. toda folha (`NIL`) é preta;
4. um nó vermelho nunca tem filho vermelho;
5. para cada nó, todos os caminhos até as folhas descendentes têm o **mesmo número de nós pretos** (a *altura negra*).

Pelas propriedades 4 e 5, o caminho mais longo (alternando vermelho e preto) tem no máximo o dobro de nós do mais curto (só pretos). Daí sai o limite **altura ≤ 2 · log2(n + 1)**, e busca, inserção e remoção custam O(log n), mesmo com entradas ordenadas.

Todas as folhas são representadas por **um único nó sentinela** preto, que também é o "pai" da raiz:

```
                  7(P)
               /        \
           2(V)          11(V)
          /    \        /     \
       1(P)    5(P)   8(P)    14(P)
       /  \    /  \   /  \    /   \
     NIL NIL 4(V) NIL NIL NIL NIL 15(V)
             /  \                 /  \
           NIL  NIL             NIL  NIL      todos os NIL são o mesmo nó

   altura negra (contando a raiz, sem o NIL): 2 em todos os caminhos
```

## Inserção: os três casos

O nó novo `z` entra como numa BST comum e é pintado de **vermelho** (não altera a altura negra). Só a propriedade 4 pode quebrar: quando o pai de `z` também é vermelho. A correção olha para o **tio** (irmão do pai). Abaixo, o pai é filho esquerdo do avô; o outro lado é simétrico.

**Caso 1: tio vermelho → recolorir e subir.** O avô vira vermelho, pai e tio viram pretos; o problema pode ter subido dois níveis, então `z = avô` e o laço continua.

```
           avo(P)                         avo(V)   <- novo z
          /      \                       /      \
      pai(V)    tio(V)       ==>     pai(P)    tio(P)
       /                              /
     z(V)                           z(V)
```

**Caso 2: tio preto e `z` em "triângulo" → rotação no pai.** Rotaciona o pai para alinhar os três nós; o antigo pai passa a ser o `z` e cai no caso 3.

```
           avo(P)                         avo(P)
          /      \                       /      \
      pai(V)    tio(P)       ==>      z(V)     tio(P)
          \                           /
          z(V)                    pai(V)   <- novo z
```

**Caso 3: tio preto e `z` em "linha" → recolorir e rotacionar o avô.** Termina a correção (no máximo 2 rotações por inserção).

```
           avo(P)                          pai(P)
          /      \                        /      \
      pai(V)    tio(P)       ==>       z(V)     avo(V)
       /                                            \
     z(V)                                          tio(P)
```

No fim, a raiz é pintada de preto. O exemplo da figura 13.4 do Cormen (inserir 4 em `11 2 14 1 7 15 5 8`) passa pelos três casos e está nos testes e na demonstração.

## Remoção: os quatro casos

Se o nó que sai fisicamente da árvore era **preto**, o nó `x` que ocupa o lugar dele fica com um "preto extra". Se `x` for vermelho, basta pintá-lo de preto; senão, analisa-se o irmão `w` (com `x` filho esquerdo; o outro lado é simétrico):

| Caso | Situação                                          | Ação                                                           | Continua? |
|------|---------------------------------------------------|----------------------------------------------------------------|-----------|
| 1    | `w` vermelho                                      | troca cores de `w` e pai, rotação à esquerda no pai            | vira 2, 3 ou 4 |
| 2    | `w` preto, filhos de `w` pretos                   | pinta `w` de vermelho; o preto extra sobe para o pai           | sobe      |
| 3    | `w` preto, filho de dentro vermelho, de fora preto | troca cores, rotação à direita em `w`                          | vira 4    |
| 4    | `w` preto, filho de fora vermelho                 | `w` herda a cor do pai, pai e filho de fora pretos, rotação no pai | termina |

Uma remoção faz no máximo **3 rotações**; as recolorações do caso 2 podem subir até a raiz (O(log n)).

## Complexidade

| Operação                         | Médio    | Pior     | Observação                                         |
|----------------------------------|----------|----------|----------------------------------------------------|
| `buscar` / `contem`              | O(log n) | O(log n) | iterativo                                          |
| `inserir`                        | O(log n) | O(log n) | no máximo 2 rotações                               |
| `remover`                        | O(log n) | O(log n) | no máximo 3 rotações                               |
| `minimo` / `maximo`              | O(log n) | O(log n) |                                                    |
| `sucessor` / `antecessor`        | O(log n) | O(log n) | a chave consultada não precisa existir             |
| iterador `++`                    | O(1) amortizado | O(log n) | sucessor via ponteiro para o pai           |
| percurso completo (`begin..end`) | O(n)     | O(n)     | sem pilha auxiliar                                 |
| `tamanho` / `vazia`              | O(1)     | O(1)     | contador mantido na estrutura                      |
| `altura_negra`                   | O(log n) | O(log n) | desce um caminho só (propriedade 5)                |
| `altura`                         | O(n)     | O(n)     | não é armazenada (não é necessária ao algoritmo)   |
| `validar`                        | O(n)     | O(n)     | usado nos testes                                   |
| cópia                            | O(n)     | O(n)     | preserva forma e cores                             |
| destruição / `limpar`            | O(n)     | O(n)     | iterativa, memória extra O(1)                      |
| movimento / `trocar`             | O(1)     | O(1)     | `noexcept`                                         |
| **Espaço**                       | O(n)     | O(n)     | 3 ponteiros + 1 byte de cor por nó                 |

## Rubro-negra × AVL

Comparação com a [árvore AVL](https://github.com/sidsidoso/arvore-avl-cpp) do mesmo curso:

|                               | Rubro-negra                        | AVL                                      |
|-------------------------------|------------------------------------|------------------------------------------|
| Altura máxima                 | 2 · log2(n + 1)                    | ≈ 1,44 · log2(n + 2)                     |
| Informação extra por nó       | 1 bit de cor (+ ponteiro pai aqui) | altura (ou fator de balanceamento)       |
| Rotações por inserção         | ≤ 2                                | ≤ 1 (simples ou dupla)                   |
| Rotações por remoção          | **≤ 3**                            | até O(log n)                             |
| Trabalho de rebalanceamento   | O(1) amortizado por atualização    | pode subir até a raiz com rotações       |
| Buscas                        | um pouco mais lentas (mais alta)   | um pouco mais rápidas (mais baixa)       |
| Melhor para                   | muitas inserções/remoções          | muitas consultas                         |
| Usada em                      | `std::map`/`std::set` (libstdc++, libc++), `TreeMap` do Java, kernel Linux | conjuntos com consultas muito mais frequentes que atualizações |

Na prática, a rubro-negra troca um pouco de altura por **menos reestruturação**, o que a torna a escolha padrão das bibliotecas. Na demonstração, 1.000.000 de chaves crescentes geram altura 37 (o limite é 39); a AVL, com a mesma entrada, chegaria a 20.

## Decisões de implementação e otimizações

- **Sentinela `NIL` compartilhado e imutável (estilo Cormen).** Em vez de testar `nullptr` em todo lugar, todas as folhas e o pai da raiz apontam para um único nó preto. Isso deixa o código das correções idêntico ao pseudocódigo do livro (`w->esq->cor` funciona mesmo quando o filho é folha). Diferença importante: o Cormen escreve em `nil.pai` durante a remoção; aqui o **pai de `x` é passado explicitamente** para o *fix-up*, e rotações/transplante nunca escrevem no sentinela. Assim ele pode ser um único `inline static` por instanciação do template, compartilhado por todas as árvores (inclusive em threads diferentes), e o movimento da árvore continua O(1) e `noexcept` (com um sentinela por árvore, mover exigiria realocá-lo ou religar todas as folhas).
- **`NoBase` + `No`**: o sentinela só tem a parte estrutural (cor e ponteiros), então `Chave` e `Valor` **não precisam de construtor padrão**.
- **Nó guarda `std::pair<const Chave, Valor>`**, como o `std::map`: o iterador devolve uma referência de verdade, `operator->` funciona e `for (auto& [k, v] : arvore)` permite alterar o valor mas não a chave.
- **Remoção sem copiar chave/valor**: com dois filhos, o nó sucessor é desencaixado e religado no lugar do removido (só ponteiros mudam). Iteradores para outros elementos continuam válidos e tipos somente-móveis (`std::unique_ptr`) funcionam como valor.
- **Destruição iterativa com rotações**: enquanto o nó atual tem filho esquerdo, gira-se à direita; quando não tem, ele é apagado e segue-se para a direita. O(n) de tempo, O(1) de memória extra, nenhuma recursão.
- **Regra dos 5**: cópia profunda (copia-e-troca na atribuição, garantia forte); se a cópia de um valor lançar exceção no meio, a parte já copiada é liberada (testado com um tipo que conta instâncias vivas); movimento `noexcept` deixando a origem vazia e válida.
- **Comparador como parâmetro de template** (padrão `std::less<Chave>`); só `<` é usado, nunca `==`.
- **`inserir` por valor + `std::move`** e atualização do valor se a chave já existir (como `insert_or_assign`), sem nenhuma rotação nesse caso.
- **`validar()`** confere, em uma passada: sentinela e raiz pretas, nenhum vermelho com filho vermelho, mesma altura negra em todos os caminhos, ordem estrita com limites herdados (não só pai-filho), ponteiros `pai` coerentes e contagem igual a `tamanho()`.

## Estrutura de pastas

```
arvore-rubro-negra-cpp/
├── include/
│   └── arvore_rubro_negra.hpp      # implementação (header-only)
├── src/
│   └── main.cpp                    # demonstração
├── tests/
│   └── test_arvore_rubro_negra.cpp # testes (assert + validar() + std::map como oráculo)
├── .github/workflows/ci.yml        # CI: testes com sanitizers + demo
├── Makefile
├── LICENSE
└── README.md
```

## Como compilar, executar e testar

### Linux / macOS (make)

```bash
make            # compila demo e testes em build/
make run        # executa a demonstração
make test       # executa os testes
make test SANITIZE=1   # testes com AddressSanitizer + UndefinedBehaviorSanitizer
make clean
```

### Windows (g++ do MinGW-w64 / WinLibs)

```bat
mkdir build
g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude src/main.cpp -o build/demo.exe
g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude tests/test_arvore_rubro_negra.cpp -o build/testes.exe
build\demo.exe
build\testes.exe
```

Com `mingw32-make` também funcionam `mingw32-make run` e `mingw32-make test` (o alvo `clean` usa `rm`, disponível no Git Bash).

### Uso

```cpp
#include "arvore_rubro_negra.hpp"

ArvoreRubroNegra<std::string, int> idades;
idades.inserir("ana", 20);
idades.inserir("bruno", 31);
if (int* v = idades.buscar("ana")) ++*v;
idades.remover("bruno");
for (const auto& [nome, idade] : idades) std::cout << nome << '=' << idade << '\n';
assert(idades.validar());
```

## Exemplo de saída

`make run` (a árvore é desenhada "deitada": raiz na coluna da esquerda, filho direito acima e esquerdo abaixo; `V` = vermelho, `P` = preto):

```
Insercao de 1..10 em ordem crescente
tamanho: 10, altura: 5, altura negra: 3 (uma BST sem balanceamento teria altura 10)
arvore (deitada, raiz a esquerda; V = vermelho, P = preto):
                10(V)
            9(P)
        8(V)
            7(P)
    6(P)
        5(P)
4(P)
        3(P)
    2(P)
        1(P)

Cormen, fig. 13.4 - antes de inserir 4:
        15(V)
    14(P)
11(P)
            8(V)
        7(P)
            5(V)
    2(V)
        1(P)
depois de inserir 4 (recoloracao + rotacao dupla):
            15(V)
        14(P)
    11(V)
        8(P)
7(P)
        5(P)
            4(V)
    2(V)
        1(P)

buscar(5) -> valor 5
contem(42)? false
minimo: 1, maximo: 10, sucessor(7): 8

Apos remover 4, 1, 2 e 3:
        10(V)
    9(P)
8(P)
        7(P)
    6(V)
        5(P)
altura: 3, invariantes ok? true

Em ordem: 5="valor 5" 6="valor 6" 7="valor 7" 8="valor 8" 9="valor 9" 10="valor 10"
Frequencia: caju:1 maca:1 pera:2 uva:3
1.000.000 chaves crescentes -> altura 37, altura negra 19 (limite 2*log2(n+1) = 39), valida? true
```

`make test`:

```
[ok] arvore vazia
[ok] um elemento
[ok] casos de insercao (recoloracao e rotacoes)
[ok] chave duplicada atualiza valor
[ok] casos de remocao (fix-up 1 a 4)
[ok] todas as permutacoes de 8 chaves
[ok] minimo, maximo, sucessor e antecessor
[ok] iterador em ordem
[ok] sequencias ordenadas respeitam 2*log2(n+1)
[ok] 200 mil operacoes aleatorias vs std::map
[ok] copia e movimento
[ok] sem vazamentos (RAII e excecao na copia)
[ok] comparador customizado e valor somente movel
[ok] limpar
14 testes passaram
```

Os testes conferem a forma e as cores exatas após cada caso clássico de inserção e de remoção (incluindo a figura 13.4 do Cormen), inserem e removem as **40.320 permutações** de 8 chaves validando a árvore a cada passo, inserem 100 mil chaves crescentes e decrescentes checando o limite 2 · log2(n + 1), executam **200 mil operações aleatórias** comparadas com `std::map` como oráculo e usam um tipo que conta instâncias para garantir que nada vaza, nem quando a cópia lança exceção no meio.

## Referências

- CORMEN, T. H.; LEISERSON, C. E.; RIVEST, R. L.; STEIN, C. *Algoritmos: Teoria e Prática*. 3. ed. Elsevier, 2012. Capítulo 13 (Árvores Vermelho-Preto).
- GUIBAS, L. J.; SEDGEWICK, R. A dichromatic framework for balanced trees. *19th Annual Symposium on Foundations of Computer Science*, p. 8-21, 1978.
- BAYER, R. Symmetric binary B-Trees: data structure and maintenance algorithms. *Acta Informatica*, v. 1, p. 290-306, 1972.
- SEDGEWICK, R.; WAYNE, K. *Algorithms*. 4. ed. Addison-Wesley, 2011. Seção 3.3 (Balanced Search Trees).
- SZWARCFITER, J. L.; MARKENZON, L. *Estruturas de Dados e Seus Algoritmos*. 3. ed. LTC, 2010.
- cppreference: [std::map](https://en.cppreference.com/w/cpp/container/map), [regra de três/cinco/zero](https://en.cppreference.com/w/cpp/language/rule_of_three).

## Licença

Distribuído sob a licença MIT. Veja [LICENSE](LICENSE).
