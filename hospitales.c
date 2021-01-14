#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>


int shared=20;
int camas[3]={20,55,10};
int HOSPITLES=3;
sem_t consultarNCamasDelHospital[3];
sem_t binary_sem;//use like a mutex

struct argsPaciente {
    int vivo;
    int idHospital;
    int reposo_en_casa;
};

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
int diagnostico()
{
    time_t t;
    srand((unsigned) time(&t));
    return rand()% 4;
}

void* paciente()
{
    //declaracion de variables
    int fue_atendido;
    pthread_t thread_ID;
    struct argsPaciente *args = (struct argsPaciente *)malloc(sizeof(struct argsPaciente));
    //inicializacion 
    args->vivo=1;
    while (1)
    {   
        if (args->vivo!=1)
        {
            break;
        }
        fue_atendido=0;

        for ( int i = 0; i < HOSPITLES; i++)
        {
            semWait( &consultarNCamasDelHospital[i] );
                if (camas[i] > 0)
                {
                    args->idHospital=i;
                    pthread_create(&thread_ID,NULL,irHospital,(void *)args);
                    fue_atendido <- 1;
                    break;
                }
            semPost( &consultarNCamasDelHospital[i] );
        }
        
        if (args->vivo!=1)//por si muere
        {
            break;
        }
        if (fue_atendido)
        {
            if (args->reposo_en_casa)
            {
                //recibirAtencionVoluntaria( &vivo )
            }
            else
            {
                //tiempoSano( random() )
            }
            
            
        }
        

    }
    

}

int main(int argc, char const *argv[])
{

    

    return 0;
}
