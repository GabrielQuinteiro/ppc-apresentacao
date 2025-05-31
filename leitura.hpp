
#ifndef LEITURA_HPP
#define LEITURA_HPP

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

using namespace std;
using Linha = vector<string>;

#define MB (1024 * 1024)

// remove espaços, \r, \n e \t do início e fim da string
static inline string trim(const string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// detecta se a string representa um numero (int/float)
inline bool eh_numero(const string& valor) {
    string val = trim(valor);
    if (val.empty()) return false;

    // remove aspas externas, se houver
    if (val.front() == '"' && val.back() == '"' && val.length() > 1)
        val = val.substr(1, val.length() - 2);

    bool ponto = false;
    size_t i = 0;
    if (val[0] == '-' || val[0] == '+') i++;

    for (; i < val.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(val[i]);
        if (!isdigit(c)) {
            if (c == '.' && !ponto) {
                ponto = true;
            } else {
                return false;
            }
        }
    }
    return true;
}

inline void detectar_colunas_categoricas(const vector<string>& nomeColunas,
                                         const vector<vector<string>>& amostras,
                                         vector<int>& indicesAlvo) {

    size_t numLinhas = min<size_t>(amostras.size(), 5);

    for (size_t col = 0; col < nomeColunas.size(); ++col) {
        bool temAlfanumerico = false;

        // se em alguma das linhas existir valor “não vazio” e não-numérico, é categórica
        for (size_t lin = 0; lin < numLinhas; ++lin) {
            const string& raw = amostras[lin][col];
            string val = trim(raw);
            if (!val.empty() && val != "\"\"" && !eh_numero(val)) {
                temAlfanumerico = true;
                break;
            }
        }

        if (temAlfanumerico) {
            indicesAlvo.push_back(col);
        }
    }
}


inline void ler_cabecalho_e_amostras(const string& caminho,
                                     vector<string>& nomeColunas,
                                     vector<Linha>& amostras,
                                     int linhasAmostra = 5) {
    ifstream arquivo(caminho);
    if (!arquivo.is_open()) {
        cerr << "Nao foi possivel abrir o arquivo." << endl;
        exit(1);
    }

    string cabecalho;
    getline(arquivo, cabecalho);
    stringstream ss(cabecalho);
    string coluna;
    while (getline(ss, coluna, ',')) {
        nomeColunas.push_back(coluna);
    }

    string linha;
    while (linhasAmostra-- > 0 && getline(arquivo, linha)) {
        stringstream ssl(linha);
        string valor;
        Linha linhaDados;
        while (getline(ssl, valor, ',')) {
            linhaDados.push_back(valor);
        }
        amostras.push_back(linhaDados);
    }
    arquivo.close();
}


size_t calcular_tamanho_chunk_mb(size_t bytes_por_linha, size_t linhas_por_chunk) {
    return (bytes_por_linha * linhas_por_chunk) / MB;
}

size_t calcular_linhas_por_chunk(size_t memoria_disponivel_mb, size_t media_bytes_por_linha = 200) {
    size_t memoria_bytes = memoria_disponivel_mb * MB;
    size_t memoria_para_uso = memoria_bytes / 25; // Usa 10% da RAM disponível
    return memoria_para_uso / media_bytes_por_linha;
}

inline size_t calcular_bytes_por_linha_com_amostras(const vector<Linha>& amostras) {
    if (amostras.empty()) return 200;
    size_t total = 0;
    for (const auto& linha : amostras) {
        for (const auto& campo : linha) {
            total += campo.size();
        }
        total += linha.size() - 1; // vírgulas
    }
    return total / amostras.size();
}

#endif // LEITURA_HPP
