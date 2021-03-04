#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../RefQueue.h"
#define MAX 10

static char* show_int( void* int_value );

int main(){
    // Prueba simple (1), usando memoria dinámica:
    RefQueue cola;
    refqueue_init( &cola , NULL , NULL );
    int* var;
    for( int i = 0 ; i < MAX ; i += 1 ){
        var  = (int*) malloc( sizeof(int) );
        *var = i;
        refqueue_put( &cola , var );
    }

    printf( "[*1] Prueba con memoria Dinámica:\n"
            "     Cola de Referencias Opaca (longitud %d): " , MAX );
    refqueue_show_in( &cola , stdout );
    printf( "\n" );

    // Imprime y libera la variable:
    var = refqueue_get( &cola );
    printf( "     Frente [debe ser 0]: %d\n" , *var );
    free( var );

    var = refqueue_get( &cola );
    printf( "     Frente [debe ser 1]: %d\n" , *var );
    free( var );

    // Totalmente requerido en este caso!
    refqueue_deallocateAll( &cola );  // Elimina los objetos de la cola.


    // Prueba Con una constante (2)
    refqueue_init( &cola , NULL , NULL );
    refqueue_put( &cola , (void*) 99 ); // Cast explícito de una constante a un apuntador void
    printf( "[*2] Prueba mediante el uso de una constante:\n"
            "     Cola de Referencias Opaca con una constante (longitud %d): " , 1 );
    refqueue_show_in( &cola , stdout );
    printf( "\n" );
    refqueue_clean( &cola );    // Sólo descarta las referencias que estaban en la cola.


    // Prueba Con constantes (3)
    refqueue_init( &cola , NULL , show_int );
    refqueue_put( &cola , (void*) 22 ); // Cast explícito de una constante a un apuntador void
    refqueue_put( &cola , (void*) 49 ); // Cast explícito de una constante a un apuntador void
    refqueue_put( &cola , (void*) 35 ); // Cast explícito de una constante a un apuntador void
    printf( "[*3] Prueba con varias constantes:\n"
            "     Cola de Referencias Opaca con varias constantes (longitud %d): " , 3 );
    refqueue_show_in( &cola , stdout );
    printf( "\n" );
    refqueue_clean( &cola );    // Sólo descarta las referencias que estaban en la cola.
    return 0;
}

// NOTE: Esto genera warnings al compilar, por lo que recomiendo usar los métodos que usan memoria dinámica.
// Función para imprimir enteros:
static char* show_int( void* int_value ){
    const char max_size = 15;
    long  valor  = (long) int_value;
    char* buffer = malloc( max_size * sizeof(long) );
    sprintf( buffer , "%d" , valor );
    return buffer;
}
