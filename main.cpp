#include <iostream>
#include <cstdlib>
#include <vector>
#include <set>
#include <fstream>
#include <climits>
#include <random>
#include <mutex>
#include <algorithm>
#include <thread>

using namespace std;

// Parâmetros do algoritmo
int NUM_FORMIGAS = 100;
int NUM_ITER = 200;
double FEROMONIO_INICIAL = 10.0;
int BETA = 3;
int ALFA = 1;
double TAXA_EVAPORACAO = 0.05;
int nos;

// Dados da geração aleatória
int SEED = 1;
mt19937 origGen(SEED);

// Representações do grafo da entrada
vector<vector<long long>> pesos;
vector<vector<pair<int, long long>>> listaAdj;

// Mutex para sincronização da escrita nos dados globais pelas threads
mutex acessoMelhorCaminho;

// Dados globais do ACO
vector<vector<double>> feromonios;
vector<int> melhorCaminho;
long long melhorCusto = -1;
int invalidos = 0;

// Função que representa o caminhamento de uma formiga
void caminharFormiga()
{
    // Preenche os possíveis vértices iniciais, que devem ser vizinhos do vértice 1
    vector<int> possiveisIniciais;
    for (pair<int, long long> viz : listaAdj[0])
    {
        if (viz.first != 0 && viz.first != nos - 1)
        {
            possiveisIniciais.push_back(viz.first);
        }
    }
    // Inicializa as distribuições uniformes aleatórias que serão usadas
    uniform_int_distribution<> intRand(0, possiveisIniciais.size() - 1);
    uniform_real_distribution<> doubleRand(0, 1);
    // Escolhe aleatoriamente o nó inicial da formiga
    int noInicial = possiveisIniciais[intRand(origGen)];
    // Conjunto de nós já visitados
    set<int> visitados;
    // Lista que representa o caminho percorrido até o momento pela formiga
    vector<int> caminho;
    // No em que a formiga está atualmente
    int noAtual = noInicial;
    // Insere os nós 1 e N como visitados para simular um subgrafo que não os contém no caminhamento
    visitados.insert(0);
    visitados.insert(nos - 1);
    // Realiza a simulação
    while (true)
    {
        // Insere o nó atual no caminhamento e indica que ele já foi visitado
        caminho.push_back(noAtual);
        visitados.insert(noAtual);
        // Lista os vizinhos do nó atual
        vector<pair<int, long long>> vizinhos = listaAdj[noAtual];
        // Salva as probabilidades de cada aresta ser visitada e a soma dessa probabilidades
        vector<pair<int, double>> probabilidades;
        double somaProbabilidades = 0.0;
        // Para cada vizinho do nó atual
        for (pair<int, long long> viz : vizinhos)
        {
            // Se o vizinho não foi visitado e possui um custo válido
            if (viz.second >= 0 && visitados.find(viz.first) == visitados.end())
            {
                // Calcula o peso atribuído à aresta que será usado para calcular a probabilidade de se visitar essa aresta
                double probabilidade = pow((double)viz.second, BETA) * pow(feromonios[noAtual][viz.first], ALFA);
                // Insere esse peso no vetor e na soma
                probabilidades.push_back({viz.first, probabilidade});
                somaProbabilidades += probabilidade;
            }
        }
        // Se nenhum vizinho é válido, encerra a simulação
        if (somaProbabilidades == 0)
        {
            break;
        }
        // Normaliza o vetor de probabilidades para ele representar a probabilidade de fato, e não o peso atribuído à aresta
        for (unsigned i = 0; i < probabilidades.size(); i++)
        {
            probabilidades[i].second /= somaProbabilidades;
        }
        // Gera um valor entre 0 e 1 uniformemente
        double prob = doubleRand(origGen);
        // Seleciona qual aresta será utilizada de acordo com sua probabilidade e o número gerado aleatoriamente
        double accProb = 0.0;
        int escolhido = -1;
        for (unsigned i = 0; i < probabilidades.size(); i++)
        {
            if (prob <= accProb + probabilidades[i].second)
            {
                escolhido = probabilidades[i].first;
                break;
            }
            accProb += probabilidades[i].second;
        }
        // Se não nenhuma aresta foi escolhida, registra o erro
        if (escolhido == -1)
        {
            cout << "ERRO" << endl;
            return;
        }
        // Salva como no atual o nó escolhido
        noAtual = escolhido;
    }
    // Constroi o caminho real, entre 1 e N
    vector<int> caminhoReal;
    // Insere o primeiro nó
    caminhoReal.push_back(0);
    // Insere o caminho do subgrafo que não contém 1 e N construído pela formiga
    caminhoReal.insert(caminhoReal.end(), caminho.begin(), caminho.end());
    // Enquanto o último nó do caminho não possuir uma aresta para N, remove esse nó
    while (pesos[caminhoReal.back()][nos - 1] == -1 && caminhoReal.size())
    {
        caminhoReal.pop_back();
    }
    // Inicializa o custo com o custo de um caminho inválido
    long long custo = 0;
    // Verifica se o caminho construído não é vazio
    if (!caminhoReal.empty())
    {
        // Insere o nó N no caminho
        caminhoReal.push_back(nos - 1);
        // Calcula o custo do caminho
        for (unsigned i = 0; i < caminhoReal.size() - 1; i++)
        {
            custo += pesos[caminhoReal[i]][caminhoReal[i + 1]];
        }
    }
    else
    {
        // Um caminho vazio indica uma solução inválida, portanto registra essa estatística
        acessoMelhorCaminho.lock();
        invalidos++;
        acessoMelhorCaminho.unlock();
    }
    // Se o custo da solução atual é o melhor da iteração até o momento
    if (custo > melhorCusto)
    {
        // Salva o custo e o caminho nas variáveis globais
        acessoMelhorCaminho.lock();
        melhorCusto = custo;
        melhorCaminho = caminhoReal;
        acessoMelhorCaminho.unlock();
    }
}

