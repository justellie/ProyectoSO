/**
 * @file actor_jefe_cuidados_intensivos.c
 * @author Luis Alfonzo 26581268 (elalfonzo2.1@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2021-03-03
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "definiciones.h"
#include "actores.h"

extern UGC        gestor_central;

void* actor_jefe_cuidados_intensivos( void * datos)
{
    jefe_uci *datos_jefe = (jefe_uci *) datos;
    int cantidad;
    TuplaRecursos *refrecursos = &datos_jefe->refHospital.estadis_recursos;
    switch (datos_jefe->refHospital.tipo)
    {
    case Centinela:
        cantidad = 3;
        break;
    
    case Intermedio:
        cantidad=2;
        break;
    
    case General:
        cantidad=1;
        break;
    }

    while (true)
    {
        pthread_mutex_lock(&datos_jefe->espera);
            while (refrecursos->nenfermeras >= cantidad * refrecursos->ncamasInt 
                                                + 16 / refrecursos->ncamasBas 
                                            || 
                    refrecursos->nmedicos <= cantidad * refrecursos->ncamasInt 
                                                + 16 / refrecursos->ncamasBas)
            {
                pthread_cond_wait(&datos_jefe->refHospital.stast, &datos_jefe->espera);
            }
            TuplaInventario *pedido = malloc(sizeof(TuplaInventario *));
            pedido->idHospital=datos_jefe->refHospital.id;
            pedido->cantidad=cantidad;

            if(refrecursos->nenfermeras <= cantidad * refrecursos->ncamasInt + 16 / refrecursos->ncamasBas)
            {
                pedido->tipo_recurso= PideEnfermera;
            }
            else
            {
                pedido->tipo_recurso= PideMedico;
            }
            refqueue_put(&gestor_central.peticionesPersonal, pedido);
            pthread_mutex_lock(&gestor_central.espera_personal);
        pthread_mutex_unlock(&datos_jefe->espera);
            
    }

    return NULL;
}
