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
#include "actores.h"
#include "definiciones.h"
// [@] Sincronizacion global ---------
//extern Barrier    Paso_Inicializacion;
//
//// [+] Tablas globales global -------------------
//extern Paciente   Tabla_Pacientes[NPACIENTES];
//extern Personal   Tabla_Medicos[NMEDICOS];
//extern Personal   Tabla_Enfermeras[NENFERMERAS];
//extern Hospital   Tabla_Hospitales[NHOSPITALES];
//extern GestorCama Tabla_Gestores[GESTORES_H];
//extern Voluntario Tabla_Voluntarios[NVOLUNTARIOS];
//
//// [*] Voluntarios -----------
//extern RefQueue pacienteEnCasa;
/**
 * @brief Funcion que ejecuta el actor gestor para realizar sus funciones
 * 
 * @param datos_hospital estructura que contiene los datos basicos del hospital 
 */
void* actor_personal_ugc(void *datos_UGC)
{
    UGC *gestion_central = (UGC *) datos_UGC;
    while (true)
    {
        TuplaInventario *peticion = refqueue_get(&gestion_central->peticionesPersonal);
        int maxDisponible=0,indexMax=0;
        int actual=0, i=0, j=0,contadorHospital=0;
        TipoPersonal *t;

        for (i = 0; i < NHOSPITALES; i++)
        {
            switch (peticion->tipo_recurso)
            {
            case PideEnfermera:
                // NOTE: esto es un arreglo.
                // TODO: Bloquear el nivel más alto?
                refmap_unsafe_lock(&Tabla_Hospitales[i].enfermeras[MAX_ATENCION-1]);
                 refqueue_unsafe_lock(&gestion_central->enfermeras);
                break;
            case PideMedico:
                // NOTE: esto es un arreglo.
                // TODO: Bloquear el nivel más alto?
                refmap_unsafe_lock(&Tabla_Hospitales[i].medicos[MAX_ATENCION-1]);
                break;
            default:
                break;
            }
            
        }
        while ((peticion->cantidad != 0) && (contadorHospital < NHOSPITALES))
        {
            switch (peticion->tipo_recurso)
            {
            case PideEnfermera:
                actual=refqueue_unsafe_len(&gestion_central->enfermeras);
                if (actual > 0)
                {
                    maxDisponible = actual;
                    for (j = 0; j < maxDisponible; j++)
                    {
                        t = refqueue_unsafe_get(&gestion_central->enfermeras);
                        refmap_unsafe_put(&Tabla_Hospitales[peticion->idHospital].enfermeras[4], t, t);
                    }
                }
                else{
                    for (i = 0; i < NHOSPITALES; i++)
                    {
                        actual = refmap_unsafe_size(&Tabla_Hospitales[i].enfermeras[4]); // verifico cual es la cantidad del recurso en ese momento
                        if ((actual >= peticion->cantidad) && peticion->idHospital != i) // encontre el uno que tiene la cantidad o mas del recurso
                        {
                            maxDisponible = peticion->cantidad;
                            indexMax = i;
                        }
                        else
                        {
                            if ((actual > maxDisponible) && peticion->idHospital != i)
                            {
                                maxDisponible = actual;
                                indexMax = i;
                            }
                        }
                    }
                    for (j = 0; j < maxDisponible; j++)
                    {
                        t = refmap_unsafe_minkey(&Tabla_Hospitales[indexMax].enfermeras[4]);
                        refmap_unsafe_put(&Tabla_Hospitales[peticion->idHospital].enfermeras[4], t, t);
                    }
                    contadorHospital++;
                }

                peticion->cantidad -= maxDisponible;
                break;

            case PideMedico:
                actual = refqueue_unsafe_len(&gestion_central->medicos);
                if (actual > 0)
                {
                    maxDisponible = actual;
                    for (j = 0; j < maxDisponible; j++)
                    {
                        t = refqueue_unsafe_get(&gestion_central->medicos);
                        refmap_unsafe_put(&Tabla_Hospitales[peticion->idHospital].medicos[4], t, t);
                    }
                }
                else
                {
                    for (i = 0; i < NHOSPITALES; i++)
                    {
                        actual = refmap_unsafe_size(&Tabla_Hospitales[i].medicos[4]);    // verifico cual es la cantidad del recurso en ese momento
                        if ((actual >= peticion->cantidad) && peticion->idHospital != i) // encontre el uno que tiene la cantidad o mas del recurso
                        {
                            maxDisponible = peticion->cantidad;
                            indexMax = i;
                        }
                        else
                        {
                            if ((actual > maxDisponible) && peticion->idHospital != i)
                            {
                                maxDisponible = actual;
                                indexMax = i;
                            }
                        }
                    }
                    for (j = 0; j < maxDisponible; j++)
                    {
                        t = refmap_unsafe_minkey(&Tabla_Hospitales[indexMax].medicos[4]);
                        refmap_unsafe_put(&Tabla_Hospitales[peticion->idHospital].medicos[4], t, t);
                    }
                    contadorHospital++;
                }
                peticion->cantidad -= maxDisponible;
                break;
            default:
                break;
            }
           
        }
        for ( i = 0; i < NHOSPITALES; i++)
        {
            switch (peticion->tipo_recurso)
            {
            case PideTanque:
                refmap_unsafe_unlock(&Tabla_Hospitales[i].enfermeras[MAX_ATENCION-1]);
                break;
            case PideRespirador:
                refmap_unsafe_unlock(&Tabla_Hospitales[i].medicos[MAX_ATENCION-1]);
                break;
            default:
                break;
            }
            
        }
        pthread_mutex_unlock(&gestion_central->espera_personal);
        free(peticion);

    }
    
    return NULL;
}
