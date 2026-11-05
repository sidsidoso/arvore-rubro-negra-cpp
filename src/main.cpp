// Demonstracao da ArvoreRubroNegra.
#include <iostream>
#include <string>

#include "arvore_rubro_negra.hpp"

int main() {
    std::cout << std::boolalpha;

    // Insercao em ordem crescente: numa BST comum viraria uma lista (altura 10).
    ArvoreRubroNegra<int, std::string> arvore;
    for (int i = 1; i <= 10; ++i) arvore.inserir(i, "valor " + std::to_string(i));
    std::cout << "Insercao de 1..10 em ordem crescente\n";
    std::cout << "tamanho: " << arvore.tamanho() << ", altura: " << arvore.altura()
              << ", altura negra: " << arvore.altura_negra()
              << " (uma BST sem balanceamento teria altura 10)\n";
    std::cout << "arvore (deitada, raiz a esquerda; V = vermelho, P = preto):\n"
              << arvore.desenhar() << '\n';

    // Exemplo classico do Cormen (fig. 13.4): inserir 4 dispara os casos 1, 2 e 3.
    ArvoreRubroNegra<int, int> cormen;
    for (int k : {11, 2, 14, 1, 7, 15, 5, 8}) cormen.inserir(k, 0);
    std::cout << "Cormen, fig. 13.4 - antes de inserir 4:\n" << cormen.desenhar();
    cormen.inserir(4, 0);
    std::cout << "depois de inserir 4 (recoloracao + rotacao dupla):\n"
              << cormen.desenhar() << '\n';

    // Consultas.
    if (const std::string* v = arvore.buscar(5)) std::cout << "buscar(5) -> " << *v << '\n';
    std::cout << "contem(42)? " << arvore.contem(42) << '\n';
    std::cout << "minimo: " << *arvore.minimo() << ", maximo: " << *arvore.maximo()
              << ", sucessor(7): " << *arvore.sucessor(7) << "\n\n";

    // Remocao com fix-up.
    for (int k : {4, 1, 2, 3}) arvore.remover(k);
    std::cout << "Apos remover 4, 1, 2 e 3:\n" << arvore.desenhar();
    std::cout << "altura: " << arvore.altura() << ", invariantes ok? " << arvore.validar()
              << "\n\n";

    // Iterador em ordem com structured bindings.
    std::cout << "Em ordem:";
    for (const auto& [chave, valor] : arvore) std::cout << ' ' << chave << '=' << '"' << valor << '"';
    std::cout << '\n';

    // Mapa palavra -> contagem com chave std::string.
    ArvoreRubroNegra<std::string, int> frequencia;
    for (const char* palavra : {"pera", "uva", "maca", "uva", "pera", "uva", "caju"}) {
        if (int* atual = frequencia.buscar(palavra))
            ++*atual;
        else
            frequencia.inserir(palavra, 1);
    }
    std::cout << "Frequencia:";
    for (const auto& [palavra, n] : frequencia) std::cout << ' ' << palavra << ':' << n;
    std::cout << '\n';

    // Sequencia grande: a altura fica perto de log2(n).
    ArvoreRubroNegra<int, int> grande;
    for (int i = 0; i < 1000000; ++i) grande.inserir(i, i);
    std::cout << "1.000.000 chaves crescentes -> altura " << grande.altura() << ", altura negra "
              << grande.altura_negra() << " (limite 2*log2(n+1) = 39), valida? "
              << grande.validar() << '\n';
    return 0;
}
