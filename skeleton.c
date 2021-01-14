#include <pthread.h>

void *thread_function(void *arg)
{
    int *incoming=(int *)arg;
    //hago lo que sea necesario
    
    // el hilo muere cuando retorna la funcion
    return NULL;
}

int main(int argc, char const *argv[])
{
    /* code */
    pthread_t thread_ID;
    void *exit_status;
    int value=42;

    //creo el hilo
    /*
    *Args:
    *   Identificador del hilo que servira para otras llamadas
    *   Puntero a thread attribute object
    *   Puntero a la funcion que es ejecutada
    *   Parametro de la funcion anterior
    */
    pthread_create(&thread_ID,NULL,thread_function,&value);

    // el programa principal sigue corriendo 

    //Espera por el hilo para terminarlo
    pthread_join(thread_ID,&exit_status);

    //Only the main thread is running now


    return 0;
}
