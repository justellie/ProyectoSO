/**
 * @file actor_paciente_ugc.c
 * @author Juan Herrera 26.972.881
 * @brief 
 * @version 0.1
 * @date 2021-03-03
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "actores.h"
#include "definiciones.h"
//#include "main.c"

//extern UGC gestor_central;

void* actor_jefe_admin ( void* hospital ){
    int i;

    Hospital *datosHospital = (Hospital *) hospital;
    TuplaInventario * pedir;
    
    while (true)
    {

        if(refqueue_unsafe_len(&datosHospital->PCR) < NMIN_PCR )
        {
                pedir = malloc (sizeof(TuplaInventario));
                
                pedir->idHospital=datosHospital->id;
                pedir->cantidad=1;
                pedir->tipo_recurso=PidePCR;
                
                refqueue_put(&gestor_central.peticiones, pedir);
                pthread_mutex_lock(&gestor_central.EsperandoPorRecurso);

            for (i=0; i < NLOTE_PCR; i++){

                refqueue_put(&datosHospital->PCR,NULL);
            }

        }
    }
}
