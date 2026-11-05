// arvore_rubro_negra.hpp - arvore rubro-negra generica (mapa ordenado chave -> valor).
//
// Implementacao no estilo do Cormen (CLRS, cap. 13): cada no tem cor, ponteiro
// para o pai e um no sentinela NIL faz o papel de todas as folhas e do pai da
// raiz. Invariantes mantidos apos toda insercao e remocao:
//   1. todo no e vermelho ou preto;
//   2. a raiz e preta;
//   3. as folhas (NIL) sao pretas;
//   4. um no vermelho nunca tem filho vermelho;
//   5. todos os caminhos de um no ate as folhas tem o mesmo numero de nos pretos.
// Consequencia: altura <= 2 * log2(n + 1), logo todas as operacoes sao O(log n).
//
// Diferenca em relacao ao livro: o sentinela e UNICO por instanciacao do
// template e NUNCA e escrito. O Cormen usa nil.pai como variavel temporaria na
// remocao; aqui o pai de x e carregado explicitamente no fix-up. Assim o
// sentinela pode ser compartilhado por todas as arvores (inclusive em threads
// diferentes) e o movimento de uma arvore e O(1) e noexcept.
#ifndef ARVORE_RUBRO_NEGRA_HPP
#define ARVORE_RUBRO_NEGRA_HPP

#include <cstddef>
#include <functional>
#include <iterator>
#include <sstream>
#include <string>
#include <utility>

template <class Chave, class Valor, class Comparador = std::less<Chave>>
class ArvoreRubroNegra {
    enum class Cor : unsigned char { vermelho, preto };

    // Parte estrutural do no: e tudo o que o sentinela precisa ter.
    struct NoBase {
        Cor cor = Cor::preto;
        NoBase* pai = nullptr;
        NoBase* esq = nullptr;
        NoBase* dir = nullptr;
    };

    struct No : NoBase {
        std::pair<const Chave, Valor> par;

        template <class K, class V>
        No(K&& k, V&& v) : par(std::forward<K>(k), std::forward<V>(v)) {}
    };

    // Sentinela compartilhado (preto, nunca modificado).
    inline static NoBase sentinela_{};
    static NoBase* nil() noexcept { return &sentinela_; }

    static No* como_no(NoBase* n) noexcept { return static_cast<No*>(n); }
    static const No* como_no(const NoBase* n) noexcept { return static_cast<const No*>(n); }
    static const Chave& chave_de(const NoBase* n) noexcept { return como_no(n)->par.first; }

public:
    using key_type = Chave;
    using mapped_type = Valor;
    using value_type = std::pair<const Chave, Valor>;
    using size_type = std::size_t;

    // ---------- iterador em ordem (forward: avanca pelo sucessor) ----------

    template <bool Constante>
    class IteradorBase {
        using PtrNo = std::conditional_t<Constante, const NoBase*, NoBase*>;

    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = ArvoreRubroNegra::value_type;
        using difference_type = std::ptrdiff_t;
        using pointer = std::conditional_t<Constante, const value_type*, value_type*>;
        using reference = std::conditional_t<Constante, const value_type&, value_type&>;

        IteradorBase() noexcept : no_(nil()) {}
        // iterador -> iterador constante
        template <bool C = Constante, class = std::enable_if_t<C>>
        IteradorBase(const IteradorBase<false>& outro) noexcept : no_(outro.no_) {}

        reference operator*() const noexcept { return const_cast<No*>(como_no(no_))->par; }
        pointer operator->() const noexcept { return &**this; }

        IteradorBase& operator++() noexcept {
            no_ = sucessor_no(const_cast<NoBase*>(no_));
            return *this;
        }
        IteradorBase operator++(int) noexcept {
            IteradorBase copia = *this;
            ++*this;
            return copia;
        }

