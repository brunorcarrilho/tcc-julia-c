#=
Esse programa tem como objetivo utilizar RemoteChannels e @spawnat juntos para gerar um pipeline
Utilizando assim tanto de corotinas e do pacote Distributed.
O programa gera um número NUM_P de vetores de N casas populados com variáveis aleatórias
para cada um dos dois vetores: vetorg e vetorp.
Esses vetores são gerados no processo 1 e são recuperados pelos demais processos
através do take!() de RemoteChannels, sendo extraídos em forma de uma tupla.
Nos workers, os vetores são passados pela função calcpint através do comando map.
Após terem sido passados pelo work(), os valores encontrados são colocados num segundo RemoteChannel,
results, com objetivo de serem recuperados pelo processo 1 para armazenamento.
=#
using BenchmarkTools
using Distributed
using Statistics
using Random
Random.seed!(1234)
#=
Aqui utilizamos 5 processos no total. 4 workers são utilizados porque o desempenho entre utilizar 5 e 4 foi comparado e
o melhor resultado foi o que utilizava 5 processos, apesar da diferença ter sido pouca.
=#
addprocs(4)
#Utilizamos algo similar a um #define em C para deixar o código mais limpo e compreensível
#NUM_P se refere ao número de iterações que serão feitas
const NUM_P=1000
#N se refere ao tamanho dos vetores vetorg e vetorp, tanto quanto de vetorresultados
const N=1000
const ta = floor(Int32, NUM_P/nworkers())
#=
Utilizando os valores de NUM_P e N, é possível ajustar o código para algo orientado a ter mais
ou menos trocas de mensagens. Pode ser mais interessante ter poucas trocas de mensagens
mas um pipeline mais atrasado. Aumentando N e diminuindo NUM_P, se consegue poucas trocas
de mensagens e a mesma quantidade de elementos, porém, um atraso na saída.
=#

#aqui são declarados os RemoteChannels que serão usados
#seu propósito é permitir uma comunicação facilitada entre processos permitindo paralelismo
#I/O sem problemas
const inbox = RemoteChannel(()->Channel{}(300))

const results = RemoteChannel(()->Channel{}(300))

function popula_vetor()::Array{Int8}
	return rand(1:100, N)
end

@everywhere function calcpint(x,y::Int8)::Float64
	return (sin(cos(x))*(tan(sin(cos(y)))))
end

#esta função tem como objetivo servir como um encapsulamento para os argumentos e funções envolvidos
@everywhere function work(inbox, results)
	#um while que serve para ter certeza que inbox possui alguma coisa
	while !isready(inbox)
		wait(inbox)
	end
	b=take!(inbox)
	put!(results, (map(calcpint, b[1],b[2]),b[3]))
end

#Esta função é mais um encapsulamento para facilitar a medida do tempo usando o pacote BenchmarkTools
function englobe(inbox, results)

	#aqui, os workers passam os valores de vetorg e vetorp pela função calcpint.
	#iniciamos isso antes da produção, caso contrário, teríamos um deadlock com o processo 1
	#chegando ao limite do remotechannel e depois, esperando um take!, que nunca aconteceria.
	for i in 1:ta
		for p in workers()
			R=[@spawnat p work(inbox, results)]
		end
	end

	#aqui, caso NUM_P não seja divisível por nworkers(), é processado o resto(dif) pelo
	#último processo adicionado, o processo número nprocs().
	if dif!=0
		@spawnat nprocs() begin
			for i in 1:dif
				work(inbox, results)
			end
		end
	end

	#Parte do código referida ao processo 1 que lida com a população de vetorg e vetorp com
	#números aleatórios entre 1 e 100. A dimensão dos vetores é determinada por N.
	#Neste código, o processo 1 é o produtor enquanto os workers são os consumidores.

	for i in 0:NUM_P-1
		vetorp=popula_vetor()
		vetorg=popula_vetor()
		#envia os valores de vetorp e vetorg gerados de tamanho N para o canal inbox
		#o 'i' também é enviado, permitindo que o vetorresults não perca sua ordenação.
		#O motivo de i variar entre 0 e NUM_P-1, é para a ordem do vetor ser mantida
		#por razões matemáticas.
		put!(inbox, (vetorp, vetorg, i))
	end

	#Parte do código referida ao processo 1. Responsável por recuperar os resultados calculados
	#e armazená-los num vetor.
	for i in 1:NUM_P
		while !isready(results)
			wait(results)
		end

		buffer=take!(results)
		#buffer[2] é o 'i' armazenado anteriormente. Ele é utilizado para determinar a ordenação do vetorresults.
		for j in 1:N
			vetorresults[buffer[2]*N+j]=buffer[1][j]
		end
	end
end

#aqui determinamos se dif=0 ou não. se NUM_P/nworkers() não for um inteiro, precisamos compensar isso
#para podermos fazer o paralelismo adequado sem deixar de fazer o número exato de repetições NUM_P
if (NUM_P/nworkers()-floor(Int32, NUM_P/nworkers())!=0)
	const dif=NUM_P-ta*nworkers()
else const dif=0
end

vetorresults=zeros(Float64, NUM_P*N)

b=@benchmarkable englobe(inbox, results) memory_tolerance=0.01 time_tolerance=0.01 seconds=1500 samples=20
println(mean(run(b)))