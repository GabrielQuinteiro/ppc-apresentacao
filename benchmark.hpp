
#ifndef BENCHMARK_HPP
#define BENCHMARK_HPP

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <iomanip>
#include "sistema.hpp"
#include "leitura.hpp"

using namespace std;

struct BenchmarkInfo {
    string nome_arquivo;
    size_t tamanho_mb;
    size_t ram_disponivel_mb;
    size_t ram_total_mb;
    size_t bytes_por_linha;
    size_t linhas_por_chunk;
    double tamanho_chunk_mb;
    string cpu;
    int num_threads;
    int colunas_categoricas;
    double tempo_execucao;
};

inline BenchmarkInfo coletar_info_benchmark(const string& caminho_dataset,
                                            const vector<Linha>& amostras,
                                            int colunas_categoricas,
                                            int num_threads
                                            ) {
    BenchmarkInfo info;
    info.nome_arquivo = extrair_nome_arquivo(caminho_dataset);
    info.tamanho_mb = get_tamanho_arquivo_mb(caminho_dataset);
    info.ram_disponivel_mb = get_memoria_disponivel_mb();
    info.ram_total_mb = get_memoria_total_mb();
    info.bytes_por_linha = calcular_bytes_por_linha_com_amostras(amostras);
    info.linhas_por_chunk = calcular_linhas_por_chunk(info.ram_disponivel_mb, info.bytes_por_linha);
    info.tamanho_chunk_mb = calcular_tamanho_chunk_mb(info.bytes_por_linha, info.linhas_por_chunk);
    info.num_threads = num_threads;
    info.colunas_categoricas = colunas_categoricas;
    info.cpu = get_nome_cpu();
    return info;
}

inline void exibir_info_benchmark(const BenchmarkInfo& info) {
    cout << "\n===== INFOS =====" << endl;
    cout << "Nome do arquivo: " << info.nome_arquivo << endl;
    cout << "Tamanho do arquivo: " << info.tamanho_mb << " MB" << endl;
    cout << "RAM disponivel: " << info.ram_disponivel_mb << " MB" << endl;
    cout << "RAM total do sistema: " << info.ram_total_mb << " MB" << endl;
    cout << "Bytes por linha (estimado): " << info.bytes_por_linha << endl;
    cout << "Linhas por chunk: " << info.linhas_por_chunk << endl;
    cout << "Tamnho por chunk: " << info.tamanho_chunk_mb << " MB" << endl;

    cout << "Processador: " << info.cpu << endl;
    cout << "Cores (threads): " << info.num_threads << endl;
    cout << "Colunas categóricas detectadas: " << info.colunas_categoricas << endl;
    cout << "======================\n" << endl;
}

inline string formatar_tempo(double segundos) {
    int minutos = static_cast<int>(segundos) / 60;
    int seg = static_cast<int>(segundos) % 60;
    ostringstream oss;
    oss << minutos << "m " << seg << "s";
    return oss.str();
}

inline void salvar_info_benchmark_csv(const BenchmarkInfo& info, const string& caminho_saida) {
    bool arquivo_existe = ifstream(caminho_saida).good();
    ofstream arq(caminho_saida, ios::app);
    if (!arq.is_open()) {
        cerr << "Erro ao salvar informacoes no CSV." << endl;
        return;
    }

    if (!arquivo_existe) {
        arq << "nome_arquivo,nome_cpu,num_threads,ram_total_mb,ram_disponivel_mb,tamanho_arq_mb,bytes_por_linha,linhas_por_chunk,tamanho_chunk_mb,total_col_categoricas,tempo_execucao_segundos,tempo_formatado\n";
    }

    arq << info.nome_arquivo << ","
        << '"' << info.cpu << '"' << ","
        << info.num_threads << ","
        << info.ram_total_mb << ","
        << info.ram_disponivel_mb << ","
        << info.tamanho_mb << ","
        << info.bytes_por_linha << ","
        << info.linhas_por_chunk << ","
        << fixed << setprecision(2) << info.tamanho_chunk_mb << ","
        << info.colunas_categoricas << ","
        << fixed << setprecision(2) << info.tempo_execucao << ","
        << formatar_tempo(info.tempo_execucao) << "\n";

    arq.close();
}

#endif // BENCHMARK_HPP
