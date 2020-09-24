//Este programa faz a distribuição de NUM_P elementos dos arrays vetorg e vetorp para size processos
//utilizando MPI_Send e MPI_Recv. Esses elementos são processados e retornados ao processo mestre através
//do MPI_Send e MPI_Recv.
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <time.h>
#define NUM_P 1000*1000

double calc_real_p(short int x, short int y){
	return (sin(cos(x))*(tan(sin(cos(y)))));
}

int main(int argc, char** argv)
{
	//declaração de variáveis auxiliares
	int dif=0, ta, rank, size, type=99;
	double tata;
	//criando uma seed para o random
	srand(time(NULL));

	//os vetores a seguir definem nossos dados iniciais que serão processados.
	short int *vetorp=(short int*)malloc(NUM_P*sizeof(short int));
	short int *vetorg=(short int*)malloc(NUM_P*sizeof(short int));

	double *vetorresultado=(double*)malloc(NUM_P*sizeof(double));

	//declarações de MPI
	MPI_Init(&argc, &argv);
	MPI_Status status;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);


	//Populamos os vetores através de loops simples.
	//Para termos os mesmo números que o random contraparte em Julia, restringimos o valor que eles
	//podem tomar para [1..100]
	//Utilizamos um if antes de popularmos os vetores. O processo 0 é o único que possui esses valores,
	//garantindo que SE E SOMENTE SE ocorrer o envio de mensagem pelo processo 0 aos workers, eles terão acesso
	//aos valores produzidos.
	if (rank==0) {
			for(int i=0; i<NUM_P; i++){
				vetorp[i]=rand() % 100+1;
				vetorg[i]=rand() % 100+1;
			}
	}


	if (NUM_P<(size)) {
		printf("Erro: Foi entrado um valor de dados NUM_P inferior ao numero de workers.\n");
		return 0;
	}

	//Encontramos o tamanho para podermos dividir os dados do vetor para que possam ser
	//enviados para os workers.
	//Utilizamos tata que é double caso a divisão NUM_P/size seja inexata
	//Passamos a ta o valor inteiro de tata para podermos compara-los mais a frente
	tata=(double)NUM_P/size;
	ta=(int)tata;

	//Verificando se sobrou resto na divisão NUM_P/size, caso tenha sobrado
	//signifca que o processo 0 deverá pegar a porção dele de dados normal
	//'ta' e mais um adicional que será dado por 'dif'
	if (tata-(double)ta!=0 && rank==0) {
		dif=NUM_P-ta*size;
	}

	double *vetorprovisorio=(double*)malloc(ta*sizeof(double));

	int repeticao=20;
	//Iniciando a medição do tempo de execução
	clock_t Ticks[2];
	Ticks[0]=clock();
	while (repeticao>0) {
	repeticao-=1;

	//Enviando vetorg e vetorp para os workers através de mensagens simples
	if (rank==0){
		for (int i=1; i<size; i++){
			MPI_Send(&vetorg[(i-1)*ta], ta, MPI_SHORT, i, type, MPI_COMM_WORLD);
			MPI_Send(&vetorp[(i-1)*ta], ta, MPI_SHORT, i, type, MPI_COMM_WORLD);
		}
	}
	else{
		//os demais processos recebem do processo 0 suas porções dos dados
		MPI_Recv(vetorg, ta, MPI_SHORT, 0, type, MPI_COMM_WORLD, &status);
		MPI_Recv(vetorp, ta, MPI_SHORT, 0, type, MPI_COMM_WORLD, &status);
	}

	//Aqui executamos a caixa preta em cada um dos workers
	if (rank!=0) {
		for(int i=0; i<ta; i++){
			vetorprovisorio[i]=calc_real_p(vetorp[i],vetorg[i]);
		}
	}

	//Os workers retornam os valores encontrados para o processo 0
	if (rank!=0) {
		MPI_Send(vetorprovisorio, ta, MPI_DOUBLE, 0, type, MPI_COMM_WORLD);
	}
	else{
		//executamos a caixa preta no processo 0 e o resultado do calculo vai
		//ser armazenado direto no vetorresultado
		for (int i = 0; i<ta+dif; i++) {
			vetorresultado[NUM_P-ta-dif+i]=calc_real_p(vetorp[NUM_P-ta-dif+i],vetorg[NUM_P-ta-dif+i]);
		}
		//o processo 0 recebe os valores encontrados nos workers
		for (int i = 1; i < size; i++) {
			MPI_Recv(&vetorresultado[(i-1)*ta], ta, MPI_DOUBLE, i, type, MPI_COMM_WORLD, &status);
		}
	}

}
	MPI_Finalize();
	//Paramos o relógio e imprimimos o tempo de execução
	Ticks[1]=clock();
	double Tempo = (Ticks[1]-Ticks[0]) * 1000.0 / CLOCKS_PER_SEC;
	if (rank==0) {
		printf("Tempo de execução médio:%fms\n", Tempo/20);
	}
	return 0;
}
