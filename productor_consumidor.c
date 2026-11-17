/*
 * Problema Productor-Consumidor con múltiples productores y consumidores
 * (2 productores y 2 consumidores) usando la biblioteca pthread.
 *
 * Sincronización:
 *   - Un mutex protege el buffer circular compartido.
 *   - Dos variables de condición:
 *       no_lleno  -> los productores esperan cuando el buffer está lleno.
 *       no_vacio  -> los consumidores esperan cuando el buffer está vacío.
 *
 * Compilar:  gcc -Wall -o productor_consumidor productor_consumidor.c -lpthread
 * Ejecutar:  ./productor_consumidor
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define TAM_BUFFER          5   /* Capacidad del buffer circular        */
#define NUM_PRODUCTORES     2
#define NUM_CONSUMIDORES    2
#define ITEMS_POR_PRODUCTOR 10  /* Cada productor genera 10 elementos   */
#define ITEMS_POR_CONSUMIDOR 10 /* Cada consumidor consume 10 elementos */

/* Recurso compartido: buffer circular */
static int buffer[TAM_BUFFER];
static int entrada = 0;   /* Índice donde se inserta el siguiente elemento */
static int salida  = 0;   /* Índice de donde se extrae el siguiente        */
static int cuenta  = 0;   /* Número de elementos actualmente en el buffer  */

/* Primitivas de sincronización */
static pthread_mutex_t mutex     = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  no_lleno  = PTHREAD_COND_INITIALIZER;
static pthread_cond_t  no_vacio  = PTHREAD_COND_INITIALIZER;

/* Imprime el contenido actual del buffer (se llama con el mutex tomado) */
static void imprimir_buffer(void)
{
    printf("    Buffer [%d/%d]: ", cuenta, TAM_BUFFER);
    for (int i = 0; i < cuenta; i++) {
        printf("%d ", buffer[(salida + i) % TAM_BUFFER]);
    }
    printf("\n");
}

/* Hilo productor */
static void *productor(void *arg)
{
    int id = *(int *)arg;

    for (int i = 1; i <= ITEMS_POR_PRODUCTOR; i++) {
        int item = id * 100 + i;      /* Ej.: productor 1 -> 101, 102, ... */

        usleep(rand() % 200000);      /* Simula tiempo de producción */

        pthread_mutex_lock(&mutex);

        while (cuenta == TAM_BUFFER) {
            printf("Productor %d: buffer LLENO, esperando...\n", id);
            pthread_cond_wait(&no_lleno, &mutex);
        }

        buffer[entrada] = item;
        entrada = (entrada + 1) % TAM_BUFFER;
        cuenta++;

        printf("Productor %d: produjo el elemento %d\n", id, item);
        imprimir_buffer();

        pthread_cond_signal(&no_vacio);
        pthread_mutex_unlock(&mutex);
    }

    printf("Productor %d: termino su trabajo.\n", id);
    return NULL;
}

/* Hilo consumidor */
static void *consumidor(void *arg)
{
    int id = *(int *)arg;

    for (int i = 1; i <= ITEMS_POR_CONSUMIDOR; i++) {
        pthread_mutex_lock(&mutex);

        while (cuenta == 0) {
            printf("Consumidor %d: buffer VACIO, esperando...\n", id);
            pthread_cond_wait(&no_vacio, &mutex);
        }

        int item = buffer[salida];
        salida = (salida + 1) % TAM_BUFFER;
        cuenta--;

        printf("Consumidor %d: consumio el elemento %d\n", id, item);
        imprimir_buffer();

        pthread_cond_signal(&no_lleno);
        pthread_mutex_unlock(&mutex);

        usleep(rand() % 300000);      /* Simula tiempo de consumo */
    }

    printf("Consumidor %d: termino su trabajo.\n", id);
    return NULL;
}

int main(void)
{
    pthread_t hilos_prod[NUM_PRODUCTORES];
    pthread_t hilos_cons[NUM_CONSUMIDORES];
    int ids_prod[NUM_PRODUCTORES];
    int ids_cons[NUM_CONSUMIDORES];

    srand(42); /* Semilla fija para tiempos reproducibles */

    printf("=== Productor-Consumidor con pthreads ===\n");
    printf("Productores: %d | Consumidores: %d | Tamano del buffer: %d\n",
           NUM_PRODUCTORES, NUM_CONSUMIDORES, TAM_BUFFER);
    printf("Elementos por productor: %d | por consumidor: %d\n\n",
           ITEMS_POR_PRODUCTOR, ITEMS_POR_CONSUMIDOR);

    /* Crear productores */
    for (int i = 0; i < NUM_PRODUCTORES; i++) {
        ids_prod[i] = i + 1;
        if (pthread_create(&hilos_prod[i], NULL, productor, &ids_prod[i]) != 0) {
            perror("pthread_create (productor)");
            exit(EXIT_FAILURE);
        }
    }

    /* Crear consumidores */
    for (int i = 0; i < NUM_CONSUMIDORES; i++) {
        ids_cons[i] = i + 1;
        if (pthread_create(&hilos_cons[i], NULL, consumidor, &ids_cons[i]) != 0) {
            perror("pthread_create (consumidor)");
            exit(EXIT_FAILURE);
        }
    }

    /* Esperar a que todos terminen */
    for (int i = 0; i < NUM_PRODUCTORES; i++)
        pthread_join(hilos_prod[i], NULL);
    for (int i = 0; i < NUM_CONSUMIDORES; i++)
        pthread_join(hilos_cons[i], NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&no_lleno);
    pthread_cond_destroy(&no_vacio);

    printf("\n=== Ejecucion finalizada. Buffer final con %d elementos. ===\n", cuenta);
    return 0;
}
