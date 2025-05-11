#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

#define INICIO 3.14
#define FIN -3.14
#define PASO 0.15
#define COLUMNAS 12

int numero_elementos;
double matriz_resultados[COLUMNAS][42]; 
double valores_paso[42]; // 43 elementos para el rango de -3.14 a 3.14 con paso de 0.15

double funcion(int n, double x) {
    return (8.0 / n) * pow(-1, n) * sin(n * x);
}

typedef struct {
    int n;  // fila que calculara el hilo
} hilo_args_t;

void *calcular_fila(void *arg) {
    hilo_args_t *args = (hilo_args_t *)arg;
    int n = args->n;

    if (n == 0) {
        for (int i = 0; i < numero_elementos; i++) {
            matriz_resultados[0][i] = 6.0;
        }
    } else {
        for (int i = 0; i < numero_elementos; i++) {
            double x = valores_paso[i];
            matriz_resultados[n][i] = funcion(n, x);
        }
    }

    free(args);  // liberar memoria del struct
    pthread_exit(NULL);
}

int main() {
    int indice = 0;
    for (double valor = FIN; valor <= INICIO; valor += PASO) {
        valores_paso[indice++] = valor;
    }
    numero_elementos = indice;

    pthread_t hilos[COLUMNAS - 1];

    for (int n = 0; n < COLUMNAS - 1; n++) {
        hilo_args_t *args = malloc(sizeof(hilo_args_t));
        args->n = n;
        if (pthread_create(&hilos[n], NULL, calcular_fila, args) != 0) {
            perror("Error al crear hilo");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < COLUMNAS - 1; i++) {
        pthread_join(hilos[i], NULL);
    }

    // Calcular suma final en la ultima fila
    for (int i = 0; i < numero_elementos; i++) {
        double suma = 0.0;
        for (int j = 0; j < COLUMNAS - 1; j++) {
            suma += matriz_resultados[j][i];
        }
        matriz_resultados[COLUMNAS - 1][i] = suma;
    }

    // Imprimir resultado
    FILE *archivo = fopen("resultados.csv", "w");
    if (!archivo) {
        perror("Error al abrir resultados.csv");
        exit(EXIT_FAILURE);
    }

    printf("Matriz Resultados:\n");
    for (int j = 0; j < numero_elementos; j++) {
        printf("%.2f ", valores_paso[j]);
        fprintf(archivo, "%.2f", valores_paso[j]);
        for (int i = 0; i < COLUMNAS; i++) {
            printf("%.4f ", matriz_resultados[i][j]);
            fprintf(archivo, ",%.4f", matriz_resultados[i][j]);
        }
        printf("\n");
        fprintf(archivo, "\n");
    }

    fclose(archivo);
    return 0;
}
