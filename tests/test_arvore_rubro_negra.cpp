// Testes da ArvoreRubroNegra. Usa assert, validar() e std::map como oraculo.
#undef NDEBUG
#include <algorithm>
#include <cassert>
#include <cmath>
#include <functional>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "arvore_rubro_negra.hpp"

namespace {

int testes_passados = 0;

template <class F>
void executar(const char* nome, F teste) {
    teste();
    ++testes_passados;
    std::cout << "[ok] " << nome << '\n';
}

using Arvore = ArvoreRubroNegra<int, int>;

// Limite teorico da rubro-negra: altura <= 2 * log2(n + 1).
bool altura_dentro_do_limite(int altura, std::size_t n) {
    return altura <= static_cast<int>(2.0 * std::log2(static_cast<double>(n) + 1.0));
}

// Forma da arvore como lista (chave, vermelho, profundidade) em pre-ordem.
using Forma = std::vector<std::tuple<int, bool, int>>;
constexpr bool V = true;   // vermelho
constexpr bool P = false;  // preto

Forma forma(const Arvore& a) {
    Forma f;
    a.pre_ordem([&f](int k, bool vermelho, int prof) { f.emplace_back(k, vermelho, prof); });
    return f;
}

Arvore construir(std::initializer_list<int> chaves) {
    Arvore a;
    for (int k : chaves) {
        assert(a.inserir(k, k * 10));
        assert(a.validar());
    }
    return a;
}

template <class K, class Val, class C>
bool igual(const ArvoreRubroNegra<K, Val, C>& a, const std::map<K, Val, C>& m) {
    return a.tamanho() == m.size() && std::equal(a.begin(), a.end(), m.begin(), m.end());
}

// Tipo que conta instancias vivas e pode lancar na copia (testa RAII).
struct Rastreado {
    static int vivos;
    static int copias_ate_falhar;  // < 0: nunca falha
    int v;

