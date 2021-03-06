/**
 * @file definiciones.h
 * @brief Definiciones de las estructuras y variables globales usadas dentro del programa.
 * @author Todos
 * @version 0.0.3.1
 * @date 2021-03-04
 */
#ifndef __DEFINICIONES_H__
#define __DEFINICIONES_H__

#define MAX_ATENCION    5       // Maximo número de pacientes que pueden atender.
#define NHOSPITALES     3      // Maximo número de pacientes.
#define NPACIENTES      10     // Se asume un número máximo de pacientes en el sistema.
#define NMEDICOS        5      // Se asume un número máximo de médicos a nivel nacional.
#define NENFERMERAS     8      // Se asume un número máximo de enfermeras a nivel nacional.
#define GESTORES_H      1       // Numeros de gestores por hospital
#define NANALISTAS      (NHOSPITALES * NSALA_MUESTRA)

// Más gestores por cada hospital.
// NOTE: posiblemente sólo se necesite uno de ellos, no estoy seguro.
#define NVOLUNTARIOS        5   // Cuántos voluntarios hay en el país
#define NACTUALIZACIONES    2   // Cuántas veces se actualizan las estadísticas de la UGC.

#define NSALA_ESPERA        20  // # de puestos en la sala de espera.
#define NSALA_MUESTRA       5   // # de habitaciones de toma de muestras.

#define NLOTE_PCR     30    // # de PCR que se solicitan por vez
#define NMIN_PCR      5    // # mínimo de PCR en el hospital antes de pedir otro lote
// ---------------------------
#define NCAMAS_CENTINELA    25
#define NCAMAS_INTERMEDIO   20
#define NCAMAS_GENERAL      15
#define NCAMAS_UGC          200

#define PORCENTAJE_INT_CENTINELA   0.20
#define PORCENTAJE_INT_INTERMEDIO  0.15
#define PORCENTAJE_INT_GENERAL     0.05
// ---------------------------

#include <stdbool.h>
#include <errno.h>
#include <math.h> // Requires -lm

// [T] Tipos de datos -------------
#include "Tipos/RefMap.h"
#include "Tipos/RefQueue.h"
// --------------------------------

// [>] POSIX ----------------------
#include <pthread.h>
#include <semaphore.h>
typedef pthread_mutex_t   Mutex;
typedef pthread_cond_t    Condicion;
typedef pthread_barrier_t Barrier;
// --------------------------------



// [*] PERSONAL:
typedef enum { Ninguno, EnCasa, Basica, Intensivo, Muerto} TipoAtencion; // Antes: enum cama.
typedef enum { PidePCR , PideTanque, PideRespirador,PideEnfermera,PideMedico} Recurso; 
typedef enum { Medico , Enfermera } TipoPersonal;
typedef enum { test=1} MuestraPcr;
typedef enum { dummyTanque } TanqueDato;
typedef enum { dummyRespirador } Respirador;

typedef struct {
    int          id;
    TipoPersonal tipo;
} Personal;

void construirPersonal( Personal* p , int id , TipoPersonal profesion );
void destruirPersonal ( Personal* p );

// [*] PACIENTE:
typedef struct {
    int          id;
    int          vivo;
    int          deAlta;
    int          fueAtendido;
    int          ingresando;
    int          tiene_cama;
    TipoAtencion servicio;
    

    // Uno de cada uno a la vez. ni más, ni menos.
    int       medID[MAX_ATENCION];  // TODO: Verificar el número de médicos.
    int       enfID[MAX_ATENCION];  // 
    Mutex     medLock;
    Mutex     enfLock;
    //Mutex     dondeLock;

    sem_t muestraTomada;

    Mutex     atendidoLock;     // Permita pausar el hilo actor_paciente
    Condicion atendido;         // mientras espera por ser atendido por algún
                                // Médico/Enfermero ó Voluntario.
} Paciente;
void construirPaciente( Paciente* p , int id );
void destruirPaciente ( Paciente* p );


// [*] HOSPITAL:

//Permite llevar el conteo de los movimientos de los pacientes
typedef struct {
    int muertos;
    int hospitalizados;
    int dadosDeAlta;
    int monitoreados;
    int covid;
}Estadistica;
//      medicos y pacientes deben ser diccionarios de la forma:
//      { (TipoPersonal,id) : Personal }
typedef enum { Centinela , Intermedio , General } TipoHospital;
// Agrupa ciertos campos de una vez.
typedef struct {
    int ncamasBas;
    int ncamasInt;
    int nenfermeras;
    int nmedicos;
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
    RefQueue     pacientesEnSilla;	
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

    Estadistica  estadis_pacientes;
    RefQueue     pacientes;
    sem_t        salaEspera;
    sem_t        salaMuestra;

    int          id;
    TipoHospital tipo;
    sem_t        camasBasico;
    sem_t        camasIntensivo;

    RefQueue     tanquesOxigeno;
    RefQueue     respiradores;
    RefQueue     PCR;
    RefQueue     reporte;

    Condicion        stast;
    TuplaRecursos estadis_recursos; // Se actualizan siempre (incremento en valores), pero son reiniciadas
    Mutex         estadisticasLock; // 2 veces al día. Por ello usan un seguro de escritura/lectura
                                    // (Así como en base de datos)
} Hospital;

