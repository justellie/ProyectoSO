#include "definiciones.h"
#include "Tipos/RefMap.h"
#include "Tipos/RefQueue.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define COMPARE(x,y) (                          \
                        ((x) < (y))?            \
                            -1 :                \
                            (                   \
                                ((x) > (y))?    \
                                        1 : 0   \
                            )                   \
                     )
// Define todos los constructores y destructores dentro de definiciones.h
static const int CompartidoEntreHilos = 0;

static void* ignorar_copia( void* personal );
static int  comparar_por_id( void* personal_a , void* personal_b );
static void ignorar_borrardo( void* personal );

void construirPersonal( Personal* p , int id , TipoPersonal profesion , TipoAtencion servicio ){
    p->id   = id;
    p->tipo = profesion;
    p->servicio = servicio;

    // TODO: Evaluar si se puede eliminar todo este bloque, ¿por qué? Los médicos/enfermeras son recursos del hospital,
    //       y los pacientes contienen el id del médico y enfermera.
    p->idHospital = -1; // ES NECESARIO??
    for( int i = 0 ; i < MAX_ATENCION ; i += 1 ){
        p->pacientes[i] = -1;
    }
    p->cuantos = 0;
}

void destruirPersonal ( Personal* p ){
    p->id       = -1;
    p->servicio = Ninguno;

    // TODO: Evaluar si se puede eliminar todo este bloque, ¿por qué? Los médicos/enfermeras son recursos del hospital,
    //       y los pacientes contienen el id del médico y enfermera.
    p->idHospital = -1; // ES NECESARIO??
    for( int i = 0 ; i < MAX_ATENCION ; i += 1 ){
        p->pacientes[i] = -1;
    }
    p->cuantos = 0;
}

void construirPaciente( Paciente* p , int id ){
    p->id    = id;
    p->vivo  = 1;
    //p->donde.id_lugar = -1;
    //p->donde.lugar    = EnCasa;
    p->servicio       = Ninguno;

    // TODO: finalizar parte de sintomas y diagnostico
    p->sintomas = "nada";
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
    //p->donde.id_lugar = -1;
    //p->donde.lugar    = EnCasa;
    p->servicio       = Ninguno;

    // Ningún médico:
    for( int i = 0 ; i < MAX_ATENCION ; i += 1 ){
        p->medID[i] = -1;
        p->enfID[i] = -1;
    }

    pthread_mutex_destroy( &p->medLock   );
    pthread_mutex_destroy( &p->enfLock   );
    //pthread_mutex_destroy( &p->dondeLock );

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

void construirHospital( Hospital* h , int id , TuplaRecursos* descripcion ){
    h->id = id;

    // TODO: Decidir qué tipo de claves deben usarse en el diccionario:
    //              (obj , compare , copy-key , free-key)
    for( int i = 0 ; i < MAX_ATENCION ; i += 1 ){
        refmap_init( &h->medicos[i]    , comparar_por_id , ignorar_copia , ignorar_borrardo );
        refmap_init( &h->enfermeras[i] , comparar_por_id , ignorar_copia , ignorar_borrardo );
    }

    sem_init( &h->salaEspera  , CompartidoEntreHilos , NSALA_ESPERA  );
    sem_init( &h->salaMuestra , CompartidoEntreHilos , NSALA_MUESTRA );


    sem_init( &h->camasBasico    , CompartidoEntreHilos , descripcion->ncamasBas );
    sem_init( &h->camasIntensivo , CompartidoEntreHilos , descripcion->ncamasInt );
    refqueue_singleton( &h->tanquesOxigeno );
    refqueue_singleton( &h->respiradores   );

    // No se ha usado nada:
    h->estadis_recursos.ncamasBas = 0;
    h->estadis_recursos.ncamasInt = 0;
    h->estadis_recursos.ntanques  = 0;
    h->estadis_recursos.nrespira  = 0;
    pthread_mutex_init( &h->estadisticasLock , NULL );
}

void destruirHospital ( Hospital* h ){
    for( int i = 0 ; i < MAX_ATENCION ; i += 1 ){
        refmap_destroy( &h->medicos[i]    );
        refmap_destroy( &h->enfermeras[i] );
    }

    sem_destroy( &h->salaEspera  );
    sem_destroy( &h->salaMuestra );

    sem_destroy( &h->camasBasico    );
    sem_destroy( &h->camasIntensivo );
    refqueue_destroy( &h->tanquesOxigeno );
    refqueue_destroy( &h->respiradores   );

    // No se ha usado nada:
    h->estadis_recursos.ncamasBas = 0;
    h->estadis_recursos.ncamasInt = 0;
    h->estadis_recursos.ntanques  = 0;
    h->estadis_recursos.nrespira  = 0;
    pthread_mutex_destroy( &h->estadisticasLock );

}

void construirUGC( UGC* ugc , int id , TuplaRecursos* descripcion ){
    // NOTE: Por ahora no es necesario saber cómo son los elementos de la cola.
    refqueue_singleton( &ugc->medicos     );
    refqueue_singleton( &ugc->enfermeras  );
    refqueue_singleton( &ugc->pacientes   );
    refqueue_singleton( &ugc->voluntarios );

    sem_init( &ugc->camasBasico    , CompartidoEntreHilos , descripcion->ncamasBas );
    sem_init( &ugc->camasIntensivo , CompartidoEntreHilos , descripcion->ncamasInt );
    sem_init( &ugc->tanquesOxigeno , CompartidoEntreHilos , descripcion->ntanques  );
    sem_init( &ugc->respiradores   , CompartidoEntreHilos , descripcion->nrespira  );

    for( int i = 0 ; i < NACTUALIZACIONES ; i += 2 ){
        ugc->estadisticas[i].ncamasBas = 0;
        ugc->estadisticas[i].ncamasInt = 0;
        ugc->estadisticas[i].ntanques  = 0;
        ugc->estadisticas[i].nrespira  = 0;
    }

    pthread_mutex_init( &ugc->estadisticasLock , NULL );

    ugc->turno = 0;
    pthread_mutex_init( &ugc->turnoLock , NULL );
}
void destruirUGC( UGC* ugc ){
    refqueue_destroy( &ugc->medicos     );
    refqueue_destroy( &ugc->enfermeras  );
    refqueue_destroy( &ugc->pacientes   );
    refqueue_destroy( &ugc->voluntarios );

    sem_destroy( &ugc->camasBasico    );
    sem_destroy( &ugc->camasIntensivo );
    sem_destroy( &ugc->tanquesOxigeno );
    sem_destroy( &ugc->respiradores   );

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
    // TODO: Decidir si hay que ignorar a free luego de extraer un elmento de la cola
    //                      obj , free , str
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
