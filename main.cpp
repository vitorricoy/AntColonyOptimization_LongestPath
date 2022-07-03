#include <iostream>
#include <vector>
#include <set>
#include <fstream>
#include <climits>
#include <random>
#include <mutex>
#include <algorithm>
#include <thread>

using namespace std;

int NUM_FORMIGAS = 100;
int NUM_ITER = 200;
double FEROMONIO_INICIAL = 10.0;
int BETA = 3;
int ALFA = 1;
double TAXA_EVAPORACAO = 0.05;

const int SEED = 1;

// n_ij = d_ij

vector<vector<long long>> pesos;
vector<vector<double>> feromonios;
vector<vector<pair<int, long long>>> listaAdj;
int nos;

mutex acessoMelhorCaminho;

vector<int> melhorCaminho;
long long melhorCusto = -1;
int invalidos = 0;

mt19937 origGen(SEED);

// Testar resultados de criar nós com valor muito alto e de não adicionar novos nós
void caminharFormiga()
{
    vector<int> possiveisIniciais;
    for (pair<int, long long> viz : listaAdj[0])
    {
        if (viz.first != 0 && viz.first != nos - 1)
        {
            possiveisIniciais.push_back(viz.first);
        }
    }
    uniform_int_distribution<> intRand(0, possiveisIniciais.size() - 1);
    uniform_real_distribution<> doubleRand(0, 1);
    int noInicial = possiveisIniciais[intRand(origGen)];
    set<int> visitados;
    vector<int> caminho;
    int noAtual = noInicial;
    visitados.insert(0);
    visitados.insert(nos - 1);
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
                double probability = pow((double)viz.second, BETA) * pow(feromonios[noAtual][viz.first], ALFA);
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

        double prob = doubleRand(origGen);
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
            return;
        }
        noAtual = escolhido;
    }
    vector<int> caminhoReal;
    caminhoReal.push_back(0);
    caminhoReal.insert(caminhoReal.end(), caminho.begin(), caminho.end());
    while (pesos[caminhoReal.back()][nos - 1] == -1 && caminhoReal.size())
    {
        caminhoReal.pop_back();
    }

    long long custo = 0;
    if (!caminhoReal.empty())
    {
        caminhoReal.push_back(nos - 1);
        for (unsigned i = 0; i < caminhoReal.size() - 1; i++)
        {
            custo += pesos[caminhoReal[i]][caminhoReal[i + 1]];
        }
    }
    else
    {
        acessoMelhorCaminho.lock();
        invalidos++;
        acessoMelhorCaminho.unlock();
    }

    if (custo > melhorCusto)
    {
        acessoMelhorCaminho.lock();
        melhorCusto = custo;
        melhorCaminho = caminhoReal;
        acessoMelhorCaminho.unlock();
    }
}

void executarAlgoritmo(int iter)
{
    feromonios = vector<vector<double>>(nos, vector<double>(nos, FEROMONIO_INICIAL));
    ofstream output("outputs/output_" + to_string(ALFA) + "_" + to_string(BETA) + "_" + to_string(iter) + ".txt");
    if (!output.is_open())
    {
        cout << "Erro ao abrir o arquivo de saída" << endl;
        exit(0);
    }

    long long melhor = -1;
    for (int iter = 0; iter < NUM_ITER; iter++)
    {
        melhorCaminho.clear();
        melhorCusto = -1;
        invalidos = 0;
        vector<thread> threads;
        for (int form = 0; form < NUM_FORMIGAS; form++)
        {
            threads.emplace_back(caminharFormiga);
        }

        for (int form = 0; form < NUM_FORMIGAS; form++)
        {
            threads[form].join();
        }

        for (int i = 0; i < nos; i++)
        {
            for (int j = 0; j < nos; j++)
            {
                feromonios[i][j] *= (1 - TAXA_EVAPORACAO);
            }
        }
        double valCustoVariacao = melhorCusto == 0 ? 0 : 1.0 / ((double)melhorCusto);
        double variacaoFeromonio = 1.0 - valCustoVariacao;
        for (unsigned i = 0; i < melhorCaminho.size() - 1; i++)
        {
            feromonios[melhorCaminho[i]][melhorCaminho[i + 1]] += variacaoFeromonio;
            feromonios[melhorCaminho[i]][melhorCaminho[i + 1]] = min(feromonios[melhorCaminho[i]][melhorCaminho[i + 1]], FEROMONIO_INICIAL);
        }
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
        melhor = max(melhor, melhorCusto);
    }
    cout << melhor << endl;
    output.close();
}

int main()
{
    string arquivo = "entrada3.txt";
    nos = 1000;
    ifstream arq("dataset/" + arquivo);
    if (!arq.is_open())
    {
        cout << "Erro ao abrir o arquivo" << endl;
        exit(0);
    }

    int vert1, vert2;
    long long peso;
    pesos = vector<vector<long long>>(nos, vector<long long>(nos, -1));
    listaAdj = vector<vector<pair<int, long long>>>(nos);

    while (arq >> vert1 >> vert2 >> peso)
    {
        if (vert1 == vert2)
        {
            continue;
        }
        pesos[vert1 - 1][vert2 - 1] = peso;
        listaAdj[vert1 - 1].push_back({vert2 - 1, peso});
    }
    vector<pair<int, int>> valores = {{1, 1}, {1, 2}, {1, 3}, {2, 1}, {2, 2}, {2, 3}, {3, 1}, {3, 2}, {3, 3}};
    for (pair<int, int> t : valores)
    {
        ALFA = t.first;
        BETA = t.second;
        cout << "ALFA = " << ALFA << ", BETA = " << BETA << endl;
        for (int i = 0; i < 10; i++)
        {
            origGen = mt19937(i);
            executarAlgoritmo(i);
        }
    }
    arq.close();
}