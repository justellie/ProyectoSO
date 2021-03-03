#ifndef __DEFINICIONES_H__
#define __DEFINICIONES_H__

#define MAX_ATENCION    5       // Maximo número de pacientes que pueden atender.
#define NHOSPITALES     10      // Maximo número de pacientes.
#define NPACIENTES      100     // Se asume un número máximo de pacientes en el sistema.
#define NMEDICOS        15      // Se asume un número máximo de médicos a nivel nacional.
#define NENFERMERAS     30      // Se asume un número máximo de enfermeras a nivel nacional.
#define GESTORES_H      2       // Numeros de gestores por hospital

// Más gestores por cada hospital.
// NOTE: posiblemente sólo se necesite uno de ellos, no estoy seguro.
#define NGESTORES   (NHOSPITALES * GESTORES_H) 
#define NVOLUNTARIOS        5   // Cuántos voluntarios hay en el país
#define NACTUALIZACIONES    2   // Cuántas veces se actualizan las estadísticas de la UGC.

#define NSALA_ESPERA        20  // # de puestos en la sala de espera.
#define NSALA_MUESTRA       5   // # de habitaciones de toma de muestras.

#include <stdbool.h>

// [>] POSIX ----------------------
#include <pthread.h>
#include <semaphore.h>
typedef pthread_mutex_t  Mutex;
typedef pthread_cond_t   Condicion;
// --------------------------------

// [T] Tipos de datos -------------
#include "Tipos/RefMap.h"
#include "Tipos/RefQueue.h"
// --------------------------------


// [*] PERSONAL:
typedef enum { Ninguno, EnCasa, Basica, Intensivo, Muerto} TipoAtencion; // Antes: enum cama.
typedef enum { Medico , Enfermera } TipoPersonal;
typedef struct {
    int          id;
    int          idHospital;                // QUESTION(sGaps): ¿Es necesario? (digo, sólo se puede transferir si está totalmente libre)
    TipoPersonal tipo;
    TipoAtencion servicio;
    int          pacientes[MAX_ATENCION];   // QUESTION(sGaps): ¿Es necesario? sabemos que los pacientes
    int          cuantos;                   // almacenan el id de alguno del personal. Por lo que se pueden
                                            // Ubicar desde allí.
                                            //
                                            // Además, la ocupación del médico/enfermera la decide el hospital
                                            // con los arreglos de árboles(medicos/enfermeras)
} Personal;
void construirPersonal( Personal* p , int id , TipoPersonal profesion , TipoAtencion servicio );
void destruirPersonal ( Personal* p );


//typedef enum { EnHospital , EnCasa } Lugar;
//typedef struct{
//    int   id_lugar; // Tiene sentido cuando es un hospital.
//    Lugar lugar;    // En casa o en Hospital
//} Ubicacion;

// [*] PACIENTE:
typedef struct {
    int          id;
    int          vivo;
    //Ubicacion    donde;
    char*        sintomas;
    TipoAtencion servicio;
    int          tiene_cama;

    // Uno de cada uno a la vez. ni más, ni menos.
    int       medID[MAX_ATENCION];  // TODO: Verificar el número de médicos.
    int       enfID[MAX_ATENCION];  // 
    Mutex     medLock;
    Mutex     enfLock;
    //Mutex     dondeLock;

    Mutex     atendidoLock;     // Permita pausar el hilo actor_paciente
    Condicion atendido;         // mientras espera por ser atendido por algún
                                // Médico/Enfermero ó Voluntario.
} Paciente;
void construirPaciente( Paciente* p , int id );
void destruirPaciente ( Paciente* p );


