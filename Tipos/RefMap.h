/**
 * @file RefMap.h
 * @brief Define el tipo de datos RefMap, que puede ser usado para
 *        preservar el orden de los elementos al proporcionar llaves
 *        de búsqueda.
 * @author Gabriel Peraza
 * @version 1.0.0.0
 * @date 2021-02-08 */

// Estándar usado: __PAQUETE__SUBPAQUETE__ ... __NOMBRE_MODULO_
#ifndef __REFMAP_H__
#define __REFMAP_H__

// Liberías de C y POSIX/Unix:
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>  // errno y perror()
#include <math.h>   // REQUIRES -lm

// ----------------------------------------------------------------------------------
// NOTE:    Se ha utilizado la estructura de datos Red-Black Trees del curso
//          Algorithms de Robert Sedgewick y Kevin Wayne. El material está disponible
//          en línea en su página https://algs4.cs.princeton.edu/
// ----------------------------------------------------------------------------------

/// @typedef COLOR
/// @brief Tipo de enlace.
/// @details BLACK implica que se tiene un nodo con dos enlaces (L, R) en cambio, RED implica
///          que el nodo tiene tres enlaces (L, C, R).
typedef enum { BLACK = 0 , RED = 1 } COLOR;

/// @brief Nodo Usado dentro del de RefMap.
/// @details No almacena espacio para los valores(value) sólo sus referencias. Sin embargo
///          Puede reservar espacio para las llaves(key) si se sumistra un protocolo de copia
///          Adecuado.
typedef struct _nodeRB{
    void*  key;             ///< Clave de comparación.
    void*  value;           ///< Valor arbitrario.
    struct _nodeRB* left;   ///< sub-árbol izquierdo.
    struct _nodeRB* right;  ///< sub-árbol derecho.
    COLOR  color;           ///< color del enlace desde el padre hasta este nodo. [RED ó BLACK].
                            ///< En el material, se utiliza un booleano para representar el color.
                            ///< donde true === RED y black === false.
    int             N;      ///< número de nodos en este sub-árbol.
    // NOTE: Se ha cambiado el orden de los elementos para reducir el tamaño de la estructura
    //       causado por el padding. Antes, se usaban 8 bytes extras para el alignment &
    //       padding y utilizaba 48 Bytes. con este ajuste, se utilizan 40 bytes... Una
    //       mejora del 16.67% en cuanto la administración de memoria!
} NodeRB;

/// @brief Diccionario o Mapa basado en la estructura de datos Red-Black Trees.
typedef struct RefMap{
    // Atomicidad:
    pthread_mutex_t lock;               ///< Asegura la exclusión mutua.

    NodeRB* root;                       ///< Raíz del árbol
    // Metodos especiales:
    void* (*copykey)( void* key );      ///< Protocolo para copiar las claves. Asegura que sean inmutables.
    void  (*freekey)( void* key );      ///< Protocolo para liberar el espacio reservado para las claves.
    int (*cmp) ( void* k1 , void* k2 ); ///< Protocolo para comparar las claves.
} RefMap;

//      -----------------------------
//      [*] Declaración de funciones:
//      -----------------------------

/// @brief Inicializa un diccionario o mapa.
/// @param t Mapa destino.
/// @param cmp Protocolo de comparación de las claves.
/// @param copyProtocol Protocolo para copiar las claves.
/// @param deallocationProtocol Protocolo para liberar las claves.
void   refmap_init       ( RefMap* t,
                           int   (*cmp) (void*,void*),
                           void* (*copyProtocol)(void*),
                           void  (*deallocationProtocol)(void*) );  // ()
void   refmap_put        ( RefMap* t , void* key , void* value );   // ()
void*  refmap_extract    ( RefMap* t , void* key );         // TODO: cambiar todos los métodos de extracción por:
void*  refmap_extract_min( RefMap* t );                     //      int refmap_extract( RefMap* t , void* key , void* value )
void*  refmap_extract_max( RefMap* t );                     //      para devolver el estado de error en lugar de usar al errno.
void   refmap_delete     ( RefMap* t , void* key );         // ()
void   refmap_deleteMin  ( RefMap* t );                     // ()
void   refmap_deleteMax  ( RefMap* t );                     // ()
void   refmap_clear      ( RefMap* t );                     // ()
void   refmap_destroy    ( RefMap* t );                     // ()

// Miscelaneous:
/// @brief Extrae la clave más pequeña del mapa si cumple con la condición.
/// @param t Mapa objetivo.
/// @param predicate Predicado que se aplica sobre la clave mas pequeña de t.
/// @return El valor asociado a la clave más pequeña de t, siempre que t no esté vacío
//          y dicha clave cumpla con el predicado.
void*  refmap_extract_min_if_key( RefMap* t , int (*predicate)(void*) );
/// @brief Extrae la clave más grande del mapa si cumple con la condición.
/// @param t Mapa objetivo.
/// @param predicate Predicado que se aplica sobre la clave mas pequeña de t.
/// @return El valor asociado a la clave más grande de t, siempre que t no esté vacío
//          y dicha clave cumpla con el predicado.
void*  refmap_extract_max_if_key( RefMap* t , int (*predicate)(void*) );

