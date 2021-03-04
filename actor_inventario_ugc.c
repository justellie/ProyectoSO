/**
 * @file actor_inventario_ugc.c
 * @author Elio Ruiz
 * @brief 
 * @version 0.1
 * @date 2021-03-03
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "definiciones.h"

/**
 * @brief Funcion que ejecuta el actor gestor para realizar sus funciones
 * 
 * @param datos_hospital estructura que contiene los datos basicos del hospital 
 */
void actor_inventario_ugc(void *datos_UGC)
{
    UGC *gestion_central = (UGC *) datos_UGC;
    
    
    while (true)
    {
        TuplaInventario *peticion = refqueue_get(&gestion_central->peticiones);
        int maxDisponible=0,indexMax=0;
        int actual=0;

        for ( int i = 0; i < NHOSPITALES; i++)
        {
            switch (peticion->tipo_recurso)
            {
            case PideTanque:
                sem_wait(&Tabla_Hospitales[i].consultaTanques);
                break;
            case PideRespirador:
                sem_wait(&Tabla_Hospitales[i].consultaOxigeno);
                break;
            default:
                break;
            }
            
        }
        while (peticion->cantidad !=0)
        {
            switch (peticion->tipo_recurso)
            {
            case PideTanque:
                actual = refqueue_unsafe_len(&Tabla_Hospitales[i].tanquesOxigeno); // verifico cual es la cantidad del recurso en ese momento
                Tanque *t;
                for (int i = 0; i < NHOSPITALES; i++)
                {
                    if (actual >= peticion->cantidad) // encontre el uno que tiene la cantidad o mas del recurso
                    {
                        maxDisponible = peticion->cantidad;
                        indexMax = i;
                    }
                    else
                    {
                        if (actual > maxDisponible)
                        {
                            maxDisponible = actual;
                            indexMax = i;
                        }
                    }
                }
                for (int j = 0; j < maxDisponible; j++)
                {
                    *t = refqueue_get(&Tabla_Hospitales[indexMax].tanquesOxigeno);
                    refqueue_put(&Tabla_Hospitales[peticion->idHospital].tanquesOxigeno, t);
                }
                peticion->cantidad -= maxDisponible;
                break;

            case PideRespirador:
                actual = refqueue_unsafe_len(&Tabla_Hospitales[i].respiradores); // verifico cual es la cantidad del recurso en ese momento
                Respirador *r;
                for (int i = 0; i < NHOSPITALES; i++)
                {
                    if (actual >= peticion->cantidad) // encontre el uno que tiene la cantidad o mas del recurso
                    {
                        maxDisponible = peticion->cantidad;
                        indexMax = i;
                    }
                    else
                    {
                        if (actual > maxDisponible)
                        {
                            maxDisponible = actual;
                            indexMax = i;
                        }
                    }
                }
                for (int j = 0; j < maxDisponible; j++)
                {
                    *r = refqueue_get(&Tabla_Hospitales[indexMax].respiradores);
                    refqueue_put(&Tabla_Hospitales[peticion->idHospital].respiradores, r);
                    
                }
                peticion->cantidad -= maxDisponible;
                break;
            case PidePCR:
                for (int i = 0; i < peticion->cantidad ; i++)
                {
                       
                    refqueue_put(&Tabla_Hospitales[peticion->idHospital].PCR, &VAR_PCR);
                    
                }
                peticion->cantidad=0;
                break;
            default:
                break;
            } 
        }
        for ( int i = 0; i < NHOSPITALES; i++)
        {
            switch (peticion->tipo_recurso)
            {
            case PideTanque:
                sem_signal(&Tabla_Hospitales[i].consultaTanques);
                break;
            case PideRespirador:
                sem_signal(&Tabla_Hospitales[i].consultaOxigeno);
                break;
            default:
                break;
            }
            
        }

    }
    
}