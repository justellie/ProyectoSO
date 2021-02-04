// las funciones para el proyecto son reservarPersonal y liberarPersonal, todo lo demas esta con motivo de probar el funcionamiento 
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
la asignacion de personal se hace respecto a el tipo de hospital y el tipo de tratamiento que requiere el paciente
+++++++++++++++++++++++++++++++++++++++++++++++
|   cama    | centinela | intermedio | general|
|---------------------------------------------|
|basica     | 0.25      | 0.25       | 0.25   |
|intensiva  | 3         | 2          | 1      |
+++++++++++++++++++++++++++++++++++++++++++++++
*/

typedef struct _argsPacientes  { //agregamos 2 arrays para almacenar los id de los medicos y aPersonals que estan asignados a cada paciente
    int vivo;
    int idHospital;
    int reposo_en_casa;
    //personal hospital
    sem_t sEnfermera;
    int enfermera[3]; // Inicializar en -1 en otra parte
    sem_t sMedico;
    int medico[3]; // Inicializar en -1 en otra parte
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
    sem_t aPersonals;   
    sem_t medicos;
    sem_t voluntarios;

    //contador de disponibilidad de las enfemeras y medicos
    int num_enfermeras;//es necesario saber el numero de aPersonals
    int num_medicos; 
    int contadoraPersonals[6];//esto debe ser inicializado en 4
    int contadorMedicos[6];
} hospital;

enum cama {ninguno,en_casa,basica,intensiva,muerto};

hospital H[3];

void * ir(void *);
void paciente();
//reservarPersonal(tipo_de_hospital, tipo_de_cama, semaforo_del_array_paciente, array_del_paciente, semaforo_correspondiente, array_hospital, numero_personal_correspodiente)
int reservarPersonal(int, enum cama, sem_t *,int [], sem_t *, int [], int);
//liberarPersonal(tipo_de_hospital, tipo_de_cama, semaforo_del_array_paciente, array_del_paciente, semaforo_correspondiente, array_hospital)
int liberarPersonal(int, enum cama, sem_t *, int [], sem_t *, int []);

int main(int argc, char const *argv[]) //se inicializan los valores necesarios para el hospital en especifico
{
    for(int i = 0; i < 6; i++) H[0].contadoraPersonals[i] = 4;
    for(int i = 0; i < 6; i++) H[0].contadorMedicos[i] = 4;
    H[0].tipoH = 3;
    H[0].num_enfermeras = 6;
    H[0].num_medicos = 6;
    sem_init(&H[0].aPersonals, 0, 1);
    sem_init(&H[0].medicos, 0, 1);
    paciente();

    return 0;
}

void paciente() //esto solo es necesario ya que se esta haciendo a parte del codigo central, deberia funcionar la funcion reservaraPersonal por su cuenta
{
    pthread_t hilo;
    void * exit_status;
    int j;
    argsPaciente args = { 0 };
    args.vivo = 1;
    args.reposo_en_casa = 0;
    args.idHospital = 0;
    sem_init(&args.sMedico, 0, 1);
    sem_init(&args.sEnfermera, 0, 1);
    for(int i = 0; i < 3; i++) args.enfermera[i] = -1;
    for(int i = 0; i < 3; i++) args.medico[i] = -1;
    pthread_create(&hilo, NULL, ir, &args);
    pthread_join(hilo, &exit_status); //usado para comprobar el funcionamineto
    j= (int)exit_status;
    printf("%d", j);

}

void *ir(void *input) //esto solo es necesario ya que se esta haciendo a parte del codigo central, deberia funcionar la funcion reservaraPersonal por su cuenta
{
    argsPaciente* usr = (argsPaciente*) input;
    //reservacion del personal (enfermeras/medicos)
    int funciono = reservarPersonal(H[0].tipoH,intensiva, &usr->sEnfermera, usr->enfermera, &H[0].aPersonals, H[0].contadoraPersonals, H[0].num_enfermeras);
    int funciono2 = reservarPersonal(H[0].tipoH,intensiva, &usr->sMedico, usr->medico, &H[0].medicos, H[0].contadorMedicos, H[0].num_medicos);
    //liberacion del personal (enfermeras/medicos)
    int funciono3 = liberarPersonal(H[0].tipoH,intensiva, &usr->sEnfermera, usr->enfermera, &H[0].aPersonals, H[0].contadoraPersonals);
    int funciono4 = liberarPersonal(H[0].tipoH,intensiva, &usr->sMedico, usr->medico, &H[0].medicos, H[0].contadorMedicos);
    //usado para comprobar el funcionamineto
    if (funciono == 1) 
    {
        return (void *)1;
    }
    return (void *)0;
}

