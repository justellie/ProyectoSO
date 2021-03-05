#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

pthread_t child;

int keepItUp = 1;
void ctrl_c( int signo , siginfo_t* info , void* context ){
    static int lives = 10;

    if( signo != SIGINT ) return;
    lives -= 1;
    fprintf( stderr , "%d: Se pulsó Ctrl+C! (restantes %d)\n" , pthread_self() , lives );

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
    sigset_t mask, oldmask;
    int   status;
    void* retval = NULL;
    sigemptyset( &mask );
    sigaddset( &mask , SIGINT  );
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

    if( sigaction( SIGINT  , &on_quit_request , NULL ) == -1 )
        { fprintf(stderr, "Unable to set sigaction\n"); exit( EXIT_FAILURE); }

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


