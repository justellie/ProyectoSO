#ifndef __ACTORES_H__
#define __ACTORES_H__

#include "definiciones.h"
// Para propósitos de legibilidad, los hilos son nombrados como 'actores' ya que realizan
// diversas tareas dentro del programa.
//
// Cada identificador de un hilo, debe comenzar con 'actor_' seguido del nombre de la tarea
// que debe realizar.
// NOTE: SE DEBE USAR snake_case PARA CADA ACTOR.

void actor_paciente( void* datos_iniciales );

void actor_inventario_ugc( void* inv_ugc );
void actor_personal_ugc  ( void* inv_ugc );
// daemon. Estadísticas, inventario, uso, resumen de todo, etc.
void actor_status_ugc    ( void* inv_ugc );

void actor_jefe_admin   ( void* hospital );
void actor_jefe_epidemia( void* hospital );
void actor_director     ( void* hospital );
void actor_analista     ( void* hospital );

// Revisan la cola de pacientes interna, 
void actor_voluntario( void* datos_voluntario );
void actor_gestor    ( void* hospital );

// ---------------------------- RELACIONES ENTRE ACTORES -----------------------------------------
// actor_paciente       --> actor_analista, actor_gestor, actor_inventario_ugc*, actor_voluntario*
// actor_voluntario     --> actor_paciente
//
// actor_inventario_ugc --> actor_director, actor_jefe_admin, actor_paciente*, actor_gestor
// actor_personal_ugc   --> actor_jefe_admin, actor_jefe_admin
// actor_status_ugc     --> todos
//
// actor_jefe_admin     --> actor_gestor, actor_personal_ugc
// actor_jefe_apidemia  --> actor_analista, actor_status_ugc
// actor_analista       --> actor_paciente, actor_jefe_epidemia
// actor_gestor         --> actor_paciente* , actor_inventario_ugc
//
// -----------------------------------------------------------------------------------------------

// ...
// NOTE(sGaps): Más adelante dejaré únicamente los tipos de datos en un archivo que deberá ser citado
//              por los demás archivos fuentes. Se llamará:
//                  definiciones.h
//
//              Los hilos deben implementarse en archivos diferentes para no romper accidentalmente
//              las demás sub-rutinas.
//                  suponiendo que el nombre del hilo a implementar es el símbolo '@',
//                  entonces se tendría que crear el archivo.
//                      'actor_@.c'
//              Las declaraciones de los hilos estarán más adelante en el archivo actores.h, el cual
//              debe ser incluído (#include "actores.h") por cada 'actor_@.c'
//              

// ---- ACERCA DE LAS TRANSFERENCIAS ----
// NOTE(sGaps): Llegará un momento donde los pacientes y el personal son transferidos
//              A otros centros hospitalarios.
//
//              Para el caso del Personal, sólo se podrá extraer aquellos que tengan el
//              mayor nivel de disponibilidad. Es decir, aquellos totalmente desocupados.
//              

//              IDEA(sGaps): Para actualizar la información de un Paciente, se necesita los permisos de
//                           escritura (rwlock)

// NOTE(sGaps): Si un hospital requiere de más personal, se lo pide a la UGC mediante un
//              mensaje (request) que tenga información del emisor y qué necesita.
//              quien gestione el inventario buscará entre los demás hospitales hasta conseguir
//              alguno que tenga suficientes recursos.
//

// NOTE(sGaps): La transferencia del paciente inicia cuando el Gestor revisa el estado del paciente
//              (a través del personal asignado) y detecta que no hay recursos para transferirlo.
//              Éste procederá a llevarlo a la UGC.
//
//              la UGC se encargará de buscar algún hospital que tenga espacio para el paciente, para
//              ello, realiza una reservación (tomando en cuenta el tipo de atención que necesita) en
//              algún hospital.
//
//              Si la operación anterior es exitosa, se actualizarán los datos del paciente y se procederá
//              a liberar al personal del hospital anterior junto a los recursos empleados. En caso
//              contrario, no será posible liberar sus recursos del hospital.
//                  CABE DESTACAR QUE: La UGC toma el control del paciente. por lo tanto, el gestor de cama
//                                     colocará al paciente dentro de la cola de atención de pacientes de
//                                     la UGC.
//