        friend bool operator==(const IteradorBase& a, const IteradorBase& b) noexcept {
            return a.no_ == b.no_;
        }
        friend bool operator!=(const IteradorBase& a, const IteradorBase& b) noexcept {
            return a.no_ != b.no_;
        }

    private:
        friend class ArvoreRubroNegra;
        template <bool>
        friend class IteradorBase;
        explicit IteradorBase(PtrNo no) noexcept : no_(no) {}
        PtrNo no_;
    };

    using iterator = IteradorBase<false>;
    using const_iterator = IteradorBase<true>;

    // ---------- construcao, copia, movimento (regra dos 5) ----------

    ArvoreRubroNegra() = default;
    explicit ArvoreRubroNegra(Comparador comp) : comp_(std::move(comp)) {}

    ArvoreRubroNegra(const ArvoreRubroNegra& outra) : comp_(outra.comp_) {
        try {
            clonar(outra.raiz_, nil(), raiz_);
        } catch (...) {
            destruir(raiz_);  // a copia parcial esta sempre ligada a raiz_
            throw;
        }
        tamanho_ = outra.tamanho_;
    }

    ArvoreRubroNegra(ArvoreRubroNegra&& outra) noexcept
        : raiz_(std::exchange(outra.raiz_, nil())),
          tamanho_(std::exchange(outra.tamanho_, 0)),
          comp_(std::move(outra.comp_)) {}

    ArvoreRubroNegra& operator=(const ArvoreRubroNegra& outra) {
        if (this != &outra) {
            ArvoreRubroNegra temp(outra);  // copia-e-troca: garantia forte
            trocar(temp);
        }
        return *this;
    }

    ArvoreRubroNegra& operator=(ArvoreRubroNegra&& outra) noexcept {
        if (this != &outra) {
            destruir(raiz_);
            raiz_ = std::exchange(outra.raiz_, nil());
            tamanho_ = std::exchange(outra.tamanho_, 0);
            comp_ = std::move(outra.comp_);
        }
        return *this;
    }

    ~ArvoreRubroNegra() { destruir(raiz_); }

    void trocar(ArvoreRubroNegra& outra) noexcept {
        using std::swap;
        swap(raiz_, outra.raiz_);
        swap(tamanho_, outra.tamanho_);
        swap(comp_, outra.comp_);
    }

    // ---------- consultas ----------

    size_type tamanho() const noexcept { return tamanho_; }
    bool vazia() const noexcept { return tamanho_ == 0; }

    // Ponteiro para o valor associado a chave, ou nullptr. Iterativo.
    Valor* buscar(const Chave& chave) noexcept {
        NoBase* n = localizar(chave);
        return n == nil() ? nullptr : &como_no(n)->par.second;
    }
    const Valor* buscar(const Chave& chave) const noexcept {
        return const_cast<ArvoreRubroNegra*>(this)->buscar(chave);
    }

    bool contem(const Chave& chave) const noexcept { return buscar(chave) != nullptr; }

    const Chave* minimo() const noexcept {
        return raiz_ == nil() ? nullptr : &chave_de(minimo_no(raiz_));
    }
    const Chave* maximo() const noexcept {
        return raiz_ == nil() ? nullptr : &chave_de(maximo_no(raiz_));
    }

    // Menor chave estritamente maior que `chave` (a chave nao precisa existir).
    const Chave* sucessor(const Chave& chave) const noexcept {
        const NoBase* candidato = nullptr;
        for (const NoBase* n = raiz_; n != nil();) {
            if (comp_(chave, chave_de(n))) {
                candidato = n;
                n = n->esq;
            } else {
                n = n->dir;
            }
        }
        return candidato ? &chave_de(candidato) : nullptr;
    }

    // Maior chave estritamente menor que `chave`.
    const Chave* antecessor(const Chave& chave) const noexcept {
        const NoBase* candidato = nullptr;
        for (const NoBase* n = raiz_; n != nil();) {
            if (comp_(chave_de(n), chave)) {
                candidato = n;
                n = n->dir;
            } else {
                n = n->esq;
            }
        }
        return candidato ? &chave_de(candidato) : nullptr;
    }

