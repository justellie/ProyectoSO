/**
 * @file main.c
 * @brief Programa principal. Inicializa todas las variables e hilos para el funcionamiento del programa.
 * @author Gabriel Peraza
 * @version 0.0.0.2
 * @date 2021-03-04
 */
#include "actores.h"
#include "definiciones.h"

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

// Crea los timers y eventos particulares:
// SIGUSR1 <-> Llegó el momento de hacer un reporte.
// SIGINT  <-> Hay que liberar los recursos y finalizar el programa.
#define NANOSEGUNDOS_EN_SEGUNDOS 1000000000;
#define ABSOLUTE TIMER_ABSTIME
#define RELATIVE 0

// [@] Sincronizacion global ---------
Barrier    Paso_Inicializacion;

// [+] Tablas globales de Datos -----------------
Paciente   Tabla_Pacientes[NPACIENTES];
Personal   Tabla_Medicos[NMEDICOS];
Personal   Tabla_Enfermeras[NENFERMERAS];
Hospital   Tabla_Hospitales[NHOSPITALES];
GestorCama Tabla_Gestores[NHOSPITALES];
Voluntario Tabla_Voluntarios[NVOLUNTARIOS];
UGC        gestor_central;

// [^] Tablas globales de Hilos(Actores) --------
typedef struct HilosActores{
    pthread_t Pacientes  [NPACIENTES];

    pthread_t Gestores   [NHOSPITALES];
    pthread_t Analistas  [NANALISTAS];
    pthread_t Voluntarios[NVOLUNTARIOS];

    //pthread_t Director  [NHOSPITALES];    //TODO: Verificar si se puede eliminar. (¿Hace algo?)
    pthread_t JefeEpidemia[NHOSPITALES];
    pthread_t JefeCuidados[NHOSPITALES];
    //pthread_t JefeAdmin   [NHOSPITALES];

    pthread_t InventarioUGC;
    pthread_t PersonalUGC;
    //pthread_t StatusUGC;
} HilosActores;

HilosActores Actores;


// [*] Voluntarios -----------
RefQueue pacienteEnCasa;

/// @brief Cambia las condiciones globales para obligar a la actualización de las estadísticas.
/// @details Cambia ciertos flags y variables de condición en el programa para forzar la evaluación y
///          actualización de las estadísticas globales. **Debe usarse como manejador de la señal**
///          **SIGUSR1, que es establecida en el main()**
/// 
/// @param signo código de la señal. Debe ser SIGUSR1
/// @param info información de control.
/// @param context contexto de control.
void peticion_actualizar_estadisticas( int signo , siginfo_t* info , void* context );

///
/// @brief Realiza los pasos de limpieza en caso de detectar SIGINT (Ctrl + C).
/// @details Cambia ciertos flags para forzar la finalización del programa y la limpieza de recursos.
///          ***debe usarse como manjeador de la señal SIGINT.***
/// 
/// @param signo código de la señal. Debe ser SIGUSR1
/// @param info información de control.
/// @param context contexto de control.
void forzar_finalizacion( int signo , siginfo_t* info , void* context );

