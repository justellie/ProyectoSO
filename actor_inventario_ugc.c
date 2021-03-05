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
// [@] Sincronizacion global ---------
extern Barrier    Paso_Inicializacion;

// [+] Tablas globales global -------------------
extern Paciente   Tabla_Pacientes[NPACIENTES];
extern Personal   Tabla_Medicos[NMEDICOS];
extern Personal   Tabla_Enfermeras[NENFERMERAS];
extern Hospital   Tabla_Hospitales[NHOSPITALES];
extern GestorCama Tabla_Gestores[GESTORES_H];
extern Voluntario Tabla_Voluntarios[NVOLUNTARIOS];

// [*] Voluntarios -----------
extern RefQueue pacienteEnCasa;
/**
 * @brief Funcion que ejecuta el actor gestor para realizar sus funciones
 * 
 * @param datos_hospital estructura que contiene los datos basicos del hospital 
 */
void* actor_inventario_ugc(void *datos_UGC)
{
    UGC *gestion_central = (UGC *) datos_UGC;
    MuestraPcr prueba=1;
    
    
    while (true)
    {
        TuplaInventario *peticion = refqueue_get(&gestion_central->peticiones);
        int maxDisponible=0,indexMax=0;
        int actual=0, i=0, j=0,contadorHospital=0;

        for (i = 0; i < NHOSPITALES; i++)
        {
            switch (peticion->tipo_recurso)
            {
            case PideTanque:
                refqueue_unsafe_lock(&Tabla_Hospitales[i].tanquesOxigeno);
                break;
            case PideRespirador:
                refqueue_unsafe_lock(&Tabla_Hospitales[i].respiradores);
                break;
            default:
                break;
            }
            
        }
        while ((peticion->cantidad !=0) && (contadorHospital<NHOSPITALES) )
        {
            switch (peticion->tipo_recurso)
            {
            case PideTanque:
                actual = refqueue_unsafe_len(&Tabla_Hospitales[i].tanquesOxigeno); // verifico cual es la cantidad del recurso en ese momento
                TanqueDato *t;
                for (i = 0; i < NHOSPITALES; i++)
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
                for ( j = 0; j < maxDisponible; j++)
                {
                    t = refqueue_unsafe_get(&Tabla_Hospitales[indexMax].tanquesOxigeno);
                    refqueue_unsafe_put(&Tabla_Hospitales[peticion->idHospital].tanquesOxigeno, t);
                }
                peticion->cantidad -= maxDisponible;
                Tabla_Hospitales[peticion->idHospital].estadis_recursos.ntanques+=maxDisponible;
                break;

            case PideRespirador:
                actual = refqueue_unsafe_len(&Tabla_Hospitales[i].respiradores); // verifico cual es la cantidad del recurso en ese momento
                Respirador *r;
                for (i = 0; i < NHOSPITALES; i++)
                {
                    if ((actual >= peticion->cantidad) && peticion->idHospital!=i ) // encontre el uno que tiene la cantidad o mas del recurso
                    {
                        maxDisponible = peticion->cantidad;
                        indexMax = i;
                    }
                    else
                    {
                        if ((actual > maxDisponible) && peticion->idHospital!=i)
                        {
                            maxDisponible = actual;
                            indexMax = i;
                        }
                    }
                }
                for (j = 0; j < maxDisponible; j++)
                {
                    r = refqueue_unsafe_get(&Tabla_Hospitales[indexMax].respiradores);
                    refqueue_unsafe_put(&Tabla_Hospitales[peticion->idHospital].respiradores, r);
                    
                }
                peticion->cantidad -= maxDisponible;
                Tabla_Hospitales[peticion->idHospital].estadis_recursos.nrespira+=maxDisponible;
                break;
            case PidePCR:
                for (i = 0; i < peticion->cantidad ; i++)
                {
                       
                    refqueue_put(&Tabla_Hospitales[peticion->idHospital].PCR, &prueba);
                    
                }
                peticion->cantidad=0;
                break;
            default:
                break;
            }
            contadorHospital++; 
        }
        for ( i = 0; i < NHOSPITALES; i++)
        {
            switch (peticion->tipo_recurso)
            {
            case PideTanque:
                refqueue_unsafe_unlock(&Tabla_Hospitales[i].tanquesOxigeno);
                break;
            case PideRespirador:
                refqueue_unsafe_unlock(&Tabla_Hospitales[i].respiradores);
                break;
            default:
                break;
            }
            
        }
        pthread_mutex_unlock(&gestion_central->EsperandoPorRecurso);
        free(peticion);
        

    }
    
    return NULL;
}
