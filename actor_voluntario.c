// ---- RESPONSABILIDADES DEL VOLUNTARIO ----
// 1. Esperar a que algún paciente(en Casa) requiera de atención.
// 2. Tomar diagnóstico.
// 3. Acción según diagnóstico
//          -> transferir a hospital.
//          -> dar de alta.
//          -> reportar fallecido.
// 4. Atender al siguiente paciente.

#include "actores.h"
#include "definiciones.h"

extern UGC      gestor_central;
extern RefQueue pacienteEnCasa;
///@fn void actor_voluntario(void* datos_voluntario)
///@brief funcion que ejecuta el actor voluntario para realizar sus funciones
///@param datos_voluntario estructura que contiene los datos basicos de un voluntario
void* actor_voluntario(void* datos_voluntario)
{
	
	Voluntario *datos = (Voluntario *) datos_voluntario;
	TipoAtencion diagAct;
	while(true)
	{
		Paciente *atendiendo = refqueue_get(&pacienteEnCasa);
		diagAct = obtener_diagnostico_compuesta(atendiendo);
		atendiendo->servicio=diagAct;

		switch(diagAct)
		{
			case EnCasa:
			{
				refqueue_put(&pacienteEnCasa, atendiendo);
				break;
			}

			case Ninguno:
			{
				atendiendo->deAlta++;
				break;
			}

			case Muerto:
			{
				atendiendo->vivo=0;
				break;
			}
			
			default: // Ingresa de nuevo al hospital
			{
				atendiendo->ingresando = 1;
				refqueue_put(&gestor_central.pacientes, atendiendo);
				break;
			}
			
		}

        atendiendo->fueAtendido++;
		pthread_cond_signal(&atendiendo->atendido);
	}

    return NULL;
}
