#include "definiciones.h"
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

    // TODO: Evaluar si se puede eliminar todo este bloque, ¿por qué? Los médicos/enfermeras son recursos del hospital,
    //       y los pacientes contienen el id del médico y enfermera.
    p->idHospital = -1; // ES NECESARIO??
    for( int i = 0 ; i < MAX_ATENCION ; i += 1 ){
        p->pacientes[i] = -1;
    }
    p->cuantos = 0;
}


