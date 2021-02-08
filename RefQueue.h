/**
 * @file RefQueue.h
 * @brief Define el tipo de datos RefQueue, que puede ser usado para
 *        sincronizar múltiples hilos mediante el paso de referencias a objetos.
 * @author Gabriel Peraza
 * @version 1.0.0.0
 * @date 2021-02-01
 */

#ifndef __QUEUE__H__
#define __QUEUE__H__

#include <stdlib.h> // Memoria dinámica
#include <assert.h> // Pruebas y errores
#include <string.h> // Cadenas
#include <stddef.h> // tipos size_t y ssize_t
#include <stdio.h>  // fprintf
#include <errno.h>  // errno y perror()

// DEPENDIENTE DE UNIX:
#include <pthread.h>

/// @brief Almacena las referencias de cada objeto dentro del tipo RefQueue.
typedef struct _LNode {
    void*          item;    ///< Referencia al objeto del nodo.
    struct _LNode* next;    ///< Referencia al siguiente nodo de la cola.
} _LNode;

/// @brief Estructura FIFO general, con operaciones atómicas.
typedef struct RefQueue {
    _LNode* head;               ///< Primer elemento de la Cola.
    _LNode* last;               ///< Ultimo elemento de la Cola.
    size_t  n;                  ///< Número de elementos actualmente en la cola.

    // Atomicidad:
    pthread_mutex_t lock;       ///< Asegura la exclusión mutua.
    pthread_cond_t  has_item;   ///< Hace que los hilos esperen hasta que la cola contenga algún elemento.

    void  (*free)( void* e );   ///< Protocolo para liberar los objetos de la cola, el cual puede ser **NULL**.
    char* (*str)( void* e );    ///< Protocolo para representar los objetos como cadena, el cual puede ser **NULL**.
} RefQueue;

void   refqueue_singleton( RefQueue* qs );
/// @brief Inicializa una cola de referencias.
/// @param qs Cola a inicializar.
/// @param free Protocolo de liberación de los elementos de la cola.
/// @param str Protocolo para generar la representación de los elementos como cadena de caracteres.
void   refqueue_init   ( RefQueue* qs , void  (*free)(void*) ,
                                        char* (*str) (void*) );
// Operaciones Atómicas:
void   refqueue_put    ( RefQueue* qs , void* item );
void*  refqueue_get    ( RefQueue* qs );
void*  refqueue_tryget ( RefQueue* qs );    // retorna NULL y errno = EBUSY si qs está vacío.
void   refqueue_clean  ( RefQueue* qs );    // Just uses pop.
void   refqueue_destroy( RefQueue* qs );    // Uses free. TODO: Renombrar a refqueue_freeobjs()
                                            // TODO: Agregar refqueue_destroy() para liberar todo.
char*  refqueue_str    ( RefQueue* qs );    // New string must be freed.
void   refqueue_show_in( RefQueue* qs , FILE* stream );

// Operaciones No-Atómicas:
size_t refqueue_unsafe_len  ( RefQueue* qs );
int    refqueue_unsafe_empty( RefQueue* qs );

/// @fn void refqueue_singleton( RefQueue* qs )
/// @brief Crea una cola de referencias trivial.
/// @param qs Cola a inicializar.
/// @details Inicializa una cola de referencias sin protocolos de liberación de memoria dinámica ni
//           protocolos para mostrar los elementos de la misma.
/// @see refqueue_init.

/// @fn void refqueue_init( RefQueue* qs , void (*free)(void*) , char* (*str) (void*) )
/// @brief Inicializa una cola de referencias.
/// @param qs Cola a inicializar.
/// @param free Protocolo de liberación de los elementos de la cola.
/// @param str Protocolo para generar la representación de los elementos como cadena de caracteres.

/// @fn void refqueue_put( RefQueue* qs , void * item )
/// @brief Inserta un objeto en la cola.
/// @param qs Cola destino.
/// @param item Referencia al objeto a insertar.
/// @details Usa la exclusión mutua para insertar la referencia del objeto al final de la cola.

/// @fn void* refqueue_get( RefQueue* qs )
/// @brief Extrae un elemento de la cola de manera atómica.
/// @param qs Cola objetivo.
/// @return la referencia del objeto en caso de exito.
/// @details Usa la exclusión mutua para extraer la referencia del objeto que está en el frente de la cola.
///          **El hilo invocador quedará bloqueado hasta que sea insertado algún elemento dentro de qs**.

/// @fn void* refqueue_tryget( RefQueue* qs )
/// @brief Intenta extraer un elemento de la cola.
/// @param qs Cola objetivo.
/// @return la referencia del objeto en caso de exito. NULL en caso contrario.
/// @details Similar a refqueue_get. retorna NULL y se establece a errno = EBUSY cuando la cola está
//           vacía. Se puede utilizar para extraer elementos de manera segura y manejar los errores
//           cuando sea necesario.
/// @see refqueue_get.

/// @fn void refqueue_clean( RefQueue* qs )
/// @brief Elimina todas las referencias de la cola.
/// @param qs Cola objetivo.
//  @details Las referencias de la cola son descartadas. Es particularmente útil cuando los elementos de
//           qs fueron reservados de forma estática.

// TODO: Modificar para simplemente destruir el semáforo interno y la variable de condición.
/// @fn void refqueue_destroy( RefQueue* qs )
/// @brief Libera todas las referencias de la cola.
/// @param qs Cola objetivo.
/// @details Aplica el protocolo de liberación en todos los elementos de la cola y la deja vacía.
/// @see refqueue_clean

/// @fn char* refqueue_str( RefQueue* qs )
/// @brief Obtiene la representación de la cola como cadena.
/// @param qs Cola objetivo.
/// @return Representación de la cola en el formato: "[ e1 , e2 , ... ]" 
/// @details Convierte a cadena cada elemento de la cola y los junta en una sola cadena.

/// @fn void refqueue_show_in( RefQueue* qs , FILE* stream )
/// @brief Imprime la cola en un archivo.
/// @param qs Cola objetivo.
/// @param stream flujo de caracteres objetivo.
/// @details Muestra la cola en un archivo. Útil durante las tareas de depuración.
/// @see refqueue_str.

/// @fn size_t refqueue_unsafe_len( RefQueue* qs )
/// @brief Obtiene la longitud de la cola.
/// @param qs Cola objetivo.
/// @warning No es Thread-Safe.
/// @return longitud de la cola. No se asegura la exclusión mutua.

/// @fn int refqueue_unsafe_empty( RefQueue* qs )
/// @brief Verifica si la cola está vacía.
/// @param qs Cola objetivo.
/// @warning No es Thread-Safe.
/// @return Lógico. No se asegura la exclusión mutua.

#endif
