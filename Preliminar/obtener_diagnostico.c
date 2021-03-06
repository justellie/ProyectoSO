#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "definiciones.h"

TipoAtencion obtener_diagnostico_simple()
{
    srand(time(NULL));
    return rand()% 2;
}

TipoAtencion obtener_diagnostico_compuesta(void *paciente)
{
    Paciente *atendiendo=(Paciente *)paciente;
    int diagnostico_nuevo=0;
    

    srand(time(NULL));
    diagnostico_nuevo= rand()% 5;

    diagnostico_nuevo=atendiendo->servicio*0.5+diagnostico_nuevo*0.5;
    diagnostico_nuevo=(diagnostico_nuevo-2)/2;
    return diagnostico_nuevo;

}
int main(int argc, char const *argv[])
{
    int respuesta;
    for (int i = 0; i < 80; i++)
    {
        respuesta=obtener_diagnostico_simple();
        printf("valor: %d \n", respuesta);
        sleep(2);
    }
    
    return 0;
}
