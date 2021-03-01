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
void actor_analista(void *datos_hospital)
{
    Hospital *hospital = (Hospital *) datos_hospital;
    while (true)
    {
        Paciente *atendiendo = refqueue_get(&hospital->pacientesEnSilla);
        atendiendo->sintomas=obtener_diagnostico();
        refqueue_put(&hospital->pacientesListoParaAtender, atendiendo);
        //TODO avisar del diagnostico
    }
    
}