    // Numero de nos no caminho mais longo raiz -> folha (vazia = 0). O(n).
    int altura() const { return altura_rec(raiz_); }

    // Nos pretos no caminho da raiz ate uma folha, contando a raiz e sem contar
    // o NIL. Pela propriedade 5 qualquer caminho serve: O(log n).
    int altura_negra() const noexcept {
        int bh = 0;
        for (const NoBase* n = raiz_; n != nil(); n = n->esq)
            if (n->cor == Cor::preto) ++bh;
        return bh;
    }

    // ---------- iteracao ----------

    iterator begin() noexcept { return iterator(raiz_ == nil() ? nil() : minimo_no(raiz_)); }
    iterator end() noexcept { return iterator(nil()); }
    const_iterator begin() const noexcept { return cbegin(); }
    const_iterator end() const noexcept { return cend(); }
    const_iterator cbegin() const noexcept {
        return const_iterator(raiz_ == nil() ? nil() : minimo_no(raiz_));
    }
    const_iterator cend() const noexcept { return const_iterator(nil()); }

    // Visita (chave, valor) em ordem crescente.
    template <class Visitante>
    void em_ordem(Visitante&& visitante) const {
        for (const auto& par : *this) visitante(par.first, par.second);
    }

    // Visita (chave, eh_vermelho, profundidade) em pre-ordem. Usado nos testes
    // para inspecionar a forma e as cores da arvore.
    template <class Visitante>
    void pre_ordem(Visitante&& visitante) const {
        pre_ordem_rec(raiz_, visitante, 0);
    }

    // Desenho "deitado": raiz a esquerda, filho direito acima, esquerdo abaixo.
    // Nos vermelhos aparecem como chave(V) e pretos como chave(P).
    std::string desenhar() const {
        std::ostringstream saida;
        desenhar_rec(saida, raiz_, 0);
        return saida.str();
    }

    // ---------- modificacao ----------

    // Insere a chave; se ja existir, apenas atualiza o valor.
    // Retorna true se a chave era nova.
    bool inserir(Chave chave, Valor valor) {
        NoBase* pai = nil();
        NoBase* n = raiz_;
        while (n != nil()) {
            pai = n;
            if (comp_(chave, chave_de(n))) {
                n = n->esq;
            } else if (comp_(chave_de(n), chave)) {
                n = n->dir;
            } else {
                como_no(n)->par.second = std::move(valor);
                return false;
            }
        }
        No* z = new No(std::move(chave), std::move(valor));
        z->cor = Cor::vermelho;
        z->pai = pai;
        z->esq = z->dir = nil();
        if (pai == nil())
            raiz_ = z;
        else if (comp_(z->par.first, chave_de(pai)))
            pai->esq = z;
        else
            pai->dir = z;
        ++tamanho_;
        corrigir_insercao(z);
        return true;
    }

    // Remove a chave. Retorna false se ela nao existia.
    bool remover(const Chave& chave) {
        NoBase* z = localizar(chave);
        if (z == nil()) return false;

        NoBase* y = z;  // no que sai fisicamente da sua posicao
        Cor cor_original_y = y->cor;
        NoBase* x;      // no que ocupa a antiga posicao de y (pode ser NIL)
        NoBase* pai_x;  // pai de x apos a remocao (substitui nil.pai do Cormen)

        if (z->esq == nil()) {
            x = z->dir;
            pai_x = z->pai;
            transplantar(z, z->dir);
        } else if (z->dir == nil()) {
            x = z->esq;
            pai_x = z->pai;
            transplantar(z, z->esq);
        } else {
            // Dois filhos: o sucessor y (minimo da subarvore direita) e religado
            // no lugar de z; nenhuma chave/valor e copiado.
            y = minimo_no(z->dir);
            cor_original_y = y->cor;
            x = y->dir;
            if (y->pai == z) {
                pai_x = y;
            } else {
                pai_x = y->pai;
                transplantar(y, y->dir);
                y->dir = z->dir;
                y->dir->pai = y;
            }
            transplantar(z, y);
            y->esq = z->esq;
            y->esq->pai = y;
            y->cor = z->cor;
        }
        delete como_no(z);
        --tamanho_;
        if (cor_original_y == Cor::preto) corrigir_remocao(x, pai_x);
        return true;
    }

