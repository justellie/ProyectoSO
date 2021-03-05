#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

pthread_t           child;
pthread_cond_t      FinalizarAhora;
int                 Continuar = 1;
pthread_mutex_t     FinalizarAhoraLock;

void ctrl_c( int signo , siginfo_t* info , void* context ){
    if( signo != SIGINT ) return;

    Continuar = 0;
    pthread_cond_signal( &FinalizarAhora );  // Despierta a main para finalizar.
    fprintf( stderr , "Finalizando...\n" );
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

    // Recupera el control de las señales previamente modificadas.
    status = pthread_sigmask( SIG_UNBLOCK , &mask , NULL );

    pthread_mutex_init( &FinalizarAhoraLock , NULL );
    pthread_cond_init ( &FinalizarAhora     , NULL );

    pthread_mutex_lock( &FinalizarAhoraLock );
    while( Continuar ) pthread_cond_wait( &FinalizarAhora , &FinalizarAhoraLock );
    pthread_mutex_unlock( &FinalizarAhoraLock );

    pthread_mutex_destroy( &FinalizarAhoraLock );
    pthread_cond_destroy( &FinalizarAhora      );
    return 0;
}

