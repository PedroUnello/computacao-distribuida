#include <stdio.h>
#include <stdlib.h>
#include <openssl/md5.h>
#include <string.h>

#include <mpi.h>

#define MAX 10

#define MAIOR(a,b) a > b ? a : b

typedef unsigned char byte;

char letters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890";

/*
 * Print a digest of MD5 hash.
*/
void print_digest(byte * hash){
	int x;

	for(x = 0; x < MD5_DIGEST_LENGTH; x++)
        	printf("%02x", hash[x]);
	printf("\n");
}

/*
 * This procedure generate all combinations of possible letters
*/
void iterate(byte * hash1, byte * hash2, char *str, int idx, int len, int *ok) {
	int c;

	// 'ok' determines when the algorithm matches.
	if(*ok) return;

	if (idx < (len - 1)) {
		// Iterate for all letter combination.
		for (c = 0; c < strlen(letters) && *ok==0; ++c) {
			str[idx] = letters[c];
			// Recursive call
			iterate(hash1, hash2, str, idx + 1, len, ok);
		}
	} else {
		// Include all last letters and compare the hashes.
		for (c = 0; c < strlen(letters) && *ok==0; ++c) {
			str[idx] = letters[c];
			MD5((byte *) str, strlen(str), hash2);
			if(strncmp((char*)hash1, (char*)hash2, MD5_DIGEST_LENGTH) == 0){
				printf("found: %s\n", str);
				//print_digest(hash2);
				*ok = 1;
			}
		}
	}
}

/*
 * Convert hexadecimal string to hash byte.
*/
void strHex_to_byte(char * str, byte * hash){
	char * pos = str;
	int i;

	for (i = 0; i < MD5_DIGEST_LENGTH/sizeof *hash; i++) {
		sscanf(pos, "%2hhx", &hash[i]);
		pos += 2;
	}
}

int main(int argc, char **argv) {

	MPI_Init(NULL, NULL);

	// Numero de processos
    int nProcessos;
    MPI_Comm_size(MPI_COMM_WORLD, &nProcessos);
    // Rank do processo
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	char str[MAX+1];
	int len;
	int lenMax = MAX;
	int ok = 0, r;
	char hash1_str[2*MD5_DIGEST_LENGTH+1];
	byte hash1[MD5_DIGEST_LENGTH]; // password hash
	byte hash2[MD5_DIGEST_LENGTH]; // string hashes
	
	int* indices;

	if (rank == 0 ){ //Apenas o rank zero faz a leitura

		// Input:
		r = scanf("%s", hash1_str);

		// Check input.
		if (r == EOF || r == 0)
		{
			fprintf(stderr, "Error!\n");
			exit(1);
		}
	
		// Convert hexadecimal string to hash byte.
		strHex_to_byte(hash1_str, hash1);

		//Aloca um vetor de indices na quantidade de processos, e atribui o proprio indice
		indices = malloc(sizeof(int) * nProcessos);
		for (int i = 0; i < nProcessos; i++){ *(indices + i) = i; }

	}

	//Feita a leitura, o rank zero espalha por broadcast a hash a ser encontrada
	MPI_Bcast(hash1, MD5_DIGEST_LENGTH, MPI_BYTE, 0, MPI_COMM_WORLD);

	memset(hash2, 0, MD5_DIGEST_LENGTH);
	//print_digest(hash1);


	int count = 0; //Contador de caracteres passiveis de combinacao (vetor letters)
	/*Tendo que cada passo deveria fazer combinacoes com 36 caracteres, incrementando o tamanho das combinacoes
	*divide/seciona a quantidade de passos dependendo do numero de processos, afinal se existem, ao exemplo, 72 processos
	para 36 letras, nao seria preciso de duas iteracoes para dividir 72 letras em 1 letra por processo*/
	for(len = 1; len <= lenMax && ok == 0; len += 1 + ((int)(nProcessos/36))){ 
		
		//A operacao de modulo garante (ou pelo menos tenta garantir), que nao serao repetidas letras
		count %= 36;

		///enquanto nao for achado a resposta e nao forem feitas n de len combinaçoes para cada um dos caracteres
		while (count < 36 && ok == 0) { 
		
			memset(str, 0, len+1);

			//divide as letras em n processos
			
			int cur;

			//utilizando do scatter, para distribuir os indices antes alocados pelo rank 0
			MPI_Scatter(indices, 1, MPI_INT, &cur, 1, MPI_INT, 0, MPI_COMM_WORLD);
			
			//Cada processo pega uma letra inicial para as combinacoes
			*str = letters[(count + cur)%36];

			/*tendo que a qtd de processos pode exceder a quantidade de caracteres
			*para os processos com rank excedente dessa qtd, o tamanho da combinacao atual
			sera equivalentemente incrementado*/
			int curLen = len + ((int)(cur/36));
			//Se ainda estiver no limite de lenMax, chama a iteracao com o primeiro indice preenchido
			if ( curLen <= lenMax) { iterate(hash1, hash2, str, 1, curLen, &ok); }

			//Utiliza a funcao de maximo no reduce para descobrir se alguem achou a senha,
			//neste caso o Allreduce e utilizado pois tal condicao devera ser espalha entre os processos
			MPI_Allreduce(&ok, &ok, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

			count+= nProcessos;
		}
	}
	
	if ( rank == 0) { free(indices); }

	MPI_Finalize();

	return 0;

}
