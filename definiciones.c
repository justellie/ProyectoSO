#include "definiciones.h"
#include "Tipos/RefMap.h"
#include "Tipos/RefQueue.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <math.h> // Requires -lm

#define COMPARE(x,y) (                          \
                        ((x) < (y))?            \
                            -1 :                \
                            (                   \
                                ((x) > (y))?    \
                                        1 : 0   \
                            )                   \
                     )

// ...................................................
// [@] Sincronizacion global ---------
//extern Barrier    Paso_Inicializacion;
//
//// [+] Tablas globales global -------------------
//extern Paciente   Tabla_Pacientes[NPACIENTES];
//extern Personal   Tabla_Medicos[NMEDICOS];
//extern Personal   Tabla_Enfermeras[NENFERMERAS];
//extern Hospital   Tabla_Hospitales[NHOSPITALES];
//extern GestorCama Tabla_Gestores[NHOSPITALES];
//extern Voluntario Tabla_Voluntarios[NVOLUNTARIOS];
//extern UGC        gestion_central;
//
//// [*] Voluntarios -----------
//extern RefQueue pacienteEnCasa;
// ...................................................


// Define todos los constructores y destructores dentro de definiciones.h
static const int CompartidoEntreHilos = 0;

static void* ignorar_copia( void* personal );
static int  comparar_por_id( void* personal_a , void* personal_b );
static void ignorar_borrardo( void* personal );

void construirPersonal( Personal* p , int id , TipoPersonal profesion ){
    p->id   = id;
    p->tipo = profesion;
}

void destruirPersonal ( Personal* p ){
    p->id       = -1;
}

void construirPaciente( Paciente* p , int id ){
    p->id           = id;
    p->vivo         = 1;
    p->deAlta       = 0;
    p->fueAtendido  = 0;
    p->ingresando   = 1;
    p->tiene_cama   = 0;
    p->servicio     = Ninguno;

    // Ningún médico:
    for( int i = 0 ; i < MAX_ATENCION ; i += 1 ){
        p->medID[i] = -1;
        p->enfID[i] = -1;
    }
    // Ningún atributo por ahora.
    pthread_mutex_init( &p->medLock   , NULL );
    pthread_mutex_init( &p->enfLock   , NULL );
    //pthread_mutex_init( &p->dondeLock , NULL );

    // Permite pausar al paciente hasta que sea necesario.
    pthread_mutex_init( &p->atendidoLock , NULL );
    pthread_cond_init  ( &p->atendido     , NULL );


    pthread_mutex_init( &p->medLock , NULL );
}

void destruirPaciente ( Paciente* p ){
    p->id    = -1;
    p->vivo  = 0;
    p->servicio       = Ninguno;

    // Ningún médico:
    for( int i = 0 ; i < MAX_ATENCION ; i += 1 ){
        p->medID[i] = -1;
        p->enfID[i] = -1;
    }

    pthread_mutex_destroy( &p->medLock   );
    pthread_mutex_destroy( &p->enfLock   );

    pthread_mutex_destroy( &p->atendidoLock );
    pthread_cond_destroy ( &p->atendido );
}

static void* ignorar_copia( void* personal ){
    return personal;
}

static void ignorar_borrardo( void* personal ){
    return;
}

static int comparar_por_id( void* personal_a , void* personal_b ){
    Personal* a = personal_a;
    Personal* b = personal_b;
    return COMPARE(a->id,b->id);
}

void construirHospital( Hospital* h , int id , TipoHospital tipo , int camasBas , int camasInt ){
    for( int i = 0 ; i < MAX_ATENCION ; i += 1 ){
        refmap_init( &h->medicos[i]    , comparar_por_id , ignorar_copia , ignorar_borrardo );
        refmap_init( &h->enfermeras[i] , comparar_por_id , ignorar_copia , ignorar_borrardo );
    }

    // Init:
    refqueue_singleton( &h->pacientesEnSilla );
    refqueue_singleton( &h->reporte );
    // Init:
    h->estadis_pacientes = (Estadistica) {0};

    // Init:
    refqueue_singleton( &h->pacientes );
    sem_init( &h->salaEspera  , CompartidoEntreHilos , NSALA_ESPERA  );
    sem_init( &h->salaMuestra , CompartidoEntreHilos , NSALA_MUESTRA );

    // Init:
    h->id   = id;
    h->tipo = tipo;

    // Init:
    // TODO: revisar el número de camas. Debe ser fijo!!
    sem_init( &h->camasBasico    , CompartidoEntreHilos , camasBas );
    sem_init( &h->camasIntensivo , CompartidoEntreHilos , camasInt );
    refqueue_singleton( &h->tanquesOxigeno );
    refqueue_singleton( &h->respiradores   );
    refqueue_singleton( &h->PCR );
    refqueue_singleton( &h->reporte );

    // Init:
    // TODO: Hay que inicilaizar los campos nuevos!!
    // No se ha usado nada:
    pthread_cond_init  ( &h->stast, NULL );
    h->estadis_recursos = (TuplaRecursos) { .ncamasBas=camasBas,
                                            .ncamasInt=camasInt, 
                                            .nrespira=0, 
                                            .ntanques=0, 
                                            .nenfermeras=0, 
                                            .nmedicos=0};
    pthread_mutex_init( &h->estadisticasLock , NULL );
}