/*
la funcion retorna un entero como indicativo de exito o fallo
si el valor de retorno es 1 el paciente reservo con exito el personal que requeria
si el valor de retorno es -1 el paciente no encontro personal disponible o no tiene un tipo de hospital valido
(se debe decidir que hacer con los pacientes cuando no hay suficiente personal)
la funcion bloquea el acceso al array general del personal requerido (enfermeras/medicos) del hospital con un semaforo binario,
busca en el array respecto a lo que requiera el paciente, si hay disponibles, se le asignan al paciente.
para la funcion se necesita como parametros: 
    +El tipo de hospital en el que se encuentra el paciente
    +El tipo de cama en la que se encuentra el paciente
    +Un semaforo binario para controlar el acceso al array del paciente
    +Un array de 3, en el cual se guardaran los id del personal asignados a cada paciente 
        (deben ser array independientes para cada tipo de personal)
    +Puntero al semaforo especifico del personal que se desea reservar
    +Array del personal requerido del hospital en el que se encuentra el paciente
    +El numero total del personal especifico

int reservarPersonal(int, enum cama, sem_t*, int [], sem_t *, int [], int);
*/

int reservarPersonal(int tipoH, enum cama tipo_cama, sem_t *sPersonal, int aPersonal[], sem_t *sContadorPersonal, int contadorPersonal[], int numero){
    int resultado, fallo =-1, exito=1 ;
    resultado=exito;
    switch (tipoH)
    {
    case 1://general
        if (tipo_cama == basica)
        {//paciente que se encuentra en un hospital general y en una cama basica, se le asignara el 0.25% del personal requerido
            sem_wait(sContadorPersonal);
            sem_wait(sPersonal);
                for (int i = 0; i < numero; i++)
                {
                    if(contadorPersonal[i] != 0)
                    {
                        contadorPersonal[i] -= 1;                        
                        aPersonal[0]= i;                        
                        break;
                    }
                }
            sem_post(sContadorPersonal);
            if (aPersonal[0]==-1) resultado = fallo;
            sem_post(sPersonal);
            return resultado;

        }
        else//intesivo
        {//paciente que se encuentra en un hospital general y en una cama intensiva, se le asignara 1 del personal requerido
            sem_wait(sContadorPersonal);
            sem_wait(sPersonal);
                for (int i = 0; i < numero; i++)
                {
                    if(contadorPersonal[i] == 4)
                    {
                        contadorPersonal[i] = 0;//se le asigna 0, en caso de que se reserve el personal                        
                        aPersonal[0]= i;                        
                        break;
                    }
                }
            sem_post(sContadorPersonal);
            if (aPersonal[0]==-1) resultado = fallo;
            sem_post(sPersonal);
            return resultado;
        }

        break;
    case 2://intermedio
        if (tipo_cama == basica)
        {//paciente que se encuentra en un hospital intermedio y en una cama basica, se le asignara el 0.25% del personal requerido
            sem_wait(sContadorPersonal);
            sem_wait(sPersonal);
                for (int i = 0; i < numero; i++)
                {
                    if(contadorPersonal[i] != 0)
                    {
                        contadorPersonal[i] -= 1;                        
                        aPersonal[0]= i;                        
                        break;
                    }
                }
            sem_post(sContadorPersonal);            
            if (aPersonal[0]==-1) resultado = fallo;
            sem_post(sPersonal);
            return resultado;

        }
        else//intensivo
        {//paciente que se encuentra en un hospital intermedio y en una cama intensiva, se le asignaran 2 de personal requerido
            int contador_per=0;
            sem_wait(sContadorPersonal);
            sem_wait(sPersonal);
                for (int i = 0; i < numero; i++)
                {
                    if(contadorPersonal[i] == 4)
                    {
                        if (contador_per != 2)
                        {
                            aPersonal[contador_per] = i;
                            contador_per++;
                        }
                        else
                        {
                            contadorPersonal[aPersonal[0]] = 0;
                            contadorPersonal[aPersonal[1]] = 0;
                            break;
                        }
                    }
                }
            sem_post(sContadorPersonal);
            if (contador_per<2)//en caso de que no haya suficiente personal vuele a vaciar el array del paciente
                {
                    aPersonal[0]=-1;
                    resultado = fallo;
                }
            sem_post(sPersonal);
            return resultado;
        }
        break;

    case 3://centinela 
        if (tipo_cama == basica)
        {//paciente que se encuentra en un hospital centinela y en una cama basica, se le asignara el 0.25% del personal requerido
            sem_wait(sContadorPersonal);
            sem_wait(sPersonal);
                for (int i = 0; i < numero; i++)
                {
                    if(contadorPersonal[i] != 0)
                    {
                        contadorPersonal[i] -= 1;                        
                        aPersonal[0]= i;                        
                        break;
                    }
                }
            sem_post(sContadorPersonal);
            if (aPersonal[0]==-1) resultado = fallo;
            sem_post(sPersonal);
            return resultado;

        }
        else//intesivo
        {//paciente que se encuentra en un hospital centinela y en una cama intensiva, se le asignaran 3 del personal requerido
            int contador_per=0;
            sem_wait(sContadorPersonal);
            sem_wait(sPersonal);
                for (int i = 0; i < numero; i++)
                {
                    if(contadorPersonal[i] == 4)
                    {
                        if (contador_per != 3)
                        {

                            aPersonal[contador_per] = i;
                            contador_per++;
                        }
                        else
                        {
                            contadorPersonal[aPersonal[0]] = 0;
                            contadorPersonal[aPersonal[1]] = 0;
                            contadorPersonal[aPersonal[2]] = 0;
                            break;
                        }
                    }
                }
            sem_post(sContadorPersonal);
            if (contador_per<3)//en caso de que no haya suficiente personal vuele a vaciar el array del paciente
                {
                    aPersonal[0]=-1;
                    aPersonal[1]=-1;
                    resultado = fallo;
                }
            sem_post(sPersonal);
            return resultado;
        }
        break;
    default:
        return fallo;
        break;
    }
}

