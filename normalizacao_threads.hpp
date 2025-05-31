
#ifndef NORMALIZACAO_THREADS_HPP
#define NORMALIZACAO_THREADS_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <map>
#include <thread>
#include <queue>

using namespace std;

using Linha = vector<string>;


inline void construir_mapas_codificacao_por_chunks(const string& caminho,
                                                   const vector<string>& nomeColunas,
                                                   const vector<int>& indicesAlvo,
                                                   size_t linhas_por_chunk,
                                                   unordered_map<string, unordered_map<string, int>>& mapas,
                                                   unordered_map<string, int>& contadores) {
    ifstream arquivo(caminho);
    if (!arquivo.is_open()) {
        cerr << "Erro ao abrir o arquivo para processar chunks." << endl;
        exit(1);
    }

    string cabecalho;
    getline(arquivo, cabecalho); // descarta cabeçalho

    string linha;
    vector<Linha> chunk;
    mutex mtx;

    auto processar_chunk = [&](const vector<Linha>& chunk_local) {
        unsigned n_threads = thread::hardware_concurrency();
        if (n_threads == 0) n_threads = 2;

        vector<unordered_map<string, unordered_map<string, int>>> mapasLocais(n_threads);
        vector<thread> threads;

        for (unsigned tid = 0; tid < n_threads; ++tid) {
            threads.emplace_back([&, tid]() {
                for (size_t i = tid; i < indicesAlvo.size(); i += n_threads) {
                    int col = indicesAlvo[i];
                    string nomeCol = nomeColunas[col];
                    for (const auto& linhaChunk : chunk_local) {
                        if (col >= linhaChunk.size()) continue;
                        string valor = linhaChunk[col];
                        if (valor.empty()) continue;
                        mapasLocais[tid][nomeCol][valor];
                    }
                }
            });
        }

        for (auto& t : threads) t.join();

        lock_guard<mutex> lock(mtx);
        for (const auto& local : mapasLocais) {
            for (const auto& [coluna, mapa] : local) {
                for (const auto& [valor, _] : mapa) {
                    if (mapas[coluna].find(valor) == mapas[coluna].end()) {
                        mapas[coluna][valor] = contadores[coluna]++;
                    }
                }
            }
        }
    };

    while (getline(arquivo, linha)) {
        stringstream ss(linha);
        string valor;
        Linha linhaDados;
        while (getline(ss, valor, ',')) {
            linhaDados.push_back(valor);
        }
        chunk.push_back(linhaDados);

        if (chunk.size() >= linhas_por_chunk) {
            processar_chunk(chunk);
            chunk.clear();
        }
    }

    if (!chunk.empty()) {
        processar_chunk(chunk);
    }

    arquivo.close();
}

inline void gerar_arquivos_id_por_coluna(const unordered_map<string, unordered_map<string, int>>& mapas) {
    unsigned n_threads = thread::hardware_concurrency();
    if (n_threads == 0) n_threads = 2;

    vector<thread> threads;
    mutex mtx_erro;

    vector<string> chaves;
    for (const auto& [coluna, _] : mapas) {
        chaves.push_back(coluna);
    }

    auto worker = [&](unsigned tid) {
        for (size_t i = tid; i < chaves.size(); i += n_threads) {
            const string& nomeCol = chaves[i];
            const auto& mapa = mapas.at(nomeCol);

            ofstream arquivo("ID_" + nomeCol + ".csv");
            if (!arquivo.is_open()) {
                lock_guard<mutex> lock(mtx_erro);
                cerr << "Erro ao criar arquivo ID_" << nomeCol << ".csv" << endl;
                continue;
            }

            arquivo << "COD,CONTEUDO\n";
            for (const auto& [conteudo, cod] : mapa) {
                arquivo << cod << "," << conteudo << "\n";
            }

            arquivo.close();
        }
    };

    for (unsigned tid = 0; tid < n_threads; ++tid) {
        threads.emplace_back(worker, tid);
    }

    for (auto& t : threads) t.join();
}

