#include "RefQueue.h"
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>

// DEPENDIENTE DE UNIX:
#include <pthread.h>

#define STOP_IF_UNSAFE( ptr , err_msg ) \
    do { if( ptr == NULL ){             \
            perror( err_msg );          \
            exit( 1 ); }                \
    } while(0);

// Declaracion de funciones estáticas (visibles sólo para el archivo actual)
static void  refqueue_unsafe_put  ( RefQueue* qs , void* elem );
static void* refqueue_unsafe_get  ( RefQueue* qs );
static char* refqueue_unsafe_str  ( RefQueue* xs );
static char* opaque_object_str    ( void* ignored );

// Nodo:
static _LNode* new_node( void* e );
static void only_free_node( _LNode* node );
static void free_node( _LNode* node , void (*free_obj)(void*) );

static _LNode* new_node( void* e ){
    _LNode* node = (_LNode*) malloc(sizeof(_LNode));
    STOP_IF_UNSAFE( node , "[new_node] allocation_error" );
    node->item = e;
    node->next = NULL;
    return node;
}

// {Pre: node != NULL}
static void only_free_node( _LNode* node ){
    free(node);
}

// {Pre: node != NULL}
static void free_node( _LNode* node ,
                       void (*free_obj)(void*) ){
    (*free_obj)(node->item);
    free(node);
}

void refqueue_singleton( RefQueue* qs ){
    refqueue_init( qs , NULL , NULL );
}

// NOTE: xs->str() debe ser una función que tome un objeto de la cola
//       y devuelva una cadena que deba ser liberada más adelante.
void refqueue_init( RefQueue* qs         ,
                    void  (*free)(void*) ,  // can be NULL -> doesn't free
                    char* (*str) (void*) ){ // can be NULL -> doesn't contents 
    qs->head = NULL;
    qs->last = NULL;
    qs->n    = 0;

    // Atomicity:
    pthread_mutex_init( &(qs->lock)     , NULL );
    pthread_cond_init ( &(qs->has_item) , NULL );

    qs->free = free;
    qs->str  = str;
}

// Modelo: Productor-Consumidor:
void refqueue_put( RefQueue* qs , void* item ){
    RefQueue* self = qs;
    // |> Begin Critical Region:
    pthread_mutex_lock( &self->lock );

    // Unsafe operations:
    refqueue_unsafe_put( self , item );
    pthread_cond_signal( &self->has_item );  // Wake up only a thread

    // |> End Critical Region:
    pthread_mutex_unlock( &self->lock );
}

void* refqueue_get( RefQueue* qs ){
    RefQueue* self = qs;
    void*     item;
    // |> Begin Critical Region:
    pthread_mutex_lock( &self->lock );

    // Unsafe operations:
    while( refqueue_unsafe_empty(self) )
        pthread_cond_wait( &self->has_item , &self->lock ); // it will acquire the lock.

    item = refqueue_unsafe_get( self );

    // |> End Critical Region:
    pthread_mutex_unlock( &self->lock );
    return item;
}

// Si qs está vacío, retorna NULL y establece a errno = EBUSY.
void* refqueue_tryget( RefQueue* qs ){
    RefQueue* self = qs;
    void*     item;
    // |> Begin Critical Region:
    pthread_mutex_lock( &self->lock );

    // Unsafe operations:
    if( refqueue_unsafe_empty(self) ){
        // |> End Critical Region:
        pthread_mutex_unlock( &self->lock );
        errno = EBUSY;
        return NULL;
    } else {
        item = refqueue_unsafe_get( self );

        // |> End Critical Region:
        pthread_mutex_unlock( &self->lock );
        return item;
    }
}

static void refqueue_unsafe_put( RefQueue* qs , void* item ){
    _LNode* node = new_node( item );

    if( refqueue_unsafe_empty(qs) ) {
        qs->head = node;
        qs->last = node;
    } else {
        qs->last->next = node;
        qs->last       = node;
    }
    qs->n         += 1;
}

static void* refqueue_unsafe_get( RefQueue* qs ){
    _LNode* target = qs->head;
    void*   item   = target->item;

    if( refqueue_unsafe_len(qs) == 1 )
        qs->last = NULL;
        
    target   = qs->head;
    qs->head = qs->head->next;
    qs->n   -= 1;
    only_free_node( target );
    return item;
}

int    refqueue_unsafe_empty( RefQueue* qs ){ return qs->head == NULL; }
size_t refqueue_unsafe_len  ( RefQueue* qs ){ return qs->n; }

