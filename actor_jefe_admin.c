#include "definiciones.h"

void actor_jefe_admin ( void* hospital ){
    int i;

    Hospital *datosHospital = (Hospital *) hospital;
    TuplaInventario * pedir;
    pedir = malloc (sizeof(TuplaInventario));
                
    pedir->idHospital=datosHospital->id;
    pedir->cantidad=1;
    pedir->tipo_recurso=PidePCR;
    
    while (true)
    {

        if(refqueue_unsafe_len(&datosHospital->PCR) < NMIN_PCR )
        {
                refqueue_put(&gestor_central.peticiones, pedir);
                sem_wait(&datosHospital->EsperandoPorRecurso);

            for (i=0; i < NLOTE_PCR; i++){

                refqueue_put(&datosHospital->PCR,NULL);
            }

        }
    }
}