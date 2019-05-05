/*
 * Contador de números primos multiprocesso
 *
 * Rafael Sartori, 186154
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

/** Número de processos utilizados */
#define num_processos 4

int main() {
    /* O processo principal irá preparar as entradas para os processos-filho,
     * que irão apenas interpretar se é primo ou não através do que é lido em
     * pipe. Todos os processos-filho, ao encontrarem um número primo, irão
     * incrementar uma variável compartilhada. */

    /* Criamos o pipe para passar os números */
    #define LEITURA 0
    #define ESCRITA 1
    int pipes[2];
    if (pipe(pipes) != 0) {
        fprintf(
            stderr, "Falha ao criar pipe para comunicação interprocessos!\n"
        );
        exit(-1);
    }

    /* Criamos a memória para incrementar */
    int *primos = mmap(
        NULL, sizeof(int),
        PROT_WRITE | PROT_READ,
        MAP_SHARED | MAP_ANONYMOUS,
        0, 0
    );
    if (primos == MAP_FAILED) {
        fprintf(stderr, "Falha ao criar região compartilhada!\n");
        exit(-1);
    }
    *primos = 0;

    /* Fazemos todos os processos */
    int is_filho = 0;
    int pids[num_processos];
    for (int i = 0; i < num_processos; i++) {
        /* Criamos um novo processo */
        pids[i] = fork();
        is_filho = (pids[i] == 0);

        /* Saímos do laço de criação de forks se não somos o processo
         * principal */
        if (is_filho) {
            break;
        }
    }

    if (is_filho) {
        /* Se o processo é filho, precisamos fechar pipe de escrita */
        close(pipes[ESCRITA]);

        /* Enquanto conseguimos ler do pipe de leitura, verificamos se é
         * primo */
        int inteiro;
        while (read(pipes[LEITURA], &inteiro, sizeof(int)) > 0) {
            int is_primo = (inteiro > 1);

            /* Procuramos um divisor entre 2 e a metade do número que temos */
            // OBS: podemos fazer sqrt(inteiro)
            for (int i = 2; i <= (inteiro / 2); i++) {
                /* Se é divisor, não é um primo, continuamos para o próximo
                 * read. Não precisamos mais buscar divisores */
                if ((inteiro % i) == 0) {
                    is_primo = 0;
                    break;
                }
            }

            /* Se percorremos o laço todo e não encontramos divisor, é primo */
            if (is_primo) {
                (*primos)++;
            }
        }

        /* Agora que acabamos, terminamos */
        return 0;
    } else {
        /* Se o processo é pai, precisamos fechar pipe de leitura */
        close(pipes[LEITURA]);

        /* Escrevemos os números da entrada enquanto ela não acabou */
        int escrito;
        while (scanf(" %d ", &escrito) != EOF) {
            write(pipes[ESCRITA], &escrito, sizeof(int));
        }

        /* Fechamos o pipe de escrita, esperamos filhos */
        close(pipes[ESCRITA]);

        /* Aguardamos todos os filhos */
        int status;
        for (int i = 0; i < num_processos; i++) {
            waitpid(pids[i], &status, 0);
        }

        /* Informamos quantos primos existem */
        printf("%d\n", *primos);

        /* Desmapeamos memória */
        if (munmap(primos, sizeof(int)) != 0) {
            fprintf(stderr, "Falha ao remover mapeamento de memória\n");
        }

        /* Retornamos */
        return 0;
    }
}
