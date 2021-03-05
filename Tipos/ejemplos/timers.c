#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

#define RELATIVE 0
#define ABSOLUTE TIMER_ABSTIME

pthread_t child;

int keepItUp = 1;
int lives = 10;
void ctrl_c( int signo , siginfo_t* info , void* context ){
    if( signo != SIGINT ) return;
    lives -= 1;
    fprintf( stderr , "%d: Se pulsó Ctrl+C! (restantes %d)\n" , pthread_self() , lives );

    // Realizar tareas de finalización.
    if( !lives ) keepItUp = 0;
}

void myTimeout( int signo , siginfo_t* info , void* context ){
    if( signo != SIGUSR1 ) return;
    lives -= 1;
    fprintf( stderr , "%d: El timer ha finalizado! (restantes %d)\n" , pthread_self() , lives );

    // Realizar tareas de finalización.
    if( !lives ) keepItUp = 0;
}

void* regular( void* _ ){
    while( keepItUp ){
        fprintf( stdout , "%d: Ejecutando tareas...\n" , pthread_self() );
        sleep(1);
    }
    return NULL;
}

int main(){
    int SYS_CLOCK = CLOCK_REALTIME;
    timer_t           timer_id;
    struct sigevent   sev;
    struct itimerspec interval;

    sev.sigev_notify = SIGEV_SIGNAL;    // Notify the process by sending the signal specified in .ssigevn_signo.
    sev.sigev_signo  = SIGUSR1;         // se envía la señal SIGUSR1.
    sev.sigev_value.sival_ptr = &timer_id;  // información que pasa junto a una señal.

    if( timer_create( SYS_CLOCK , &sev , &timer_id ) == -1 )
        { fprintf( stderr , "Error al crear el timer." ); exit(EXIT_FAILURE); }

    int segundos      = 3;
    int nano_segundos = 0;
    interval.it_value.tv_sec  = segundos;       // Segundos para activarse por primera vez
    interval.it_value.tv_nsec = nano_segundos;  // Nano Segundos para activarse por primera vez

    interval.it_interval.tv_sec  = segundos;       // Segundos para activarse regularmente.
    interval.it_interval.tv_nsec = nano_segundos;  // Nano Segundos para activarse regularmente.

    // [!] Activar Timer:
    if( timer_settime( timer_id , RELATIVE , &interval , NULL ) )   // No interesa retomar el valor anterior
        { fprintf( stderr , "Error al crear iniciar el timer." ); exit(EXIT_FAILURE); }


    sigset_t mask, oldmask;
    int   status;
    void* retval = NULL;
    sigemptyset( &mask );
    sigaddset( &mask , SIGINT  );
    sigaddset( &mask , SIGUSR1 );   // De usuario
    status = pthread_sigmask( SIG_BLOCK , &mask , &oldmask );
    if( status != 0 ){
        // Manejar el error.
        fprintf( stderr , "No pueden bloquearse los hilos hijos" );
        exit( EXIT_FAILURE );
    }

    struct sigaction on_quit_request;
    sigemptyset( &on_quit_request.sa_mask );
    on_quit_request.sa_sigaction = ctrl_c;
    on_quit_request.sa_flags     = SA_SIGINFO;                  // Hace que se lea sa_sigaction como manejador.

    struct sigaction on_timeout;
    sigemptyset( &on_timeout.sa_mask );
    on_timeout.sa_sigaction = myTimeout;
    on_timeout.sa_flags     = SA_SIGINFO;                       // Hace que se lea sa_sigaction como manejador.

    if( sigaction( SIGINT  , &on_quit_request , NULL ) == -1 )
        { fprintf(stderr, "Unable to set sigaction\n"); exit( EXIT_FAILURE ); }
    // Establece la accion para el timer
    if( sigaction( SIGUSR1 , &on_timeout , NULL ) == -1 )
        { fprintf(stderr, "Unable to set sigaction\n"); exit( EXIT_FAILURE ); }

    status = pthread_create( &child ,    // Thread-id reference.
                    NULL,               // No special attributes.
                    &regular,           // routine.
                    NULL);              // ref. params
    // Recupera el control de las señales previamente modificadas.
    status = pthread_sigmask( SIG_UNBLOCK , &mask , NULL );
    sleep(10);

    pthread_join( child , NULL );
    fprintf( stdout , "Salida\n" );
    return 0;
}


