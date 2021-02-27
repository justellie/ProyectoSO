#define MAX_ATENCION    4       // Maximo número de pacientes que pueden atender.
#define NHOSPITALES     10      // Maximo número de pacientes.
#define NPACIENTES      100     // Se asume un número máximo de pacientes en el sistema.
#define NMEDICOS        15      // Se asume un número máximo de médicos a nivel nacional.
#define NENFERMERAS     30      // Se asume un número máximo de enfermeras a nivel nacional.
#define GESTORES_H      2       // Numeros de gestores por hospital
#define NGESTORES   (NHOSPITALES * GESTORES_H) // Más gestores por cada hospital. TODO: posiblemente sólo quede uno, no sé.
#define NVOLUNTARIOS    5
#define NACTUALIZACIONES 2

#include "Refmap.h"
#include "RefQueue.h"
// [>] POSIX
#include <pthread.h>

// [>] PRIMITIVOS
typedef pthread_mutex_t  Mutex;
typedef pthread_rwlock_t RWLock;
typedef pthread_cond_t   Condicion;

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

// Inventario de Transferencias de la UGC
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


// Agilizan los procesos. No requieren de id
typedef struct {
    int      id;
    int      idHospital;
    RefQueue pacientes;
} GestorCama;
// Verifica el estado del paciente.
void construirGestorCama( GestorCama* g , idHospital ); 
void destruirGestorCama ( GestorCama* g );

// Similar al gestor de cama, pero fuera del hospital.
typedef struct {
    int      id;
    RefQueue pacientes;
} Voluntario;

// -- Todos los hilos deben comenzar con la palabra 'actor_' para identificarlos sin problemas --
//    USAR SNAKE CASE PARA LOS HILOS.
void actor_paciente( void* datos_iniciales );
void actor_voluntario( void* hospital_paciente );

void actor_inventario_ugc( void* datos );
void actor_personal_ugc  ( void* datos );
void actor_status_ugc    ( void* datos );    // daemon. Estadísticas, inventario, uso, resumen de todo, etc.
// Revisan la cola de pacientes interna, 
void actor_jefe_admin   ( void* hospital );
void actor_jefe_epidemia( void* hospital );
void actor_director     ( void* hospital );
void actor_analista( void* hospital );
void actor_gestor  ( void* hospital );

// actor_paciente       --> actor_analista, actor_gestor, actor_inventario_ugc*, actor_voluntario*
// actor_voluntario     --> actor_paciente
//
// actor_inventario_ugc --> actor_director, actor_jefe_admin, actor_paciente*, actor_gestor
// actor_personal_ugc   --> actor_jefe_admin, actor_jefe_admin
// actor_status_ugc     --> todos
//
// actor_jefe_admin     --> actor_gestor, actor_personal_ugc
// actor_jefe_apidemia  --> actor_analista, actor_status_ugc
// actor_analista       --> actor_paciente, actor_jefe_epidemia
// actor_gestor         --> actor_paciente* , actor_inventario_ugc



// --- Tablas Globales ---
// TODO: Todos requieren de inicialización.
Paciente Tabla_Pacientes[NPACIENTES];
Personal Tabla_Medicos[NMEDICOS];
Personal Tabla_Enfermeras[NENFERMERAS];
Hospital Tabla_Hospitales[NHOSPITALES];
GestorCama Tabla_Gestores[NGESTORES];
Voluntario Tabla_Voluntarios[NVOLUNTARIOS];
// TODO: Escribir los sintomas correctamente. Posiblemente cambiar por un tipo de datos.
char Tabla_Sintomas[] = { "Algo" , "Nada" , "..." };


void inicializarPacientes( char* ruta_archivo_pacientes );
void inicializarMedicos( char* ruta_archivo_medicos );
void inicializarEnfermeras( char* ruta_archivo_enfermeras );
void inicializarHospitales();

// ...
// NOTE(sGaps): Más adelante dejaré únicamente los tipos de datos en un archivo que deberá ser citado
//              por los demás archivos fuentes. Se llamará:
//                  definiciones.h
//
//              Los hilos deben implementarse en archivos diferentes para no romper accidentalmente
//              las demás sub-rutinas.
//                  suponiendo que el nombre del hilo a implementar es el símbolo '@',
//                  entonces se tendría que crear el archivo.
//                      'actor_@.c'
//              Las declaraciones de los hilos estarán más adelante en el archivo actores.h, el cual
//              debe ser incluído (#include "actores.h") por cada 'actor_@.c'
//              

// ---- ACERCA DE LAS TRANSFERENCIAS ----
// NOTE(sGaps): Llegará un momento donde los pacientes y el personal son transferidos
//              A otros centros hospitalarios.
//
//              Para el caso del Personal, sólo se podrá extraer aquellos que tengan el
//              mayor nivel de disponibilidad. Es decir, aquellos totalmente desocupados.
//              

//              IDEA(sGaps): Para actualizar la información de un Paciente, se necesita los permisos de
//                           escritura (rwlock)

// NOTE(sGaps): Si un hospital requiere de más personal, se lo pide a la UGC mediante un
//              mensaje (request) que tenga información del emisor y qué necesita.
//              quien gestione el inventario buscará entre los demás hospitales hasta conseguir
//              alguno que tenga suficientes recursos.
//

