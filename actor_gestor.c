/**
 * @file actor_gestor.c
 * @author Luis Alfonzo 26581268 (elalfonzo2.1@gmail.com)
 * @brief acciones que deben realizar los gestores de camas
 * @version 0.1
 * @date 2021-02-28
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "actores.h"
#include "definiciones.h"
///@fn TipoAtencion obtenerDiagnostico(Paciente *atendiendo);
///@brief determina cual sera la atencion que se le de al paciente, segun sus sintomas
///@param atendiendo referencia a la informacion del paciente
TipoAtencion obtenerDiagnostico(Paciente *); //esto es para que no me muestre el error

///@fn int liberarRecursos(int, Paciente *, float, TipoAtencion);
///@brief libera los recursos que el paciente tenga reservados anteriormente, segun su diagnostico
///@param refHospital necesario para saber de donde se reponen los recursos
///@param atendiendo referencia al paciente que esta siendo atendido en el momento
///@param cantidad le dice a la funcion que cantidad de personal debe ser liberada, esto segun el tipo de hospital
///@param diagPrevio diagnostico que se le dio al paciente en el pasado, este permitira determinar que recursos deben liberarse
int liberarRecursos(Hospital *refHospital, Paciente *atendiendo, int cantidad, TipoAtencion diag)
{
    //se esta suponiendo que refHospital-><arreglo de mapas personal>[0] es el nivel mas bajo y por tanto en el que esta completamente ocupado el personal
    // por lo tanto refHospital-><arreglo de mapas personal>[4] es el nivel mas alto, en el que se encuentran totalmente disponibles
    bool exito = false;
    switch (diag)
    {
        case Intensivo:
            for (int i = 0; i < cantidad; i++)
            {
                //liberacion enfermeras
                Personal *enf = refmap_extract(&refHospital->enfermeras[0], atendiendo->enfID[i]);
                atendiendo->enfID[i]=-1;
                refmap_put(&refHospital->enfermeras[4], enf->id, enf);
                //liberacion medicos
                Personal *med = refmap_extract(&refHospital->medicos[0], atendiendo->medID[i]);
                atendiendo->medID[i]=-1;
                refmap_put(&refHospital->medicos[4], med->id, med);
            }
            if(atendiendo->enfID[cantidad]==-1 && atendiendo->medID[cantidad]==-1)
                exito=true;
            
        break;
        
        case Basica:
            for (int i = 0; i < MAX_ATENCION; i++)
            {
                //liberacion enfermeras
                Personal *enf = refmap_extract(&refHospital->enfermeras[i], atendiendo->enfID[0]);
                if (enf!=NULL)
                {
                    atendiendo->enfID[0]=-1;
                    refmap_put(&refHospital->enfermeras[i+1], enf->id, enf);
                }
                //liberacion medicos
                Personal *med = refmap_extract(&refHospital->medicos[i], atendiendo->medID[0]);
                if (med!=NULL)
                {
                    atendiendo->medID[0]=-1;
                    refmap_put(&refHospital->medicos[i+1], med->id, med);
                }
            }
            if(atendiendo->enfID[cantidad]==-1 && atendiendo->medID[cantidad]==-1)
                exito=true;
            
        break;

        case EnCasa:
            for (int i = 0; i < NVOLUNTARIOS; i++)
            {
                //aun no se como se reservan y se liberan voluntarios
            }
            
        break;
    }

    return exito;
}

///@fn int reservarRecursos(int, Paciente *, float, TipoAtencion);
///@brief reserva los recursos que el paciente necesite, segun el diagnostico
///@param refHospital necesario para saber de donde se toman los recursos
///@param atendiendo referencia al paciente que esta siendo atendido en el momento
///@param cantidad le dice a la funcion que cantidad de personal debe ser reservada, esto segun el tipo de hospital
///@param diagActual diagnostico que se le dio al paciente, este permitira determinar que recursos deben reservarse
int reservarRecursos(Hospital *refHospital, Paciente *atendiendo, int cantidad, TipoAtencion diagActual)
{
    //se esta suponiendo que refHospital-><arreglo de mapas personal>[0] es el nivel mas bajo y por tanto en el que esta completamente ocupado el personal
    //por lo tanto refHospital-><arreglo de mapas personal>[4] es el nivel mas alto, en el que se encuentran totalmente disponibles
    bool exito = false;
    switch (diagActual)
    {
        case Intensivo:
            for (int i = 0; i < cantidad; i++)
            {
                //reservacion de enfermeras
                Personal *enf = refmap_extract_max(&refHospital->enfermeras[4]);
                atendiendo->enfID[i]=enf->id;
                refmap_put(&refHospital->enfermeras[0], enf->id, enf);
                //reservacion de medicos
                Personal *med = refmap_extract_max(&refHospital->medicos[4]);
                atendiendo->medID[i]=med->id;
                refmap_put(&refHospital->medicos[0], med->id, med);

            }
            if(atendiendo->enfID[cantidad]!=-1 && atendiendo->medID[cantidad]!=-1)
                exito=true;
            
        break;
        
        case Basica:
            for (int i = 1; i > MAX_ATENCION; i++)
            {
                //reserva de enfermeras
                Personal *enf = refmap_extract_max(&refHospital->enfermeras[i]);
                if (enf!=NULL)
                {
                    atendiendo->enfID[0]=enf->id;
                    refmap_put(&refHospital->enfermeras[i-1], enf->id, enf);
                }
                //resserva de medicos
                Personal *med = refmap_extract_max(&refHospital->medicos[i]);
                if (med!=NULL)
                {
                    atendiendo->medID[0]=med->id;
                    refmap_put(&refHospital->medicos[i-1], med->id, med);
                }
                
            }
            
        break;

        case EnCasa:
            //reservasion de voluntarios
        break;
    }
    return exito;
}

///@fn void actor_gestor(void *datos_gestor)
///@brief funcion que ejecuta el actor gestor para realizar sus funciones
///@param datos_gestor estructura que contiene los datos basicos de un gestor de camas
void actor_gestor(void *datos_gestor)
{
    GestorCama *datos = (GestorCama *) datos_gestor;
    TipoHospital hosp_type = datos->hospital->tipo;
    while(true)
    {
        Paciente *atendiendo = refqueue_get(&datos->hospital->pacientes);
        TipoAtencion diagPrev = atendiendo->servicio;
        TipoAtencion diagAct = obtenerDiagnostico(atendiendo);

        if(atendiendo->tiene_cama && diagPrev==diagAct)
        {
                refqueue_put(&datos->hospital->pacientes, atendiendo);
        }
        else
        {
            switch (diagAct)
            {
                case Intensivo:
                    switch (hosp_type)
                    {
                        case Centinela:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 3, diagPrev);
                                reservarRecursos(datos->hospital, atendiendo, 3, Intensivo);
                            }
                            else
                            {
                                if(diagPrev==EnCasa)
                                {
                                    liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);
                                }
                                reservarRecursos(datos->hospital, atendiendo, 3, Intensivo);
                            }
                            refqueue_put(&datos->hospital->pacientes, atendiendo);
                            
                        break;

                        case Intermedio:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 2, diagPrev);
                                reservarRecursos(datos->hospital, atendiendo, 2, Intensivo);
                            }
                            else
                            {
                                if(diagPrev==EnCasa)
                                {
                                    liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);
                                }
                                reservarRecursos(datos->hospital, atendiendo, 2, Intensivo);
                            }
                            refqueue_put(&datos->hospital->pacientes, atendiendo);
                        break;
                        
                        case General:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);
                                reservarRecursos(datos->hospital, atendiendo, 1, Intensivo);
                            }
                            else
                            {
                                if(diagPrev==EnCasa)
                                {
                                    liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);
                                }
                                reservarRecursos(datos->hospital, atendiendo, 1, Intensivo);
                            }
                            refqueue_put(&datos->hospital->pacientes, atendiendo);
                        break;
                    }
                    
                break;

                case Basica:
                    switch (hosp_type)
                    {
                        case Centinela:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 3, diagPrev);
                                reservarRecursos(datos->hospital, atendiendo, 0, Basica);
                            }
                            else
                            {
                                if(diagPrev==EnCasa)
                                {
                                    liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);
                                }
                                reservarRecursos(datos->hospital, atendiendo, 0, Basica);
                            }
                            refqueue_put(&datos->hospital->pacientes, atendiendo);
                        break;

                        case Intermedio:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 2, diagPrev);
                                reservarRecursos(datos->hospital, atendiendo, 0, Basica);
                            }
                            else
                            {
                                if(diagPrev==EnCasa)
                                {
                                    liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);
                                }
                                reservarRecursos(datos->hospital, atendiendo, 0, Basica);
                            }
                            refqueue_put(&datos->hospital->pacientes, atendiendo);
                        break;
                        
                        case General:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);
                                reservarRecursos(datos->hospital, atendiendo, 0, Basica);
                            }
                            else
                            {
                                if(diagPrev==EnCasa)
                                {
                                    liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);
                                }
                                reservarRecursos(datos->hospital, atendiendo, 0, Basica);
                            }
                            refqueue_put(&datos->hospital->pacientes, atendiendo);
                        break;
                    }
                break;

                case EnCasa:
                    switch (hosp_type)
                    {
                        case Centinela:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 3, diagPrev);
                                reservarRecursos(datos->hospital, atendiendo, 1, EnCasa);
                            }
                            else
                            {
                                reservarRecursos(datos->hospital, atendiendo, 1, EnCasa);
                            }
                        break;

                        case Intermedio:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 2, diagPrev);
                                reservarRecursos(datos->hospital, atendiendo, 1, EnCasa);
                            }
                            else
                            {
                                reservarRecursos(datos->hospital, atendiendo, 1, EnCasa);
                            }
                        break;
                        
                        case General:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);
                                reservarRecursos(datos->hospital, atendiendo, 1, EnCasa);
                            }
                            else
                            {
                                reservarRecursos(datos->hospital, atendiendo, 1, EnCasa);
                            }
                        break;
                    }
                break;

                case Muerto:
                    switch (hosp_type)
                    {
                        case Centinela:
                            liberarRecursos(datos->hospital, atendiendo, 3, diagPrev);
                            //no volver a poner el hilo en la cola de pacientes
                            //finalizar el hilo
                        break;

                        case Intermedio:
                            liberarRecursos(datos->hospital, atendiendo, 2, diagPrev);
                            //no volver a poner el hilo en la cola de pacientes
                            //finalizar el hilo
                        break;
                        
                        case General:
                            liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);
                            //no volver a poner el hilo en la cola de pacientes
                            //finalizar el hilo
                        break;
                    }
                break;
                
                default://dar de alta
                    switch (hosp_type)
                    {
                        case Centinela:
                            liberarRecursos(datos->hospital, atendiendo, 3, diagPrev);
                            //no volver a poner el hilo en la cola de pacientes
                            //liberar el hilo
                        break;

                        case Intermedio:
                            liberarRecursos(datos->hospital, atendiendo, 2, diagPrev);
                            //no volver a poner el hilo en la cola de pacientes
                            //liberar el hilo
                        break;
                        
                        case General:
                            liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);
                            //no volver a poner el hilo en la cola de pacientes
                            //liberar el hilo
                        break;
                    }
                break;
            }
        }
    }
}