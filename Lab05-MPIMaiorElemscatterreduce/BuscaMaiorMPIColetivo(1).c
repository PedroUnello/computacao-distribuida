#include <mpi.h>
#include <stdio.h>
#include <stdlib.h> //rand()
#include <time.h>   //time()

/*
    Pedro Unello Neto
        41929713
*/


int main(int argc, char** argv) {
    
    //quantidade de valores por processo
    int qtdValores = argc > 1 //Se nao for informado, define como 1
                            ? atoi(argv[1]) > 0 //Se for informado porem atoi resulte em menos que 1
                                ? atoi(argv[1]) 
                                : 1 //Tambem define como 1
                            : 1;

    //MPI
    MPI_Init(&argc, &argv);

    // Numero de processos
    int tamDados;
    MPI_Comm_size(MPI_COMM_WORLD, &tamDados);

    // Rank do processo
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //Seed para num aleatorio
    srand(time(NULL));

    float* meusDados = malloc(sizeof(float) * qtdValores); //Aloca o vetor de dados locais, para cada processo

    if (rank == 0){
        float* dados = malloc(sizeof(float) * qtdValores * tamDados); //Processo 0 aloca 
                                                                      //todo o espacamento de memoria da base de dados
        printf("Base de dados:");                                                         
        for (int k = 0; k < qtdValores * tamDados; k++){ //Cria qtdValores valores para cada processo (* tamDados)
            *(dados + k) = ((float) rand() / (float)(RAND_MAX)) - 0.5f; //Valora cada entrada na base de dados (rank 0)
            printf(" %f", *(dados + k)); //Printa para melhor compreensao
        }

        //Utiliza do scatter para distribuir a base de dados
        MPI_Scatter(dados, qtdValores, MPI_FLOAT, meusDados, qtdValores, MPI_FLOAT, 0, MPI_COMM_WORLD); 
        free(dados); //Libera memoria alocada
    }
    else {  //Para o caso dos outros processos, nao e necessario alocar e/ou enviar a base de dados
        MPI_Scatter(NULL, 0, MPI_FLOAT, meusDados, qtdValores, MPI_FLOAT, 0, MPI_COMM_WORLD); //Logo send_buf e send_count
                                                                                                 //sao nulos
    }

    float dado = 0; //Dado corresponde ao maior valor local no processo
    for (int l = 0; l < qtdValores; l++){ //Utiliza um simples laco em tamanho n (qtdValores) para achar o maior localmente
        if (*(meusDados + l) > dado) { dado = *(meusDados + l); }
    }
    free(meusDados); //Libera a base de dados locais

    if (rank == 0) { //Apenas para a raiz (neste caso rank 0)
        float result; //Recebe o resultado da reducao em result 
        MPI_Reduce(&dado, &result, 1, MPI_FLOAT, MPI_MAX, 0, MPI_COMM_WORLD); //Utiliza MPI_MAX em cada resultado local
                                                                              //, e o rank 0 recebe em result
        printf("\nMaior: %f\n", result);  //Printa o resultado
    }
    else {
        MPI_Reduce(&dado, NULL, 1, MPI_FLOAT, MPI_MAX, 0, MPI_COMM_WORLD); //Os ademais processos de COMM_WORLD enviam dado na reducao, 
                                                                           //porem nao recebem o resultado
    }

    MPI_Finalize();
}
