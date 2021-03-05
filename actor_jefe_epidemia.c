/**
 * @file actor_jefe_epidemia.c
 * @author Jesús Pelay
 * @brief 
 * @version 0.1
 * @date 2021-03-28
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "actores.h"
#include "definiciones.h"
#include <stdbool.h>
/** 
 * @brief Gestiona las estadísticas de los casos de covid para los hospitales
 *        y también a la gestión nacional  
 * 
 * @param datos_hospital datos del hospital cuyas estadísticas estamos llevando
 * 
 */
    
void* actor_jefe_epidemia(void* datos_hospital) {
    Hospital *hospital = (Hospital *)datos_hospital;
    long diagnostico;
    while(true) {
        diagnostico = (long) refqueue_get(&hospital->reporte);
        if(diagnostico == Intensivo) {
            hospital->estadis_pacientes.covid += 1;
        }
    }
}
