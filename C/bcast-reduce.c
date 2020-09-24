//Este programa distribui os vetores de dados vetorg e vetorp para os workers através de
//MPI_Bcast e, dependendo se o rank do processo for par ou ímpar, executa uma operação específica.
//No final, soma todos os resultados através de MPI_Reduce para um único vetorresultado no processo
//mestre.
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <time.h>
#define NUM_P 1000*20000
double calc_real_p_par(int x, int y){
	return (sin(cos(x))*(tan(sin(cos(y)))));}
double calc_real_p_impar(int x, int y){
	return (sin(cos(x))*(tan(sin(cos(y)))))*1/tan(sin(cos(x*y)));}

int main(int argc,char *argv[])
{
	//declaração de variáveis auxiliares
	int rank, size, type=99;
	//criando uma seed para o random
	srand(time(NULL));
	//os vetores a seguir definem nossos dados iniciais que serão processados.
	//Populamos os vetores através de loops simples.
	//Para termos os mesmo números que o random contraparte em Julia, restringimos o valor que eles
	//podem tomar para [1..100]

	//declarações de MPI
	MPI_Init(&argc,&argv);
	MPI_Status status;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//Encontramos o tamanho para podermos dividir os dados do vetor para que possam ser

	double *vetorprovisorio=(double*)malloc(NUM_P*sizeof(double));

	short int *vetorp=(short int*)malloc(NUM_P*sizeof(short int));
	short int *vetorg=(short int*)malloc(NUM_P*sizeof(short int));

	if (rank==size-1) {
		for(int i=0; i<NUM_P; i++){
			vetorp[i]=rand() % 100+1;
			vetorg[i]=rand() % 100+1;
		}
	}
	double *vetorresultado=(double*)malloc(NUM_P*sizeof(double));

	int repeticao=20;
	//Iniciando a medição do tempo de execução
	clock_t Ticks[2];
	Ticks[0]=clock();
	while (repeticao>0) {
		repeticao-=1;

	//Enviando vetorg e vetorp para os workers através de MPI_Bcast
	MPI_Bcast(vetorg, NUM_P, MPI_SHORT, size-1, MPI_COMM_WORLD);
	MPI_Bcast(vetorp, NUM_P, MPI_SHORT, size-1, MPI_COMM_WORLD);


	//Aqui executamos a caixa preta em cada um dos processos dependendo do seu número do rank
	//ser par ou ímpar.

	if (rank%2==0) {
		for(int i=0; i<NUM_P; i++){
			//printf("rank[%d] -> Vetorp[%d]=%d e Vetorg[%d]=%d\n", rank , i, vetorp[i], i, vetorg[i]);
			vetorprovisorio[i]=calc_real_p_par(vetorp[i], vetorg[i]);
			//printf("rank[%d] -> vetorprovisorio[%d]=%f\n", rank , i, vetorprovisorio[i]);
		}
	}
	else{
		for(int i=0; i<NUM_P; i++){
			//printf("rank[%d] -> Vetorp[%d]=%d e Vetorg[%d]=%d\n", rank , i, vetorp[i], i, vetorg[i]);
			vetorprovisorio[i]=calc_real_p_impar(vetorp[i], vetorg[i]);
			//printf("rank[%d] -> vetorprovisorio[%d]=%f\n", rank , i, vetorprovisorio[i]);
		}
	}

	//Após o cálculo dos valores, voltamos com todos os valores para o processo mestre
	MPI_Reduce(vetorprovisorio, vetorresultado, NUM_P, MPI_DOUBLE, MPI_SUM, size-1, MPI_COMM_WORLD);
}
	MPI_Finalize();
	Ticks[1]=clock();
	double Tempo = (Ticks[1]-Ticks[0]) * 1000.0 / CLOCKS_PER_SEC;
	if (rank==size-1) {
		//Paramos o relógio e imprimimos o tempo de execução
		printf("Tempo de execução:%fms\n", Tempo/20);
	}
	return 0;
}