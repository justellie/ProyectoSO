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

///@fn int liberarRecursos(int, Paciente *, float, TipoAtencion);
///@brief libera los recursos que el paciente tenga reservados anteriormente, segun su diagnostico
///@param refHospital necesario para saber de donde se reponen los recursos
///@param atendiendo referencia al paciente que esta siendo atendido en el momento
///@param cantidad le dice a la funcion que cantidad de personal debe ser liberada, esto segun el tipo de hospital
///@param diagPrevio diagnostico que se le dio al paciente en el pasado, este permitira determinar que recursos deben liberarse
///@return -1 fallo al liberar, 1 exito al liberar
int liberarRecursos(Hospital *refHospital, Paciente *atendiendo, int cantidad, TipoAtencion diag)
{
    //se esta suponiendo que refHospital-><arreglo de mapas personal>[0] es el nivel mas bajo y por tanto en el que esta completamente ocupado el personal
    // por lo tanto refHospital-><arreglo de mapas personal>[4] es el nivel mas alto, en el que se encuentran totalmente disponibles
    bool resultado = false;
    switch (diag)
    {
        case Intensivo:
            for (int i = 0; i < cantidad; i++)
            {
                //liberacion enfermeras
                Personal *enf = refmap_extract(&refHospital->enfermeras[0], &atendiendo->enfID[i]);
                atendiendo->enfID[i]=-1;
                refmap_put(&refHospital->enfermeras[4], &enf->id, enf);
                //liberacion medicos
                Personal *med = refmap_extract(&refHospital->medicos[0], &atendiendo->medID[i]);
                atendiendo->medID[i]=-1;
                refmap_put(&refHospital->medicos[4], &med->id, med);
            }
            refqueue_put(&refHospital->respiradores, NULL);
            sem_post(&refHospital->camasIntensivo);
            atendiendo->tiene_cama=0;
            if(atendiendo->enfID[cantidad]==-1 && atendiendo->medID[cantidad]==-1)
                resultado=true;
            
        break;
        
        case Basica:
            for (int i = 0; i < MAX_ATENCION; i++)
            {
                //liberacion enfermeras
                Personal *enf = refmap_extract(&refHospital->enfermeras[i], &atendiendo->enfID[0]);
                if (enf!=NULL)
                {
                    atendiendo->enfID[0]=-1;
                    refmap_put(&refHospital->enfermeras[i+1], &enf->id, enf);
                }
                //liberacion medicos
                Personal *med = refmap_extract(&refHospital->medicos[i], &atendiendo->medID[0]);
                if (med!=NULL)
                {
                    atendiendo->medID[0]=-1;
                    refmap_put(&refHospital->medicos[i+1], &med->id, med);
                }
            }
            refqueue_put(&refHospital->tanquesOxigeno, NULL);
            sem_post(&refHospital->camasBasico);
            atendiendo->tiene_cama=0;
            if(atendiendo->enfID[cantidad]==-1 && atendiendo->medID[cantidad]==-1)
                resultado=true;
            
        break;
    }

    return resultado;
}

