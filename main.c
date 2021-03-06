/**
 * @file main.c
 * @brief Programa principal. Inicializa todas las variables e hilos para el funcionamiento del programa.
 * @author Gabriel Peraza
 * @version 0.0.0.4
 * @date 2021-03-04
 */
#include "actores.h"
#include "definiciones.h"

// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// Activa/Deactiva el timer de las estadísticas para debuggear el código.
#define ACTIVAR_ESTADISTICAS 1
//
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

// Crea los timers y eventos particulares:
// SIGUSR1 <-> Llegó el momento de hacer un reporte.
// SIGINT  <-> Hay que liberar los recursos y finalizar el programa.
#define NANOSEGUNDOS_EN_SEGUNDO 1000000000
#define SEGUNDOS_EN_MINUTO      60
#define ABSOLUTE TIMER_ABSTIME
#define RELATIVE 0

// [@] Sincronizacion global ---------
Barrier    Paso_Inicializacion;
Condicion  FinalizarAhora;
int        Continuar = 1;
Mutex      FinalizarAhoraLock;

// [+] Tablas globales de Datos -----------------
Paciente   Tabla_Pacientes[NPACIENTES];
Personal   Tabla_Medicos[NMEDICOS];
Personal   Tabla_Enfermeras[NENFERMERAS];
Hospital   Tabla_Hospitales[NHOSPITALES];
GestorCama Tabla_Gestores[NHOSPITALES];
Voluntario Tabla_Voluntarios[NVOLUNTARIOS];
jefe_uci   Tabla_JefeUCI [NHOSPITALES];
UGC        gestor_central;
Estadistica statHospital[NACTUALIZACIONES][NHOSPITALES];

// [*] Voluntarios -----------
RefQueue pacienteEnCasa;

// [^] Tablas globales de Hilos(Actores) --------
typedef struct HilosActores{
    //                                          Numeración incremental:
    pthread_t Pacientes  [NPACIENTES];      // 1P

    pthread_t Gestores   [NHOSPITALES];     // 1H
    pthread_t Analistas  [NANALISTAS];      // 1A
    pthread_t Voluntarios[NVOLUNTARIOS];    // 1V

    pthread_t Director    [NHOSPITALES];    // 2H   //TODO: Verificar si se puede eliminar. (¿Hace algo?)
    pthread_t JefeEpidemia[NHOSPITALES];    // 3H
    pthread_t JefeUCI     [NHOSPITALES];    // 4H
    pthread_t JefeAdmin   [NHOSPITALES];    // 5H

    pthread_t InventarioUGC;                // 1
    pthread_t PersonalUGC;                  // 1
    pthread_t StatusUGC;                    // 1
} HilosActores;

HilosActores Actores;

/// @brief Finaliza todos los hilos
void finalizarATodos();