void destruirHospital ( Hospital* h ){
    for( int i = 0 ; i < MAX_ATENCION ; i += 1 ){
        refmap_destroy( &h->medicos[i]    );
        refmap_destroy( &h->enfermeras[i] );
    }
    refqueue_destroy( &h->pacientesEnSilla );

    h->estadis_pacientes = (Estadistica) {0};
    refqueue_destroy( &h->pacientes   );
    sem_destroy     ( &h->salaEspera  );
    sem_destroy     ( &h->salaMuestra );

    h->id   = -1;
    h->tipo =  0;
    sem_destroy( &h->camasBasico    );
    sem_destroy( &h->camasIntensivo );

    refqueue_destroy( &h->tanquesOxigeno );
    refqueue_destroy( &h->respiradores   );
    refqueue_destroy( &h->PCR            );

    // No se ha usado nada:
    h->estadis_recursos = (TuplaRecursos) {0};
    pthread_mutex_destroy( &h->estadisticasLock );
}

void construirUGC( UGC* ugc , TuplaRecursos* descripcion ){
    sem_init( &ugc->camasBasico    , CompartidoEntreHilos , descripcion->ncamasBas );
    sem_init( &ugc->camasIntensivo , CompartidoEntreHilos , descripcion->ncamasInt );
    sem_init( &ugc->tanquesOxigeno , CompartidoEntreHilos , descripcion->ntanques  );
    sem_init( &ugc->respiradores   , CompartidoEntreHilos , descripcion->nrespira  );

    refqueue_singleton( &ugc->medicos     );
    refqueue_singleton( &ugc->enfermeras  );
    refqueue_singleton( &ugc->pacientes   );
    refqueue_singleton( &ugc->voluntarios );


    for( int i = 0 ; i < NACTUALIZACIONES ; i += 2 ){
        ugc->estadisticas[i] = (TuplaRecursos) {0};
    }

    pthread_mutex_init( &ugc->estadisticasLock , NULL );

    ugc->turno = 0;
    pthread_mutex_init( &ugc->turnoLock , NULL );
    pthread_mutex_init( &ugc->FinalizarStatLock , NULL );
    ugc->continuar = 1;
    pthread_cond_init ( &ugc->FinalizarStat     , NULL );

}
void destruirUGC( UGC* ugc ){
    sem_destroy( &ugc->camasBasico    );
    sem_destroy( &ugc->camasIntensivo );
    sem_destroy( &ugc->tanquesOxigeno );
    sem_destroy( &ugc->respiradores   );

    refqueue_destroy( &ugc->medicos     );
    refqueue_destroy( &ugc->enfermeras  );
    refqueue_destroy( &ugc->pacientes   );
    refqueue_destroy( &ugc->voluntarios );

    for( int i = 0 ; i < NACTUALIZACIONES ; i += 2 ){
        ugc->estadisticas[i].ncamasBas = 0;
        ugc->estadisticas[i].ncamasInt = 0;
        ugc->estadisticas[i].ntanques  = 0;
        ugc->estadisticas[i].nrespira  = 0;
    }

    pthread_mutex_destroy( &ugc->estadisticasLock );
    ugc->turno = 0;
    pthread_mutex_destroy( &ugc->turnoLock );
}

void construirGestorCama( GestorCama* g , int id , Hospital* hospital ){
    g->id       = id;
    g->hospital = hospital;
}

void destruirGestorCama ( GestorCama* g ){
    g->id       = -1;
    g->hospital = NULL;
}

void construirVoluntario( Voluntario* v , int id ){
    v->id       = id;
}

void destruirVoluntario( Voluntario* v ){
    v->id       = -1;
}

void construirJefeUCI( jefe_uci* j , int id , Hospital* h ){
    j->id          = id;
    j->refHospital = h;
    pthread_mutex_init( &j->espera , NULL );
}
void destruirJefeUCI ( jefe_uci* j ){
    j->id          = -1;
    j->refHospital = NULL;
    pthread_mutex_destroy( &j->espera );
}



TipoAtencion obtener_diagnostico_simple()
{
    srand(time(NULL));
    return rand()% 4;
}

TipoAtencion obtener_diagnostico_compuesta(void *paciente)
{
    Paciente *atendiendo=(Paciente *)paciente;
    int diagnostico_nuevo=0;
    

    srand(time(NULL));
    diagnostico_nuevo= rand()% 5;

    diagnostico_nuevo=atendiendo->servicio*0.5+diagnostico_nuevo*0.5;
    diagnostico_nuevo=(diagnostico_nuevo-2)/2;
    return diagnostico_nuevo;

}


// [!] Inicialización general:
void inicializarPacientes(){
    Paciente* grupo = Tabla_Pacientes;
    for( int i = 0 ; i < NPACIENTES ; i += 1 ){
        construirPaciente( grupo + i , i );
    }
}

