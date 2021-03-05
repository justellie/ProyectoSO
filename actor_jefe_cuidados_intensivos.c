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

//extern UGC        gestor_central;

///@fn void* actor_jefe_cuidados_intensivos( void * datos)
///@brief Realiza la funciones basicas del jefe de cuidados intensivos
///@param datos estructura que contiene la informacion basica que requiere el jefe de cidados intensivos
///@details Este actor se encarga de asegurar el personal en el hospital, esta pendiende de numero y pide mas si hace falta
void* actor_jefe_cuidados_intensivos( void * datos)
{
    jefe_uci *datos_jefe = (jefe_uci *) datos;
    //utilizado para determinar la cantidad de personal intensivo que es utilizado en el hospital segun su tipo
    int cantidad; 
    //utilizado para abreviar la desreferenciacion 
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
        //El actor quedara bloqueado en un mutex condicional, si la condicion es cierta
        pthread_mutex_lock(&datos_jefe->espera);
                    //condicion:hay suficiente personal como para suplir todas las camas intensivas y una 8va parte de las camas basicas
            while (refrecursos->nenfermeras >= cantidad * refrecursos->ncamasInt 
                                                + 0.12 * refrecursos->ncamasBas  //ejemplo: en caso de hospital centinela y para las enfermeras 
                                            ||                                   //seria nenfermeras >= 3 * camasIntensivas + 0.12*camasBasicas
                    refrecursos->nmedicos >= cantidad * refrecursos->ncamasInt 
                                                + 0.12 * refrecursos->ncamasBas)
            {
                //mutex condicional, el hilo esperara a que la condicion sea falsa
                pthread_cond_wait(&datos_jefe->refHospital.stast, &datos_jefe->espera);
            }
            
            

            //segun lo que se necesite, se pide enfermeras o medicos
            if(refrecursos->nenfermeras <= cantidad * refrecursos->ncamasInt + 0.12 * refrecursos->ncamasBas)
            {   
                //inicializacion del pedido de personal a la UGC
                TuplaInventario *pedido = malloc(sizeof(TuplaInventario *));
                //llenado de pedido
                pedido->idHospital=datos_jefe->refHospital.id;
                pedido->cantidad=cantidad;
                pedido->tipo_recurso= PideEnfermera;
                //enviado a la cola de peticiones de la UGC
                refqueue_put(&gestor_central.peticionesPersonal, pedido);
                //en espera de que la UGC realice la transferencia de personal
                pthread_mutex_lock(&gestor_central.espera_personal);
            }
            else if(refrecursos->nmedicos >= cantidad * refrecursos->ncamasInt + 0.12 * refrecursos->ncamasBas)
            {
                //inicializacion del pedido de personal a la UGC
                TuplaInventario *pedido = malloc(sizeof(TuplaInventario *));
                //llenado de pedido
                pedido->idHospital=datos_jefe->refHospital.id;
                pedido->cantidad=cantidad;
                pedido->tipo_recurso= PideMedico;
                //enviado a la cola de peticiones de la UGC
                refqueue_put(&gestor_central.peticionesPersonal, pedido);
                //en espera de que la UGC realice la transferencia de personal
                pthread_mutex_lock(&gestor_central.espera_personal);
            }
        pthread_mutex_unlock(&datos_jefe->espera);
            
    }

    return NULL;
}