    void limpar() noexcept {
        destruir(raiz_);
        raiz_ = nil();
        tamanho_ = 0;
    }

    // ---------- verificacao ----------

    // Confere todos os invariantes em uma passada O(n): raiz preta, nenhum
    // vermelho com filho vermelho, mesma altura negra em todos os caminhos,
    // ordem estrita (com limites herdados), ponteiros pai coerentes e
    // contagem de nos igual a tamanho().
    bool validar() const {
        if (sentinela_.cor != Cor::preto) return false;
        if (raiz_ == nil()) return tamanho_ == 0;
        if (raiz_->cor != Cor::preto || raiz_->pai != nil()) return false;
        size_type contagem = 0;
        return validar_rec(raiz_, nullptr, nullptr, contagem) >= 0 && contagem == tamanho_;
    }

private:
    NoBase* raiz_ = nil();
    size_type tamanho_ = 0;
    Comparador comp_{};

    // ---------- auxiliares de navegacao ----------

    NoBase* localizar(const Chave& chave) const noexcept {
        NoBase* n = raiz_;
        while (n != nil()) {
            if (comp_(chave, chave_de(n)))
                n = n->esq;
            else if (comp_(chave_de(n), chave))
                n = n->dir;
            else
                return n;
        }
        return nil();
    }

    template <class P>
    static P minimo_no(P n) noexcept {
        while (n->esq != nil()) n = n->esq;
        return n;
    }
    template <class P>
    static P maximo_no(P n) noexcept {
        while (n->dir != nil()) n = n->dir;
        return n;
    }

    // Sucessor em ordem usando o ponteiro para o pai: O(1) amortizado.
    static NoBase* sucessor_no(NoBase* n) noexcept {
        if (n->dir != nil()) return minimo_no(n->dir);
        NoBase* p = n->pai;
        while (p != nil() && n == p->dir) {
            n = p;
            p = p->pai;
        }
        return p;
    }

    // ---------- rotacoes e transplante (nunca escrevem no sentinela) ----------

    /* Rotacao a esquerda em x (a rotacao a direita e o espelho):
            x                y
           / \              / \
          a   y     ==>    x   c
             / \          / \
            b   c        a   b
    */
    void rotacionar_esquerda(NoBase* x) noexcept {
        NoBase* y = x->dir;
        x->dir = y->esq;
        if (y->esq != nil()) y->esq->pai = x;
        y->pai = x->pai;
        if (x->pai == nil())
            raiz_ = y;
        else if (x == x->pai->esq)
            x->pai->esq = y;
        else
            x->pai->dir = y;
        y->esq = x;
        x->pai = y;
    }

    void rotacionar_direita(NoBase* x) noexcept {
        NoBase* y = x->esq;
        x->esq = y->dir;
        if (y->dir != nil()) y->dir->pai = x;
        y->pai = x->pai;
        if (x->pai == nil())
            raiz_ = y;
        else if (x == x->pai->dir)
            x->pai->dir = y;
        else
            x->pai->esq = y;
        y->dir = x;
        x->pai = y;
    }

    // Coloca v no lugar de u (u continua com seus ponteiros antigos).
    void transplantar(NoBase* u, NoBase* v) noexcept {
        if (u->pai == nil())
            raiz_ = v;
        else if (u == u->pai->esq)
            u->pai->esq = v;
        else
            u->pai->dir = v;
        if (v != nil()) v->pai = u->pai;
    }

