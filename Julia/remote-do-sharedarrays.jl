#=
Este programa disponibiliza os vetores de dados vetorg e vetorp para os workers através do uso de
SharedArrays e, dependendo se o myid() do processo for par ou ímpar, executa uma operação específica.
No final, soma todos os resultados através de map para um único vetorresultado no processo 1.
=#
using BenchmarkTools
using Distributed
addprocs(4)
using SharedArrays
using Statistics
using Random
Random.seed!(1234)
#Utilizamos algo similar a um #define em C para deixar o código mais limpo e compreensível
#NUM_P se refere ao número de iterações que serão feitas
const NUM_P=1000*1000

#O motivo do global ser utilizado na inicialização de vetorresults é para permitir seu uso dentro do for
global vetorresults
#O vetordezeros é utilizado para zerar o valor de vetorresults na medição de tempo
const vetordezeros=zeros(Float64, NUM_P)

#Aqui declaramos um RemoteChannel para cada um dos processos que utilizaremos para permitir o
#armazenamento depois de passar os dados pelas funções.
for p in procs()
	@fetchfrom p global inbox = RemoteChannel(()->Channel{}(1))
end

function popula_vetor()::Array{Int8}
    return rand(1:100, NUM_P)
end

#as funções a seguir representam uma caixa preta. é definida por qualquer procedimento simples
#pelo qual se queira passar os dados
#@everywhere incluido permitindo aos workers utilizar as funções.
@everywhere function calc_real_p_par(x,y::Int8)::Float64
	return (sin(cos(x))*(tan(sin(cos(y)))))
end

@everywhere function calc_real_p_impar(x,y::Int8)::Float64
	return (sin(cos(x))*(tan(sin(cos(y)))))*1/tan(sin(cos(x*y)))
end

#Essa função serve ao propósito de encapsular as funções calc_real_p_par e calc_real_p_impar,
#enquanto permite a passagem dos argumentos X e Y, que, apesar de estarem disponíveis nos workers,
#não é possível acessá-los sem tê-los passado como argumentos, já que os workers não possuem os
#rótulos dos SharedArrays.
@everywhere function work(X, Y)

	if (myid()/2)-floor(myid()/2)!=0
		put!(inbox, map(calc_real_p_impar, X, Y))
	else
		put!(inbox, map(calc_real_p_par, X, Y))
	end
end

#Devido a dificuldades na medição de tempo correta decorrentes da natureza do programa, tornou-se
#necessário utilizar a rotina @benchmarkable que funciona melhor tendo uma só função para gerenciar.
function paralelizar(x,y::SharedArray)
	#aqui ocorre a chamada aos processos para passar X e Y pelas funções. é preciso utilizarmos
	#RemoteChannels para permitirmos um paralelismo mais "solto", pois remote_do() não retorna nenhum
	#valor. assim não temos de fazer uma operação de fetch ou esperarmos o valor ser
	#retornado, mas também impede que recuperemos o valor sem utilizarmos do RemoteChannel ou
	#colocarmos o resultado num SharedArray.
	#É utilizado o SharedArrays para declarar X e Y pois todos os processos utilizam o valor.
	for p in procs()
		@async remote_do(work, p, x, y)

		#aqui recuperamos os vetores resultados da aplicação das funções calc_real_p_par ou
		#calc_real_p_impar, que estavam armazenados nos RemoteChannels, em cada um dos p workers,
		#permitindo que façamos uma operação similar a MPI_Reduce usando do operador MPI_SUM.
		#for p in workers()
		g=@fetchfrom p take!(inbox)
		global vetorresults=map(+, vetorresults, g)
	end
end

#Aqui declaramos X e Y como SharedArrays e os disponibilizamos em todos os processos.
X=SharedArray{Int8}(1, NUM_P)
Y=SharedArray{Int8}(1, NUM_P)

#os vetores a seguir definem nossos dados iniciais que serão processados.
vetorp=popula_vetor()
vetorg=popula_vetor()

#A rotina @benchmarkable se tornou necessária utilizar para que o valor de vetorresults não
#fosse modificado com cada uma das samples que ele roda. O valor de vetorresults é mantido
#uniforme devido ao setup que atribui seu valor para o mesmo de vetordezeros, que, como o nome implica,
#é um vetor populado por NUM_P zeros.
b=@benchmarkable begin setup=(global vetorresults=vetordezeros)
	#passagem de vetorg e vetorp para o SharedArray X e SharedArray Y
	@distributed for i=1:NUM_P
		X[1,i]=vetorp[i]
		Y[1,i]=vetorg[i]
	end
	paralelizar(X,Y)
end memory_tolerance=0.01 time_tolerance=0.01 seconds=1500 samples=20
println(mean(run(b)))