void executarAlgoritmo()
{
    // Inicializa a matriz de feromonios
    feromonios = vector<vector<double>>(nos, vector<double>(nos, FEROMONIO_INICIAL));
    // Abre o arquivo de registro de estatisticas
    ofstream output("outputs/output.txt");
    if (!output.is_open())
    {
        cout << "Erro ao abrir o arquivo de saída" << endl;
        exit(0);
    }

    // Realiza as iterações do ACO
    long long melhor = -1;
    for (int iter = 0; iter < NUM_ITER; iter++)
    {
        // Limpa as variáveis globais
        melhorCaminho.clear();
        melhorCusto = -1;
        invalidos = 0;
        // Cria as threads para o caminhamento das formigas
        vector<thread> threads;
        for (int form = 0; form < NUM_FORMIGAS; form++)
        {
            threads.emplace_back(caminharFormiga);
        }
        // Espera o caminhamento das formigas terminar
        for (int form = 0; form < NUM_FORMIGAS; form++)
        {
            threads[form].join();
        }
        // Evapora todos os feromônios da matriz de feromônios
        for (int i = 0; i < nos; i++)
        {
            for (int j = 0; j < nos; j++)
            {
                feromonios[i][j] *= (1 - TAXA_EVAPORACAO);
            }
        }
        // Calcula a variação de feromônio nas arestas que fazem parte do melhor caminho da iteração
        double valCustoVariacao = melhorCusto == 0 ? 1 : 1.0 / ((double)melhorCusto);
        double variacaoFeromonio = 1.0 - valCustoVariacao;
        // Atualiza o feromônio das arestas do melhor caminho
        for (unsigned i = 0; i < melhorCaminho.size() - 1; i++)
        {
            feromonios[melhorCaminho[i]][melhorCaminho[i + 1]] += variacaoFeromonio;
            // Faz com que a matriz de feromônio respeito o limite máximo definido pelo Max-Min
            feromonios[melhorCaminho[i]][melhorCaminho[i + 1]] = min(feromonios[melhorCaminho[i]][melhorCaminho[i + 1]], FEROMONIO_INICIAL);
        }
        // Registra as estatísticas no arquivo de saída
        output << melhorCusto << " ";
        output << invalidos << " ";
        if (iter == 0 || iter == 4 || iter == 19 || iter == 99 || iter == NUM_ITER - 1)
        {
            for (vector<double> f : feromonios)
            {
                for (double v : f)
                {
                    output << v << " ";
                }
            }
            output << endl;
        }
        else
        {
            output << endl;
        }
        // Atualiza o melhor global
        melhor = max(melhor, melhorCusto);
    }
    // Imprime o melhor global
    cout << melhor << endl;
    // Fecha o arquivo de saída
    output.close();
}

int main(int argc, char **argv)
{
    if (argc != 9)
    {
        cout << "Uso correto do programa: './tp2 <num_formigas> <num_iter> <alfa> <beta> <taxa_evaporacao> <arquivo> <num_vertices> <seed>'" << endl;
        return 0;
    }

    // Seta os parâmetros de acordo com os argumentos do programa
    NUM_FORMIGAS = atoi(argv[1]);
    NUM_ITER = atoi(argv[2]);
    ALFA = atoi(argv[3]);
    BETA = atoi(argv[4]);
    TAXA_EVAPORACAO = atof(argv[5]);
    string arquivo = string(argv[6]);
    nos = atoi(argv[7]);
    SEED = atoi(argv[8]);

    // Abre o arquivo de entrada
    ifstream arq("dataset/" + arquivo);
    if (!arq.is_open())
    {
        cout << "Erro ao abrir o arquivo" << endl;
        exit(0);
    }

    // Inicializa as variáveis necessárias, a matriz de adjacência e a lista de adjacência
    int vert1, vert2;
    long long peso;
    pesos = vector<vector<long long>>(nos, vector<long long>(nos, -1));
    listaAdj = vector<vector<pair<int, long long>>>(nos);

    // Lê uma linha do arquivo
    while (arq >> vert1 >> vert2 >> peso)
    {
        // Se a linha lida representa um laço, a ignora
        if (vert1 == vert2)
        {
            continue;
        }
        // Insere a aresta na lista e na matriz de adjacência
        pesos[vert1 - 1][vert2 - 1] = peso;
        listaAdj[vert1 - 1].push_back({vert2 - 1, peso});
    }
    // Fecha o arquivo de entrada
    arq.close();
    // Executa o ACO
    executarAlgoritmo();
    return 0;
}