    // ---------- correcoes de cor ----------

    void corrigir_insercao(NoBase* z) noexcept {
        while (z->pai->cor == Cor::vermelho) {
            NoBase* avo = z->pai->pai;  // existe: pai vermelho nunca e a raiz
            if (z->pai == avo->esq) {
                NoBase* tio = avo->dir;
                if (tio->cor == Cor::vermelho) {
                    // Caso 1: tio vermelho -> recolorir e subir dois niveis.
                    z->pai->cor = Cor::preto;
                    tio->cor = Cor::preto;
                    avo->cor = Cor::vermelho;
                    z = avo;
                } else {
                    if (z == z->pai->dir) {
                        // Caso 2: "triangulo" -> rotacao no pai vira caso 3.
                        z = z->pai;
                        rotacionar_esquerda(z);
                    }
                    // Caso 3: "linha" -> recolorir e rotacionar o avo.
                    z->pai->cor = Cor::preto;
                    avo->cor = Cor::vermelho;
                    rotacionar_direita(avo);
                }
            } else {  // simetrico
                NoBase* tio = avo->esq;
                if (tio->cor == Cor::vermelho) {
                    z->pai->cor = Cor::preto;
                    tio->cor = Cor::preto;
                    avo->cor = Cor::vermelho;
                    z = avo;
                } else {
                    if (z == z->pai->esq) {
                        z = z->pai;
                        rotacionar_direita(z);
                    }
                    z->pai->cor = Cor::preto;
                    avo->cor = Cor::vermelho;
                    rotacionar_esquerda(avo);
                }
            }
        }
        raiz_->cor = Cor::preto;
    }

    // x carrega um "preto extra". O irmao w nunca e NIL: a subarvore dele tem
    // altura negra >= 1 porque um no preto foi removido do lado de x.
    void corrigir_remocao(NoBase* x, NoBase* pai) noexcept {
        while (x != raiz_ && x->cor == Cor::preto) {
            if (x == pai->esq) {
                NoBase* w = pai->dir;
                if (w->cor == Cor::vermelho) {
                    // Caso 1: irmao vermelho -> rotacao transforma em caso 2, 3 ou 4.
                    w->cor = Cor::preto;
                    pai->cor = Cor::vermelho;
                    rotacionar_esquerda(pai);
                    w = pai->dir;
                }
                if (w->esq->cor == Cor::preto && w->dir->cor == Cor::preto) {
                    // Caso 2: irmao preto com filhos pretos -> sobe o preto extra.
                    w->cor = Cor::vermelho;
                    x = pai;
                    pai = x->pai;
                } else {
                    if (w->dir->cor == Cor::preto) {
                        // Caso 3: filho "de dentro" vermelho -> vira caso 4.
                        w->esq->cor = Cor::preto;
                        w->cor = Cor::vermelho;
                        rotacionar_direita(w);
                        w = pai->dir;
                    }
                    // Caso 4: filho "de fora" vermelho -> rotacao final, termina.
                    w->cor = pai->cor;
                    pai->cor = Cor::preto;
                    w->dir->cor = Cor::preto;
                    rotacionar_esquerda(pai);
                    x = raiz_;
                }
            } else {  // simetrico
                NoBase* w = pai->esq;
                if (w->cor == Cor::vermelho) {
                    w->cor = Cor::preto;
                    pai->cor = Cor::vermelho;
                    rotacionar_direita(pai);
                    w = pai->esq;
                }
                if (w->dir->cor == Cor::preto && w->esq->cor == Cor::preto) {
                    w->cor = Cor::vermelho;
                    x = pai;
                    pai = x->pai;
                } else {
                    if (w->esq->cor == Cor::preto) {
                        w->dir->cor = Cor::preto;
                        w->cor = Cor::vermelho;
                        rotacionar_esquerda(w);
                        w = pai->esq;
                    }
                    w->cor = pai->cor;
                    pai->cor = Cor::preto;
                    w->esq->cor = Cor::preto;
                    rotacionar_direita(pai);
                    x = raiz_;
                }
            }
        }
        if (x != nil()) x->cor = Cor::preto;
    }

