#include "definiciones.h"
#include "Tipos/RefMap.h"
#include "Tipos/RefQueue.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// Define todos los constructores y destructores dentro de definiciones.h

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
    p->donde.id_lugar = -1;
    p->donde.lugar    = EnCasa;
    p->servicio       = Ninguno;

    // TODO: finalizar parte de sintomas y diagnostico
    p->sintomas = "nada";
    // Ningún médico:
    for( int i = 0 ; i < MAX_ATENCION ; i += 1 ){
        medID[i] = -1;
        enfID[i] = -1;
    }
    // Ningún atributo por ahora.
    pthread_rwlock_init( p->medLock   , NULL );
    pthread_rwlock_init( p->enfLock   , NULL );
    pthread_rwlock_init( p->dondeLock , NULL );

    // Permite pausar al paciente hasta que sea necesario.
    pthread_rwlock_init( p->atendidoLock , NULL );
    pthread_cond_init  ( p->atendido     , NULL );
}

void destruirPaciente ( Paciente* p ){
    p->id    = -1;
    p->vivo  = 0;
    p->donde.id_lugar = -1;
    p->donde.lugar    = EnCasa;
    p->servicio       = Ninguno;

    // Ningún médico:
    for( int i = 0 ; i < MAX_ATENCION ; i += 1 ){
        medID[i] = -1;
        enfID[i] = -1;
    }

    pthread_rwlock_destroy( p->medLock   );
    pthread_rwlock_destroy( p->enfLock   );
    pthread_rwlock_destroy( p->dondeLock );

    pthread_mutex_destroy( p->atendidoLock );
    pthread_cond_destroy ( p->atendido );
}

void construirHospital( Hospital* h , int id , TuplaRecursos* descripcion ){
    const int CompartidoEntreHilos = 0;


    h->id = id;

    // TODO: Decidir qué tipo de claves deben usarse en el diccionario:
    //              (obj , compare , copy-key , free-key)
    refmap_init( &h->medicos    , ... , ... , ... );
    refmap_init( &h->enfermeras , ... , ... , ... );

    sem_init( &h->salaEspera  , CompartidoEntreHilos , NSALA_ESPERA  );
    sem_init( &h->salaMuestra , CompartidoEntreHilos , NSALA_MUESTRA );


    sem_init( &h->camasBasico    , CompartidoEntreHilos , descripcion->ncamasBas );
    sem_init( &h->camasIntensivo , CompartidoEntreHilos , descripcion->ncamasInt );
    sem_init( &h->tanquesOxigeno , CompartidoEntreHilos , descripcion->ntanques  );
    sem_init( &h->respiradores   , CompartidoEntreHilos , descripcion->nrespira  );

    // No se ha usado nada:
    h->estadisticas.ncamasBas = 0;
    h->estadisticas.ncamasint = 0;
    h->estadisticas.ntanques  = 0;
    h->estadisticas.nrespira  = 0;
    pthread_rwlock_init( h->estadisticasLock , NULL );
}

void destruirHospital ( Hospital* h ){
    refmap_destroy( &h->medicos    );
    refmap_destroy( &h->enfermeras );

    sem_destroy( &h->salaEspera  );
    sem_destroy( &h->salaMuestra );

    sem_destroy( &h->camasBasico    );
    sem_destroy( &h->camasIntensivo );
    sem_destroy( &h->tanquesOxigeno );
    sem_destroy( &h->respiradores   );

    // No se ha usado nada:
    h->estadisticas.ncamasBas = 0;
    h->estadisticas.ncamasint = 0;
    h->estadisticas.ntanques  = 0;
    h->estadisticas.nrespira  = 0;
    pthread_rwlock_destroy( h->estadisticasLock );

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

    ugc->estadisticas.ncamasBas = 0;
    ugc->estadisticas.ncamasint = 0;
    ugc->estadisticas.ntanques  = 0;
    ugc->estadisticas.nrespira  = 0;

    pthread_rwlock_init( ugc->estadisticasLock , NULL );

    ugc->turno = 0;
    pthread_rwlock_init( ugc->turnoLock , NULL );
}
void destruirUGC( UGC* ugc ){
    refmap_queue_destroy( &ugc->medicos     );
    refmap_queue_destroy( &ugc->enfermeras  );
    refmap_queue_destroy( &ugc->pacientes   );
    refmap_queue_destroy( &ugc->voluntarios );

    sem_destroy( &ugc->camasBasico    );
    sem_destroy( &ugc->camasIntensivo );
    sem_destroy( &ugc->tanquesOxigeno );
    sem_destroy( &ugc->respiradores   );

    ugc->estadisticas.ncamasBas = 0;
    ugc->estadisticas.ncamasint = 0;
    ugc->estadisticas.ntanques  = 0;
    ugc->estadisticas.nrespira  = 0;
    pthread_rwlock_destroy( ugc->estadisticasLock );
    ugc->turno 0;
    pthread_rwlock_destroy( ugc->turnoLock );
}

void construirGestorCama( GestorCama* g , int id , Hospital* hospital ){
    // TODO: Decidir si hay que ignorar a free luego de extraer un elmento de la cola
    //                      obj , free , str
    g->id       = id;
    g->hospital = hospital;
    refqueue_singleton( &g->pacientes );
}

void destruirGestorCama ( GestorCama* g ){
    g->id       = -1;
    g->hospital = NULL;
    refmap_queue_destroy( &g->pacientes );
}

void construirVoluntario( Voluntario* v , int id ){
    v->id       = id;
    refqueue_singleton( &v->pacientes );
}

void destruirVoluntario( Voluntario* v ){
    g->id       = -1;
    refmap_queue_destroy( &v->pacientes );
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
