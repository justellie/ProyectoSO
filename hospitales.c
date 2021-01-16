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

//aqui se manejaran los recursos de cada hospital
typedef struct _hospital
{
    // [T] Zona de Triaje -----------------------
    // Es un número fijo para cada hospital:
    sem_t salaMuestra; // -med. tomando pruebas(5)
    sem_t salaEspera;  // +sillas de espera(20)

    // [H] Hospital --------------------------------
    // El número de camas puede
    // variar con el tiempo.
    sem_t camasHospital;
    // Siempre se piden estos recursos:
    sem_t oxigeno;
    sem_t respirador;
    // El número de medicos y pacientes
    // varía entre hospitales:
    sem_t enfermeras[6];
    sem_t medicos[6];
    sem_t voluntarios[6];

} hospital;

hospital hospitalH[3];

enum cama {ninguno,en_casa,basica,intensiva,muerto}; 


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
    sem_wait( &hospitalH[((argsPaciente *)input)->idHospital].salaEspera );
        // Entra en alguna de las 5 salas de muestra
        sem_wait(&hospitalH[((argsPaciente *)input)->idHospital].salaMuestra);
                diagnostico = obtener_diagnostico();
        sem_post(&hospitalH[((argsPaciente *)input)->idHospital].salaMuestra);
    sem_post( &hospitalH[((argsPaciente *)input)->idHospital].salaEspera);

    tipo_cama_actual=ninguno;
    tiene_cama=0;

    while (1)//diagnostico != sano
    {
        switch (diagnostico)
        {
        case muerto://muerto
            if (tiene_cama)
            {
                liberarRecursosDelHospital(((argsPaciente*)input)->idHospital,es_intensivo,tiene_cama);

            }
            ((argsPaciente*)input)->vivo=0;
            //reporta gestion central
            return;
            break;
        case en_casa://reposo en casa
            liberarRecursosDelHospital(((argsPaciente*)input)->idHospital,es_intensivo,tiene_cama);
            // Se tiene que avisar de alguna manera que ya está listo para continuar.
            // Puede ser con un signal.
            ((argsPaciente*)input)->reposo_en_casa=1;
            ((argsPaciente*)input)->vivo=1;
            diagnostico = obtener_diagnostico();
            return;
            break;

        case basica: //basico 
        //reservar Camas normales
        if (tiene_cama==0)
        {
            //asignamos recursos 
            sem_wait(&hospitalH[((argsPaciente *)input)->idHospital].camasHospital);
            tiene_cama=1;
            es_intensivo=0;
            tipo_cama_actual=basica;

            // Asignar un numero de efermeras(os) aquí
            //CREO QUE DEBEMOS HACER UNA MATRIZ DE SEMAFOROS POR ENFERMERA

            // También son un recurso crítico de cada hospital.

            // Le da oxigeno
            sem_wait(&hospitalH[((argsPaciente *)input)->idHospital].oxigeno);
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
        diagnostico = obtener_diagnostico();   
        break;    

        case intensiva:
            if (tiene_cama==0)
            {
                // [...] Intentar reservar Camas de Tratados Intensivos
                // [...] Si falla, reservar Camas Normales
                sem_wait(&hospitalH[((argsPaciente *)input)->idHospital].camasHospital);
                tiene_cama=1;
                es_intensivo=1;
                tipo_cama_actual=intensiva;
                sem_wait(&hospitalH[((argsPaciente *)input)->idHospital].respirador);
                //hago la busqueda de enfermeras 
            }
            else
            {
                //transferir cama en caso de que venga de basico a intesivo 
                //transferirDeCama( idHospital , tipo_cama_actual , Intensivo )
                tipo_cama_actual = intensiva;

            }
            // pido al medico 
            diagnostico = obtener_diagnostico();
            break;

        case ninguno:
            if (tiene_cama)
            {
                liberarRecursosDelHospital(((argsPaciente *)input)->idHospital,es_intensivo,tiene_cama);
            }
            ((argsPaciente *)input)->reposo_en_casa=0;
            ((argsPaciente *)input)->vivo=1;
            return;
            break;
        }
    }
    return NULL;
}

void liberarRecursosDelHospital(int idHospital,int es_intensivo,int tiene_cama)
{
    if (tiene_cama)  
    {
        sem_post( &hospitalH[idHospital].camasHospital );
        if (es_intensivo)
        {
            sem_post( &hospitalH[idHospital].respirador );
            
        }
        else
        {
            sem_post( &hospitalH[idHospital].oxigeno );
        }
        //se libera de enfermeras 
    }
    
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
                recibirAtencionVoluntaria( args );
            }
            else
            {
                //tiempoSano( random() )
            }
            
            
        }
        

    }
    

}

void recibirAtencionVoluntaria (argsPaciente* args ) 
{
    enum cama diagnostico; 
    sem_wait(&hospitalH[args->idHospital].voluntarios);
        while (1)
        {
            diagnostico = obtener_diagnostico();
            switch (diagnostico)
            {
            case muerto:

                //reportar fallecido
                break;
            case en_casa:
                continue;
            case ninguno:
                break;
               
            default:
                //debe ir al hospital de nuevo a tratarse
                break;
            }
        }
        
}





int main(int argc, char const *argv[])
{

        

    return 0;
}
