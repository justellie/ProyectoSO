#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct _argsPacientes  { //agregamos 2 arrays para almacenar los id de los medicos y enfermeras que estan asignados a cada paciente
    int vivo;
    int idHospital;
    int reposo_en_casa;
    int enfermera[3]; // Inicializar en -1 en otra parte
    int medicos[3]; // Inicializar en -1 en otra parte
} argsPaciente;

typedef struct _hospital
{
    //Identificacion del hospital (1-general, 2-intermedio, 3-centinela)
    int tipoH;
    // [T] Zona de Triaje -----------------------
    // Es un número fijo para cada hospital:
    sem_t salaMuestra; // -med. tomando pruebas(5)
    sem_t salaEspera;  // +sillas de espera(20)

    // [H] Hospital --------------------------------
    // El número de camas puede
    // variar con el tiempo.
    sem_t camasHospital;
    // Siempre se piden estos recursos:
    sem_t oxigeno;
    sem_t respirador;
    // El número de medicos y pacientes
    // varía entre hospitales:
    // Creo que no deben ser arreglos:
    sem_t enfermeras;   
    sem_t medicos;
    sem_t voluntarios;

    //contador de disponibilidad de las enfemeras y medicos
    int num_enfermeras;//es necesario saber el numero de enfermeras, esto es provicional
    int num_medicos; 
    int contadorEnfermeras[6];//esto debe ser inicializado en 4
    int contadorMedicos[6];



} hospital;

enum cama {ninguno,en_casa,basica,intensiva,muerto};

hospital H[3];

/*
para asignarle enfermeras a un paciente se debe saber:
-el tipo de antencion que esta recibiendo (basica, intensiva)
-en caso de que la atencion sea intensiva se debe saber el tipo de hospital en el que se encuentra (centinela, intermedio, general)
candidad de enfermeras por paciente segun sus necesidades
+++++++++++++++++++++++++++++++++++++++++++++++
|   cama    | centinela | intermedio | general|
|---------------------------------------------|
|basica     | 0.25      | 0.25       | 0.25   |
|intensiva  | 3         | 2          | 1      |
+++++++++++++++++++++++++++++++++++++++++++++++
*/

void * ir(void *);
void paciente();
int reservarEnfermera(int , enum cama , int []);
int liberarEnfermera(int , enum cama , int []);

int main(int argc, char const *argv[]) //se inicializan los valores necesarios para el hospital en especifico
{
    for(int i = 0; i < 6; i++) H[0].contadorEnfermeras[i] = 4;
    H[0].tipoH = 3;
    H[0].num_enfermeras = 6;
    sem_init(&H[0].enfermeras, 0, 1);
    paciente();

    return 0;
}

void paciente() //esto solo es necesario ya que se esta haciendo a parte del codigo central, deberia funcionar la funcion reservarEnfermera por su cuenta
{
    pthread_t hilo;
    void * exit_status;
    int j;
    argsPaciente args = { 0 };
    args.vivo = 1;
    args.reposo_en_casa = 0;
    args.idHospital = 0;
    for(int i = 0; i < 3; i++) args.enfermera[i] = -1;
    pthread_create(&hilo, NULL, ir, &args);
    pthread_join(hilo, &exit_status); //usado para comprobar el funcionamineto
    j= (int)exit_status;
    printf("%d", j);

}

void *ir(void *input) //esto solo es necesario ya que se esta haciendo a parte del codigo central, deberia funcionar la funcion reservarEnfermera por su cuenta
{
    argsPaciente* usr = (argsPaciente*) input;
    int funciono = reservarEnfermera(0,intensiva,usr->enfermera);
    sleep(10);
    int funciono2 = liberarEnfermera(0, intensiva, usr->enfermera);
    if (funciono2 == 1) //usado para comprobar el funcionamineto
    {
        return (void *)1;
    }
    return (void *)0;
}

