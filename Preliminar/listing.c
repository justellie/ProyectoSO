#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

pthread_mutex_t lock;
int shared_data;

void *thread_function(void *arg)
{
    int i;
    for ( i = 0; i < 1024*1024; i++)
    {
        pthread_mutex_lock(&lock);
        shared_data++;
        pthread_mutex_unlock(&lock);
    }
    
    return NULL;
}

int main(int argc, char const *argv[])
{
    /* code */
    pthread_t thread_ID;
    void *exit_status;
    int i;
    
    //inicializo el semafoto
    pthread_mutex_init(&lock,NULL);
    
    //creo el hilo
    /*
    *Args:
    *   Identificador del hilo que servira para otras llamadas
    *   Puntero a thread attribute object
    *   Puntero a la funcion que es ejecutada
    *   Parametro de la funcion anterior
    */
    pthread_create(&thread_ID,NULL,thread_function,NULL);


   for (i = 0; i < 10; i++)
   {
       sleep(1);//espera un segundo
       pthread_mutex_lock(&lock);//intento acceder al recurso
       printf("\r Shared interger's value = %d\n", shared_data);
       pthread_mutex_unlock(&lock);
   }
   printf("\n");
   pthread_join(thread_ID,&exit_status);
   pthread_mutex_destroy(&lock);

    return 0;
}
