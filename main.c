#include "actores.h"
#include "definiciones.h"
#include <stdlib.h>

#include <time.h>
#include <signal.h>

// Crea los timers y eventos particulares:
// SIGUSR1 <-- Llegó el momento de hacer un reporte.
#define NANOSEGUNDOS_EN_SEGUNDOS 1000000000;
#define ABSOLUTO TIMER_ABSTIME
#define RELATIVO 0
typedef struct DatosTemporizador{
    int               tipo;
    struct sigevent   ptr_evento:
    struct itimerspec intervalo;
}DatosTemporizador;

void activar_estadisticas_ugc( int segundos , int nano_segundos ){

    timerspec_t start    = { segundos , nano_segundos };
    timerspec_t interval = { segundos , nano_segundos };
    // start    != 0 -> sin expiración. en seguida. interval != 0 -> recarga
    DatosTemporizador info = { ABSOLUTO , {0,SIGUSR1} , {start,interval} }

    union sigval evento;
    struct sigevent evento = { };

    timer_create();
    // TODO: Usar al limipiar esto.
    // timer_delete();



}

int main(){
    exit( EXIT_SUCCESS );
}
