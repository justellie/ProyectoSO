/**
 * @file actor_analista.c
 * @author Elio Ruiz
 * @brief 
 * @version 0.1
 * @date 2021-02-28
 * 
 * @copyright Copyright (c) 2021
 * 
 */
#include "actores.h"
#include "definiciones.h"
#include <stdbool.h>
/**
 * @brief Funcion que ejecuta el actor gestor para realizar sus funciones
 * 
 * @param datos_hospital estructura que contiene los datos basicos del hospital 
 */
void* actor_analista(void *datos_hospital)
{
    Hospital *hospital = (Hospital *) datos_hospital;
    int* diag = malloc( sizeof(int) );
    while (true)
    {   
        Paciente *atendiendo = refqueue_get(&hospital->pacientesEnSilla);
        refqueue_get(&hospital->PCR);
        atendiendo->servicio=obtener_diagnostico_simple();
        sem_signal(&atendiendo->muestraTomada);
        *diag = atendiendo->servicio;
        refqueue_put(&hospital->pacientes, atendiendo);
        refqueue_put(&hospital->reporte, diag);
    }
    
    return NULL;
}
