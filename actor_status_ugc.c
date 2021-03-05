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
 * @param param datos del hospital cuyas estadísticas estamos llevando
 * 
 */
//extern Estadistica statHospital[NACTUALIZACIONES][NHOSPITALES];
//extern Hospital   Tabla_Hospitales[NHOSPITALES];

void *actor_status_ugc(void* actor_ugc) {
    Estadistica daily_stats;
    int i, j;
    FILE *fptr;
    UGC* datos_ugc = (UGC *)actor_ugc;
    while(true) {
        pthread_mutex_lock(&(datos_ugc->FinalizarStatLock));
        while(datos_ugc->continuar){
            pthread_cond_wait(&(datos_ugc->FinalizarStat), &(datos_ugc->FinalizarStatLock));
            daily_stats.covid = 0;
            daily_stats.dadosDeAlta = 0;
            daily_stats.hospitalizados = 0;
            daily_stats.monitoreados = 0;
            daily_stats.muertos = 0;

            for(i = 0; i < NACTUALIZACIONES; i++) {
                for(j = 0; j < NHOSPITALES; j++){
                    daily_stats.covid += statHospital[i][j].covid;
                    daily_stats.dadosDeAlta += statHospital[i][j].dadosDeAlta;
                    daily_stats.hospitalizados += statHospital[i][j].hospitalizados;
                    daily_stats.monitoreados += statHospital[i][j].monitoreados;
                    daily_stats.muertos += statHospital[i][j].muertos;
                }
            }
            printf("\n****************************************************\n");
            printf("Estadísticas diarias para el manejo del COVID-19. Sistema Facyt-SO\n");
            printf("Nro. de pacientes con covid: %d\n",daily_stats.covid);
            printf("Nro. de pacientes con dados de alta: %d\n",daily_stats.dadosDeAlta);
            printf("Nro. de pacientes con hospitalizados: %d\n",daily_stats.hospitalizados);
            printf("Nro. de pacientes con monitoreados: %d\n",daily_stats.monitoreados);
            printf("Nro. de pacientes con muertos: %d\n",daily_stats.muertos);

            fptr = fopen("dailystats.txt", "a");
            fprintf(fptr,"\n****************************************************\n");
            fprintf(fptr,"Estadísticas diarias para el manejo del COVID-19. Sistema Facyt-SO\n");
            fprintf(fptr,"Nro. de pacientes con covid: %d\n",daily_stats.covid);
            fprintf(fptr,"Nro. de pacientes con dados de alta: %d\n",daily_stats.dadosDeAlta);
            fprintf(fptr,"Nro. de pacientes con hospitalizados: %d\n",daily_stats.hospitalizados);
            fprintf(fptr,"Nro. de pacientes con monitoreados: %d\n",daily_stats.monitoreados);
            fprintf(fptr,"Nro. de pacientes con muertos: %d\n",daily_stats.muertos);
            fclose(fptr);
        }
        pthread_mutex_unlock(&(datos_ugc->FinalizarStatLock));
        datos_ugc->continuar = 1;
    }
        
        
    return NULL;
}