/*
la funcion retorna un entero como indicativo de exito o fallo
si el valor de retorno es 1 el paciente reservo con exito las enfermeras que ameritaba
si el valor de retorno es -1 el paciente no encontro enfermeras suficientes para su atencion
(se debe decidir que hacer con los pacientes cuando no hay enfermeras suficientes)
la funcion bloquea el acceso al array general de enfermeras del hospital con un semaforo binario,
busca en el array respecto a lo que requiera el paciente, si hay disponibles, se le asignan al paciente.
para la funcion se necesita como parametro el id del hospital en el que se encuentra el paciente,
el tipo de cama en la que se encuentra el paciente y un array de 3, en el cual se guardaran los id de las
enfermeras asignadas a cada paciente
int reservarEnfermera(int , enum cama, int [])
*/

int reservarEnfermera(int hospital, enum cama tipo_cama, int enfermera[]){

    switch (H[hospital].tipoH)
    {
    case 1://general
        if (tipo_cama == basica)
        {//paciente que se encuentra en un hospital general y en una cama basica, se le asignara el 0.25% de una enfermera
            sem_wait(&H[hospital].enfermeras);
                for (int i = 0; i < H[hospital].num_enfermeras; i++)
                {
                    if(H[hospital].contadorEnfermeras[i] != 0)
                    {
                        H[hospital].contadorEnfermeras[i] -= 1;
                        enfermera[0]= i;
                        break;
                    }

                }
            sem_post(&H[hospital].enfermeras);
            if (enfermera[0]==-1)
                {
                    return -1;
                }
            return 1;

        }
        else//intesivo
        {//paciente que se encuentra en un hospital general y en una cama intensiva, se le asignara una enfermera
            sem_wait(&H[hospital].enfermeras);
                for (int i = 0; i < H[hospital].num_enfermeras; i++)
                {
                    if(H[hospital].contadorEnfermeras[i] == 4)
                    {
                        H[hospital].contadorEnfermeras[i] = 0;//se le asigna 0, en caso de que sea asiganda
                        enfermera[0]= i;
                        break;
                    }

                }
            sem_post(&H[hospital].enfermeras);
            if (enfermera[0]==-1)
            {
                return -1;
            }
            return 1;
        }

        break;
    case 2://intermedio
        if (tipo_cama == basica)
        {//paciente que se encuentra en un hospital intermedio y en una cama basica, se le asignara el 0.25% de una enfermera
            sem_wait(&H[hospital].enfermeras);
                for (int i = 0; i < H[hospital].num_enfermeras; i++)
                {
                    if(H[hospital].contadorEnfermeras[i] != 0)
                    {
                        H[hospital].contadorEnfermeras[i] -= 1;
                        enfermera[0]= i;
                        break;
                    }

                }
            sem_post(&H[hospital].enfermeras);
            if (enfermera[0]==-1)
                {
                    return -1;
                }
            return 1;

        }
        else//intensivo
        {//paciente que se encuentra en un hospital intermedio y en una cama intensiva, se le asignaran 2 enfermeras
            int contador_enf=0;
            sem_wait(&H[hospital].enfermeras);
                for (int i = 0; i < H[hospital].num_enfermeras; i++)
                {
                    if(H[hospital].contadorEnfermeras[i] == 4)
                    {
                        if (contador_enf != 2)
                        {
                            enfermera[contador_enf] = i;
                            contador_enf++;
                        }
                        else
                        {
                            H[hospital].contadorEnfermeras[enfermera[0]] = 0;
                            H[hospital].contadorEnfermeras[enfermera[1]] = 0;
                            break;
                        }
                    }
                }
            sem_post(&H[hospital].enfermeras);
            if (contador_enf<2)//en caso de que no haya suficientes enfermeras vuele a vaciar el array
                {
                    enfermera[0]=-1;
                    return -1;
                }
            return 1;
        }
        break;

    case 3://centinela 
        if (tipo_cama == basica)
        {//paciente que se encuentra en un hospital centinela y en una cama basica, se le asignara el 0.25% de una enfermera
            sem_wait(&H[hospital].enfermeras);
                for (int i = 0; i < H[hospital].num_enfermeras; i++)
                {
                    if(H[hospital].contadorEnfermeras[i] != 0)
                    {
                        H[hospital].contadorEnfermeras[i] -= 1;
                        enfermera[0]= i;
                        break;
                    }

                }
            sem_post(&H[hospital].enfermeras);
            if (enfermera[0]==-1)
                {
                    return -1;
                }
            return 1;

        }
        else//intesivo
        {//paciente que se encuentra en un hospital centinela y en una cama intensiva, se le asignaran 3 enfermeras
            int contador_enf=0;
            sem_wait(&H[hospital].enfermeras);
                for (int i = 0; i < H[hospital].num_enfermeras; i++)
                {
                    if(H[hospital].contadorEnfermeras[i] == 4)
                    {
                        if (contador_enf != 3)
                        {
                            enfermera[contador_enf] = i;
                            contador_enf++;
                        }
                        else
                        {
                            H[hospital].contadorEnfermeras[enfermera[0]] = 0;
                            H[hospital].contadorEnfermeras[enfermera[1]] = 0;
                            H[hospital].contadorEnfermeras[enfermera[2]] = 0;
                            break;
                        }
                    }
                }
            sem_post(&H[hospital].enfermeras);
            if (contador_enf<3)//en caso de que no haya suficientes enfermeras vuele a vaciar el array
                {
                    enfermera[0]=-1;
                    enfermera[1]=-1;
                    return -1;
                }
            return 1;
        }
        break;
    default:
        return -1;
        break;
    }

}

