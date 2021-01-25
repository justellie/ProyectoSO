#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // usleep() ó también µsleep()
#include <errno.h> // errno y perror()
#include "../RefQueue.h"
#define MAX 15
#define forever for (;;)
#define TO_MILISEC 1000
// shared_queue.c sin ningún usleep

RefQueue cola;

typedef struct pair { char* name; unsigned sleep_time; } pair;

static char* show_dynamic_int( void* int_ptr );
void* productor ( void* thread_name );
void* consumidor( void* thread_name );

void* productor( void* pair_info ){
    pair*    p    = (pair*) pair_info;
    char*    name = p->name;
    unsigned t    = p->sleep_time;

    int* var;
    int remaining;
    for( int i = 0 ; i < MAX ; i += 1 ){
        var  = (int*) malloc( sizeof(int) );
        *var = i;
        printf( "|> %s: Produciendo ( %d ) a la cola.\n" , name , *var );
        refqueue_put( &cola , var );
        printf( "|| %s: Añadido ( %d ) a la cola.\n" , name , i );
        usleep( t );
    }

    return NULL;
}

void* consumidor( void* pair_info ){
    pair*    p    = (pair*) pair_info;
    char*    name = p->name;
    unsigned t    = p->sleep_time;

    int* ext;
    forever {
        ext = refqueue_get( &cola );
        printf( "<| %s: Se ha extraido (%d) de la cola\n" , name , *ext );
        free( ext );
        usleep( t );
    }

    return NULL;
}

void* konsumidor( void* pair_info ){
    pair*    p    = (pair*) pair_info;
    char*    name = p->name;
    unsigned t    = p->sleep_time;

    extern int errno;
    int        safe_err;

    int* ext;
    forever {
        ext      = refqueue_tryget( &cola );
        safe_err = errno;
        if( ext == NULL && safe_err == EBUSY ){
            printf( "<| %s: Cola vacia.\n" , name );
            printf( "   %s: Manejando Errores.\n" , name );
        } else {
            printf( "<| %s: Se ha extraido (%d) de la cola\n" , name , *ext );
            free( ext );
        }
        usleep( t );
    }

    return NULL;
}

int main(){
    pthread_t prodID , consAD , consBD;
    void     *prodEx ,*consAx ,*consBx;

    refqueue_init( &cola , NULL , NULL );
    pthread_create( &prodID  , NULL , productor  , &((pair) { .name = "Productor   " , .sleep_time = 346 * TO_MILISEC }) );
    pthread_create( &consAD  , NULL , konsumidor , &((pair) { .name = "Consumidor C" , .sleep_time =  99 * TO_MILISEC }) ); // <||
    pthread_create( &consAD  , NULL , consumidor , &((pair) { .name = "Consumidor A" , .sleep_time = 299 * TO_MILISEC }) );
    pthread_create( &consBD  , NULL , consumidor , &((pair) { .name = "Consumidor B" , .sleep_time = 131 * TO_MILISEC }) );

    // Solo espera por el productor, la forma segura de crear a los consumidores es 
    // creando una nueva función: refqueue_try_get
    pthread_join( prodID , &prodEx );
    return 0;
}


// NOTE: Esto genera warnings al compilar, por lo que recomiendo usar los métodos que usan memoria dinámica.
// Función para imprimir enteros:
static char* show_dynamic_int( void* int_ptr ){
    const char max_size = 15;
    int   valor  = *((int*) int_ptr);
    char* buffer = malloc( max_size * sizeof(int) );
    sprintf( buffer , "%d" , valor );
    return buffer;
}