// ---- RESPONSABILIDADES DEL PACIENTE ----
// [Trayecto al hospital]
// 1. Ir al hospital
// 2. Esperar su turno en la cola
// 3. Elegir alguna sala de muestra.
// 4. Notificar cuándo está listo para que le tomen la muestra.
// 5. Esperar a que terminen de tormar la muestra.
// 6. Notificar cuándo sale de la silla.
// 7. Esperar por el servicio en el hospital.
//      (Esto implica transferencias implícitas entre hospitales)
// 9. Salir del Hospital
//
// [De Alta del hospital]
// 1. Esperar a estar enfermo.
//
// [Cuidado en Casa]
// 1. Esperar a estar curado.
// 2. Esperar a estar enfermo.
//
// [Morir]
// 1. Finaliza el hilo
//

// ---- RESPONSABILIDADES DEL ANALISTA ----
// 1. Esperar por silla ocupada
// 2. Tomar muestra
//      NOTIFICAR: cuando no hay material para muestras ni PCRs.
// 4. Dejar la muestra en un lugar accesible para el jefe de epidemiologia
// 3. Notificar muestra lista

// ---- RESPONSABILIDADES DEL JEFE DE EPIDEMIOLOGIA ----
// 1. Esperar algún resultado de una muestra tomada
// 2. Actualizar estadísticas.
// 3. Comprobar si no hay riesgo (misceláneo)
//

// ---- RESPONSABILIDADES DEL GESTOR DE CAMAS ----
// 1. Esperar que algún paciente necesite de atención. (alguno en la cola)
// 2. Diagnóstico según estado del paciente.
// 3. Actualizar días de estancia.
// 4. Reservar cama si necesita:
//    SI TIENE:
//      a. reservar cama, recursos y personal dentro del mismo hospital.
//      b. liberar recursos, cama y personal anterior.
//      ERROR -> pedir personal/recursos ó pedir transferencia [no fue posible atenderlo].
//    NO TIENE
//      a. reservar cama, recursos y personal dentro del mismo hospital.
//      ERROR -> pedir personal/recursos transferencia [no fue posible atenderlo]
//    NO NECESITA
//      a. liberar recursos, cama y personal anterior.
// 5. Pasar al siguiente paciente.
//

// ---- RESPONSABILIDADES DEL INVENTARIO UGC ----
// 1. Esperar por alguna petición de recursos (según descripción)
// 2. Atender petición:     (emisor: idHospital, recurso: físico o paciente, ...)
//      Si no hay recursos en la UGC:
//          traer recursos de otros hospitales
// 3. Brindar recursos al emisor.
//

// ---- RESPONSABILIDADES DEL PERSONAL UGC ----
// 1. Esperar por alguna petición de personal (según descripción)
// 2. Atender petición:     (emisor: idHospital, Tipo: físico o paciente, ...)
//      Si no hay personal en la UGC:
//          traer personal de otros hospitales.
// 3. Brindar personal al emisor.
//

// ---- RESPONSABILIDADES DEL STATUS UGC ----
// 1. Esperar a la hora de actualizacion
// 2. Escribir datos diarios
// 3. Reportar en pantalla
// NOTE: Debe realizarse en horas específicas, 2 veces al día. DEBE INTERRUMPIR
//       CUALQUIER OTRA TAREA QUE SE ESTÉ REALIZANDO EN ESE MOMENTO.

// ---- RESPONSABILIDADES DEL JEFE DE EPIDEMIOLOGIA ----
// 1. Esperar algún resultado de una muestra tomada
// 2. Actualizar estadísticas del hospital
// 3. Tomar medidas en caso de COVID
//

// ---- RESPONSABILIDADES DEL JEFE ADMINISTRATIVO ----
// 1. Esperar a que falte personal.
// 2. Pedir personal
// 3. Actualizar estadísticas del personal/recursos (CADA 12 horas)

// ---- RESPONSABILIDADES DEL DIRECTOR ----
// 0. Inicializar Hospital
// 1. Esperar a que falten pruebas.
// 2. Pedir pruebas a la UGC y recursos.
//
// 3.* Notificar a la UGC cuando un paciente necesita atención en casa (voluntario) a la UGC.

// ---- RESPONSABILIDADES DEL VOLUNTARIO ----
// 1. Esperar a que algún paciente(en Casa) requiera de atención.
// 2. Tomar diagnóstico.
// 3. Acción según diagnóstico
//          -> transferir a hospital.
//          -> dar de alta.
//          -> reportar fallecido.
// 4. Atender al siguiente paciente.
//

#endif
