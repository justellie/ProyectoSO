#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


// IDEA(sGaps): Creo que hay que usar UpperCamelCase para los nuevos tipos de datos.
//              En este caso sería: ArgsPaciente, Hospital, etc...


// Declaración de tipos de datos:
// ------------------------------
typedef struct _argsPacientes  {
    int vivo;
    int idHospital;
    int reposo_en_casa;
    //personal hospital
    sem_t sEnfermera;
    int enfermera[3]; // Inicializar en -1 en otra parte
    sem_t sMedico;
    int medico[3]; // Inicializar en -1 en otra parte
} argsPaciente;

//aqui se manejaran los recursos de cada hospital
typedef struct _hospital
{
    //Identificacion del hospital (1-general, 2-intermedio, 3-centinela)
    int tipoH;
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
    // Creo que no deben ser arreglos:
    sem_t enfermeras;   
    sem_t medicos;
    sem_t voluntarios;

    //contador de disponibilidad de las enfemeras y medicos 
    int num_enfermeras;//es necesario saber el numero de aPersonals
    int num_medicos;
    int contadorEnfermeras[6];
    int contadorMedicos[6];

} hospital;

enum cama {ninguno,en_casa,basica,intensiva,muerto}; 

// Definición de variables globales: (process scope)
// ---------------------------------
int shared=20;
int camas[3]={20,55,10};//esto tiene que ser un semaforo, se lo dejo al elio del futuro o a quien sea que vaya a tocar este codigo 
int HOSPITLES=3;
sem_t consultarNCamasDelHospital[3];

sem_t binary_sem;//use like a mutex


// Declaración de funciones:
// -------------------------
void liberarRecursosDelHospital( int id_hospital , int tipo_atencion , int tiene_cama );
void transferirDeCama( int id_hospital , int tipo_de_cama , int tipo_atencion );
void recibirAtencionVoluntaria( argsPaciente* datos_paciente );
void tiempoSano();


// TODO(sGaps): Definir luego
void transferirDeCama( int id_hospital , int tipo_de_cama , int tipo_atencion ) {}


hospital hospitalH[3];


void tiempoSano(){
    // TODO(sGaps): Agregar un límite de tiempo razonable
    sleep( rand() );
}

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
    // NOTE(sGaps): srand() deber ir en la inicialización. Se encarga de inicializar la semilla de números aleatorios.
    // time_t t;
    // srand((unsigned) time(&t));
    return rand()% 4;
}


void *irHospital(void *input)
{
    int tiene_cama, es_intensivo, diagnostico;
    enum cama tipo_cama_actual;
    argsPaciente* usr = (argsPaciente*) input;

    es_intensivo=0;
    // Entra a la sala de espera
    sem_wait( &hospitalH[usr->idHospital].salaEspera );
        // Entra en alguna de las 5 salas de muestra
        sem_wait(&hospitalH[usr->idHospital].salaMuestra);
                diagnostico = obtener_diagnostico();
        sem_post(&hospitalH[usr->idHospital].salaMuestra);
    sem_post( &hospitalH[usr->idHospital].salaEspera);

    tipo_cama_actual=ninguno;
    tiene_cama=0;

    while (1)//diagnostico != sano
    {
        switch (diagnostico)
        {
        case muerto://muerto
            if (tiene_cama)
            {
                liberarRecursosDelHospital(usr->idHospital,es_intensivo,tiene_cama);

            }
            usr->vivo=0;
            //reporta gestion central
            return NULL;
            break;
        case en_casa://reposo en casa
            liberarRecursosDelHospital(usr->idHospital,es_intensivo,tiene_cama);
            // Se tiene que avisar de alguna manera que ya está listo para continuar.
            // Puede ser con un signal.
            usr->reposo_en_casa=1;
            usr->vivo=1;
            diagnostico = obtener_diagnostico();
            return NULL;
            break;

        case basica: //basico 
        //reservar Camas normales
        if (tiene_cama==0)
        {
            //asignamos recursos 
            sem_wait(&hospitalH[usr->idHospital].camasHospital);
            tiene_cama=1;
            es_intensivo=0;
            tipo_cama_actual=basica;

            // Asignar un numero de efermeras(os) aquí
            //CREO QUE DEBEMOS HACER UNA MATRIZ DE SEMAFOROS POR ENFERMERA

            // También son un recurso crítico de cada hospital.

            // Le da oxigeno
            sem_wait(&hospitalH[usr->idHospital].oxigeno);
        }
        else
        {
                // Tenía una cama asignada, por lo tanto debemos transferirlo a las camas básicas:
                transferirDeCama( usr->idHospital , tipo_cama_actual , basica );
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
                sem_wait(&hospitalH[usr->idHospital].camasHospital);
                tiene_cama=1;
                es_intensivo=1;
                tipo_cama_actual=intensiva;
                sem_wait(&hospitalH[usr->idHospital].respirador);
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
                liberarRecursosDelHospital(usr->idHospital,es_intensivo,tiene_cama);
            }
            usr->reposo_en_casa=0;
            usr->vivo=1;
            return NULL;
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


int buscarHospitalLibre(argsPaciente* args, int fue_atendido, pthread_t * thread_ID)
{

    for ( int i = 0; i < HOSPITLES; i++)
    {
        semWait( &consultarNCamasDelHospital[i] );
            if (camas[i] > 0)
            {
                args->idHospital=i;
                pthread_create(thread_ID,NULL,irHospital,(void *)args);
                fue_atendido = 1;
                break;
            }
        semPost( &consultarNCamasDelHospital[i] );
    }
    
    return fue_atendido;
}

void *paciente()
{
    //declaracion de variables
    int fue_atendido;
    pthread_t thread_ID;

    // NOTE(sGaps): No es necesario reservar memoria dinámica. usar un inicializador estático en su lugar.
    //            > Cada paciente es un hilo, por lo que los valores creados aquí son locales. Los demás
    //              no podrán acceder a éstos mientras no se publiquen en un ámbito global
    //              (mediante estructuras globales)

    // argsPaciente *args = (argsPaciente *)malloc(sizeof(argsPaciente));
    argsPaciente args = { 0 }; // TODO(sGaps): Inicializar luego con valores aleatorios.
    //inicializacion 
    args.vivo = 1;
    while (1)
    {   
        if (args.vivo != 1)
        {
            break;
        }

        fue_atendido=0;

        fue_atendido=buscarHospitalLibre(&args, fue_atendido, &thread_ID);

        if (args.vivo != 1)//por si muere
        {
            break;
        }
        if (fue_atendido)
        {
            if (args.reposo_en_casa)
            {
                recibirAtencionVoluntaria( &args );
            }
            else
            {
                tiempoSano();
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
