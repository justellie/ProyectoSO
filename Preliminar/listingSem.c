#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>

int shared=20;
sem_t binary_sem;//use like a mutex

void *thread_function(void *arg)
{
    
    //hago lo que sea necesario
    sem_wait(&binary_sem);
        shared--;// uso el recurso
        printf("valor de dato compartido en hilo: %d \n", shared);
    sem_post(&binary_sem);
    // el hilo muere cuando retorna la funcion
    return NULL;
}

int main(int argc, char const *argv[])
{
    /* code */
    pthread_t thread_ID;
    void *exit_status;

    //inicializo semaforo
    //
    sem_init(&binary_sem,0,1);


    //creo el hilo
    /*
    *Args:
    *   Identificador del hilo que servira para otras llamadas
    *   Puntero a thread attribute object
    *   Puntero a la funcion que es ejecutada
    *   Parametro de la funcion anterior
    */
    pthread_create(&thread_ID,NULL,thread_function,NULL);

    // el programa principal sigue corriendo 

    sem_wait(&binary_sem);
        shared++;// uso el recurso
        printf("valor de dato compartido en programa principal: %d \n", shared);
    sem_post(&binary_sem);

    //Espera por el hilo para terminarlo
    pthread_join(thread_ID,&exit_status);

    //Only the main thread is running now
    sem_destroy(&binary_sem);

    return 0;
}
