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

//extern UGC        gestor_central;
//extern RefQueue   pacienteEnCasa;

///@fn int liberarRecursos(int, Paciente *, float, TipoAtencion);
///@brief Libera los recursos que el paciente tenga reservados anteriormente, segun su diagnostico
///@param refHospital Necesario para saber de donde se reponen los recursos
///@param atendiendo Referencia al paciente que esta siendo atendido en el momento
///@param cantidad Le dice a la funcion que cantidad de personal debe ser liberada, esto segun el tipo de hospital
///@param diagPrevio Diagnostico que se le dio al paciente en el pasado, este permitira determinar que recursos deben liberarse
///@return -1 Fallo al liberar, 1 Exito al liberar
int liberarRecursos(Hospital *refHospital, Paciente *atendiendo, int cantidad, TipoAtencion diag)
{
    //Se esta suponiendo que refHospital-><arreglo de mapas personal>[0] es el nivel mas bajo y por tanto en el que esta completamente ocupado el personal
    //Por lo tanto refHospital-><arreglo de mapas personal>[4] es el nivel mas alto, en el que se encuentran totalmente disponibles
    /*
        totalmente ocupado 
                 v
                [0, 1, 2, 3, 4]
                             ^
                        totalmente libre
    */
    
    bool resultado = false;
    switch (diag)
    {
        case Intensivo:
            for (int i = 0; i < cantidad; i++)
            {
                //Liberacion enfermeras
                Personal *enf = refmap_extract(&refHospital->enfermeras[0], &atendiendo->enfID[i]);
                atendiendo->enfID[i]=-1;
                refHospital->estadis_recursos.nenfermeras++;
                refmap_put(&refHospital->enfermeras[4], &enf, enf);
                pthread_cond_signal(&refHospital->stast);
                //Liberacion medicos
                Personal *med = refmap_extract(&refHospital->medicos[0], &atendiendo->medID[i]);
                atendiendo->medID[i]=-1;
                refHospital->estadis_recursos.nmedicos++;
                refmap_put(&refHospital->medicos[4], &med, med);
                pthread_cond_signal(&refHospital->stast);
            }
            //Liberacion de los respiradores
            refqueue_put(&refHospital->respiradores, (void *) 1);
            refHospital->estadis_recursos.nrespira++;
            //Liberacion de la cama
            sem_post(&refHospital->camasIntensivo);
            refHospital->estadis_recursos.ncamasInt++;
            //Se especifica que el paciente ya no tiene una cama, es decir, no esta hospitalizado
            atendiendo->tiene_cama=0;
            //Comprobacion de que la liberacion se hizo correctamente (no se si esto sea necesario)
            if(atendiendo->enfID[cantidad]==-1 && atendiendo->medID[cantidad]==-1)
                resultado=true;
            
        break;
        
        case Basica:
            for (int i = 0; i < MAX_ATENCION; i++)
            {
                //Liberacion enfermeras, se  busca a la enfermera en todos los niveles de ocupacion
                Personal *enf = refmap_extract(&refHospital->enfermeras[i], &atendiendo->enfID[0]);
                if (enf!=NULL)//Si la enfermera es encontrada
                {
                    //Se quita el id de la enfermera del registro del paciente
                    atendiendo->enfID[0]=-1; //Siempre sera 0 ya que, un paciente basico no puede tener mas de una enfermera
                    refmap_put(&refHospital->enfermeras[i+1], &enf, enf); //Se recoloca a la enfermera en el mapa, un nivel de ocupacion por debajo
                                                                              //por como esta pensado, bajar un nivel de ocupacion implica sumar 
                    if(i+1==4)
                        refHospital->estadis_recursos.nenfermeras++;
                }

                //liberacion medicos, se comporta de la misma forma que la liberacion de enfermeras
                Personal *med = refmap_extract(&refHospital->medicos[i], &atendiendo->medID[0]);
                if (med!=NULL)
                {
                    atendiendo->medID[0]=-1;
                    refmap_put(&refHospital->medicos[i+1], &med, med);
                    if(i+1==4)
                        refHospital->estadis_recursos.nmedicos++;
                }
            }
            //Liberacion del tanque de oxigeno que esta en uso por el paciente            
            refqueue_put(&refHospital->tanquesOxigeno, (void *) 1);
            refHospital->estadis_recursos.ntanques++;
            //Liberacion de la cama basica
            sem_post(&refHospital->camasBasico);
            refHospital->estadis_recursos.ncamasBas++;
            //Se especifica que el paciente ya no posee una cama, es decir, ya no esta hospitalizado
            atendiendo->tiene_cama=0;
            //Comprobacion de que la liberacion se hizo correctamente (no se si esto sea necesario)
            if(atendiendo->enfID[cantidad]==-1 && atendiendo->medID[cantidad]==-1)
                resultado=true;
            
        break;

        default:
            printf("esto no se deberia mostrar nunca: liberacion");
        break;
    }

    return resultado;
}

