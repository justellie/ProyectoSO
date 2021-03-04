/**
 * @file actor_paciente_ugc.c
 * @author Juan Herrera
 * @brief 
 * @version 0.1
 * @date 2021-03-03
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "definiciones.h"
#include <unistd.h>

/**
 * @brief Funcion que ejecuta el actor gestor para reasignar pacientes a los hospitales
 * 
 * @param inv_ugc estructura que contiene los datos basicos del hospital 
 */
void actor_paciente_ugc(void *inv_ugc)
{
    int i,     
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

        for(i=0; i < NHOSPITALES; i++)
        {

            busqHospital = &(Tabla_Hospitales[i]);              // Consulta el hospital
            sem_getvalue(&busqHospital->camasBasico,&numDisp);  // Ve cuantas camas tiene disponible

            if(! numDisp < maxCamDisp)
            {
                maxCamDisp = numDisp;
                hospMaxCam = i;
            }

        }
        if (maxCamDisp)
        {
            busqHospital = &(Tabla_Hospitales[hospMaxCam]); 
            peticion->fueAtendido;
            refqueue_put(&busqHospital->pacientes, peticion);
        }else{
            i=-1;       // Se reinicia la búsqueda
            sleep (1);  // Espera antes de reiniciar la búsqueda
        }

    }
    
}