void*  refmap_unsafe_get   ( RefMap* t , void* key );       // ()
int    refmap_unsafe_empty ( RefMap* t );                   // ()
int    refmap_unsafe_size  ( RefMap* t );                   // ()
int    refmap_unsafe_contains ( RefMap* t , void* key );    // ()
void*  refmap_unsafe_minkey( RefMap* t );                      // ()
void*  refmap_unsafe_maxkey( RefMap* t );                      // ()
/// @brief Muestra el mapa en stderr.
/// @param t Mapa objetivo.
/// @param use_ascii_color Lógico. si es 0, no muestra colores. si es 1 muestra colores. (Formato Unix)
/// @param print_key Protocolo para mostrar la clave en stderr.
/// @param print_value Protocolo para mostrar el valor en stderr.
/// @warning No es Thread-safe.
void   refmap_debug( RefMap* t , int use_ascii_color , void (*print_key)(void*) , void (*print_value)(void*) );

/// @fn void refmap_init( RefMap* t, int (*cmp) (void*,void*), void* (*copyProtocol)(void*), void (*deallocationProtocol)(void*) )
/// @brief Inicializa un diccionario o mapa.
/// @param t Mapa destino.
/// @param cmp Protocolo de comparación de las claves.
/// @param copyProtocol Protocolo para copiar las claves.
/// @param deallocationProtocol Protocolo para liberar las claves.

/// @fn void refmap_put( RefMap* t , void* key , void* value )
/// @brief Inserta o Reemplaza la referencia de un valor dentro del mapa.
/// @param t Mapa objetivo.
/// @param key Clave de comparación.
/// @param value Referencia del valor a insertar/reemplazar

/// TODO: modificar errno cuando no se encuentra ningún elemento.
/// @fn void* refmap_extract( RefMap* t , void* key )
/// @brief Extrae la referencia del valor asociado para key.
/// @param t Mapa objetivo.
/// @param key Clave de comparación.
/// @param value Referencia del valor a insertar/reemplazar
/// @returns Referencia del valor asociado con key. Si no se encuentra key, retorna NULL.
/// @details Obtiene y remueve la referencia del mapa, mediante la exclusión mutua.


/// @fn void* refmap_extract_min( RefMap* t )
/// @brief Extrae el valor asociado para la clave más pequeña del mapa.
/// @param t Mapa objetivo.
/// @returns Referencia del valor asociado con key. Si no se encuentra key, retorna NULL.
/// @see refmap_extract


/// @fn void refmap_delete( RefMap* t , void* key )
/// @brief Remueve el valor asociado para la clave proporcionada.
/// @param t Mapa objetivo.
/// @param key Clave de comparación.
/// @details Remueve la referencia del mapa, mediante la exclusión mutua.

/// @fn void refmap_deleteMin( RefMap* t )
/// @brief Remueve el valor asociado para la clave más pequeña del mapa.
/// @param t Mapa objetivo.
/// @see refmap_delete

/// @fn void refmap_clear( RefMap* t )
/// @brief Remueve todas las referencias del mapa.
/// @param t Mapa objetivo.
/// @details El mapa queda vacío una vez aplicada la operación.

/// @fn void refmap_destroy( RefMap* t )
/// @brief Remueve todas las referencias del mapa y destruye el mapa.
/// @param t Mapa objetivo.
/// @details El mapa queda vacío una vez aplicada la operación. Destruye los objetos usados
///          para asegurar la exclusión mutua.
/// @see refmap_clear

/// @fn void* refmap_unsafe_get( RefMap* t , void* key )
/// @brief Consulta el valor asociado a la clave proporcionada dentro de un mapa.
/// @param t Mapa objetivo.
/// @param key Clave de comparación.
/// @warning No es Thread-safe.
/// @return Referencia del objeto asociado con key. NULL si no se encuentra dentro del mapa.

/// @fn int refmap_unsafe_empty( RefMap* t );
/// @brief Verifica si el mapa está vacío.
/// @param t Mapa objetivo.
/// @warning No es Thread-safe.
/// @return Lógico. Verdad si está vacío.

/// @fn int refmap_unsafe_size( RefMap* t );
/// @brief Consulta el número de elementos dentro del mapa.
/// @param t Mapa objetivo.
/// @warning No es Thread-safe.
/// @return Número de elementos dentro del mapa. 0 si está vacío.

/// @fn int refmap_unsafe_contains( RefMap* t , void* key )
/// @brief Consulta si la clave está dentro del mapa.
/// @param t Mapa objetivo.
/// @param key Clave de comparación.
/// @warning No es Thread-safe.
/// @return Lógico. Verdad si key está dentro de t.

/// @fn void* refmap_unsafe_minkey( RefMap* t )
/// @brief Función que consulta la clave más pequeña del mapa.
/// @param t Mapa objetivo.
/// @warning No es Thread-safe.
/// @return Referencia a la clave de búsqueda más pequeña. NULL si está vacío.

/// @fn void* refmap_unsafe_maxkey( RefMap* t )
/// @brief Función que consulta la clave más grande del mapa.
/// @param t Mapa objetivo.
/// @warning No es Thread-safe.
/// @return Referencia a la clave de búsqueda más grande. NULL si está vacío.

/// @fn void refmap_debug( RefMap* t , int use_ascii_color , void (*print_key)(void*) , void (*print_value)(void*) );
/// @brief Muestra el mapa en stderr.
/// @param t Mapa objetivo.
/// @param use_ascii_color Lógico. si es 0, no muestra colores. si es 1 muestra colores. (Formato Unix)
/// @param print_key Protocolo para mostrar la clave en stderr.
/// @param print_value Protocolo para mostrar el valor en stderr.
/// @warning No es Thread-safe.

#endif
