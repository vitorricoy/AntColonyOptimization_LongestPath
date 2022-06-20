#include <iostream>
#include <vector>
#include <set>
#include <fstream>
#include <climits>
#include <random>

using namespace std;

#define NUM_FORMIGAS 100
#define NUM_ITER 100
#define FEROMONIO_INICIAL 0.2
#define BETA 2
#define ALFA 1
#define TAXA_EVAPORACAO 0.1

// n_ij = d_ij
// Considerando que não tem aresta negativa

vector<vector<long long>> pesos;
vector<vector<double>> feromonios;
vector<vector<pair<int, long long>>> listaAdj;
int nos;

std::mt19937 gen(1); // seed the generator

// Testar resultados de criar nós com valor muito alto e de não adicionar novos nós
vector<int> caminhar_formiga()
{
    std::uniform_int_distribution<> intRand(0, nos - 1);
    std::uniform_real_distribution<> doubleRand(0, 1);
    int noInicial = intRand(gen);
    set<int> visitados;
    vector<int> caminho;
    int noAtual = noInicial;
    while (true)
    {
        caminho.push_back(noAtual);
        visitados.insert(noAtual);
        vector<pair<int, long long>> vizinhos = listaAdj[noAtual];
        vector<pair<int, double>> probabilities;
        double probabilitySum = 0.0;
        for (pair<int, long long> viz : vizinhos)
        {
            if (viz.second >= 0 && visitados.find(viz.first) == visitados.end())
            {
                double probability = pow(viz.second, BETA) * pow(feromonios[noAtual][viz.first], ALFA);
                probabilities.push_back({viz.first, probability});
                probabilitySum += probability;
            }
        }
        if (probabilitySum == 0)
        {
            break;
        }
        for (unsigned i = 0; i < probabilities.size(); i++)
        {
            probabilities[i].second /= probabilitySum;
        }

        double prob = doubleRand(gen);
        double accProb = 0.0;
        int escolhido = -1;
        for (unsigned i = 0; i < probabilities.size(); i++)
        {
            if (prob <= accProb + probabilities[i].second)
            {
                escolhido = probabilities[i].first;
                break;
            }
            accProb += probabilities[i].second;
        }
        if (escolhido == -1)
        {
            cout << "ERRO" << endl;
            return {};
        }
        noAtual = escolhido;
    }
    return caminho;
}

int main()
{
    string arquivo = "entrada1.txt";
    nos = 100;
    ifstream arq("dataset/" + arquivo);
    if (!arq.is_open())
    {
        cout << "Erro ao abrir o arquivo" << endl;
        exit(0);
    }

    int vert1, vert2;
    long long peso;
    pesos = vector<vector<long long>>(nos, vector<long long>(nos, LLONG_MIN));
    listaAdj = vector<vector<pair<int, long long>>>(nos);
    feromonios = vector<vector<double>>(nos, vector<double>(nos, FEROMONIO_INICIAL));

    while (arq >> vert1 >> vert2 >> peso)
    {
        pesos[vert1 - 1][vert2 - 1] = peso;
        listaAdj[vert1 - 1].push_back({vert2 - 1, peso});
    }

    arq.close();

    long long melhor = -1;
    for (int iter = 0; iter < NUM_ITER; iter++)
    {
        vector<int> melhorCaminho;
        long long melhorCusto = -1;
        for (int form = 0; form < NUM_FORMIGAS; form++)
        {
            vector<int> caminhoFormiga = caminhar_formiga();
            long long custo = 0;
            for (unsigned i = 0; i < caminhoFormiga.size() - 1; i++)
            {
                custo += pesos[caminhoFormiga[i]][caminhoFormiga[i + 1]];
            }
            if (custo > melhorCusto)
            {
                melhorCusto = custo;
                melhorCaminho = caminhoFormiga;
            }
        }

        for (int i = 0; i < nos; i++)
        {
            for (int j = 0; j < nos; j++)
            {
                feromonios[i][j] *= (1 - TAXA_EVAPORACAO);
            }
        }
        double variacaoFeromonio = 1.0 - 1.0 / ((double)melhorCusto);
        for (unsigned i = 0; i < melhorCaminho.size() - 1; i++)
        {
            feromonios[melhorCaminho[i]][melhorCaminho[i + 1]] += variacaoFeromonio;
        }
        melhor = max(melhor, melhorCusto);
    }
    cout << melhor << endl;
}