static char* opaque_object_str( void* ignored ){
    const char dummyObj[] = "(*)";
    const int  size       = sizeof(dummyObj);
    char* str = malloc( size * sizeof(char) + 1 );
    STOP_IF_UNSAFE( str , "[opaque_object_str] allocation error" );
    strcpy( str , dummyObj );
    return str;
}

static char* refqueue_unsafe_str( RefQueue* xs ){
    static const char start[] = "[" , end[] = "]" , comma[] = ", ";
    static const int  slen  = sizeof(start) - 1; // Removes the '\0'
    static const int  clen  = sizeof(comma) - 1;
    static const int  elen  = sizeof(end) - 1;

    _LNode* node   = xs->head;
    char** elems   = malloc(xs->n * sizeof(char*));
    int*   lengths = malloc(xs->n * sizeof(int)  );
    int    size    = 0;
    char* (*toStr)(void*) = xs->str? xs->str : opaque_object_str; // Which will be used to print?

    STOP_IF_UNSAFE( elems   , "[refqueue_unsafe_str] allocation error (str set)" );
    STOP_IF_UNSAFE( lengths , "[refqueue_unsafe_str] allocation error (lengths set)" );

    // Prints something cool:
    for( int i = 0 ; i < xs->n ; i += 1 ){
        elems[i]   = (*toStr)( node->item );
        lengths[i] = strlen( elems[i] );
        node       = node->next;
        size      += lengths[i];
    }

    if( refqueue_unsafe_empty(xs) )
        size += slen + elen;
    else
        size   += slen + ((xs->n - 1) * clen) + elen;
    char* str  = malloc( size + 1 );
    char* curr = str;

    STOP_IF_UNSAFE( elems , "[refqueue_unsafe_str] allocation error (render str)" );

    strncpy( curr , start , slen );
    curr += slen;
    if( !refqueue_unsafe_empty(xs) ){
        for( int i = 0 ; i < xs->n - 1 ; i += 1 ){
            // Element:
            strncpy( curr , elems[i] , lengths[i] );
            curr += lengths[i];

            // Comma:
            strncpy( curr , comma , clen );
            curr += clen;
        }
    }

    strncpy( curr , elems[xs->n-1] , lengths[xs->n-1] );
    curr += lengths[xs->n-1];

    strncpy( curr , end , elen );
    str[size] = '\0';

    for( int i = 0 ; i < xs->n ; i += 1 ){
        free( elems[i] );
    }
    free( elems   );
    free( lengths );
    return str;
}

char* refqueue_str( RefQueue* qs ){ // New string must be freed.
    char* str; 
    RefQueue* self = qs;
    pthread_mutex_lock( &self->lock );
    str = refqueue_unsafe_str( self );
    pthread_mutex_unlock( &self->lock );
    return str;
}     

void refqueue_show_in( RefQueue* qs , FILE* stream ){
    char* strQueue = refqueue_str(qs);
    fprintf( stream , "%s" , strQueue );
    free( strQueue );
}

// NOTE: Ser cuidadoso con esto... Sólo usar si no importa perder
//       las referencias de los objetos
void refqueue_clean( RefQueue* qs ){
    RefQueue* self    = qs;
    pthread_mutex_lock( &self->lock );
    while( !refqueue_unsafe_empty(self) ){
        refqueue_unsafe_get(self); // result ignored. reference thrown
    }
    pthread_mutex_unlock( &self->lock );
}

void refqueue_destroy( RefQueue* qs ){
    RefQueue* self    = qs;
    pthread_mutex_lock( &self->lock );
    while( !refqueue_unsafe_empty(self) ){
        refqueue_unsafe_get(self); // result ignored. reference thrown
    }
    pthread_mutex_unlock ( &self->lock );
    pthread_mutex_destroy( &self->lock );
    pthread_cond_destroy( &self->has_item );
}

void refqueue_deallocateAll( RefQueue* qs ){
    RefQueue* self    = qs;
    void (*freeObj)() = self->free? self->free : free;

    pthread_mutex_lock( &self->lock );
    while( !refqueue_unsafe_empty(self) ){
        (*freeObj)( refqueue_unsafe_get(self) );
    }

    pthread_mutex_unlock ( &self->lock );
    pthread_mutex_destroy( &self->lock );
    pthread_cond_destroy( &self->has_item );
}

#undef IS_SAFE_PTR