int main(){
    int SYS_CLOCK = CLOCK_REALTIME;
    int status;

    inicializarPacientes();
    inicializarMedicos();
    inicializarEnfermeras();
    inicializarHospitales( 0.20 , 0.30 , 0.50 ); // 0.20 + 0.30 + 0.50 ~= 1.0
    inicializarPacientesEnCasa();
    inicializarVoluntarios();


    // ----------------------------------------------------------------------------------
    // Se establecen los manejadores de señales y los timers específicos para generarlas:
    struct sigaction on_timeout;
    sigemptyset( &on_timeout.sa_mask );
    on_timeout.sa_sigaction = peticion_actualizar_estadisticas;
    on_timeout.sa_flags     = SA_SIGINFO;                       // Hace que se lea sa_sigaction como manejador.

    struct sigaction on_quit_request;
    sigemptyset( &on_quit_request.sa_mask );
    on_quit_request.sa_sigaction = forzar_finalizacion;
    on_quit_request.sa_flags     = SA_SIGINFO;                  // Hace que se lea sa_sigaction como manejador.
    // ----------------------------------------------------------------------------------



    // TODO: Mover este bloque debajo del bloque donde se establecen los manejadores de señales.
    // ------------------------------------------------------------------------
    // Conjunto que cambia el comportamiento de los hilos bajo ciertas señales:
    sigset_t mask, oldmask;
    sigemptyset( &mask );

    // Bloquea Ctrl+C (Finalizar programa), SIGUSR1: (Acciones regulares definidas por nosotros)
    sigaddset( &mask , SIGINT  );
    sigaddset( &mask , SIGUSR1 );
    status = pthread_sigmask( SIG_BLOCK , &mask , &oldmask );
    if( status != 0 ){
        // Manejar el error.
        fprintf( stderr , "No pueden bloquearse los hilos hijos" );
        exit( EXIT_FAILURE );
    }
    // Desde aquí se pueden crear el resto de los hilos.
    // TODO: Crear hilos aquí
    // TODO: Añadir un barrier a todos los hilos para que esperen hasta que el main esté listo para continuar.
    // pthread_create(...)
    // ...
    
    HilosActores* act = &Actores;

    for( int id = 0 ; id < NPACIENTES ; id += 1 ){
        pthread_create( act->Pacientes + id ,   // Thread-id reference.
                        NULL,                   // No special attributes.
                        &actor_paciente,        // routine.
                        &Tabla_Pacientes + id); // ref. attributes.
    }

    // Hilos relacionados con los hospitales:
    int analista      = 0;
    for( int id = 0 ; id < NHOSPITALES ; id += 1 ){
        // TODO: Verificar si se puede eliminar. Hace algo?
        //pthread_create( &act->StatusUGC    ,        // Thread-id reference.
        //                NULL,                       // No special attributes.
        //                &actor_status_ugc,          // routine.
        //                &gestion_central        );  // ref. attributes.

        pthread_create( act->Gestores + id,     // Thread-id reference.
                        NULL,                   // No special attributes.
                        &actor_gestor,          // routine.
                        &Tabla_Gestores + id);  // ref. attributes.

        //pthread_create( act->JefeEpidemia + id, // Thread-id reference.
        //                NULL,                   // No special attributes.
        //                &actor_jefe_epidemia,   // routine.
        //                &Tabla_Hospitales + id);// ref. attributes.

        pthread_create( act->JefeCuidados + id, // Thread-id reference.
                        NULL,                   // No special attributes.
                        &actor_jefe_cuidados_intensivos, // routine
                        &Tabla_Hospitales + id);// ref. attributes.

        //pthread_create( act->JefeAdmin + id,    // Thread-id reference.
        //                NULL,                   // No special attributes.
        //                &actor_jefe_admin;      // routine
        //                &Tabla_Hospitales + id);// ref. attributes.

        for( int iter = 0 ; iter < NSALA_MUESTRA ; iter += 1 ){
            pthread_create( act->Analistas + analista,      // Thread-id reference.
                            NULL,                           // No special attributes.
                            &actor_analista,                // routine.
                            &Tabla_Hospitales + analista ); // ref. attributes.
            analista += 1;
        }
    }

    // UGC:
    pthread_create( &act->InventarioUGC,        // Thread-id reference.
                    NULL,                       // No special attributes.
                    &actor_inventario_ugc,      // routine.
                    &gestor_central);           // ref. attributes.

    pthread_create( &act->PersonalUGC  ,        // Thread-id reference.
                    NULL,                       // No special attributes.
                    &actor_personal_ugc,        // routine.
                    &gestor_central         );  // ref. attributes.

    //pthread_create( &act->StatusUGC    ,        // Thread-id reference.
    //                NULL,                       // No special attributes.
    //                &actor_status_ugc,          // routine.
    //                &gestor_central         );  // ref. attributes.

    for( int id = 0 ; id < NVOLUNTARIOS ; id += 1 ){
        pthread_create( act->Voluntarios + id ,     // Thread-id reference.
                        NULL,                       // No special attributes.
                        &actor_voluntario,          // routine.
                        &Tabla_Voluntarios + id);   // ref. attributes.
    }
    // ------------------------------------------------------------------------



    // ------------------------------------------------------------------------
    // Al finalizar la creación de hilos:
    // se recupera el bloqueo original
    status = pthread_sigmask( SIG_BLOCK , &oldmask , NULL );

    // Ahora, establece los manejadores de la señal:
    if( sigaction( SIGUSR1 , &on_timeout      , NULL ) == -1 )
        { fprintf(stderr, "Unable to set sigaction\n"); exit( EXIT_FAILURE); }
    if( sigaction( SIGINT  , &on_quit_request , NULL ) == -1 )
        { fprintf(stderr, "Unable to set sigaction\n"); exit( EXIT_FAILURE); }
    // ------------------------------------------------------------------------




    // ------------------------------------------------------------------------
    // Crea y Establece el timer:
    timer_t           timer_id;
    struct sigevent   sev;
    struct itimerspec interval;

    sev.sigev_notify = SIGEV_SIGNAL;    // Notify the process by sending the signal specified in .ssigevn_signo.
    sev.sigev_signo  = SIGUSR1;
    sev.sigev_value.sival_ptr = &timer_id;  // información que pasa junto a una señal.

    if( timer_create( SYS_CLOCK , &sev , &timer_id ) == -1 )
        { fprintf( stderr , "Error al crear el timer." ); exit(EXIT_FAILURE); }

    int segundos      = 10;
    int nano_segundos = 0;
    interval.it_value.tv_sec  = segundos;       // Segundos para activarse por primera vez
    interval.it_value.tv_nsec = nano_segundos;  // Nano Segundos para activarse por primera vez

    interval.it_interval.tv_sec  = segundos;       // Segundos para activarse regularmente.
    interval.it_interval.tv_nsec = nano_segundos;  // Nano Segundos para activarse regularmente.

    // [!] Activar Timer:
    if( timer_settime( timer_id , RELATIVE , &interval , NULL ) )   // No interesa retomar el valor anterior
        { fprintf( stderr , "Error al crear iniciar el timer." ); exit(EXIT_FAILURE); }
    // ------------------------------------------------------------------------





    // Unblock signals:
    if( pthread_sigmask( SIG_UNBLOCK , &mask , NULL ) == -1 )
        { fprintf( stderr , "Error al desbloquear el timer" ); exit(EXIT_FAILURE); }
    exit( EXIT_SUCCESS );
}

// TODO: Finalizar. falta integración con una rutina que libere de todos los recursos e hilos.
void forzar_finalizacion( int signo , siginfo_t* info , void* context ){
    if( signo != SIGINT ) return;
    fprintf( stderr , "Finalizando..." );
    // Realizar tareas de finalización.
}

// TODO: Finalizar. falta integración con status UGC.
void peticion_actualizar_estadisticas( int signo , siginfo_t* info , void* context ){
    if( signo != SIGUSR1 ) return;
    fprintf( stderr , "Peticion: Actualizar estadisticas:..." );
}
