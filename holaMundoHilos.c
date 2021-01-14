#include <pthread.h>
#include <stdio.h>
void *thread_function(void *arg)
{
    int *incoming=(int *)arg;
    printf("Hola hilo: %d \n",incoming);//imprimo el numero
 
    return NULL;
}

int main(int argc, char const *argv[])
{
    /* code */
    pthread_t thread_ID[10];
    void *exit_status;
    int value[10]={1,2,3,4,5,6,7,8,9,10};

    //creo el hilo
    /*
    *Args:
    *   Identificador del hilo que servira para otras llamadas
    *   Puntero a thread attribute object
    *   Puntero a la funcion que es ejecutada
    *   Parametro de la funcion anterior
    */
   for (int i = 0; i < 10; i++)
   {
    pthread_create(&thread_ID[i],NULL,thread_function,value[i]);
    pthread_join(thread_ID[i],&exit_status);
   }
    return 0;
}
