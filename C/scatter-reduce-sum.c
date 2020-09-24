//Faz uma operação complexa de NUM_P elementos, que são distribuídos entre size processos através
//do comando MPI_Scatter.
//Reduz todos os valores para um só valor, armazenado em global_sum, através de MPI_Reduce.
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <time.h>
#define NUM_P 1000*1000
double calc_real_p(short int x, short int y){
	return (sin(cos(x))*(tan(sin(cos(y)))));}
int main(int argc,char *argv[]){
	//declaração de variáveis auxiliares
	int dif=0, ta, rank, size, type=99;
	double tata;
	//criando uma seed para o random
	srand(time(NULL));
	//declarações de MPI
	MPI_Init(&argc,&argv);
	MPI_Status status;
	MPI_Comm_size(MPI_COMM_WORLD, &size);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	if (NUM_P<(size)) {
		printf("Erro: Foi entrado um valor de dados NUM_P inferior ao numero de workers.\n");
		return 0;}
	//Encontramos o tamanho para podermos dividir os dados do vetor para que possam ser
	//enviados para os workers.
	//Utilizamos tata que é double caso a divisão NUM_P/size seja inexata
	//Passamos a ta o valor inteiro de tata para podermos compara-los mais a frente
	tata=(double)NUM_P/size;
	ta=(int)tata;
	//Verificando se sobrou resto na divisão NUM_P/size, caso tenha sobrado
	//signifca que o processo 0 deverá pegar a porção dele de dados normal
	//'ta' e mais um adicional que será dado por 'dif'
	if (tata-(double)ta!=0 && rank==size-1) {
		dif=NUM_P-ta*size;}
	double local_sum=0;
	double global_sum=0;
	//os vetores a seguir definem nossos dados iniciais que serão processados.
	//Populamos os vetores através de loops simples.
	//Para termos os mesmo números que o random contraparte em Julia, restringimos o valor que eles
	//podem tomar para [1..100]
	short int *vetorp=(short int*)malloc(NUM_P*sizeof(short int));
	short int *vetorg=(short int*)malloc(NUM_P*sizeof(short int));
	if (rank==size-1) {
		for(int i=0; i<NUM_P; i++){
			vetorp[i]=rand() % 100+1;
			vetorg[i]=rand() % 100+1;}
	}
	//Iniciando vetores de apoio para os quais o scatter irá passar a quantidade ta
	//de elementos tanto de vetorg para vetorgg quanto de vetorp para vetorpp
	short int *vetorpp=(short int*)malloc(ta*sizeof(short int));
	short int *vetorgg=(short int*)malloc(ta*sizeof(short int));
	int repeticao=20;
	//Iniciando a medição do tempo de execução
	clock_t Ticks[2];
	Ticks[0]=clock();
	while(repeticao>0){
		repeticao-=1;
		//Enviando vetorg e vetorp para os workers através de MPI_Scatter
		//O envio ocorre de um vetorg para o vetorgg e de um vetorp para o vetorpp;
		//isso ocorre porque o processo mestre também envia para a sua própria instância do vetor
		//escolhido a receber os dados. Logo o processo mestre fazendo o scatter em vetorg
		//enviaria do próprio vetorg do processo mestre para o vetorg do processo mestre,
		//sendo o vetorg do processo mestre então o vetor que provê os dados e o vetor que os receberia.
		//Por esses motivos, é uma operação ilegal no sistema.
		MPI_Scatter(vetorg, ta, MPI_SHORT, vetorgg, ta, MPI_SHORT, size-1, MPI_COMM_WORLD);
		MPI_Scatter(vetorp, ta, MPI_SHORT, vetorpp, ta, MPI_SHORT, size-1, MPI_COMM_WORLD);
		//Aqui executamos a caixa preta em cada um dos processos
		for(int i=0; i<ta; i++){
			local_sum+=calc_real_p(vetorpp[i], vetorgg[i]);}
			//O processo mestre executa a caixa preta nos elementos que não foram distribuídos por
			//terem ficado como um resto na divisão inteira para calcular 'ta'.
			if (rank==size-1 && dif!=0) {
				for (int i = 0; i < dif; i++) {
					local_sum+=calc_real_p(vetorp[NUM_P-dif+i], vetorg[NUM_P-dif+i]);}
			}
		//Após o cálculo dos valores, usamos o MPI_Reduce para encontrarmos o valor final de global_sum
		MPI_Reduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, size-1, MPI_COMM_WORLD);
	}
		MPI_Finalize();
		//Paramos o relógio e imprimimos o tempo de execução
		Ticks[1]=clock();
		double Tempo = (Ticks[1]-Ticks[0]) * 1000.0 / CLOCKS_PER_SEC;
	if (rank==size-1) {
			printf("Tempo de execução médio:%fms\n", Tempo/20);
			printf("Valor da soma global= %f\n", global_sum);}
	return 0;
}