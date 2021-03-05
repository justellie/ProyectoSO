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
#include <unistd.h>

//extern Hospital   Tabla_Hospitales[NHOSPITALES];

/**
 * @brief Funcion que ejecuta el actor gestor para reasignar pacientes a los hospitales
 * 
 * @param inv_ugc estructura que contiene los datos basicos del hospital 
 */
void* actor_paciente_ugc(void *inv_ugc)
{
    int i,      // Discriminador de tipo de hospital 0)General 1)Intermedio 2)Centinela  
    j,          // Recorre todos los hospitales    
    numDisp,    // Valor de camas disponibles en el hospital
    maxCamDisp, // Valor máximo de camas encontrados en la lista de hospitales
    hospMaxCam; // Id del hospital con más camas

    UGC *gestion_central = (UGC *) inv_ugc;
    Paciente *peticion;
    Hospital *busqHospital;
    
    while (true)
    {
        
        peticion = refqueue_get(&gestion_central->pacientes);   // Revisa si hay un paciente disponible, en caso contrario se queda esperando
        maxCamDisp = 0;                                         // Se establece el valor de referencia del máximo
        
        for ( i = General; i <= Centinela; i++)
        {
            for(j = 0; j < NHOSPITALES; j++)
            {

                busqHospital = &(Tabla_Hospitales[i]);              // Consulta el hospital

                
                // Discriminador de camas si requiere cuidado intensivo o no //
                if(busqHospital->tipo == i)
                {   
                    if(peticion->servicio == Intensivo)
                        sem_getvalue(&busqHospital->camasIntensivo,&numDisp);   // Ve cuantas camas tiene disponible
                    else
                        sem_getvalue(&busqHospital->camasBasico,&numDisp);      // Ve cuantas camas tiene disponible


                    if(! numDisp < maxCamDisp)
                    {
                        maxCamDisp = numDisp;
                        hospMaxCam = i;
                    }
                }

            }

            if(maxCamDisp) 
                break;
        }
            

        if (maxCamDisp)
        {
            busqHospital = &(Tabla_Hospitales[hospMaxCam]); 
            peticion->fueAtendido = 1;
            refqueue_put(&busqHospital->pacientes, peticion);

        }
        else
        {
            i=-1;       // Se reinicia la búsqueda
            sleep (1);  // Espera antes de reiniciar la búsqueda
        }

    }
    
    return NULL;
}
