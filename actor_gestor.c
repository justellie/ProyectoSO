/**
 * @file actor_gestor.c
 * @author Luis Alfonzo 26581268 (elalfonzo2.1@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2021-02-28
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "actores.h"
#include "definiciones.h"
///@fn diagnostico obtenerDiagnostico(Paciente *atendiendo);
///@brief determina cual sera la atencion que se le de al paciente, segun sus sintomas
///@param atendiendo referencia a la informacion del paciente
diagnostico obtenerDiagnostico(Paciente *atendiendo);

///@fn int liberarRecursos(int, Paciente *, float, TipoAtencion);
///@brief libera los recursos que el paciente tenga reservados anteriormente, segun su diagnostico
///@param idhospital necesario para saber de donde se reponen los recursos
///@param atendiendo referencia al paciente que esta siendo atendido en el momento
///@param cantidad le dice a la funcion que cantidad de personal debe ser liberada, esto segun el tipo de hospital
///@param diagPrevio diagnostico que se le dio al paciente en el pasado, este permitira determinar que recursos deben liberarse
int liberarRecursos(int, Paciente *, float, TipoAtencion);

///@fn int reservarRecursos(int, Paciente *, float, TipoAtencion);
///@brief reserva los recursos que el paciente necesite, segun el diagnostico
///@param idhospital necesario para saber de donde se toman los recursos
///@param atendiendo referencia al paciente que esta siendo atendido en el momento
///@param cantidad le dice a la funcion que cantidad de personal debe ser reservada, esto segun el tipo de hospital
///@param diagActual diagnostico que se le dio al paciente, este permitira determinar que recursos deben reservarse
int reservarRecursos(int, Paciente *, float, TipoAtencion);

///@fn void actor_gestor(void *datos_gestor)
///@brief funcion que ejecuta el actor gestor para realizar sus funciones
///@param datos_gestor estructura que contiene los datos basicos de un gestor de camas

void actor_gestor(void *datos_gestor)
{
    GestorCama *datos = (GestorCama *) datos_gestor;
    TipoHospital hosp_type = H[datos->idHospital].tipo;
    while(TRUE)
    {
        Paciente *atendiendo = malloc(sizeof(Paciente));
        atendiendo = refqueue_get(&datos->pacientes);
        diagnostico diag = obtenerDiagnostico(atendiendo);
        if(atendiendo->tiene_cama && diag.diag_previo==diag.diag_actual)
        {
                refqueue_put(&datos->pacientes, atendiendo);
        }
        else
        {
            switch (diag.diag_actual)
            {
                case Intensivo:
                    switch (hosp_type)
                    {
                        case centinela:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->idHospital, atendiendo, 3, diag.diag_previo);
                                reservarRecursos(datos->idHospital, atendiendo, 3, Intensivo);
                            }
                            else
                            {
                                if(diag.diag_previo==EnCasa)
                                {
                                    liberarRecursos(datos->idHospital, atendiendo, 1, diag.diag_previo);
                                }
                                reservarRecursos(datos->idHospital, atendiendo, 3, Intensivo);
                            }
                            
                        break;

                        case intermedio:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->idHospital, atendiendo, 2, diag.diag_previo);
                                reservarRecursos(datos->idHospital, atendiendo, 2, Intensivo);
                            }
                            else
                            {
                                if(diag.diag_previo==EnCasa)
                                {
                                    liberarRecursos(datos->idHospital, atendiendo, 1, diag.diag_previo);
                                }
                                reservarRecursos(datos->idHospital, atendiendo, 2, Intensivo);
                            }
                        break;
                        
                        case general:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->idHospital, atendiendo, 1, diag.diag_previo);
                                reservarRecursos(datos->idHospital, atendiendo, 1, Intensivo);
                            }
                            else
                            {
                                if(diag.diag_previo==EnCasa)
                                {
                                    liberarRecursos(datos->idHospital, atendiendo, 1, diag.diag_previo);
                                }
                                reservarRecursos(datos->idHospital, atendiendo, 1, Intensivo);
                            }
                        break;
                    }
                    
                break;

                case Basica:
                    switch (hosp_type)
                    {
                        case centinela:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->idHospital, atendiendo, 3, diag.diag_previo);
                                reservarRecursos(datos->idHospital, atendiendo, 0.25, Basica);
                            }
                            else
                            {
                                if(diag.diag_previo==EnCasa)
                                {
                                    liberarRecursos(datos->idHospital, atendiendo, 1, diag.diag_previo);
                                }
                                reservarRecursos(datos->idHospital, atendiendo, 0.25, Basica);
                            }
                        break;

                        case intermedio:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->idHospital, atendiendo, 2, diag.diag_previo);
                                reservarRecursos(datos->idHospital, atendiendo, 0.25, Basica);
                            }
                            else
                            {
                                if(diag.diag_previo==EnCasa)
                                {
                                    liberarRecursos(datos->idHospital, atendiendo, 1, diag.diag_previo);
                                }
                                reservarRecursos(datos->idHospital, atendiendo, 0.25, Basica);
                            }
                        break;
                        
                        case general:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->idHospital, atendiendo, 1, diag.diag_previo);
                                reservarRecursos(datos->idHospital, atendiendo, 0.25, Basica);
                            }
                            else
                            {
                                if(diag.diag_previo==EnCasa)
                                {
                                    liberarRecursos(datos->idHospital, atendiendo, 1, diag.diag_previo);
                                }
                                reservarRecursos(datos->idHospital, atendiendo, 0.25, Basica);
                            }
                        break;
                    }
                break;

                case EnCasa:
                    switch (hosp_type)
                    {
                        case centinela:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->idHospital, atendiendo, 3, diag.diag_previo);
                                reservarRecursos(datos->idHospital, atendiendo, 1, EnCasa);
                            }
                            else
                            {
                                reservarRecursos(datos->idHospital, atendiendo, 1, EnCasa);
                            }
                        break;

                        case intermedio:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->idHospital, atendiendo, 2, diag.diag_previo);
                                reservarRecursos(datos->idHospital, atendiendo, 1, EnCasa);
                            }
                            else
                            {
                                reservarRecursos(datos->idHospital, atendiendo, 1, EnCasa);
                            }
                        break;
                        
                        case general:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->idHospital, atendiendo, 1, diag.diag_previo);
                                reservarRecursos(datos->idHospital, atendiendo, 1, EnCasa);
                            }
                            else
                            {
                                reservarRecursos(datos->idHospital, atendiendo, 1, EnCasa);
                            }
                        break;
                    }
                break;

                case Muerto:
                    switch (hosp_type)
                    {
                        case centinela:
                            liberarRecursos(datos->idHospital, atendiendo, 3, diag.diag_previo);
                            //finalizar el hilo
                        break;

                        case intermedio:
                            liberarRecursos(datos->idHospital, atendiendo, 2, diag.diag_previo);
                            //finalizar el hilo
                        break;
                        
                        case general:
                            liberarRecursos(datos->idHospital, atendiendo, 1, diag.diag_previo);
                            //finalizar el hilo
                        break;
                    }
                break;
                
                default://dar de alta
                    switch (hosp_type)
                    {
                        case centinela:
                            liberarRecursos(datos->idHospital, atendiendo, 3, diag.diag_previo);
                        break;

                        case intermedio:
                            liberarRecursos(datos->idHospital, atendiendo, 2, diag.diag_previo);
                        break;
                        
                        case general:
                            liberarRecursos(datos->idHospital, atendiendo, 1, diag.diag_previo);
                        break;
                    }
                break;
            }
        free(atendiendo);
        }
    }
}