/*
la funcion retorna un entero como indicativo de exito o fallo
si el valor de retorno es 1, se libero con exito el personal que se encontraba reservado
si el valor de retorno es -1 el paciente no posee personal reservado, o no tiene un tipo de hospital valido
la funcion bloquea el acceso al array general del personal requerido (enfermeras/medicos) del hospital con un semaforo binario
para la funcion se necesita como parametros: 
    +El tipo de hospital en el que se encuentra el paciente
    +El tipo de cama en la que se encuentra el paciente
    +Un semaforo binario para controlar el acceso al array del paciente
    +Un array de 3, en el cual se guarda los id del personal asignados a cada paciente 
        (deben ser array independientes para cada tipo de personal)
    +Puntero al semaforo especifico del personal que se desea liberar
    +Array de personal requerido del hospital en el que se encuentra el paciente
*/

int liberarPersonal(int tipoH, enum cama tipo_cama, sem_t *sPersonal, int aPersonal[], sem_t *sContadorPersonal, int contadorPersonal[])
{
    switch (tipoH)
    {
    case 1://general
        if (tipo_cama == basica)
        {
            if (aPersonal[0] == -1)
            {
                return -1;
            }
            else
            {
                sem_wait(sContadorPersonal);
                    contadorPersonal[aPersonal[0]] += 1;
                sem_post(sContadorPersonal);
                sem_wait(sPersonal);
                aPersonal[0] = -1;
                sem_post(sPersonal);
                return 1;
            }
            
        }
        else//intesivo
        {
            if (aPersonal[0] == -1)
            {
                return -1;
            }
            else
            {
                sem_wait(sContadorPersonal);
                    contadorPersonal[aPersonal[0]] = 4;
                sem_post(sContadorPersonal);
                sem_wait(sPersonal);
                aPersonal[0] = -1;
                sem_post(sPersonal);
                return 1;
            }
        }
        break;

    case 2://intermedio
        if (tipo_cama == basica)
        {
            if (aPersonal[0] == -1)
            {
                return -1;
            }
            else
            {
                sem_wait(sContadorPersonal);
                    contadorPersonal[aPersonal[0]] += 1;
                sem_post(sContadorPersonal);
                sem_wait(sPersonal);
                aPersonal[0] = -1;
                sem_post(sPersonal);
                return 1;
            }
        }
        else//intensivo
        {
            if (aPersonal[0] == -1)
            {
                return -1;
            }
            else
            {
                sem_wait(sContadorPersonal);
                    contadorPersonal[aPersonal[0]] = 4;
                    contadorPersonal[aPersonal[1]] = 4;
                sem_post(sContadorPersonal);
                sem_wait(sPersonal);
                aPersonal[0] = -1;
                aPersonal[1] = -1;
                sem_post(sPersonal);
                return 1;
            }
        }
        break;

    case 3://centinela 
        if (tipo_cama == basica)
        {
            if (aPersonal[0] == -1)
            {
                return -1;
            }
            else
            {
                sem_wait(sContadorPersonal);
                    contadorPersonal[aPersonal[0]] += 1;
                sem_post(sContadorPersonal);
                sem_wait(sPersonal);
                aPersonal[0] = -1;
                sem_post(sPersonal);
                return 1;
            }
        }
        else//intesivo
        {
            if (aPersonal[0] == -1)
            {
                return -1;
            }
            else
            {
                sem_wait(sContadorPersonal);
                    contadorPersonal[aPersonal[0]] = 4;
                    contadorPersonal[aPersonal[1]] = 4;
                    contadorPersonal[aPersonal[2]] = 4;
                sem_post(sContadorPersonal);
                sem_wait(sPersonal);
                aPersonal[0] = -1;
                aPersonal[1] = -1;
                aPersonal[2] = -1;
                sem_post(sPersonal);
                return 1;
            }
        }
        break;
    default:
        return -1;
        break;
    }
}