///@fn int reservarRecursos(int, Paciente *, float, TipoAtencion);
///@brief reserva los recursos que el paciente necesite, segun el diagnostico
///@param refHospital necesario para saber de donde se toman los recursos
///@param atendiendo referencia al paciente que esta siendo atendido en el momento
///@param cantidad le dice a la funcion que cantidad de personal debe ser reservada, esto segun el tipo de hospital
///@param diagActual diagnostico que se le dio al paciente, este permitira determinar que recursos deben reservarse
///@return -1 error al reservar, 1 exito al reservar, 2 transferencia del paciente
int reservarRecursos(Hospital *refHospital, Paciente *atendiendo, int cantidad, TipoAtencion diagActual)
{
    //se esta suponiendo que refHospital-><arreglo de mapas personal>[0] es el nivel mas bajo y por tanto en el que esta completamente ocupado el personal
    //por lo tanto refHospital-><arreglo de mapas personal>[4] es el nivel mas alto, en el que se encuentran totalmente disponibles
    
    bool respirador_flag, oxigeno_flag;
    int resultado = -1, enf_flag= 0, med_flag = 0;
    switch (diagActual)
    {
        case Intensivo:
            //reservacion cama intensiva
            if( sem_trywait(&refHospital->camasIntensivo) == -1 )
            {
                if( errno == EAGAIN )
                {
                    // Enviar al paciente a la UGC para que sea transferido de hospital
                    resultado = -1;
                }
            }
            else
            {
                for (int i = 0; i < cantidad; i++)
                {
                    //reservacion de enfermeras
                    Personal *enf = refmap_extract_max(&refHospital->enfermeras[4]);
                    if (enf!=NULL)
                    {
                        atendiendo->enfID[i]=enf->id;
                        refmap_put(&refHospital->enfermeras[0], &enf->id, enf);
                        enf_flag++;
                    }
                    //reservacion de medicos
                    Personal *med = refmap_extract_max(&refHospital->medicos[4]);
                    if (enf!=NULL)
                    {
                        atendiendo->medID[i]=med->id;
                        refmap_put(&refHospital->medicos[0], &med->id, med);
                        med_flag++;
                    }
                }
                //reservacion respirador artificial
                if(refqueue_tryget(&refHospital->respiradores)==NULL)
                {
                    if (errno= EAGAIN)
                        respirador_flag=false;
                }
                else
                {
                    respirador_flag=true;
                }

                if(respirador_flag && (med_flag==cantidad) && (enf_flag==cantidad))
                    resultado=1;
                else
                {
                    //antes de las condiciones inferiores se debe realizar la pedida del recurso al UGC
                    //por ahora no se como se realizara esto, asi que lo dejo como devolucion de los recursos

                    if(respirador_flag)
                        refqueue_put(&refHospital->respiradores, NULL);
                    for (int i = 0; i < enf_flag; i++)
                    {
                        if(atendiendo->enfID[i]!=-1){
                            Personal *enf = refmap_extract(&refHospital->enfermeras[0], &atendiendo->enfID[i]);
                            atendiendo->enfID[i]=-1;
                            refmap_put(&refHospital->enfermeras[4], &enf->id, enf);
                        }
                    }
                    for (int i = 0; i < med_flag; i++)
                    {
                        if(atendiendo->medID[i]!=-1){
                            Personal *med = refmap_extract(&refHospital->medicos[0], &atendiendo->medID[i]);
                            atendiendo->medID[i]=-1;
                            refmap_put(&refHospital->medicos[4], &med->id, med);
                        }
                    }
                    resultado=-1;
                }
            }
            
            
        break;
        
        case Basica:
        if( sem_trywait(&refHospital->camasBasico) == -1 )
            {
                if( errno == EAGAIN )
                {
                    // Enviar al paciente a la UGC para que sea transferido de hospital
                    resultado = -1;
                }
            }
            else
            {
                for (int i = 1; i > MAX_ATENCION; i++)
                {
                    //reserva de enfermeras
                    Personal *enf = refmap_extract_max(&refHospital->enfermeras[i]);
                    if (enf!=NULL)
                    {
                        atendiendo->enfID[0]=enf->id;
                        refmap_put(&refHospital->enfermeras[i-1], &enf->id, enf);
                    }
                    //resserva de medicos
                    Personal *med = refmap_extract_max(&refHospital->medicos[i]);
                    if (med!=NULL)
                    {
                        atendiendo->medID[0]=med->id;
                        refmap_put(&refHospital->medicos[i-1], &med->id, med);
                    }
                }
                //reserva de tanque de oxigeno
                if(refqueue_tryget(&refHospital->tanquesOxigeno)==NULL)
                {
                    if (errno= EAGAIN)
                       oxigeno_flag=false;
                }
                else
                {
                    oxigeno_flag=true;
                }

                if (oxigeno_flag && (atendiendo->enfID[0]!=-1) && (atendiendo->medID[0]!=-1))
                {
                    resultado=1;
                }
                else
                {
                    //antes de las condiciones inferiores se debe realizar la pedida del recurso al UGC
                    //por ahora no se como se realizara esto, asi que lo dejo como devolucion de los recursos

                    if(respirador_flag)
                        refqueue_put(&refHospital->tanquesOxigeno, NULL);

                    if(atendiendo->enfID[0]!=-1){
                        for (int i = 0; i < MAX_ATENCION; i++)
                        {
                            Personal *enf = refmap_extract_max(&refHospital->enfermeras[i]);
                            if (enf!=NULL)
                            {
                                atendiendo->enfID[0]=-1;
                                refmap_put(&refHospital->enfermeras[i+1], &enf->id, enf);
                            }                        
                        }
                    }
                    if (atendiendo->medID[0]!=-1)
                    {
                        for (int i = 0; i < MAX_ATENCION; i++)
                        {
                            Personal *med = refmap_extract(&refHospital->medicos[i], &atendiendo->medID[0]);
                            if (med!=NULL)
                            {
                                atendiendo->medID[0]=-1;
                                refmap_put(&refHospital->medicos[i+1], &med->id, med);
                            }
                        }                                        
                    }
                    resultado=-1;
                }                
            }
        break;
    }
    return resultado;
}