inline vector<string> codificar_linhas_chunk(const vector<string>& chunk,
                                             const vector<string>& nomeColunas,
                                             const unordered_map<string, unordered_map<string, int>>& mapas) {
    vector<string> linhasCodificadas(chunk.size());

    unsigned n_threads = thread::hardware_concurrency();
    if (n_threads == 0) n_threads = 2;
    vector<thread> threads;

    size_t linhas_por_thread = (chunk.size() + n_threads - 1) / n_threads;

    for (unsigned tid = 0; tid < n_threads; ++tid) {
        threads.emplace_back([&, tid]() {
            size_t inicio = tid * linhas_por_thread;
            size_t fim = min(inicio + linhas_por_thread, chunk.size());

            for (size_t i = inicio; i < fim; ++i) {
                stringstream ss(chunk[i]);
                string valor;
                vector<string> campos;
                while (getline(ss, valor, ',')) {
                    campos.push_back(valor);
                }

                stringstream codificada;
                for (size_t j = 0; j < campos.size(); j++) {
                    const string& nomeCol = nomeColunas[j];
                    if (mapas.count(nomeCol)) {
                        codificada << mapas.at(nomeCol).at(campos[j]);
                    } else {
                        codificada << campos[j];
                    }
                    if (j < campos.size() - 1) codificada << ",";
                }

                linhasCodificadas[i] = codificada.str();
            }
        });
    }

    for (auto& t : threads) t.join();

    return linhasCodificadas;
}

inline void gerar_dataset_codificado(const string& caminho,
                                     const vector<string>& nomeColunas,
                                     const vector<int>& indicesAlvo,
                                     const unordered_map<string, unordered_map<string, int>>& mapas,
                                     size_t linhas_por_chunk) {
    ifstream entrada(caminho);
    ofstream saida("dataset_codificado_th.csv");

    if (!entrada.is_open() || !saida.is_open()) {
        cerr << "Erro ao abrir arquivos." << endl;
        return;
    }

    string cabecalho;
    getline(entrada, cabecalho);
    saida << cabecalho << "\n";

    queue<pair<size_t, vector<string>>> fila_chunks;
    map<size_t, vector<string>> resultados;

    mutex mtx_fila, mtx_resultado;
    condition_variable cv_fila, cv_resultado;
    atomic<bool> leitura_concluida{false};

    size_t seq_id = 0;
    const unsigned N_CONSUMIDORES = thread::hardware_concurrency();
    vector<thread> consumidores;

    // CONSUMIDORES
    for (unsigned t = 0; t < N_CONSUMIDORES; ++t) {
        consumidores.emplace_back([&]() {
            while (true) {
                pair<size_t, vector<string>> item;

                {
                    unique_lock<mutex> lock(mtx_fila);
                    cv_fila.wait(lock, [&]() {
                        return !fila_chunks.empty() || leitura_concluida;
                    });

                    if (!fila_chunks.empty()) {
                        item = move(fila_chunks.front());
                        fila_chunks.pop();
                    } else if (leitura_concluida) {
                        break;
                    } else {
                        continue;
                    }
                }

                auto codificadas = codificar_linhas_chunk(item.second, nomeColunas, mapas);

                {
                    lock_guard<mutex> lock(mtx_resultado);
                    resultados[item.first] = move(codificadas);
                }

                cv_resultado.notify_one();
            }
        });
    }

    // ESCRITOR
    thread escritor([&]() {
        size_t proximo_seq = 0;

        while (true) {
            vector<string> linhas;
            {
                unique_lock<mutex> lock(mtx_resultado);
                cv_resultado.wait(lock, [&]() {
                    return resultados.count(proximo_seq) || (leitura_concluida && resultados.empty());
                });

                if (resultados.count(proximo_seq)) {
                    linhas = move(resultados[proximo_seq]);
                    resultados.erase(proximo_seq);
                    ++proximo_seq;
                } else if (leitura_concluida && resultados.empty()) {
                    break;
                } else {
                    continue;
                }
            }

            for (const auto& linha : linhas) {
                saida << linha << "\n";
            }
        }
    });

    // PRODUTOR (leitura do arquivo)
    string linha;
    vector<string> chunk;
    while (getline(entrada, linha)) {
        chunk.push_back(linha);
        if (chunk.size() >= linhas_por_chunk) {
            {
                lock_guard<mutex> lock(mtx_fila);
                fila_chunks.emplace(seq_id++, move(chunk));
            }
            cv_fila.notify_one();
            chunk.clear();
        }
    }

    if (!chunk.empty()) {
        {
            lock_guard<mutex> lock(mtx_fila);
            fila_chunks.emplace(seq_id++, move(chunk));
        }
        cv_fila.notify_one();
    }

    leitura_concluida = true;
    cv_fila.notify_all();
    cv_resultado.notify_all();

    for (auto& th : consumidores) th.join();
    escritor.join();

    entrada.close();
    saida.close();
}

#endif // NORMALIZACAO_THREADS_HPP