int liberarEnfermera(int hospital, enum cama tipo_cama, int enfermera[])
{
    switch (H[hospital].tipoH)
    {
    case 1://general
        if (tipo_cama == basica)
        {
            if (enfermera[0] == -1)
            {
                return -1;
            }
            else
            {
                sem_wait(&H[hospital].enfermeras);
                    H[hospital].contadorEnfermeras[enfermera[0]] += 1;
                sem_post(&H[hospital].enfermeras);
                enfermera[0] = -1;
                return 1;
            }
            
        }
        else//intesivo
        {
            if (enfermera[0] == -1)
            {
                return -1;
            }
            else
            {
                sem_wait(&H[hospital].enfermeras);
                    H[hospital].contadorEnfermeras[enfermera[0]] = 4;
                sem_post(&H[hospital].enfermeras);
                enfermera[0] = -1;
                return 1;
            }
        }
        break;

    case 2://intermedio
        if (tipo_cama == basica)
        {
            if (enfermera[0] == -1)
            {
                return -1;
            }
            else
            {
                sem_wait(&H[hospital].enfermeras);
                    H[hospital].contadorEnfermeras[enfermera[0]] += 1;
                sem_post(&H[hospital].enfermeras);
                enfermera[0] = -1;
                return 1;
            }
        }
        else//intensivo
        {
            if (enfermera[0] == -1)
            {
                return -1;
            }
            else
            {
                sem_wait(&H[hospital].enfermeras);
                    H[hospital].contadorEnfermeras[enfermera[0]] = 4;
                    H[hospital].contadorEnfermeras[enfermera[1]] = 4;
                sem_post(&H[hospital].enfermeras);
                enfermera[0] = -1;
                enfermera[1] = -1;
                return 1;
            }
        }
        break;

    case 3://centinela 
        if (tipo_cama == basica)
        {
            if (enfermera[0] == -1)
            {
                return -1;
            }
            else
            {
                sem_wait(&H[hospital].enfermeras);
                    H[hospital].contadorEnfermeras[enfermera[0]] += 1;
                sem_post(&H[hospital].enfermeras);
                enfermera[0] = -1;
                return 1;
            }
        }
        else//intesivo
        {
            if (enfermera[0] == -1)
            {
                return -1;
            }
            else
            {
                sem_wait(&H[hospital].enfermeras);
                    H[hospital].contadorEnfermeras[enfermera[0]] = 4;
                    H[hospital].contadorEnfermeras[enfermera[1]] = 4;
                    H[hospital].contadorEnfermeras[enfermera[2]] = 4;
                sem_post(&H[hospital].enfermeras);
                enfermera[0] = -1;
                enfermera[1] = -1;
                enfermera[2] = -1;
                return 1;
            }
        }
        break;
    default:
        return -1;
        break;
    }
}