    // ---------- copia e destruicao ----------

    // Copia preservando forma e cores. O no e ligado ao destino ANTES de copiar
    // os filhos, entao, se uma copia lancar excecao, a parte ja copiada continua
    // alcancavel a partir da raiz e pode ser destruida.
    static void clonar(const NoBase* origem, NoBase* pai, NoBase*& destino) {
        destino = nil();
        if (origem == nil()) return;
        const No* o = como_no(origem);
        No* n = new No(o->par.first, o->par.second);
        n->cor = o->cor;
        n->pai = pai;
        n->esq = n->dir = nil();
        destino = n;
        clonar(origem->esq, n, n->esq);
        clonar(origem->dir, n, n->dir);
    }

    // Destruicao iterativa em O(n) e memoria extra O(1): enquanto o no atual
    // tiver filho esquerdo, rotaciona a direita; senao, apaga-o e segue para a
    // direita. Nao usa recursao nem pilha auxiliar.
    static void destruir(NoBase* n) noexcept {
        while (n != nil()) {
            if (n->esq != nil()) {
                NoBase* e = n->esq;
                n->esq = e->dir;
                e->dir = n;
                n = e;
            } else {
                NoBase* d = n->dir;
                delete como_no(n);
                n = d;
            }
        }
    }

    // ---------- recursoes limitadas pela altura (<= 2 log2(n+1)) ----------

    static int altura_rec(const NoBase* n) {
        if (n == nil()) return 0;
        int e = altura_rec(n->esq), d = altura_rec(n->dir);
        return 1 + (e > d ? e : d);
    }

    template <class Visitante>
    static void pre_ordem_rec(const NoBase* n, Visitante& visitante, int profundidade) {
        if (n == nil()) return;
        visitante(chave_de(n), n->cor == Cor::vermelho, profundidade);
        pre_ordem_rec(n->esq, visitante, profundidade + 1);
        pre_ordem_rec(n->dir, visitante, profundidade + 1);
    }

    static void desenhar_rec(std::ostringstream& saida, const NoBase* n, int nivel) {
        if (n == nil()) return;
        desenhar_rec(saida, n->dir, nivel + 1);
        saida << std::string(static_cast<std::size_t>(nivel) * 4, ' ') << chave_de(n)
              << (n->cor == Cor::vermelho ? "(V)" : "(P)") << '\n';
        desenhar_rec(saida, n->esq, nivel + 1);
    }

    // Retorna a altura negra da subarvore (contando o NIL como 1) ou -1 se
    // algum invariante for violado.
    int validar_rec(const NoBase* n, const Chave* inf, const Chave* sup,
                    size_type& contagem) const {
        if (n == nil()) return 1;
        ++contagem;
        const Chave& k = chave_de(n);
        if (inf && !comp_(*inf, k)) return -1;
        if (sup && !comp_(k, *sup)) return -1;
        if (n->esq != nil() && n->esq->pai != n) return -1;
        if (n->dir != nil() && n->dir->pai != n) return -1;
        if (n->cor == Cor::vermelho &&
            (n->esq->cor == Cor::vermelho || n->dir->cor == Cor::vermelho))
            return -1;
        int be = validar_rec(n->esq, inf, &k, contagem);
        if (be < 0) return -1;
        int bd = validar_rec(n->dir, &k, sup, contagem);
        if (bd < 0 || be != bd) return -1;
        return be + (n->cor == Cor::preto ? 1 : 0);
    }
};

#endif  // ARVORE_RUBRO_NEGRA_HPP
