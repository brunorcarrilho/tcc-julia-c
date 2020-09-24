#=
Este programa faz a distribuição de NUM_P elementos dos arrays vetorg e vetorp para n processos
utilizando @distributed. Esses elementos são processados e retornados ao processo 1.
=#
using BenchmarkTools
using Distributed
using Statistics
using Random
#Aqui utilizamos 5 processos no total. 4 workers são utilizados porque a rotina @distributed funciona
#de forma que esta somente utiliza os workers para paralelizar a computação. Se usássemos 4 processos,
#3 workers, somente 3 processos seriam utilizados enquanto o processo 1 ficaria subaproveitado.
#Essa mudança, utilizando NUM_P=1000000, já dá uma diferença de 100ms. O programa usando 5 processos
#termina 100ms mais rápido.
addprocs(4)
Random.seed!(1234)
#armazenarmos nosso resultado final em um SharedArray que é vetorresults, permitindo que façamos
#alterações diretamente nesse vetor dentro dos workers, ou seja, sem precisarmos voltar com os valores
#para o processo 1 com a finalidade de colocá-los num vetor de resultados.
using SharedArrays
#Utilizamos algo similar a um #define em C para deixar o código mais limpo e compreensível
#NUM_P se refere ao número de iterações que serão feitas
const NUM_P=1000*10000

function popula_vetor()::Array{Int8}
    return rand(1:100, NUM_P)
end

#essa é a função aleatória que usaremos para teste
@everywhere function calcpint(x,y::Int8)::Float64
	return (sin(cos(x))*(tan(sin(cos(y)))))
end
	#os vetores a seguir definem nossos dados iniciais que serão processados.
	const vetorp=popula_vetor()
	const vetorg=popula_vetor()

	#declaramos o SharedArray vetorresults que será o local de armazenamento dos resultados
	vetorresults=SharedArray{Float64}(NUM_P)


#aqui começamos a medir o tempo gasto
b=@benchmarkable begin
	#aqui atribuímos a casa i de vetorresults, o valor do cálculo resultante da casa i de vetorp
	#com a casa i de vetorg passados na caixa-preta, calcpint.
	#como já foi mencionado, os valores são setados diretamente no vetorresults pois este é um
	#SharedArray que está incluso em todos os processos.
	#a função @distributed partilha automaticamente as tarefas entre os workers.
	@sync @distributed (+) for i in 1:NUM_P
		vetorresults[i]=calcpint(vetorp[i],vetorg[i])
	end
end memory_tolerance=0.01 time_tolerance=0.01 seconds=1500 samples=20
println(median(run(b)))