void construirHospital( Hospital* h , int id , TipoHospital tipo , int camasInt , int camasBas );
void destruirHospital ( Hospital* h );

// [*] INVENTARIO UGC:
//      Inventario de Transferencias de la UGC
typedef struct {
    // Todos los recursos a disposición:
    sem_t        camasBasico;
    sem_t        camasIntensivo;
    sem_t        tanquesOxigeno;
    sem_t        respiradores;
    // TODO: convertir ambos a mutex
    Mutex        espera_personal;
    Mutex        EsperandoPorRecurso;

    // TODO: Inicializar primero antes de usar.
    // Todos están disponibles.
    RefQueue     medicos;
    RefQueue     enfermeras;
    RefQueue     pacientes;
    RefQueue     voluntarios;
    RefQueue     peticiones;
    RefQueue     peticionesPersonal;
    

    TuplaRecursos estadisticas[NACTUALIZACIONES];
    Mutex         estadisticasLock;
    int           turno;        // Indica cuál tabla de estadisticas se debe leer
    Mutex         turnoLock;    // Una vez que se necesite actualizar, simplemente se pasará al valor
                                // turno = (turno+1) % NACTUALIZACIONES;

    Condicion FinalizarStat;
    int continuar;
    Mutex FinalizarStatLock;

} UGC;
void construirUGC( UGC* ugc , TuplaRecursos* descripcion );
void destruirUGC ( UGC* ugc );

// Tupla de peticion a inventario.
typedef struct {
    int idHospital;
    Recurso tipo_recurso; 
    int cantidad;
} TuplaInventario;




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
} Voluntario;
void construirVoluntario( Voluntario* v , int id );
void destruirVoluntario ( Voluntario* v );

typedef struct {
    int id;
    Hospital*        refHospital;
    pthread_mutex_t  espera;
}jefe_uci;
void construirJefeUCI( jefe_uci* j , int id , Hospital* h );
void destruirJefeUCI ( jefe_uci* j );

//RefQueue pacienteEnCasa;

// referencias globales que se puedan alcanzar desde
// cualquier hilo.
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv

// ---- Tablas Globales ----
// NOTE: Tdos requieren de inicializacion
//Paciente   Tabla_Pacientes[NPACIENTES];
//Personal   Tabla_Medicos[NMEDICOS];
//Personal   Tabla_Enfermeras[NENFERMERAS];
//Hospital   Tabla_Hospitales[NHOSPITALES];
//GestorCama Tabla_Gestores[NHOSPITALES];
//Voluntario Tabla_Voluntarios[NVOLUNTARIOS];

void inicializarPacientes();
void inicializarMedicos();
void inicializarEnfermeras();
void inicializarHospitales( float porc_centinelas, float porc_intermedio , float porc_general );
void inicializarPacientesEnCasa();
void inicializarVoluntarios();
void inicializarGestorCama();
void inicializarJefeUCI();


void borrarPacientes();
void borrarMedicos();
void borrarEnfermeras();
void borrarHospitales();
void borrarPacientesEnCasa();
void borrarVoluntarios();
void borrarGestorCama();
void borrarJefeUCI();


TipoAtencion obtener_diagnostico_simple();
TipoAtencion obtener_diagnostico_compuesta(void *paciente);
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// [@] Sincronizacion global ---------
extern Barrier    Paso_Inicializacion;
extern Condicion  FinalizarAhora;
extern int        Continuar;
extern Mutex      FinalizarAhoraLock;

// [+] Tablas globales de Datos -----------------
extern Paciente   Tabla_Pacientes[NPACIENTES];
extern Personal   Tabla_Medicos[NMEDICOS];
extern Personal   Tabla_Enfermeras[NENFERMERAS];
extern Hospital   Tabla_Hospitales[NHOSPITALES];
extern GestorCama Tabla_Gestores[NHOSPITALES];
extern jefe_uci   Tabla_JefeUCI [NHOSPITALES];
extern Voluntario Tabla_Voluntarios[NVOLUNTARIOS];
extern UGC        gestor_central;
extern Estadistica statHospital[NACTUALIZACIONES][NHOSPITALES];

// [*] Voluntarios -----------
extern RefQueue pacienteEnCasa;

#endif
