/*
Esse programa tem como objetivo simular e utilizar RemoteChannels para gerar um pipeline.
O programa gera um número NUM_P de vetores de N casas populados com variáveis aleatórias
para cada um dos dois vetores: vetorg e vetorp.
Esses vetores são gerados no processo '0' e são recuperados pelos demais processos através da troca de
mensagens por MPI.
Nos processos 1:n, os vetores vetorg e vetorp têm cada um de seus elementos passados pela função
calc_real_p.
Após terem sido passados pela função, os valores encontrados são devolvidos ao processo 0.
*/
#include <stdbool.h>
#include <stdio.h>
#include <mpi.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#define NUM_P 1000
#define N 10000
double calc_real_p(int x, int y){
	return (sin(cos(x))*(tan(sin(cos(y)))));}
int main(int argc, char** argv) {
	//declaração de variáveis auxiliares
	int rank, size;
	//declarações de MPI
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	double *vetorresultado=(double*)malloc((N*NUM_P)*sizeof(double));
	int repeticao=20;
	//Iniciando a medição do tempo de execução
	clock_t Ticks[2];
	Ticks[0]=clock();
	while(repeticao>0){
		repeticao-=1;
		//Produtor
		if (rank == 0) {
			srand(time(NULL));
			short int *vetorp=(short int*)malloc(N*sizeof(short int));
			short int *vetorg=(short int*)malloc(N*sizeof(short int));
			double *temp=(double*)malloc((N)*sizeof(double));
			for (int j = 1; j <= NUM_P; ++j) {
				MPI_Status status;
				//populando vetorg e vetorp para ser passado a um dos demais processos.
				for (int i = 0; i<N; i++) {
					vetorg[i]=rand()%100+1;
					vetorp[i]=rand()%100+1;}
				//Aqui recebe-se o vetor temp com os valores já passados por calc_real_p.
				//Caso seja a primeira vez do consumidor entrando em contato com o produtor,
				//essa mensagem serve somente para ligá-los e, após a primeira comunicação,
				//passa a ter dados reais até o consumidor ser desligado.
				MPI_Recv(temp, N, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
				//Na primeira comunicação, o TAG virá como 0, este if serve para separar a primeira comunicação que não deverá ter seu valor cometido ao vetorresultado das demais.
				if (status.MPI_TAG > 0) {
					for (int i=0; i<N; i++) {
						vetorresultado[((status.MPI_TAG-1)*N)+i]=temp[i];}
				}
				//Aqui enviamos os vetores vetorg e vetorp para os demais processos.
				MPI_Send((vetorg), N, MPI_SHORT, status.MPI_SOURCE, j, MPI_COMM_WORLD);
				MPI_Send((vetorp), N, MPI_SHORT, status.MPI_SOURCE, j, MPI_COMM_WORLD);
			}
			//Aqui são rodadas as últimas execuções recebendo os últimos vetores e "encerrando o canal" com os demais processos.
			int num_terminated = 0;
			for (int num_terminated = 0; num_terminated < size-1; num_terminated++) {
				MPI_Status status;
				MPI_Recv(temp, N, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
				if (status.MPI_TAG > 0) {
					for (int i=0; i<N; i++) {
						vetorresultado[((status.MPI_TAG-1)*N)+i]=temp[i];}
				}
				//Aqui enviamos os sinais com tag 0 para indicar desligamento dos processos
				MPI_Send(vetorg, N, MPI_SHORT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
				MPI_Send(vetorp, N, MPI_SHORT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
			}
		}
		//Consumidores
		else {
			short int *vetorp=(short int*)malloc(N*sizeof(short int));
			short int *vetorg=(short int*)malloc(N*sizeof(short int));
			double *temp=(double*)malloc((N)*sizeof(double));
			//O sinal abaixo funciona como um greeting para o produtor. Serve para que eles possam estabelecer um canal entre eles.
			MPI_Send(temp, N, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
			bool terminated = false;
			do {
				MPI_Status status;
				MPI_Recv(vetorg, N, MPI_SHORT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
				MPI_Recv(vetorp, N, MPI_SHORT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
				//Aqui, caso TAG=0, isso indica o canal sendo fechado e o fim do loop.
				if (status.MPI_TAG == 0) {
					terminated = true;}
				else {
					//Aqui armazenamos em temp os valores passados pela função calc_real_p
					for (int i=0; i<N; i++) {
					  temp[i]=calc_real_p(vetorg[i], vetorp[i]);
					}
					MPI_Send(temp, N, MPI_DOUBLE, 0, status.MPI_TAG, MPI_COMM_WORLD);}
			} while (!terminated);
		}
	}
	MPI_Finalize();
    if (rank == 0) {
      Ticks[1]=clock();
      double Tempo = (Ticks[1]-Ticks[0]) * 1000.0 / CLOCKS_PER_SEC;
      printf("Tempo de execução médio:%fms\n", Tempo/20);}
}