// NOTE(sGaps): La transferencia del paciente inicia cuando el Gestor revisa el estado del paciente
//              (a través del personal asignado) y detecta que no hay recursos para transferirlo.
//              Éste procederá a llevarlo a la UGC.
//
//              la UGC se encargará de buscar algún hospital que tenga espacio para el paciente, para
//              ello, realiza una reservación (tomando en cuenta el tipo de atención que necesita) en
//              algún hospital.
//
//              Si la operación anterior es exitosa, se actualizarán los datos del paciente y se procederá
//              a liberar al personal del hospital anterior junto a los recursos empleados. En caso
//              contrario, no será posible liberar sus recursos del hospital.
//                  CABE DESTACAR QUE: La UGC toma el control del paciente. por lo tanto, el gestor de cama
//                                     colocará al paciente dentro de la cola de atención de pacientes de
//                                     la UGC.
//

// ---- RESPONSABILIDADES DEL PACIENTE ----
// [Trayecto al hospital]
// 1. Ir al hospital
// 2. Esperar su turno en la cola
// 3. Elegir alguna sala de muestra.
// 4. Notificar cuándo está listo para que le tomen la muestra.
// 5. Esperar a que terminen de tormar la muestra.
// 6. Notificar cuándo sale de la silla.
// 7. Esperar por el servicio en el hospital.
//      (Esto implica transferencias implícitas entre hospitales)
// 9. Salir del Hospital
//
// [De Alta del hospital]
// 1. Esperar a estar enfermo.
//
// [Cuidado en Casa]
// 1. Esperar a estar curado.
// 2. Esperar a estar enfermo.
//
// [Morir]
// 1. Finaliza el hilo
//

// ---- RESPONSABILIDADES DEL ANALISTA ----
// 1. Esperar por silla ocupada
// 2. Tomar muestra
//      NOTIFICAR: cuando no hay material para muestras ni PCRs.
// 4. Dejar la muestra en un lugar accesible para el jefe de epidemiologia
// 3. Notificar muestra lista

// ---- RESPONSABILIDADES DEL JEFE DE EPIDEMIOLOGIA ----
// 1. Esperar algún resultado de una muestra tomada
// 2. Actualizar estadísticas.
// 3. Comprobar si no hay riesgo (misceláneo)
//

// ---- RESPONSABILIDADES DEL GESTOR DE CAMAS ----
// 1. Esperar que algún paciente necesite de atención. (alguno en la cola)
// 2. Diagnóstico según estado del paciente.
// 3. Actualizar días de estancia.
// 4. Reservar cama si necesita:
//    SI TIENE:
//      a. reservar cama, recursos y personal dentro del mismo hospital.
//      b. liberar recursos, cama y personal anterior.
//      ERROR -> pedir personal/recursos ó pedir transferencia [no fue posible atenderlo].
//    NO TIENE
//      a. reservar cama, recursos y personal dentro del mismo hospital.
//      ERROR -> pedir personal/recursos transferencia [no fue posible atenderlo]
//    NO NECESITA
//      a. liberar recursos, cama y personal anterior.
// 5. Pasar al siguiente paciente.
//

// ---- RESPONSABILIDADES DEL INVENTARIO UGC ----
// 1. Esperar por alguna petición de recursos (según descripción)
// 2. Atender petición:     (emisor: idHospital, recurso: físico o paciente, ...)
//      Si no hay recursos en la UGC:
//          traer recursos de otros hospitales
// 3. Brindar recursos al emisor.
//

// ---- RESPONSABILIDADES DEL PERSONAL UGC ----
// 1. Esperar por alguna petición de personal (según descripción)
// 2. Atender petición:     (emisor: idHospital, Tipo: físico o paciente, ...)
//      Si no hay personal en la UGC:
//          traer personal de otros hospitales.
// 3. Brindar personal al emisor.
//

// ---- RESPONSABILIDADES DEL STATUS UGC ----
// 1. Esperar a la hora de actualizacion
// 2. Escribir datos diarios
// 3. Reportar en pantalla
// NOTE: Debe realizarse en horas específicas, 2 veces al día. DEBE INTERRUMPIR
//       CUALQUIER OTRA TAREA QUE SE ESTÉ REALIZANDO EN ESE MOMENTO.

// ---- RESPONSABILIDADES DEL JEFE DE EPIDEMIOLOGIA ----
// 1. Esperar algún resultado de una muestra tomada
// 2. Actualizar estadísticas del hospital
// 3. Tomar medidas en caso de COVID
//

// ---- RESPONSABILIDADES DEL JEFE ADMINISTRATIVO ----
// 1. Esperar a que falte personal.
// 2. Pedir personal
// 3. Actualizar estadísticas del personal/recursos (CADA 12 horas)

// ---- RESPONSABILIDADES DEL DIRECTOR ----
// 0. Inicializar Hospital
// 1. Esperar a que falten pruebas.
// 2. Pedir pruebas a la UGC y recursos.
//
// 3.* Notificar a la UGC cuando un paciente necesita atención en casa (voluntario) a la UGC.

// ---- RESPONSABILIDADES DEL VOLUNTARIO ----
// 1. Esperar a que algún paciente(en Casa) requiera de atención.
// 2. Tomar diagnóstico.
// 3. Acción según diagnóstico
//          -> transferir a hospital.
//          -> dar de alta.
//          -> reportar fallecido.
// 4. Atender al siguiente paciente.
//


