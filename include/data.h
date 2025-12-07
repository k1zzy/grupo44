/**
 * @file data.h
 * @brief Interface para a estrutura de dados.
 *
 * define funcoes estruturas manipulacao dados automovel
 *
 * Projeto: Sistemas Distribuídos 2025/2026
 * Autor: José Cecílio
 * Data: 13/09/2025
 */
#ifndef _DATA_H
#define _DATA_H /* modulo data */

enum marca_t
{
	MARCA_TOYOTA,
	MARCA_BMW,
	MARCA_RENAULT,
	MARCA_AUDI,
	MARCA_MERCEDES
};

enum combustivel_t
{
	COMBUSTIVEL_GASOLINA,
	COMBUSTIVEL_GASOLEO,
	COMBUSTIVEL_ELETRICO,
	COMBUSTIVEL_HIBRIDO
};

/* estrutura dados automovel */
struct data_t
{
	int ano;
	float preco;
	enum marca_t marca;
	char *modelo;
	enum combustivel_t combustivel;
};

/* funcao cria novo elemento dados data_t
 * retorna nova estrutura ou null erro
 */
struct data_t *data_create(int ano, float preco, enum marca_t marca, const char *modelo, enum combustivel_t combustivel);

/* funcao elimina bloco dados liberta memoria
 * retorna 0 ok ou -1 erro
 */
int data_destroy(struct data_t *data);

/* funcao duplica estrutura data_t
 * retorna nova estrutura ou null erro
 */
struct data_t *data_dup(struct data_t *data);

/* funcao substitui conteudo elemento data_t
 * retorna 0 ok ou -1 erro
 */
int data_replace(struct data_t *data, int ano, float preco, enum marca_t marca, const char *modelo, enum combustivel_t combustivel);

#endif