/// @brief Espera a todos los hilos y destruye los recursos empleados.
void esperarATodos();


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

    // Datos aleatorios
    srand(time(NULL));

    inicializarPacientes();
    inicializarMedicos();
    inicializarEnfermeras();

    TuplaRecursos RecUGC = {.ncamasBas=NCAMAS_CENTINELA,.ncamasInt=NCAMAS_GENERAL};
    construirUGC        ( &gestor_central , &RecUGC );
    asignarMedicosUGC   ( &gestor_central , Tabla_Medicos    , NMEDICOS    );
    asignarEnfermerasUGC( &gestor_central , Tabla_Enfermeras , NENFERMERAS );

    // TODO: Verficar valores
    inicializarHospitales( 0.20 , 0.30 , 0.50 ); // 0.20 + 0.30 + 0.50 ~= 1.0
    inicializarPacientesEnCasa();
    inicializarVoluntarios();
    inicializarGestorCama();
    inicializarJefeUCI();


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

    // Ahora, establece los manejadores de la señal:
    if( sigaction( SIGUSR1 , &on_timeout      , NULL ) == -1 )
        { fprintf(stderr, "No se puede registrar la señal del reloj (SIGUSR1)\n"); exit( EXIT_FAILURE); }
    if( sigaction( SIGINT  , &on_quit_request , NULL ) == -1 )
        { fprintf(stderr, "No se puede registrar el manejador de (SIGINT)\n"); exit( EXIT_FAILURE); }
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
    // TODO: Añadir un barrier a todos los hilos para que esperen hasta que el main esté listo para continuar.
    HilosActores* act = &Actores;

    for( int id = 0 ; id < NPACIENTES ; id += 1 ){
        pthread_create( &act->Pacientes[id],   // Thread-id reference.
                        NULL,                   // No special attributes.
                        &actor_paciente,        // routine.
                        &Tabla_Pacientes[id]); // ref. attributes.
    }

    // Hilos relacionados con los hospitales:
    int analista      = 0;
    for( int id = 0 ; id < NHOSPITALES ; id += 1 ){
        // TODO: Verificar si se puede eliminar. Hace algo?
        pthread_create( &act->Director[id],         // Thread-id reference.
                        NULL,                       // No special attributes.
                        &actor_director,            // routine.
                        &Tabla_Hospitales[id]);// ref. attributes.

        pthread_create( &act->Gestores[id],     // Thread-id reference.
                        NULL,                   // No special attributes.
                        &actor_gestor,          // routine.
                        &Tabla_Gestores[id]);   // ref.attributes.

        pthread_create( &act->JefeEpidemia[id], // Thread-id reference.
                        NULL,                   // No special attributes.
                        &actor_jefe_epidemia,   // routine.
                        &Tabla_Hospitales[id]);// ref. attributes.

        pthread_create( &act->JefeUCI     [id], // Thread-id reference.
                        NULL,                   // No special attributes.
                        &actor_jefe_cuidados_intensivos, // routine
                        &Tabla_JefeUCI[id]);// ref. attributes.

        pthread_create( &act->JefeAdmin[id],    // Thread-id reference.
                        NULL,                   // No special attributes.
                        &actor_jefe_admin,      // routine
                        &Tabla_Hospitales[id]);// ref. attributes.

        for( int iter = 0 ; iter < NSALA_MUESTRA ; iter += 1 ){
            pthread_create( &act->Analistas[analista],      // Thread-id reference.
                            NULL,                           // No special attributes.
                            &actor_analista,                // routine.
                            &Tabla_Hospitales[id] );        // ref. attributes.
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

    pthread_create( &act->StatusUGC    ,        // Thread-id reference.
                    NULL,                       // No special attributes.
                    &actor_status_ugc,          // routine.
                    &gestor_central         );  // ref. attributes.

    for( int id = 0 ; id < NVOLUNTARIOS ; id += 1 ){
        pthread_create( act->Voluntarios + id ,     // Thread-id reference.
                        NULL,                       // No special attributes.
                        &actor_voluntario,          // routine.
                        &Tabla_Voluntarios + id);   // ref. attributes.
    }
    // ------------------------------------------------------------------------



    // ------------------------------------------------------------------------
    // Al finalizar de la creación de hilos:
    // se recupera el bloqueo original
    status = pthread_sigmask( SIG_BLOCK , &oldmask , NULL );
    // ------------------------------------------------------------------------




    // ------------------------------------------------------------------------
    // Crea y Establece el timer:
    timer_t           timer_id;
    struct sigevent   sev;
    struct itimerspec interval;

    sev.sigev_notify = SIGEV_SIGNAL;    // Notify the process by sending the signal specified in .ssigevn_signo.
    sev.sigev_signo  = SIGUSR1;
    sev.sigev_value.sival_ptr = &timer_id;  // información que pasa junto a una señal.

#if !defined(ACTIVAR_ESTADISTICAS) || (ACTIVAR_ESTADISTICAS != 0 )
    // [!] Crear Timer:
    if( timer_create( SYS_CLOCK , &sev , &timer_id ) == -1 )
        { fprintf( stderr , "Error al crear el timer." ); exit(EXIT_FAILURE); }
#endif

    int segundos      = 10;
    int nano_segundos = 0;
    interval.it_value.tv_sec  = segundos;       // Segundos para activarse por primera vez
    interval.it_value.tv_nsec = nano_segundos;  // Nano Segundos para activarse por primera vez

    interval.it_interval.tv_sec  = segundos;       // Segundos para activarse regularmente.
    interval.it_interval.tv_nsec = nano_segundos;  // Nano Segundos para activarse regularmente.

#if !defined(ACTIVAR_ESTADISTICAS) || (ACTIVAR_ESTADISTICAS != 0 )
    // [!] Activar Timer:
    if( timer_settime( timer_id , RELATIVE , &interval , NULL ) )   // No interesa retomar el valor anterior
        { fprintf( stderr , "Error al crear iniciar el timer." ); exit(EXIT_FAILURE); }
    // ------------------------------------------------------------------------
#endif
    
    // Unblock signals:
    if( pthread_sigmask( SIG_UNBLOCK , &mask , NULL ) == -1 )
        { fprintf( stderr , "Error al desbloquear el timer" ); exit(EXIT_FAILURE); }

    // NOTE: El main será el único quien sirva las señales
    //      especiales SIGINT y SIGUSR1(usada para el timer).
    
    // ------------------------------------
    // Registra los protocolos de limpieza:
    atexit( &esperarATodos   ); // es una pila de llamadas. <- aquella <-.
    atexit( &finalizarATodos ); // primero llama a esta.       luego a --/
    // ------------------------------------
    
    printf( "[H] Nuevo sistema hospitalario\n" );
    pthread_mutex_init( &FinalizarAhoraLock , NULL );
    pthread_cond_init ( &FinalizarAhora     , NULL );

    // 
    // Espera hasta Ctrl+C
    pthread_mutex_lock( &FinalizarAhoraLock );
    while( Continuar ) pthread_cond_wait( &FinalizarAhora , &FinalizarAhoraLock );
    pthread_mutex_unlock( &FinalizarAhoraLock );

    pthread_mutex_destroy( &FinalizarAhoraLock );
    pthread_cond_destroy( &FinalizarAhora      );

    exit( EXIT_SUCCESS );
}

void finalizarATodos(){
    fprintf( stderr , "Cancelando ejecución de hilos.\n" );
    // TODO: No está listo todavía.
    //// Envía request de finalización para todos los hilos:
    //HilosActores* act = &Actores;
    //for( int id = 0 ; id < NPACIENTES ; id += 1 ){
    //    pthread_cancel( act->Pacientes[id] );
    //}

    //for( int id = 0 ; id < NHOSPITALES ; id += 1 ){
    //    pthread_cancel( act->Director[id] );
    //    pthread_cancel( act->Gestores[id] );
    //    pthread_cancel( act->JefeEpidemia[id] );
    //    pthread_cancel( act->JefeUCI[id] );
    //    pthread_cancel( act->JefeAdmin[id] );
    //}

    //for( int iter = 0 ; iter < NANALISTAS ; iter += 1 ){
    //    pthread_cancel( act->Analistas[iter] );
    //}

    //// Voluntarios:
    //for( int iter = 0 ; iter < NANALISTAS ; iter += 1 ){
    //    pthread_cancel( act->Voluntarios[iter] );
    //}

    //pthread_cancel( act->InventarioUGC );
    //pthread_cancel( act->PersonalUGC   );
    //pthread_cancel( act->StatusUGC     );

}

void esperarATodos(){
    fprintf( stderr , "Esperando por finalización de hilos.\n" );
    pthread_mutex_destroy( &FinalizarAhoraLock );
    pthread_cond_destroy ( &FinalizarAhora     );

    //// Espera por los hilos!
    //for( int id = 0 ; id < NPACIENTES ; id += 1 ){
    //    pthread_join( act->Pacientes[id] , NULL );
    //}

    //for( int id = 0 ; id < NHOSPITALES ; id += 1 ){
    //    pthread_join( act->Director[id]     , NULL );
    //    pthread_join( act->Gestores[id]     , NULL );
    //    pthread_join( act->JefeEpidemia[id] , NULL );
    //    pthread_join( act->JefeUCI[id]      , NULL );
    //    pthread_join( act->JefeAdmin[id]    , NULL );
    //}

    //for( int iter = 0 ; iter < NANALISTAS ; iter += 1 ){
    //    pthread_join( act->Analistas[iter] , NULL );
    //}

    //// Voluntarios:
    //for( int iter = 0 ; iter < NVOLUNTARIOS ; iter += 1 ){
    //    pthread_join( act->Voluntarios[iter] , NULL );
    //}

    //pthread_join( act->InventarioUGC , NULL );
    //pthread_join( act->PersonalUGC   , NULL );
    //pthread_join( act->StatusUGC     , NULL );
}

// TODO: Finalizar. falta integración con una rutina que libere de todos los recursos e hilos.
void forzar_finalizacion( int signo , siginfo_t* info , void* context ){
    static int peticion = 0;
    if( peticion || signo != SIGINT ) return;

    peticion  = 1;
    Continuar = 0;
    pthread_cond_signal( &FinalizarAhora );  // Despierta a main para finalizar.
    fprintf( stderr , "Finalizando...\n" );
}


// TODO: Finalizar. falta integración con status UGC.
void peticion_actualizar_estadisticas( int signo , siginfo_t* info , void* context ){
    if( signo != SIGUSR1 ) return;
    fprintf( stderr , "Peticion: Actualizar estadisticas:\n" );
    gestor_central.continuar = 0;
    pthread_cond_signal(&(gestor_central.FinalizarStat));
}

