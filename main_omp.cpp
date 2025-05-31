
#include <iostream>
#include <vector>
#include <string>
#include <omp.h>
#include <unordered_map>
#include "leitura.hpp"
#include "sistema.hpp"
#include "benchmark.hpp"
#include "normalizador_omp.hpp"

constexpr const char* ARQUIVO_DATASET = "dataset_00_sem_virg.csv";

int main() {
    vector<string> nomeColunas;
    vector<int> indicesAlvo;
    vector<Linha> amostras;

    double inicio = omp_get_wtime();

    ler_cabecalho_e_amostras(ARQUIVO_DATASET, nomeColunas, amostras, 10);

    detectar_colunas_categoricas(nomeColunas, amostras, indicesAlvo);

    cout << "Quantidade de colunas categoricas detectadas: " << indicesAlvo.size() << endl;
    cout << "Colunas categoricas detectadas:" << endl;
    for (int indice : indicesAlvo) {
        if (indice < nomeColunas.size()) {
            cout << "- " << nomeColunas[indice] << endl;
        }
    }

    BenchmarkInfo info = coletar_info_benchmark(ARQUIVO_DATASET, amostras, indicesAlvo.size(), omp_get_max_threads());
    info.linhas_por_chunk = calcular_linhas_por_chunk(info.ram_disponivel_mb, info.bytes_por_linha);
    exibir_info_benchmark(info);

    unordered_map<string, unordered_map<string, int>> mapas;
    unordered_map<string, int> contadores;
    construir_mapas_codificacao_por_chunks(ARQUIVO_DATASET, nomeColunas, indicesAlvo, info.linhas_por_chunk, mapas, contadores);

    gerar_arquivos_id_por_coluna(mapas);
    gerar_dataset_codificado(ARQUIVO_DATASET, nomeColunas, indicesAlvo, mapas, info.linhas_por_chunk);

    double fim = omp_get_wtime();
    info.tempo_execucao = fim - inicio;

    salvar_info_benchmark_csv(info, "benchmark_omp.csv");

    cout << "Tempo de execucao: " << info.tempo_execucao << " segundos" << endl;
    cout << "Tempo de execucao formatado: " << formatar_tempo(info.tempo_execucao) << endl;
    return 0;
}
