#ifndef __DEFINICIONES_H__
#define __DEFINICIONES_H__

#define MAX_ATENCION    4       // Maximo número de pacientes que pueden atender.
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


// [>] POSIX ----------------------
#include <pthread.h>
typedef pthread_mutex_t  Mutex;
typedef pthread_rwlock_t RWLock;
typedef pthread_cond_t   Condicion;
// --------------------------------

// [T] Tipos de datos -------------
#include "Refmap.h"
#include "RefQueue.h"
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


typedef enum { EnHospital , EnCasa } Lugar;
typedef {
    int   id_lugar; // Tiene sentido cuando es un hospital.
    Lugar lugar;    // En casa o en Hospital
} Ubicacion;

// [*] PACIENTE:
typedef struct {
    int       id;
    int       vivo;
    Ubicacion donde;
    char*     sintomas;

    // Uno de cada uno a la vez. ni más, ni menos.
    int       medID;
    int       enfID;
    rwlock    medLock;
    rwlock    enfLock;
    rwlock    dondeLock;

    Mutex     atendidoLock;     // Permita pausar el hilo actor_paciente
    Condicion atendido;         // mientras espera por ser atendido por algún
                                // Médico/Enfermero ó Voluntario.
} Paciente;
void construirPaciente( Paciente* p , int id );
void destruirPaciente ( Paciente* p );


// [*] HOSPITAL:
typedef enum { TIPO_A , TIPO_B , TIPO_C } TipoHospital;
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
    Refmap       medicos   [MAX_ATENCION];
    RefMap       enfermeras[MAX_ATENCION];
    // TODO:    ^^^ Inicializar ambos grupos de diccionarios.
    //       >>>    Se indexarán por su id.     <<<

    // NOTE: No se necesita saber cuántos hay

    sem_t        salaEspera;
    sem_t        salaMuestra;

    int          id;
    TipoHospital tipo;
    sem_t        camasBasico;
    sem_t        camasIntensivo;
    sem_t        tanquesOxigeno;
    sem_t        respiradores;

    TuplaRecursos estadisticas;     // Se actualizan siempre (incremento en valores), pero son reiniciadas
    RWLock        estadisticasLock; // 2 veces al día. Por ello usan un seguro de escritura/lectura
                                    // (Así como en base de datos)
} Hospital;

void construirHospital( Hospital* h , int id , TuplaRecursos* descripcion );
void destruirHospital ( Hospital* h );

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
    RWLock        estadisticasLock;
    int           turno;        // Indica cuál tabla de estadisticas se debe leer
    RWLock        turnoLock;    // Una vez que se necesite actualizar, simplemente se pasará al valor
                                // turno = (turno+1) % NACTUALIZACIONES;
} UGC;
void construirUGC( UGC* ugc , int id , TuplaRecursos* descripcion );
void destruirUGC( UGC* ugc );




// [*] GESTOR DE CAMA:
//      Agilizan los procesos para dar de alta al paciente.
//      verifican el estado del paciente. y sus días de estancia.
typedef struct {
    int      id;
    int      idHospital;
    RefQueue pacientes;
} GestorCama;
void construirGestorCama( GestorCama* g , int id ); 
void destruirGestorCama ( GestorCama* g );




// [*] VOLUNTARIO:
//      Similar al gestor de cama, pero fuera del hospital.
typedef struct {
    int      id;
    RefQueue pacientes;
} Voluntario;
void construirVoluntario( Voluntario* g , int id );
void destruirVoluntario ( Voluntario* g );




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

// TODO: Describir mejor los sintomas... Combinar con la función que elige aleatoriamente un síntoma.
// TODO: Inicializar de forma estática en el archivo definiciones.c
char Tabla_Sintomas[] = { "Algo" , "Nada" , "..." };

void inicializarPacientes( char* ruta_archivo_pacientes );
void inicializarMedicos( char* ruta_archivo_medicos );
void inicializarEnfermeras( char* ruta_archivo_enfermeras );
void inicializarHospitales();

// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

#endif
