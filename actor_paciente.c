/**
 * @file actor_gestor.c
 * @author Juan Herrera 26972881 (onemoreguy3@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2021-02-28
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#ifndef __ACTOR_PACIENT__
#define __ACTOR_PACIENT__

#include "actores.h"
#include "definiciones.h"
#include <time.h>
#include <unistd.h>




///@fn void actor_paciente(void *datos_paciente)
///@brief funcion que ejecuta el actor paciente para realizar sus funciones
///@param datos_paciente estructura que contiene los datos basicos de un paciente
void actor_paciente(void *datos_paciente)
{
    Paciente *datos = (Paciente *) datos_paciente;
    int decidirHosp;                                // Entero que se obtiene al azar y decide a cual hospital ir
    int seSienteMal;
    Hospital * HospElegid = NULL;                   // Variable que almacena el hospital elegido con el valor entero
 
    while (datos->vivo)
    {    
        // Se decide el hospital al que va a ir
        seSienteMal = autoexamen();

        if (seSienteMal){
            decidirHosp = rand ()%NHOSPITALES;
            HospElegid = &(Tabla_Hospitales[decidirHosp]);
            datos->fueAtendido = 0;
            datos->deAlta = 0;



            // Entra a la sala de espera para poder ir luego a la sala de muestras
            sem_wait(&HospElegid->salaEspera);       // Espera a que haya espacio dentro de la sala de espera hospital
            sem_wait(&HospElegid->salaMuestra);      // Espera a que esté disponible la entrada a la sala de muestra
            sem_signal(&HospElegid->salaEspera);     // Sale de la sala de espera

            // Se realiza la cola para ingresar
            refqueue_put(&HospElegid->pacientesEnSilla, datos); // Hace la cola en la sala de muestra
            sem_wait(&datos->muestraTomada);                    // Espera a que le tomen la muestra
            sem_signal(&HospElegid->salaMuestra);               // Sale de la sala de muestra

            // Espera a que lo atiendan
            pthread_mutex_lock( &datos->atendidoLock);
                while (!datos->fueAtendido) {
                    pthread_cond_wait(&datos->atendido, &datos->atendidoLock);
                }
            pthread_mutex_unlock( &datos->atendidoLock);
            

            //Espera a estar sano
            pthread_mutex_lock( &datos->medLock);
                while (!datos->vivo ||datos->deAlta) {
                    pthread_cond_wait(&datos->atendido, &datos->medLock);
                }
            pthread_mutex_unlock( &datos->medLock);
            
            
        }
    }

    printf("Adios mundo cruel\n");
}


int autoexamen()
{
    //Espera 3 segundos
    sleep(3);
    return rand ()%100 < 30; //Hay un 30% de posibilidades de que se sienta mal
}
#endif