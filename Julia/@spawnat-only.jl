#=
Este programa faz a distribuição de NUM_P elementos dos arrays vetorg e vetorp para n processos
utilizando @spawnat. Esses elementos são passados por uma função nos workers e retornam ao
processo 1.
=#
using BenchmarkTools
using Distributed
using Statistics
using Random

#Aqui são utilizados 5 processos no total. Este programa foi feito de forma que somente os workers
#processam os dados distribuídos, sendo assim, tendo só 3 processos trabalhando simultaneamente é
#subaproveitar o processador com 4 cores.
addprocs(4)
#Utilizamos algo similar a um #define em C para deixar o código mais limpo e compreensível
#NUM_P se refere ao número de iterações que serão feitas
const NUM_P=1000*10000

Random.seed!(1234)

function popula_vetor()::Array{Int8}
    return rand(1:100, NUM_P)
end

#utilizamos ta para encontrar o valor do intervalo do vetor que cada worker deverá computar
const ta = floor(Int32, NUM_P/nworkers())

#essa é a função aleatória que usaremos para teste
@everywhere function calcpint(x,y::Int8)::Float64
    return (sin(cos(x))*(tan(sin(cos(y)))))
end

#aqui determinamos se dif=0 ou não. se NUM_P/nworkers() não for um inteiro, precisamos compensar isso
#para podermos fazer o paralelismo adequado sem deixar de fazer o número exato de repetições NUM_P
if (NUM_P/nworkers()-floor(Int32, NUM_P/nworkers())!=0)
    const dif=NUM_P-ta*nworkers()
else const dif=0
end

#declaramos e populamos vetorp e vetorg com NUM_P números aleatórios de 1 a 100
vetorp=popula_vetor()
vetorg=popula_vetor()

#aqui começamos a medir o tempo gasto
b= @benchmarkable begin
	#buffer é um vetor necessário, caso NUM_P/nworkers() seja uma divisão inexata
	buffer=zeros(0)
	#aqui declaramos R, R[1]:R[nworkers()] são futuros que podem ser resgatados
	#fazendo R[1][1] acessamos ao primeiro futuro disponível no vetor R e acessamos dentro dele,
	#o primeiro elemento do primeiro vetor. fazendo R[1][1:ta] estamos pegando
	#o primeiro vetor do seu primeiro elemento até o último elemento do intervalo em que ele trabalhou
	R=[@spawnat i map(calcpint, vetorp[ta*(myid()-2)+1:ta*(myid()-1)], vetorg[ta*(myid()-2)+1:ta*(myid()-1)]) for i=2:nprocs()]

	#aqui, caso NUM_P não seja divisível por nworkers(), o resto, dif, é processado pelo processo 1
	#e são guardados num buffer os valores encontrados por ele
	if dif!=0
		append!(buffer, (map(calcpint, vetorp[NUM_P-dif+1:NUM_P], vetorg[NUM_P-dif+1:NUM_P])))
	end

	#é declarado o vetorresults que será o vetor de armazenamento dos resultados calculados
	vetorresults=zeros(Float64, 0)
	#neste for, que abrange todos os workers, estamos pegando os valores encontrados e, conforme
	#explicado mais acima, são armazenados os intervalos de cada R[i] atrelado a seu worker.
	for i in 2:nprocs()
		append!(vetorresults, R[i-1][1:ta])
	end
	#aqui, se dif for diferente de 0, são armazenados os valores "resto" calculados pelo processo 0
	append!(vetorresults, buffer)
    #println("Tamanho do vetor=", length(vetorresults))
    #println(vetorresults)
    return vetorresults
end seconds=1500 samples=20 time_tolerance=0.01 memory_tolerance=0.01
println(mean(run(b)))
