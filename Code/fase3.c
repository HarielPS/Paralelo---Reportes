#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/ipc.h>
#include<sys/shm.h>
#include<sys/sem.h>
#include<sys/wait.h>
#include<math.h>

#define INICIO 3.14
#define FIN -3.14
#define PASO 0.15
#define PERMISOS 0666
#define ITERACION 10
#define COLUMNAS 12

// Permiso	Acceso del dueño	Grupo	Otros
// 0600	     rw-	         ---	---
// 0666	     rw-	         rw-	rw-
// 0644	     rw-	         r--	r--
// 0777	     rwx	         rwx	rwx inseguro

// Estructura de archivo en memoria
// n	     n1	     n2      ... 	suma de fila
// 6	     0.02	         ...	---
// 6	     1.20	         ...	---
// 6	     2.37	         ...	---
// ...	     ...	         ...	--- 

// definir funcion de Fourier
double  funcion(int n, double  x)
{
    double seno;
    seno=sin(n*x);
    return (double )((8.0000/n)* pow(-1,n) * seno);
}

// Funciones para el semaforo
void down(int semid) {
    struct sembuf op = {0, -1, 0};
    semop(semid, &op, 1);
}

void up(int semid) {
    struct sembuf op = {0, 1, 0};
    semop(semid, &op, 1);
}

// Crear Semaforo
int Crea_semaforo(key_t llave,int valor_inicial)
{
   int semid=semget(llave,1,IPC_CREAT|PERMISOS);
   if(semid==-1)
   {
      return -1;
   }
   semctl(semid,0,SETVAL,valor_inicial);
   return semid;
}

void crearArchivo(const char *nombreArchivo) {
    FILE *archivo = fopen(nombreArchivo, "w");
    if (archivo == NULL) {
        perror("No se pudo crear el archivo");
        exit(1);
    }
    fclose(archivo);
}


int main(int argc, char *argv[])
{
    pid_t hijos[COLUMNAS];
    int numero_elementos = (int)(((INICIO - FIN) / PASO) + 1);
    int a0 = 6;
    double  valores_paso[numero_elementos];
    double  valor_actual;
    int indice = 0;
    int tam_memoria = COLUMNAS * numero_elementos * sizeof(double );


    crearArchivo("fourier");
    crearArchivo("semaforo_general");
    key_t clave_resultados = ftok("fourier", 66);
    key_t clave_semaforo_general = ftok("semaforo_general", 66);

    // Creamos el semaforo
    int semaforo_general = Crea_semaforo(clave_semaforo_general, 1);

    if (semaforo_general == -1) {
        perror("Error al crear el semáforo");
        exit(1);
    }

    // Creamos la memoria compartida
    int id_matriz = shmget(clave_resultados, tam_memoria, IPC_CREAT|PERMISOS);
    if (id_matriz == -1) {
        perror("Error al crear memoria compartida");
        exit(1);
    }

    // Puntero a la memoria compartida
    double  (*matriz_resultados)[numero_elementos];
    matriz_resultados = (double  (*)[numero_elementos]) shmat(id_matriz, NULL, 0);
    if (matriz_resultados == (void *)-1) {
        perror("Error al enlazar memoria");
        exit(1);
    }

    // Generamos valores de paso
    for (valor_actual = FIN; valor_actual <= INICIO; valor_actual += PASO) {
        if (indice < numero_elementos) {
          valores_paso[indice] = valor_actual;
          indice++;
        }
    }

    // Primer hijo para n=0 valor constante de 6
    hijos[0] = fork(); 
    if (hijos[0] == 0) {
        // Proceso hijo
        for (int i = 0; i < numero_elementos; i++) {
            matriz_resultados[0][i] = a0;
        }
        exit(0);
    }

    // Generamos los demas hijos valores de n
    for (int n = 1; n < (COLUMNAS - 1 ); n++) {
        hijos[n] = fork();
        if (hijos[n] == 0) {
            for (int i = 0; i < numero_elementos; i++) {
                double  x = valores_paso[i];
                double  bn = funcion(n, x);
    
                down(semaforo_general);
                matriz_resultados[n][i] = bn;
                // printf("n = %d, x = %.2f, bn = %.4f\n", n, x, bn);
                up(semaforo_general);
            }
            shmdt(matriz_resultados);
            exit(0);
        }
    }

    // Esperamos a que terminen los hijos
    for (int i = 0; i < COLUMNAS - 1; i++) {
        wait(NULL);
    }    
    
    // Se calcula la suma de los resultados
    for (int i = 0; i < numero_elementos; i++) {
        double  suma = 0.0;
        for (int fila = 0; fila < COLUMNAS - 1; fila++) {
            suma += matriz_resultados[fila][i];
        }
        matriz_resultados[11][i] = suma;
    }
    
    FILE *archivo = fopen("resultados.csv", "w");
    if (archivo == NULL) {
        perror("Error al crear archivo CSV");
        exit(1);
    }

    
    // Se imprime la matriz resultados
    printf("Matriz Resultados:\n");
    for (int j = 0; j < numero_elementos; j++) {
        printf("%.2f ", valores_paso[j]);
        fprintf(archivo, "%.2f", valores_paso[j]);
        for (int i = 0; i < COLUMNAS; i++) {
            fprintf(archivo, ",%.4f", matriz_resultados[i][j]);
            printf("%.4f ", matriz_resultados[i][j]);
        }
        printf("\n");
        fprintf(archivo, "\n");
    }
    fclose(archivo);
    

    // Desvinculamos la memoria compartida
    if (shmdt(matriz_resultados) == -1) {
        perror("Error al desvincular memoria compartida");
        exit(1);
    }
    shmctl(id_matriz, IPC_RMID, NULL);
    semctl(semaforo_general, 0, IPC_RMID);
    return 0;
}