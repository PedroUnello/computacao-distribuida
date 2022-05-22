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

    float* dados = malloc(sizeof(float) * qtdValores); //Aqui cada processo aloca qtd necessaria para seus dados
                                                      //Porem poderia ser feito uma alocacao de todos os dados, ou escrita/leitura de arquivo

    float dado = 0; //Float individual para o maior valor local

    //Inicializa com valores aleatorios (para rank 0 -> manda aos ademais; para os ademais -> recebe no recv)
    if (rank == 0) {
        printf("Base de dados:");

        for (int i = 0; i < qtdValores; i++){ //Rank 0 preenche seus proprios dados
            *(dados + i) = ((float) rand() / (float)(RAND_MAX)) - 0.5f; //Atribui seu valor (rank 0)
            printf(" %f", *(dados + i));
        }

        float* dadosSend = malloc(sizeof(float) * qtdValores); //aloca mais qtdValores espacos para os dados externos
        for (int j = 1; j < tamDados; j++) //Para cada processo
        { 

            for (int k = 0; k < qtdValores; k++){ //Cria qtdValores valores
                *(dadosSend + k) = ((float) rand() / (float)(RAND_MAX)) - 0.5f; //Atribui seu valor (rank 0)
                printf(" %f", *(dadosSend + k));
            }
            MPI_Send(dadosSend, qtdValores, MPI_FLOAT, j, 0, MPI_COMM_WORLD); //Envia aos processos

        }
        printf("\n");
        free(dadosSend);
    }
    else {
        MPI_Recv(dados, qtdValores, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); //Recebem valores do rank 0
    }

    //Calcula o maior valor local
    for (int l = 0; l < qtdValores; l++){
        if (*(dados + l) > dado) { dado = *(dados + l); }
    }

    //Trabalha em pares de processos, definindo um meio onde se espelharam tais

    //aonde vai-se receber o maior num encontrado por outro processo
    float recv;  
    
    //consideirando a natureza da metodologia, defini como maximo num de processos usaveis um valor par
    int maximo = (tamDados % 2 == 0 ? tamDados - 1 : tamDados);
    int meio = ((int)(maximo / 2));

    //para o processo central
    if (rank == meio) {
        //recebe todos os valores calculados pelos pares de processos
        for (int j = maximo - 1; j > rank; j--){
            MPI_Recv(&recv, 1, MPI_FLOAT, j, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            //e substitui iteracao a iteracao pelo maior
            if (recv > dado) { dado = recv; } 
        }
    } 
    //para os acima do meio (menos o ultimo, para o caso de um num de processos par)
    else if (rank > meio && rank < maximo) 
    { 
        //Recebe o valor de seu par, abaixo do meio
        MPI_Recv(&recv, 1, MPI_FLOAT, (maximo - rank - 1), 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if (recv > dado) { dado = recv; } //atribui como dado o maior dos dois
        MPI_Send(&dado, 1, MPI_FLOAT, meio, 0, MPI_COMM_WORLD); //envia para o processo central
    }
    else if (rank < meio)
    { 
        //apenas envia seu maior valor encontrado
        MPI_Send(&dado, 1, MPI_FLOAT, (maximo - rank - 1), 0, MPI_COMM_WORLD); 
    }


    //Havendo um numero par de processos
    if (tamDados % 2 == 0) {        

        if (rank == meio) //pede para o processo central enviar seu maior num encontrado para o ultimo processo
        { 
            MPI_Send(&dado, 1, MPI_FLOAT, tamDados - 1, 0, MPI_COMM_WORLD);
        }

        else if (rank == tamDados - 1) //E novamente o ultimo processo faz a comparacao com seu valor inicial, e imprime o maior
        { 
            MPI_Recv(&recv, 1, MPI_FLOAT, meio, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (recv > dado) { dado = recv; }
            printf("Maior: %f\n", dado); 
        }
    } //do contrario apenas faz o processo central imprimir o maior valor 
    else if (rank == meio) { printf("Maior: %f\n", dado); }

    MPI_Finalize();
}