void inicializarMedicos(){
    Personal* grupo = Tabla_Medicos;
    for( int i = 0 ; i < NMEDICOS; i += 1 ){
        construirPersonal( grupo + i , i , Medico );
    }
}

void inicializarEnfermeras(){
    Personal* grupo = Tabla_Enfermeras;
    for( int i = 0 ; i < NENFERMERAS ; i += 1 ){
        construirPersonal( grupo + i , i , Enfermera );
    }
}

// TODO: Determinar el número de camas por hospitales.
// Centinelas:  20% de camas intensivos, 70% camas básicas.
// Intermedios: 15% de camas intensivas. 85% camas básicas.
// General:      5% de camas intensivas. 95% camas básicas.
// Pre: {0.0 < porc_* <= 1.0}
// Pre: { sum( x | x = porc_* ) == 1.0 }
void inicializarHospitales( float porc_centinelas, float porc_intermedio , float porc_general ){
    Hospital* grupo = Tabla_Hospitales;
    int camas_int_cent = (int) floor( PORCENTAJE_INT_CENTINELA  * NCAMAS_CENTINELA );
    int camas_int_intr = (int) floor( PORCENTAJE_INT_INTERMEDIO * NCAMAS_INTERMEDIO );
    int camas_int_gene = (int) floor( PORCENTAJE_INT_GENERAL    * NCAMAS_GENERAL );

    int hosp_cent = (int) floor( porc_centinelas * NHOSPITALES );
    int hosp_intr = (int) floor( porc_intermedio * NHOSPITALES );
    int hosp_gene = (int) floor( porc_general    * NHOSPITALES );

    int id = 0;
    // Inicializa los 3 tipos de hospitales:
    for( int i = 0 ; i < NHOSPITALES && i < hosp_cent ; i += 1 ){
        construirHospital( grupo + id , id , Centinela , camas_int_cent , NCAMAS_CENTINELA - camas_int_cent );
        id += 1;
    }

    for( int i = 0 ; i < NHOSPITALES && i < hosp_intr ; i += 1 ){
        construirHospital( grupo + id , id , Intermedio , camas_int_intr , NCAMAS_INTERMEDIO - camas_int_intr );
        id += 1;
    }

    for( int i = 0 ; i < NHOSPITALES && i < hosp_gene ; i += 1 ){
        construirHospital( grupo + id , id , General , camas_int_gene , NCAMAS_GENERAL - camas_int_gene );
        id += 1;
    }
}

void inicializarPacientesEnCasa(){
    // Global:
    refqueue_singleton( &pacienteEnCasa );
}

void inicializarVoluntarios(){
    Voluntario* grupo = Tabla_Voluntarios;
    for( int i = 0 ; i < NVOLUNTARIOS ; i += 1 ){
       construirVoluntario( grupo + i , i );
    }
}

void inicializarGestorCama(){
    Hospital *grupoh = Tabla_Hospitales;
    GestorCama *grupog = Tabla_Gestores;
    for (int i = 0; i < NHOSPITALES; i++)
    {
        construirGestorCama(grupog + i, i, grupoh + i);
    }
}

void inicializarJefeUCI(){
    Hospital *grupoh = Tabla_Hospitales;
    jefe_uci *grupoj = Tabla_JefeUCI;
    for( int i = 0; i < NHOSPITALES; i += 1 ) {
        construirJefeUCI(grupoj + i, i, grupoh + i);
    }
}

// --------------
// [!] borrado general:
void borrarPacientes(){
    Paciente* grupo = Tabla_Pacientes;
    for( int i = 0 ; i < NPACIENTES ; i += 1 ){
        destruirPaciente( grupo + i );
    }
}

void borrarMedicos(){
    Personal* grupo = Tabla_Medicos;
    for( int i = 0 ; i < NMEDICOS; i += 1 ){
        destruirPersonal( grupo + i );
    }
}

void borrarEnfermeras(){
    Personal* grupo = Tabla_Enfermeras;
    for( int i = 0 ; i < NENFERMERAS ; i += 1 ){
        destruirPersonal( grupo + i );
    }
}

void borrarHospitales(){
    Hospital* grupo    = Tabla_Hospitales;
    for( int id = 0 ; id < NHOSPITALES ; id += 1 ){
        destruirHospital( grupo + id );
    }
}

void borrarPacientesEnCasa(){
    // Global:
    refqueue_destroy( &pacienteEnCasa );
}

void borrarVoluntarios(){
    Voluntario* grupo = Tabla_Voluntarios;
    for( int i = 0 ; i < NVOLUNTARIOS ; i += 1 ){
       destruirVoluntario( grupo + i );
    }
}

void borrarGestorCama(){
    GestorCama *grupog = Tabla_Gestores;
    for (int i = 0; i < NHOSPITALES; i++)
    {
        destruirGestorCama(grupog + i);
    }
}

void borrarJefeUCI(){
    jefe_uci *grupoj = Tabla_JefeUCI;
    for( int i = 0; i < NHOSPITALES; i += 1 ) {
        destruirJefeUCI(grupoj + i);
    }
}


