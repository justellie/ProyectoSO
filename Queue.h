#ifndef __QUEUE__H__
#define __QUEUE__H__

#include <stdlib.h> // Memoria dinámica
#include <assert.h> // Pruebas y errores
#include <string.h> // Cadenas
#include <stddef.h> // tipos size_t y ssize_t
#include <stdio.h>  // fprintf

// DEPENDIENTE DE UNIX:
#include <pthread.h>

typedef struct _LNode {
    void*          item;
    struct _LNode* next;
} _LNode;

enum Ord { LT = -1 , EQ = 0 , GT = 1 };

typedef struct RefQueue {
    _LNode* head;
    _LNode* last;
    size_t  n;

    // Atomicidad:
    pthread_mutex_t lock;
    pthread_cond_t  has_item;

    // TODO(sGaps): verificar si realmente se necesita esto (creo que no)
    //int   (*compare)( void* e1 , void* e2 );
    //void* (*newobj)( void* e );
    void  (*free)( void* e );
    char* (*str)( void* e );
} RefQueue;

void   refqueue_init   ( RefQueue* qs , void  (*free)(void*) ,
                                        char* (*str) (void*) );
// Atomic
void   refqueue_put    ( RefQueue* qs , void* item );
void*  refqueue_get    ( RefQueue* qs );
void   refqueue_clean  ( RefQueue* qs );    // Just uses pop.
void   refqueue_destroy( RefQueue* qs );    // Uses free.
char*  refqueue_str    ( RefQueue* qs );      // New string must be freed.
void   refqueue_show_in( RefQueue* qs , FILE* stream );

size_t refqueue_unsafe_len  ( RefQueue* qs );
int    refqueue_unsafe_empty( RefQueue* qs );
void*  refqueue_unsafe_last ( RefQueue* xs , void* item );

#endif