    explicit Rastreado(int x = 0) : v(x) { ++vivos; }
    Rastreado(const Rastreado& o) : v(o.v) {
        if (copias_ate_falhar == 0) throw std::runtime_error("copia falhou");
        if (copias_ate_falhar > 0) --copias_ate_falhar;
        ++vivos;
    }
    Rastreado(Rastreado&& o) noexcept : v(o.v) { ++vivos; }
    Rastreado& operator=(const Rastreado&) = default;
    Rastreado& operator=(Rastreado&&) noexcept = default;
    ~Rastreado() { --vivos; }
};
int Rastreado::vivos = 0;
int Rastreado::copias_ate_falhar = -1;

// ------------------------------------------------------------------ testes

void teste_vazia() {
    Arvore a;
    assert(a.vazia() && a.tamanho() == 0 && a.altura() == 0 && a.altura_negra() == 0);
    assert(a.buscar(1) == nullptr && !a.contem(1));
    assert(!a.remover(1));
    assert(a.minimo() == nullptr && a.maximo() == nullptr);
    assert(a.sucessor(0) == nullptr && a.antecessor(0) == nullptr);
    assert(a.begin() == a.end() && a.cbegin() == a.cend());
    assert(a.validar());
    assert(a.desenhar().empty());
}

void teste_um_elemento() {
    Arvore a;
    assert(a.inserir(5, 50));
    assert(a.tamanho() == 1 && a.altura() == 1 && a.altura_negra() == 1 && a.validar());
    assert(forma(a) == (Forma{{5, P, 0}}));  // a raiz e sempre preta
    assert(*a.buscar(5) == 50);
    assert(*a.minimo() == 5 && *a.maximo() == 5);
    assert(a.remover(5));
    assert(a.vazia() && a.altura() == 0 && a.validar());
}

// Casos classicos do fix-up de insercao (e seus simetricos).
void teste_casos_insercao() {
    // Caso 1: tio vermelho -> so recoloracao (a raiz volta a ser preta).
    assert(forma(construir({10, 5, 15, 1})) ==
           (Forma{{10, P, 0}, {5, P, 1}, {1, V, 2}, {15, P, 1}}));
    assert(forma(construir({10, 5, 15, 20})) ==
           (Forma{{10, P, 0}, {5, P, 1}, {15, P, 1}, {20, V, 2}}));
    // Caso 3: "linha" -> uma rotacao no avo.
    assert(forma(construir({10, 5, 1})) == (Forma{{5, P, 0}, {1, V, 1}, {10, V, 1}}));
    assert(forma(construir({1, 5, 10})) == (Forma{{5, P, 0}, {1, V, 1}, {10, V, 1}}));
    // Caso 2 -> 3: "triangulo" -> rotacao dupla.
    assert(forma(construir({10, 5, 7})) == (Forma{{7, P, 0}, {5, V, 1}, {10, V, 1}}));
    assert(forma(construir({10, 15, 12})) == (Forma{{12, P, 0}, {10, V, 1}, {15, V, 1}}));

    // Exemplo do Cormen (fig. 13.4): inserir 4 dispara caso 1, depois 2 e 3.
    Arvore a = construir({11, 2, 14, 1, 7, 15, 5, 8});
    assert(forma(a) == (Forma{{11, P, 0}, {2, V, 1}, {1, P, 2}, {7, P, 2}, {5, V, 3},
                              {8, V, 3}, {14, P, 1}, {15, V, 2}}));
    a.inserir(4, 40);
    assert(a.validar());
    assert(forma(a) == (Forma{{7, P, 0}, {2, V, 1}, {1, P, 2}, {5, P, 2}, {4, V, 3},
                              {11, V, 1}, {8, P, 2}, {14, P, 2}, {15, V, 3}}));
}

void teste_duplicados_atualizam() {
    Arvore a = construir({3, 1, 2});
    const Forma antes = forma(a);
    assert(!a.inserir(2, 99));
    assert(a.tamanho() == 3 && *a.buscar(2) == 99);
    assert(forma(a) == antes);  // nada foi rotacionado/recolorido
}

void teste_casos_remocao() {
    // Folha vermelha: sem fix-up.
    Arvore a = construir({10, 5, 15, 1});
    assert(a.remover(1) && a.validar());
    assert(forma(a) == (Forma{{10, P, 0}, {5, P, 1}, {15, P, 1}}));

    // Folha preta com irmao preto e sobrinhos pretos (caso 2): recolore o irmao.
    assert(a.remover(5) && a.validar());
    assert(forma(a) == (Forma{{10, P, 0}, {15, V, 1}}));

    // Folha preta com irmao preto e sobrinho "de fora" vermelho (caso 4).
    Arvore b = construir({10, 5, 15, 20});
    assert(b.remover(5) && b.validar());
    assert(forma(b) == (Forma{{15, P, 0}, {10, P, 1}, {20, P, 1}}));

    // Sobrinho "de dentro" vermelho (caso 3 -> 4).
    Arvore c = construir({10, 5, 15, 12});
    assert(c.remover(5) && c.validar());
    assert(forma(c) == (Forma{{12, P, 0}, {10, P, 1}, {15, P, 1}}));

    // Irmao vermelho (caso 1 -> 2).
    Arvore d = construir({10, 5, 20, 15, 25, 30});
    d.remover(30);
    assert(forma(d) == (Forma{{10, P, 0}, {5, P, 1}, {20, V, 1}, {15, P, 2}, {25, P, 2}}));
    assert(d.remover(5) && d.validar());
    assert(forma(d) == (Forma{{20, P, 0}, {10, P, 1}, {15, V, 2}, {25, P, 1}}));

    // Raiz com dois filhos: o sucessor e religado no lugar dela.
    Arvore e = construir({2, 1, 3});
    assert(e.remover(2) && e.validar());
    assert(forma(e) == (Forma{{3, P, 0}, {1, V, 1}}));
    assert(*e.buscar(1) == 10 && *e.buscar(3) == 30);

    // Chave inexistente.
    assert(!e.remover(42) && e.tamanho() == 2);
}

// Todas as 8! ordens de insercao, removendo em uma ordem diferente a cada vez:
// percorre exaustivamente todos os casos do fix-up de insercao e remocao.
void teste_permutacoes_exaustivas() {
    std::vector<int> ordem(8);
    std::iota(ordem.begin(), ordem.end(), 1);
    std::mt19937 rng(7);
    int arvores = 0;
    do {
        Arvore a;
        for (int k : ordem) a.inserir(k, k);
        assert(a.validar() && a.tamanho() == 8);
        assert(altura_dentro_do_limite(a.altura(), a.tamanho()));
        std::vector<int> remocao = ordem;
        std::shuffle(remocao.begin(), remocao.end(), rng);
        for (int k : remocao) {
            assert(a.remover(k));
            assert(a.validar() && !a.contem(k));
        }
        assert(a.vazia());
        ++arvores;
    } while (std::next_permutation(ordem.begin(), ordem.end()));
    assert(arvores == 40320);
}

void teste_min_max_sucessor() {
    Arvore a = construir({50, 20, 80, 10, 30, 70, 90});
    assert(*a.minimo() == 10 && *a.maximo() == 90);
    assert(*a.sucessor(30) == 50 && *a.sucessor(35) == 50 && *a.sucessor(5) == 10);
    assert(a.sucessor(90) == nullptr && a.sucessor(1000) == nullptr);
    assert(*a.antecessor(50) == 30 && *a.antecessor(55) == 50 && *a.antecessor(95) == 90);
    assert(a.antecessor(10) == nullptr);
}

void teste_iterador() {
    Arvore a = construir({5, 3, 8, 1, 4, 7, 9, 2, 6});
    std::vector<int> chaves;
    for (const auto& [k, v] : a) {
        chaves.push_back(k);
        assert(v == k * 10);
    }
    assert((chaves == std::vector<int>{1, 2, 3, 4, 5, 6, 7, 8, 9}));
    assert(std::distance(a.begin(), a.end()) == 9);

    for (auto& par : a) par.second += 1;  // valor alteravel pelo iterador
    assert(*a.buscar(4) == 41);

    Arvore::const_iterator c = a.begin();  // conversao iterator -> const_iterator
    assert(c->first == 1);
    Arvore::const_iterator anterior = c++;
    assert(anterior->first == 1 && c->first == 2);
    static_assert(std::is_same_v<decltype(*a.cbegin()), const std::pair<const int, int>&>);
    static_assert(!std::is_assignable_v<decltype(a.begin()->first), int>);  // chave imutavel

    // Iterador continua valido apos remover OUTRO elemento.
    auto it = a.begin();
    ++it;  // aponta para 2
    a.remover(1);
    a.remover(9);
    assert(it->first == 2);
    std::vector<int> resto;
    for (; it != a.end(); ++it) resto.push_back(it->first);
    assert((resto == std::vector<int>{2, 3, 4, 5, 6, 7, 8}));
}

void teste_sequencial_balanceado() {
    const int n = 100000;
    for (bool crescente : {true, false}) {
        Arvore a;
        for (int i = 0; i < n; ++i) {
            const int k = crescente ? i : n - i;
            a.inserir(k, k);
            if ((i + 1) % 10000 == 0) {
                assert(a.validar());
                assert(altura_dentro_do_limite(a.altura(), a.tamanho()));
            }
        }
        assert(a.tamanho() == static_cast<std::size_t>(n));
        assert(a.altura() <= 2 * 17);  // 2 * log2(100001) ~ 33.2
        // Remove metade (os pares) e confere de novo.
        for (int i = 0; i <= n; i += 2) a.remover(i);
        assert(a.validar() && altura_dentro_do_limite(a.altura(), a.tamanho()));
        for (int i = 1; i <= n; i += 2) assert(a.remover(i));
        assert(a.vazia() && a.validar());
    }
}

void teste_aleatorio_oraculo() {
    std::mt19937 rng(20261105);
    std::uniform_int_distribution<int> chave(0, 4999);
    std::uniform_int_distribution<int> operacao(0, 9);
    Arvore a;
    std::map<int, int> m;
    for (int passo = 0; passo < 200000; ++passo) {
        const int k = chave(rng);
        const int op = operacao(rng);
        if (op < 4) {
            const bool nova = m.find(k) == m.end();
            assert(a.inserir(k, passo) == nova);
            m[k] = passo;
        } else if (op < 8) {
            assert(a.remover(k) == (m.erase(k) == 1));
        } else if (op == 8) {
            const int* v = a.buscar(k);
            auto it = m.find(k);
            assert((v == nullptr) == (it == m.end()));
            if (v) assert(*v == it->second);
        } else {
            const int* s = a.sucessor(k);
            auto it = m.upper_bound(k);
            assert((s == nullptr) == (it == m.end()));
            if (s) assert(*s == it->first);
            const int* p = a.antecessor(k);
            auto jt = m.lower_bound(k);
            assert((p == nullptr) == (jt == m.begin()));
            if (p) assert(*p == std::prev(jt)->first);
        }
        assert(a.tamanho() == m.size());
        if (passo % 1000 == 0) {
            assert(a.validar());
            assert(altura_dentro_do_limite(a.altura(), a.tamanho()));
            if (!m.empty()) assert(*a.minimo() == m.begin()->first && *a.maximo() == m.rbegin()->first);
        }
    }
    assert(a.validar() && igual(a, m));
}

void teste_copia_movimento() {
    Arvore a = construir({4, 2, 6, 1, 3, 5, 7});
    Arvore b(a);
    assert(b.validar() && forma(b) == forma(a));
    b.remover(4);
    b.inserir(100, 1);
    assert(a.tamanho() == 7 && a.contem(4) && !a.contem(100));  // copia profunda

    Arvore c(std::move(b));
    assert(c.tamanho() == 7 && c.contem(100) && c.validar());
    assert(b.vazia() && b.validar());  // origem movida: vazia e utilizavel
    b.inserir(1, 1);
    assert(b.tamanho() == 1 && b.validar());

    Arvore d;
    d = a;
    assert(forma(d) == forma(a));
    const Arvore& ref = d;
    d = ref;  // autoatribuicao
    assert(d.tamanho() == 7 && d.validar());

    d = std::move(c);
    assert(d.contem(100) && c.vazia());

    a.trocar(d);
    assert(a.contem(100) && d.contem(4) && !d.contem(100));

    static_assert(std::is_nothrow_move_constructible_v<Arvore>);
    static_assert(std::is_nothrow_move_assignable_v<Arvore>);
}

void teste_sem_vazamentos() {
    {
        ArvoreRubroNegra<int, Rastreado> a;
        for (int i = 0; i < 1000; ++i) a.inserir(i, Rastreado(i));
        assert(Rastreado::vivos == 1000);
        for (int i = 0; i < 1000; i += 3) a.remover(i);
        assert(Rastreado::vivos == static_cast<int>(a.tamanho()));
        {
            auto copia = a;
            assert(Rastreado::vivos == 2 * static_cast<int>(a.tamanho()));
        }
        // Copia que falha no meio: nada pode vazar e a original fica intacta.
        Rastreado::copias_ate_falhar = 300;
        bool lancou = false;
        try {
            auto copia = a;
        } catch (const std::runtime_error&) {
            lancou = true;
        }
        Rastreado::copias_ate_falhar = -1;
        assert(lancou);
        assert(Rastreado::vivos == static_cast<int>(a.tamanho()) && a.validar());
        a.limpar();
        assert(Rastreado::vivos == 0 && a.vazia());
        for (int i = 0; i < 100; ++i) a.inserir(i, Rastreado(i));
    }
    assert(Rastreado::vivos == 0);  // destrutor liberou tudo

    // Destruicao iterativa de uma arvore grande (sem recursao).
    auto grande = std::make_unique<Arvore>();
    for (int i = 0; i < 300000; ++i) grande->inserir(i, i);
    grande.reset();
}

void teste_comparador_e_valor_movel() {
    ArvoreRubroNegra<int, std::unique_ptr<std::string>, std::greater<int>> a;
    for (int i = 1; i <= 50; ++i) a.inserir(i, std::make_unique<std::string>(std::to_string(i)));
    assert(a.validar() && *a.minimo() == 50 && *a.maximo() == 1);  // ordem invertida
    assert(*a.sucessor(10) == 9);  // "maior" segundo std::greater
    assert(**a.buscar(7) == "7");
    for (int i = 1; i <= 50; i += 2) a.remover(i);
    assert(a.validar() && a.tamanho() == 25);
    int esperado = 50;
    for (const auto& [k, v] : a) {
        assert(k == esperado && *v == std::to_string(k));
        esperado -= 2;
    }

    ArvoreRubroNegra<std::string, int> palavras;
    for (const char* p : {"uva", "banana", "abacate", "caju", "banana"}) palavras.inserir(p, 1);
    assert(palavras.tamanho() == 4 && *palavras.minimo() == "abacate");
    assert(palavras.desenhar().find("(P)") != std::string::npos);
}

void teste_limpar() {
    Arvore a = construir({1, 2, 3, 4, 5});
    a.limpar();
    assert(a.vazia() && a.altura() == 0 && a.validar() && a.begin() == a.end());
    a.inserir(1, 1);
    assert(a.tamanho() == 1 && a.validar());
}

}  // namespace

int main() {
    executar("arvore vazia", teste_vazia);
    executar("um elemento", teste_um_elemento);
    executar("casos de insercao (recoloracao e rotacoes)", teste_casos_insercao);
    executar("chave duplicada atualiza valor", teste_duplicados_atualizam);
    executar("casos de remocao (fix-up 1 a 4)", teste_casos_remocao);
    executar("todas as permutacoes de 8 chaves", teste_permutacoes_exaustivas);
    executar("minimo, maximo, sucessor e antecessor", teste_min_max_sucessor);
    executar("iterador em ordem", teste_iterador);
    executar("sequencias ordenadas respeitam 2*log2(n+1)", teste_sequencial_balanceado);
    executar("200 mil operacoes aleatorias vs std::map", teste_aleatorio_oraculo);
    executar("copia e movimento", teste_copia_movimento);
    executar("sem vazamentos (RAII e excecao na copia)", teste_sem_vazamentos);
    executar("comparador customizado e valor somente movel", teste_comparador_e_valor_movel);
    executar("limpar", teste_limpar);
    std::cout << testes_passados << " testes passaram\n";
    return 0;
}
