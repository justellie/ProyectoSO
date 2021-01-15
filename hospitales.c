#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>


int shared=20;
int camas[3]={20,55,10};//esto tiene que ser un semaforo, se lo dejo al elio del futuro o a quien sea que vaya a tocar este codigo 
int enfermerasActivas[3]={3,4,5};   
int HOSPITLES=3;
sem_t consultarNCamasDelHospital[3];
sem_t salaMuestra[3],salaEspera[3],camasHospital[3],oxigeno[3];
sem_t enfermeras[3][5];//3 hospitales, 5 enfemeras 

sem_t binary_sem;//use like a mutex

typedef struct _argsPacientes  {
    int vivo;
    int idHospital;
    int reposo_en_casa;
} argsPaciente;

enum cama {ninguno,en_casa,basica,intesiva}; 


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
int obtener_diagnostico()
{
    time_t t;
    srand((unsigned) time(&t));
    return rand()% 4;
}


void *irHospital(void *input)
{
    int tiene_cama, es_intensivo, diagnostico;
    enum cama tipo_cama_actual;
    es_intensivo=0;
    // Entra a la sala de espera
    sem_wait( &salaEspera[((argsPaciente *)input)->idHospital] );
        // Entra en alguna de las 5 salas de muestra
        sem_wait(&salaMuestra[((argsPaciente*)input)->idHospital]);
                diagnostico = obtener_diagnostico();
        sem_post(&salaMuestra[((argsPaciente*)input)->idHospital]);
    sem_post( &salaEspera[((argsPaciente*)input)->idHospital]);

    tipo_cama_actual=ninguno;
    tiene_cama=0;

    while (1)//diagnostico != sano
    {
        switch (diagnostico)
        {
        case ninguno://muerto
            if (tiene_cama)
            {
                liberarRecursosDelHospital(((argsPaciente*)input)->idHospital,es_intensivo);

            }
            ((argsPaciente*)input)->vivo=0;
            //reporta gestion central
            return;
            break;
        case en_casa://reposo en casa
            liberarRecursosDelHospital(((argsPaciente*)input)->idHospital,es_intensivo);
            // Se tiene que avisar de alguna manera que ya está listo para continuar.
            // Puede ser con un signal.
            ((argsPaciente*)input)->reposo_en_casa=1;
            ((argsPaciente*)input)->vivo=1;
            return;
            break;

        case basica: //basico 
        //reservar Camas normales
        if (tiene_cama==0)
        {
            //asignamos recursos 
            sem_wait(&camasHospital[((argsPaciente*)input)->idHospital]);
            tiene_cama=1;
            es_intensivo=0;
            tipo_cama_actual=basica;

            // Asignar un numero de efermeras(os) aquí
            //CREO QUE DEBEMOS HACER UNA MATRIZ DE SEMAFOROS POR ENFERMERA

            // También son un recurso crítico de cada hospital.

            // Le da oxigeno
            sem_wait(&oxigeno[((argsPaciente*)input)->idHospital]);
        }
        else
        {
                // Tenía una cama asignada, por lo tanto debemos transferirlo a las camas básicas:
                transferirDeCama( ((argsPaciente*)input)->idHospital , tipo_cama_actual , basica );
                tipo_cama_actual = basica;
        }    
        // Durante un periodo, hay medicos para el paciente.
        //semWait(medico[idHospital])    //recurso compartido con otras camas
            //diagnostico <- obtenerDiagnostico()
        //semSignal(medico[idHospital] )

        //esperarEfectosDelTratamiento()    
        break;    

        case intesiva:

            

        default:
            break;
        }
    }
    
    




    sem_wait(&binary_sem);
        shared--;// uso el recurso
        printf("valor de dato compartido en hilo: %d \n", shared);
    sem_post(&binary_sem);

    return NULL;
}

void *paciente()
{
    //declaracion de variables
    int fue_atendido;
    pthread_t thread_ID;
    argsPaciente *args = (argsPaciente *)malloc(sizeof(argsPaciente));
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