///@fn int reservarRecursos(int, Paciente *, float, TipoAtencion);
///@brief Reserva los recursos que el paciente necesite, segun el diagnostico
///@param refHospital Necesario para saber de donde se toman los recursos
///@param atendiendo Referencia al paciente que esta siendo atendido en el momento
///@param cantidad Le dice a la funcion que cantidad de personal debe ser reservada, esto segun el tipo de hospital
///@param diagActual Diagnostico que se le dio al paciente, este permitira determinar que recursos deben reservarse
///@return -1 Error al reservar, 1 Exito al reservar
int reservarRecursos(Hospital *refHospital, Paciente *atendiendo, int cantidad, TipoAtencion diagActual)
{
    //se esta suponiendo que refHospital-><arreglo de mapas personal>[0] es el nivel mas bajo y por tanto en el que esta completamente ocupado el personal
    //por lo tanto refHospital-><arreglo de mapas personal>[4] es el nivel mas alto, en el que se encuentran totalmente disponibles
    /*
        totalmente ocupado 
                 v
                [0, 1, 2, 3, 4]
                             ^
                        totalmente libre
    */

    bool respirador_flag, oxigeno_flag;
    int resultado = -1, enf_flag= 0, med_flag = 0;
    switch (diagActual)
    {
        case Intensivo:
            //Reservacion cama intensiva
            if( sem_trywait(&refHospital->camasIntensivo) == -1 )//Intento de reservar la cama
            {
                if( errno == EAGAIN )
                {
                    // Enviar al paciente a la UGC para que sea transferido de hospital
                    resultado = -1;
                }
            }
            else
            {
                //Se pudo reservar cama
                refHospital->estadis_recursos.ncamasInt--;
                for (int i = 0; i < cantidad; i++)
                {
                    //Reservacion de enfermeras, solo te busca en el nivel mas alto, pacientes intensivos reservan al perosnal por completo
                    Personal *enf = refmap_extract_max(&refHospital->enfermeras[4]);
                    if (enf!=NULL)
                    {
                        //Al encontrar una enfermera, se le asigna al paciente y se coloca en el nivel mas alto de ocupacion
                        atendiendo->enfID[i]=enf->id;
                        refHospital->estadis_recursos.nenfermeras--;
                        refmap_put(&refHospital->enfermeras[0], &enf, enf);
                        //Este contador indica si se reservo la cantidad necesaria de enfermeras
                        enf_flag++;
                    }
                    //reservacion de medicos, comportamiento similar a las enfermeras
                    Personal *med = refmap_extract_max(&refHospital->medicos[4]);
                    if (enf!=NULL)
                    {
                        atendiendo->medID[i]=med->id;
                        refHospital->estadis_recursos.nmedicos--;
                        refmap_put(&refHospital->medicos[0], &med, med);
                        med_flag++;
                    }
                }
                //reservacion respirador artificial
                if(refqueue_tryget(&refHospital->respiradores)==NULL)
                {
                    if (errno==EAGAIN){
                        TuplaInventario *pedir = malloc (sizeof(TuplaInventario));
                        pedir->idHospital=refHospital->id;
                        pedir->cantidad=1;
                        pedir->tipo_recurso=PideRespirador;
                        refqueue_put(&gestor_central.peticiones, pedir);
                        pthread_mutex_lock(&gestor_central.EsperandoPorRecurso);
                        if(refqueue_tryget(&refHospital->respiradores)==NULL)
                        {
                            if (errno==EAGAIN)
                                respirador_flag=false;
                        }
                        else
                        {
                            refHospital->estadis_recursos.nrespira--;
                            respirador_flag=true;
                        }  
                    }   
                }
                else
                {
                    refHospital->estadis_recursos.nrespira--;
                    respirador_flag=true;
                }
                //verificacion de que todos los insumos necesarios fueron reservados con exito
                if(respirador_flag && (med_flag==cantidad) && (enf_flag==cantidad))
                    resultado=1;
                else
                {
                    //En caso de que la UGC no pueda brindar los insumos, se liberan los recursos que se tomaron 
                    //y se procede a transferir al paciente
                    if(respirador_flag)
                    {
                        refqueue_put(&refHospital->respiradores, (void *) 1);
                        refHospital->estadis_recursos.nrespira++;
                    }
                    for (int i = 0; i < enf_flag; i++)
                    {
                        if(atendiendo->enfID[i]!=-1){
                            Personal *enf = refmap_extract(&refHospital->enfermeras[0], &atendiendo->enfID[i]);
                            atendiendo->enfID[i]=-1;
                            refmap_put(&refHospital->enfermeras[4], &enf, enf);
                            refHospital->estadis_recursos.nenfermeras++;
                        }
                    }
                    for (int i = 0; i < med_flag; i++)
                    {
                        if(atendiendo->medID[i]!=-1){
                            Personal *med = refmap_extract(&refHospital->medicos[0], &atendiendo->medID[i]);
                            atendiendo->medID[i]=-1;
                            refmap_put(&refHospital->medicos[4], &med, med);
                            refHospital->estadis_recursos.nmedicos++;
                        }
                    }
                    resultado=-1;
                }
            }
            
            
        break;
        
        case Basica:
            //Reservacion de cama basica
            if( sem_trywait(&refHospital->camasBasico) == -1 )//Intento de reservar la cama
            {
                if( errno == EAGAIN )
                {
                    // Enviar al paciente a la UGC para que sea transferido de hospital
                    resultado = -1;
                }
            }
            else
            {
                //Se logro reservar cama
                refHospital->estadis_recursos.ncamasInt--;
                for (int i = 1; i > MAX_ATENCION; i++)
                {
                    //El personal se busca desde el nivel mas bajo de ocupacion hasta el nivel mas alto,
                    //esto evita (en la medida de lo posible) que exista mucho personal ocupado a medias

                    //reserva de enfermeras
                    Personal *enf = refmap_extract_max(&refHospital->enfermeras[i]);
                    if (enf!=NULL)
                    {
                        //Al encontrar una enfermera, se le asigna al paciente y se coloca en una nivel mas alto de ocupacion
                        atendiendo->enfID[0]=enf->id;
                        refmap_put(&refHospital->enfermeras[i-1], &enf, enf);
                    }
                    //resserva de medicos, comportamiento similar a la enfermeras
                    Personal *med = refmap_extract_max(&refHospital->medicos[i]);
                    if (med!=NULL)
                    {
                        atendiendo->medID[0]=med->id;
                        refmap_put(&refHospital->medicos[i-1], &med, med);
                    }
                }
                //reserva de tanque de oxigeno
                if(refqueue_tryget(&refHospital->tanquesOxigeno)==NULL)
                {
                    if (errno= EAGAIN)
                    {
                        TuplaInventario *pedir = malloc (sizeof(TuplaInventario));
                        pedir->idHospital=refHospital->id;
                        pedir->cantidad=1;
                        pedir->tipo_recurso=PideTanque;
                        refqueue_put(&gestor_central.peticiones, pedir);
                        pthread_mutex_lock(&gestor_central.EsperandoPorRecurso);
                        if(refqueue_tryget(&refHospital->tanquesOxigeno)==NULL)
                        {
                            if (errno==EAGAIN)
                                respirador_flag=false;
                        }
                        else
                        {
                            refHospital->estadis_recursos.nrespira--;
                            respirador_flag=true;
                        }
                    }
                       
                }
                else
                {
                    refHospital->estadis_recursos.ntanques--;
                    oxigeno_flag=true;
                }

                //Verificacion de que todos lo recursos fueron reservados correctamente
                if (oxigeno_flag && (atendiendo->enfID[0]!=-1) && (atendiendo->medID[0]!=-1))
                {
                    resultado=1;
                }
                else
                {
                    //En caso de que la UGC no pueda brindar los insumos, se liberan los recursos que se tomaron 
                    //y se procede a transferir al paciente
                    if(respirador_flag)
                    {
                        refqueue_put(&refHospital->tanquesOxigeno, (void *) 1);
                        refHospital->estadis_recursos.ntanques++;
                    }

                    if(atendiendo->enfID[0]!=-1){
                        for (int i = 0; i < MAX_ATENCION; i++)
                        {
                            Personal *enf = refmap_extract_max(&refHospital->enfermeras[i]);
                            if (enf!=NULL)
                            {
                                atendiendo->enfID[0]=-1;
                                refmap_put(&refHospital->enfermeras[i+1], &enf, enf);
                                if(i+1==4)
                                    refHospital->estadis_recursos.nenfermeras++;
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
                                refmap_put(&refHospital->medicos[i+1], &med, med);
                                if(i+1==4)
                                    refHospital->estadis_recursos.nmedicos--;
                            }
                        }                                        
                    }
                    resultado=-1;
                }                
            }
        break;
        
        default:
            printf("esto no deberia mostarse nunca: reserva");
        break;
    }
    return resultado;
}