///@fn void actor_gestor(void *datos_gestor)
///@brief funcion que ejecuta el actor gestor para realizar sus funciones
///@param datos_gestor estructura que contiene los datos basicos de un gestor de camas
void actor_gestor(void *datos_gestor)
{
    GestorCama *datos = (GestorCama *) datos_gestor;
    TipoHospital hosp_type = datos->hospital->tipo;
    TipoAtencion diagAct;
    TipoAtencion diagPrev;
    while(true)
    {
        ///Paciente que esta siendo atendio por el gestor 
        Paciente *atendiendo = refqueue_get(&datos->hospital->pacientes);
        
        if(atendiendo->ingresando)
        {
            atendiendo->ingresando=0;
            diagAct = atendiendo->servicio;
            diagPrev = Ninguno;
        }
        else 
        {
            diagPrev = atendiendo->servicio;
            diagAct = obtener_diagnostico_compuesta(atendiendo);
        }
        int status=0;
        bool alta = false, dead=false;

        if(atendiendo->tiene_cama && diagPrev==diagAct && diagAct!=Ninguno)
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
                            }
                            else
                            {
                                datos->hospital->estadis_pacientes.hospitalizados++;
                            }
                            status=reservarRecursos(datos->hospital, atendiendo, 3, Intensivo);                                                        
                        break;

                        case Intermedio:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 2, diagPrev);                                                             
                            }
                            else
                            {
                                datos->hospital->estadis_pacientes.hospitalizados++;
                            }
                            status=reservarRecursos(datos->hospital, atendiendo, 2, Intensivo);
                        break;
                        
                        case General:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);                                
                            }
                            else
                            {
                                datos->hospital->estadis_pacientes.hospitalizados++;
                            }
                            status=reservarRecursos(datos->hospital, atendiendo, 1, Intensivo);
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
                            }
                            else
                            {
                                datos->hospital->estadis_pacientes.hospitalizados++;
                            }                             
                            status=reservarRecursos(datos->hospital, atendiendo, 0, Basica);
                        break;

                        case Intermedio:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 2, diagPrev);                               
                            }
                            else
                            {
                                datos->hospital->estadis_pacientes.hospitalizados++;
                            }
                            status=reservarRecursos(datos->hospital, atendiendo, 0, Basica);
                        break;
                        
                        case General:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);                                
                            }
                            else
                            {
                                datos->hospital->estadis_pacientes.hospitalizados++;
                            }                            
                            status=reservarRecursos(datos->hospital, atendiendo, 0, Basica);
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
                            }
                            else
                            {
                                datos->hospital->estadis_pacientes.monitoreados++;
                            }
                            refqueue_put(&pacienteEnCasa, atendiendo);
                        break;

                        case Intermedio:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 2, diagPrev);                                                                
                            }
                            else
                            {
                                datos->hospital->estadis_pacientes.monitoreados++;
                            }
                            refqueue_put(&pacienteEnCasa, atendiendo);
                        break;
                        
                        case General:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);
                            }
                            else
                            {
                                datos->hospital->estadis_pacientes.monitoreados++;
                            }
                            refqueue_put(&pacienteEnCasa, atendiendo);
                        break;
                    }
                break;

                case Muerto:
                    switch (hosp_type)
                    {
                        case Centinela:
                            liberarRecursos(datos->hospital, atendiendo, 3, diagPrev);
                            datos->hospital->estadis_pacientes.muertos++;
                            dead=true;
                        break;

                        case Intermedio:
                            liberarRecursos(datos->hospital, atendiendo, 2, diagPrev);
                            datos->hospital->estadis_pacientes.muertos++;
                            dead=true;
                        break;
                        
                        case General:
                            liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);
                            datos->hospital->estadis_pacientes.muertos++;
                            dead=true;
                        break;
                    }
                break;
                
                default://dar de alta
                    switch (hosp_type)
                    {
                        case Centinela:
                            liberarRecursos(datos->hospital, atendiendo, 3, diagPrev);
                            datos->hospital->estadis_pacientes.dadosDeAlta++;
                            alta = true;
                        break;

                        case Intermedio:
                            liberarRecursos(datos->hospital, atendiendo, 2, diagPrev);
                            datos->hospital->estadis_pacientes.dadosDeAlta++;
                            alta = true;
                        break;
                        
                        case General:
                            liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);
                            datos->hospital->estadis_pacientes.dadosDeAlta++;
                            alta = true;
                        break;
                    }
                break;
            }
            if (status==-1)
            {
                //refqueue_put(&UGC, atendiendo);
                continue;
            } 
            else
            {
                atendiendo->tiene_cama=1;
                refqueue_put(&datos->hospital->pacientes, atendiendo);
            }
        }
        //aviso al paciente de que ha sido atendido
        atendiendo->servicio=diagAct;
        atendiendo->fueAtendido++;
        if (alta)
        {
            atendiendo->deAlta++;
            atendiendo->ingresando=1;
        }
            
        if (dead)
            atendiendo->vivo=0;
        
    }
}