// [*] HOSPITAL:
//      medicos y pacientes deben ser diccionarios de la forma:
//      { (TipoPersonal,id) : Personal }
typedef enum { Centinela , Intermedio , General } TipoHospital;
// Agrupa ciertos campos de una vez.
typedef struct {
    int ncamasBas;
    int ncamasInt;
    int ntanques;
    int nrespira;
} TuplaRecursos;
typedef struct {
    // NOTE: El personal estára dividido por grados de disponibilidad.
    //       Para reservar, se debe extraer algún profesional del nivel más alto
    //       de disponibilidad. Si no hay ninguno, se buscará en el nivel inferior.
    //          ``` mi_medico = extraer de medicos[MAX_ATENCION-1]```
    //
    //       Luego, se deberá colocar a ése profesional en el grupo de menor
    //       disponibilidad.
    //          ``` colocar mi_medico en medicos[MAX_ATENCION-2] ```
    //
    //       Se realizará esto hasta llegar al último nivel (0) del cual, no será
    //       posible extraer a nadie. En tal caso, debe reportarse que ha ocurrido un
    //       error. y manejarlo según corresponda.
    RefMap       medicos   [MAX_ATENCION];
    RefMap       enfermeras[MAX_ATENCION];
    RefQueue       pacientesEnSilla;
    RefQueue       pacientesListoParaAtender;
    // TODO:    ^^^ Inicializar ambos grupos de diccionarios.
    //       >>>    Se indexarán por su id.     <<<
    // NOTE: No se necesita saber cuántos hay
    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
    //      IDEA(sGaps): Se puede usar la tupla (servicio,id) para organizar al personal.
    //                      Suponiendo que:
    //                      ```
    //                      MEDICOS = medicos[disponibilidad_actual]
    //                      ```
    //
    //                   Se podría insertar un elemento con `refmap_put( MEDICOS , &un_medico , &un_medico );`
    //                      + Al crear al RefMap, se pasa una función de comparación que aplique un cast desde `void*` a `Personal*`
    //                      ```
    //                          void cmp_por_servicio_id( void* p1 , void* p2 ){ 
    //                              Personal *x = p1 , *y= p2;
    //                              int result = cmp(x->servicio,y->servicio);
    //                              return (result == IGUALES)? cmp(x->id,y->id) : result; }
    //                      ```
    //                      + Se pasa también una función de copia que devuelva la función identidad.
    //                          `void sin_copiar( void* key ){ return key; }`
    //                      + Se pasa una función de liberación que no haga nada:
    //                          `void no_borrar( void* key ){ return NULL; }`
    //
    //                      ¿Por qué usar sin_copiar/borrar? El RefMap copiará la clave, pero esta existe dentro de cada
    //                      personal. Como cada miembro es inicializado en el programa principal, no se puede borrar ningún
    //                      dato del mismo desde aquí (no es la responsabilidad del hospital ni de los gestores).
    //
    //                   Para remover un elemento (AKA: reservar), se podría usar:
    //                      `un_medico = refmap_extract_min_if_key( MEDICOS , EXTRAER_SI_BASICO );`
    //                   para pedir algún médico del servicio básico y
    //                      `un_medico = refmap_extract_min_if_key( MEDICOS , EXTRAER_SI_INTENSIVO );`
    //
    RefQueue     pacientes;
    sem_t        salaEspera;
    sem_t        salaMuestra;

    int          id;
    TipoHospital tipo;
    sem_t        camasBasico;
    sem_t        camasIntensivo;
    sem_t        tanquesOxigeno;
    sem_t        respiradores;

    TuplaRecursos estadisticas;     // Se actualizan siempre (incremento en valores), pero son reiniciadas
    Mutex         estadisticasLock; // 2 veces al día. Por ello usan un seguro de escritura/lectura
                                    // (Así como en base de datos)
} Hospital;

void construirHospital( Hospital* h , int id , TuplaRecursos* descripcion );
void destruirHospital ( Hospital* h );

Hospital H[MAX_ATENCION];

// [*] INVENTARIO UGC:
//      Inventario de Transferencias de la UGC
typedef struct {
    // Todos los recursos a disposición:
    sem_t        camasBasico;
    sem_t        camasIntensivo;
    sem_t        tanquesOxigeno;
    sem_t        respiradores;

    // TODO: Inicializar primero antes de usar.
    // Todos están disponibles.
    RefQueue     medicos;
    RefQueue     enfermeras;
    RefQueue     pacientes;
    RefQueue     voluntarios;

    TuplaRecursos estadisticas[NACTUALIZACIONES];
    Mutex         estadisticasLock;
    int           turno;        // Indica cuál tabla de estadisticas se debe leer
    Mutex         turnoLock;    // Una vez que se necesite actualizar, simplemente se pasará al valor
                                // turno = (turno+1) % NACTUALIZACIONES;
} UGC;
void construirUGC( UGC* ugc , int id , TuplaRecursos* descripcion );
void destruirUGC ( UGC* ugc );




// [*] GESTOR DE CAMA:
//      Agilizan los procesos para dar de alta al paciente.
//      verifican el estado del paciente. y sus días de estancia.
typedef struct {
    int       id;
    Hospital* hospital;
} GestorCama;
void construirGestorCama( GestorCama* g , int id , Hospital* hospital );
void destruirGestorCama ( GestorCama* g );


// [*] VOLUNTARIO:
//      Similar al gestor de cama, pero fuera del hospital.
typedef struct {
    int      id;
    RefQueue pacientes;
} Voluntario;
void construirVoluntario( Voluntario* v , int id );
void destruirVoluntario ( Voluntario* v );

// Se podría utilizar si se decide emplear tuplas como llaves dentro de los diccionarios.
void EXTRAER_SI_BASICO   ( void* personal );
void EXTRAER_SI_INTENSIVO( void* personal );




// TODO: Dejar referencias globales que se puedan alcanzar desde
//       cualquier hilo.
//       vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

// ---- Tablas Globales ----
// NOTE: Tdos requieren de inicializacion
Paciente   Tabla_Pacientes[NPACIENTES];
Personal   Tabla_Medicos[NMEDICOS];
Personal   Tabla_Enfermeras[NENFERMERAS];
Hospital   Tabla_Hospitales[NHOSPITALES];
GestorCama Tabla_Gestores[NGESTORES];
Voluntario Tabla_Voluntarios[NVOLUNTARIOS];

void inicializarPacientes( char* ruta_archivo_pacientes );
void inicializarMedicos( char* ruta_archivo_medicos );
void inicializarEnfermeras( char* ruta_archivo_enfermeras );
void inicializarHospitales();

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#endif