///@fn void actor_gestor(void *datos_gestor)
///@brief funcion que ejecuta el actor gestor para realizar sus funciones
///@param datos_gestor estructura que contiene los datos basicos de un gestor de camas
void* actor_gestor(void *datos_gestor)
{
    GestorCama *datos = (GestorCama *) datos_gestor;
    TipoHospital hosp_type = datos->hospital->tipo;
    TipoAtencion diagAct;
    TipoAtencion diagPrev;
    int status;
    bool alta, dead, volunt;
    while(true)
    {
        ///Paciente que esta siendo atendio por el gestor 
        Paciente *atendiendo = refqueue_get(&datos->hospital->pacientes);
        
        if (atendiendo->fueAtendido==0)
        {
            if(atendiendo->deAlta!=0)
            {
                atendiendo->deAlta=0;
            }
            atendiendo->fueAtendido++;
            pthread_cond_signal(&atendiendo->atendido);
        }

        //Si el paciente esta ingresando por primera vez 
        if(atendiendo->ingresando)
        {
            //en servicio se encuentra el diagnostico dado en triaje
            atendiendo->ingresando=0;
            diagAct = atendiendo->servicio;
            diagPrev = Ninguno;
        }
        else 
        {
            //si ya estaba hospitalizado, se debe realizar un nuevo diagnostico
            diagPrev = atendiendo->servicio;
            diagAct = obtener_diagnostico_compuesta(atendiendo);
        }
        status=0;
        alta = false;
        dead = false;
        volunt = false;

        //Si el diagnostico dado es el mismo que el anterior y este no es que el paciente esta sano, se coloca de nuevo en la cola
        if(atendiendo->tiene_cama && diagPrev==diagAct && diagAct!=Ninguno)
        {
                refqueue_put(&datos->hospital->pacientes, atendiendo);
        }
        else
        {
            //A todo paciente que tenga cama, se le quitan los recursos segun el servicio que estaba recibiendo antes, 
            //para reservar los servicios del nuevo diagnostico
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
                                pthread_mutex_lock(&datos->hospital->estadisticasLock);
                                datos->hospital->estadis_pacientes.hospitalizados++;
                                pthread_mutex_unlock(&datos->hospital->estadisticasLock);
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
                                pthread_mutex_lock(&datos->hospital->estadisticasLock);
                                datos->hospital->estadis_pacientes.hospitalizados++;
                                pthread_mutex_unlock(&datos->hospital->estadisticasLock);
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
                                pthread_mutex_lock(&datos->hospital->estadisticasLock);
                                datos->hospital->estadis_pacientes.hospitalizados++;
                                pthread_mutex_unlock(&datos->hospital->estadisticasLock);
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
                                pthread_mutex_lock(&datos->hospital->estadisticasLock);
                                datos->hospital->estadis_pacientes.hospitalizados++;
                                pthread_mutex_unlock(&datos->hospital->estadisticasLock);
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
                                pthread_mutex_lock(&datos->hospital->estadisticasLock);
                                datos->hospital->estadis_pacientes.hospitalizados++;
                                pthread_mutex_unlock(&datos->hospital->estadisticasLock);
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
                                pthread_mutex_lock(&datos->hospital->estadisticasLock);
                                datos->hospital->estadis_pacientes.hospitalizados++;
                                pthread_mutex_unlock(&datos->hospital->estadisticasLock);
                            }                            
                            status=reservarRecursos(datos->hospital, atendiendo, 0, Basica);
                        break;
                    }
                break;
                
                //Los pacientes que va a ser atendidos en casa son enviados con los voluntarios
                case EnCasa:
                    switch (hosp_type)
                    {
                        case Centinela:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 3, diagPrev);                                
                            }
                            pthread_mutex_lock(&datos->hospital->estadisticasLock);
                            datos->hospital->estadis_pacientes.monitoreados++;
                            pthread_mutex_unlock(&datos->hospital->estadisticasLock);
                            volunt=true;                            
                        break;

                        case Intermedio:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 2, diagPrev);                                                                
                            }
                            pthread_mutex_lock(&datos->hospital->estadisticasLock);
                            datos->hospital->estadis_pacientes.monitoreados++;
                            pthread_mutex_unlock(&datos->hospital->estadisticasLock);
                            volunt=true;
                        break;
                        
                        case General:
                            if(atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);
                            }
                            else
                            pthread_mutex_lock(&datos->hospital->estadisticasLock);
                            datos->hospital->estadis_pacientes.monitoreados++;
                            pthread_mutex_unlock(&datos->hospital->estadisticasLock);
                            volunt=true;
                        break;
                    }
                break;

                //Los pacientes que son declarados muertos, se liberan los recursos asociados a los mismos
                case Muerto:
                    switch (hosp_type)
                    {
                        case Centinela:
                            if (atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 3, diagPrev);
                            }
                            pthread_mutex_lock(&datos->hospital->estadisticasLock);
                            datos->hospital->estadis_pacientes.muertos++;
                            pthread_mutex_unlock(&datos->hospital->estadisticasLock);
                            dead=true;
                        break;

                        case Intermedio:
                            if (atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 2, diagPrev);
                            }
                            pthread_mutex_lock(&datos->hospital->estadisticasLock);
                            datos->hospital->estadis_pacientes.muertos++;
                            pthread_mutex_unlock(&datos->hospital->estadisticasLock);
                            dead=true;
                        break;
                        
                        case General:
                            if (atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);
                            }
                            pthread_mutex_lock(&datos->hospital->estadisticasLock);
                            datos->hospital->estadis_pacientes.muertos++;
                            pthread_mutex_unlock(&datos->hospital->estadisticasLock);
                            dead=true;
                        break;
                    }
                break;
                
                default://dar de alta
                    switch (hosp_type)
                    {
                        case Centinela:
                            if (atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 3, diagPrev);
                            }
                            pthread_mutex_lock(&datos->hospital->estadisticasLock);
                            datos->hospital->estadis_pacientes.dadosDeAlta++;
                            pthread_mutex_unlock(&datos->hospital->estadisticasLock);
                            alta = true;
                        break;

                        case Intermedio:
                            if (atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 2, diagPrev);
                            }
                            pthread_mutex_lock(&datos->hospital->estadisticasLock);
                            datos->hospital->estadis_pacientes.dadosDeAlta++;
                            pthread_mutex_unlock(&datos->hospital->estadisticasLock);
                            alta = true;
                        break;
                        
                        case General:
                            if (atendiendo->tiene_cama)
                            {
                                liberarRecursos(datos->hospital, atendiendo, 1, diagPrev);
                            }
                            pthread_mutex_lock(&datos->hospital->estadisticasLock);
                            datos->hospital->estadis_pacientes.dadosDeAlta++;
                            pthread_mutex_unlock(&datos->hospital->estadisticasLock);
                            alta = true;
                        break;
                    }
                break;
            }
            //si el estatus es -1, implica que al paciente no se le pudieron asignar recursos suficientes y debe ser transferido de hospial
            pthread_cond_signal(&datos->hospital->stast);
            if (status==-1)
            {
                atendiendo->ingresando=1;
                refqueue_put(&gestor_central.pacientes, atendiendo);
                continue;
            } 
            else
            {
                if (alta)
                {
                    atendiendo->deAlta++;
                    atendiendo->ingresando=1;
                    pthread_cond_signal(&atendiendo->atendido);
                }
                else
                {
                    if (dead)
                    {
                        atendiendo->vivo=0;
                        pthread_cond_signal(&atendiendo->atendido);
                    }
                    else 
                    {
                        if(volunt)
                        {
                            refqueue_put(&pacienteEnCasa, atendiendo);
                        }
                        else
                        {
                            //si el paciente no ha sido dado de alta, no ha muerto o no esta con los voluntarios, se le devuelve a la cola
                            atendiendo->tiene_cama=1;
                            refqueue_put(&datos->hospital->pacientes, atendiendo);
                        }
                    }                        
                }                
            }
        }
        //aviso al paciente de que ha sido atendido
        atendiendo->servicio=diagAct;
          
    }

    return NULL;
}
