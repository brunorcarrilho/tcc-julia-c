#=
Este programa faz a distribuição de NUM_P elementos dos arrays vetorg e vetorp para n processos
utilizando pmap. Esses elementos são processados e retornados ao processo 1.
Pmap é estruturado para o caso em que cada chamada de função faz uma quantia enorme de trabalho, não sendo
esse o caso desse programa, pmap parece subótimo em comparação aos demais.
=#
using BenchmarkTools
using Distributed
using Statistics
using Random
addprocs(3)
#Utilizamos algo similar a um #define em C para deixar o código mais limpo e compreensível
#NUM_P se refere ao número de iterações que serão feitas
const NUM_P=1000*1000

Random.seed!(1234)

function popula_vetor()::Array{Int8}
    return rand(1:100, NUM_P)
end

@everywhere function calcpint(x,y::Int8)::Float64
	return (sin(cos(x))*(tan(sin(cos(y)))))
end

const vetorp=popula_vetor()
const vetorg=popula_vetor()

b=@benchmarkable pmap(calcpint, vetorg, vetorp) seconds=1500 samples=20 time_tolerance=0.01 memory_tolerance=0.01
println(mean(run(b)))
