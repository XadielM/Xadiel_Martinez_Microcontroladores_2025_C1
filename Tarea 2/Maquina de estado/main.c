#include <stdio.h>
#include <stdlib.h>
#include <windows.h> //Para la funcion de sleep

//Definicion de estados
#define ESTADO_INICIAL 0
#define ESTADO_ERROR 1
#define ESTADO_ABRIENDO 2
#define ESTADO_CERRANDO 3
#define ESTADO_ABIERTO 4
#define ESTADO_CERRADO 5
#define ESTADO_DETENIDA 6
#define ESTADO_DESCONOCIDO 7

//Definicion de Limit Switch
#define LM_ACTIVO 1
#define LM_NOACTIVO 0

//Definicion de Motor
#define MOTOR_OFF 0
#define MOTOR_ON 1

//Definicion de Lamp
#define LAMP_OFF 0
#define LAMP_ON 1

//Definicion de Funcion detenido
#define FD_CIERRA 1
#define FD_ESPERA 2
#define FD_ABRIR 3
#define FD_PARPADEAR 4

#define PP_NOACTIVO 0
#define PP_ACTIVO 1

//Tiempo de cierre automatico de 10 segundos
#define TIME_CA 10

int Func_ESTADO_INICIAL(void);
int Func_ESTADO_ERROR(void);
int Func_ESTADO_ABRIENDO(void);
int Func_ESTADO_CERRANDO(void);
int Func_ESTADO_ABIERTO(void);
int Func_ESTADO_CERRADO(void);
int Func_ESTADO_DETENIDA(void);
int Func_ESTADO_DESCONOCIDO(void);


int ESTADO_SIGUIENTE = ESTADO_INICIAL;
int ESTADO_ANTERIOR = ESTADO_INICIAL;
int ESTADO_ACTUAL = ESTADO_INICIAL;

//Comportamiento de los DS

/*Dip switch
1-1 desconocido a cerrar
1-0 desconocido a esperar comando
0-1 desconocido a abrir
0-0 desconocido a parpadear

ds_fpp
0 - Seguir en estado antes de parar
1 - El estado contrario al cual estaba

*/

struct SYSTEM_IO
{
    unsigned int lsc:1;//Limit switch cerrado
    unsigned int lsa:1;//Limit switch abierto
    unsigned int ftc:1; //Foto celda
    unsigned int ma:1; //Motor abrir
    unsigned int mc:1; //Motor cerrar
    unsigned int lamp:1;//Lampara
    unsigned int keya:1; //Llave abrir
    unsigned int keyc:1; //Llave cerrar
    unsigned int pp:1;
    unsigned int ds_fd:2;//Dip switch dos pines, funcion desconocida
    unsigned int ds_fpp:1;

} io;

struct SYSTEM_CONFIG
{
    unsigned int cnt_TCA; //Contador en segundos para cierre automatico
    unsigned int cnt_RT; //Contador en segundos de tiempo maximo de movimiento del motor
    int CONFIG_FD;//Contiene configuracion del modo detenido

};

struct SYSTEM_CONFIG config;

int main()
{
    //Iniciando modo config
    config.cnt_TCA = 0;
    config.cnt_RT = 0;
    config.CONFIG_FD = 0;
    for(;;)
    {
        config.cnt_RT++;
        config.cnt_TCA++;

        if (ESTADO_SIGUIENTE == ESTADO_INICIAL)
        {
            ESTADO_SIGUIENTE = Func_ESTADO_INICIAL();
        }
        else if (ESTADO_SIGUIENTE == ESTADO_ERROR)
        {
            ESTADO_SIGUIENTE = Func_ESTADO_ERROR();
        }
        else if (ESTADO_SIGUIENTE == ESTADO_ABIERTO)
        {
            ESTADO_SIGUIENTE = Func_ESTADO_ABIERTO();
        }
        else if (ESTADO_SIGUIENTE == ESTADO_CERRADO)
        {
            ESTADO_SIGUIENTE = Func_ESTADO_CERRADO();
        }
        else if (ESTADO_SIGUIENTE == ESTADO_ABRIENDO)
        {
            ESTADO_SIGUIENTE = Func_ESTADO_ABRIENDO();
        }
        else if (ESTADO_SIGUIENTE == ESTADO_CERRANDO)
        {
            ESTADO_SIGUIENTE = Func_ESTADO_CERRANDO();
        }
        else if (ESTADO_SIGUIENTE == ESTADO_DETENIDA)
        {
            ESTADO_SIGUIENTE = Func_ESTADO_DETENIDA();
        }
        else if (ESTADO_SIGUIENTE == ESTADO_DESCONOCIDO)
        {
            ESTADO_SIGUIENTE = Func_ESTADO_DESCONOCIDO();
        }
    }

}

int Func_ESTADO_INICIAL(void)
{
    // Inicial
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_ABIERTO;

    // Inicializar el estado
    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.lamp = LAMP_OFF;
    config.cnt_TCA = 0;

    //Ciclo de estado
    for(;;)
    {
        //Deteccion de error en limit switch
        if(io.lsa == LM_ACTIVO && io.lsc == LM_ACTIVO )
        {
            return ESTADO_ERROR;
        }
        else if(io.lsa == LM_ACTIVO && io.lsc == LM_NOACTIVO )
        {
            return ESTADO_ABIERTO;
        }
        else if(io.lsa == LM_NOACTIVO && io.lsc == LM_ACTIVO )
        {
            return ESTADO_CERRADO;
        }
        else if(io.lsa == LM_NOACTIVO && io.lsc == LM_NOACTIVO )
        {
            return ESTADO_DESCONOCIDO;
        }
    }

}
int Func_ESTADO_ERROR(void)
{
    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;

    while(1)
    {
        //Hacer que el led/lampara parpadee mientras esta en este estado
        io.lamp = LAMP_ON;
        sleep(300);
        io.lamp = LAMP_OFF;
        sleep(300);
    }
    return ESTADO_ERROR; // Mantenerse en estado de error
}
int Func_ESTADO_ABRIENDO(void)
{
    //Inicial
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_ABRIENDO;

    //Inicializar el estado
    io.ma = MOTOR_ON; //Activamos el motor para abrir
    io.mc = MOTOR_OFF;
    io.lamp = LAMP_OFF;

    for(;;)
    {
        if(io.lsa == LM_ACTIVO && io.lsc == LM_NOACTIVO)
        {
            return ESTADO_ABIERTO;
        }
        else if (io.lsa == LM_ACTIVO && io.lsc == LM_ACTIVO)
        {
            return ESTADO_ERROR;
        }
        else if (io.pp == PP_ACTIVO)
        {
            return ESTADO_DETENIDA;
        }
    }

}
int Func_ESTADO_ABIERTO(void)
{
    //Inicial
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_ABIERTO;

    //Inicializar el estado
    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.lamp = LAMP_OFF;

    for(;;)
    {
        if(config.cnt_TCA >= TIME_CA)
        {
            return ESTADO_CERRANDO;
        }
        else if (io.keyc == 1)
        {
            return ESTADO_DETENIDA;
        }
        else if(io.pp == PP_ACTIVO)
        {
            return ESTADO_DETENIDA;
        }
        else if (io.lsa == LM_ACTIVO && io.lsc == LM_NOACTIVO)
        {
            return ESTADO_ABIERTO;
        }

    }

}
int Func_ESTADO_CERRANDO(void)
{
    //Inicial
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_ABIERTO;

    //Inicializar el estado
    io.ma = MOTOR_OFF;
    io.mc = MOTOR_ON; //Aqui activamos el motor para cerrar
    io.lamp = LAMP_OFF;

    for(;;)
    {
        if(io.lsa == LM_ACTIVO && io.lsc == LM_ACTIVO)
        {
            return ESTADO_ERROR;
        }
        else if(config.cnt_RT >= TIME_CA)
        {
            return ESTADO_ABRIENDO;
        }
        else if(io.pp == PP_ACTIVO)
        {
            return ESTADO_DETENIDA;
        }
        else if(io.lsa == LM_NOACTIVO && io.lsc == LM_ACTIVO)
        {
            return ESTADO_CERRADO;
        }
        else if (config.cnt_RT>= TIME_CA)
        {
            return ESTADO_ERROR; /*En esta parte si el motor tarda
                                   mas de lo debido dar� un error
                                   */
        }
    }

}
int Func_ESTADO_CERRADO(void)
{
    //Inicial
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_ABIERTO;

    //Inicializar el estado
    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.lamp = LAMP_OFF;

    for(;;)
    {
        if (io.lsa == LM_ACTIVO && io.lsc == LM_NOACTIVO)
        {
            return ESTADO_CERRADO;
        }

    }
}
int Func_ESTADO_DETENIDA(void)
{
    //Inicial
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_ABIERTO;

    //Inicializar el estado
    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.lamp = LAMP_OFF;

    for(;;)
    {
        if (io.lsa == LM_ACTIVO && io.lsc == LM_NOACTIVO)
        {
            return ESTADO_ABIERTO;
        }
        else if (io.lsa == LM_NOACTIVO && io.lsc == LM_ACTIVO)
        {
            return ESTADO_CERRADO;
        }
    }

}
int Func_ESTADO_DESCONOCIDO(void)
{
    //Inicial
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_ABIERTO;

    //Inicializar el estado
    io.ma = MOTOR_OFF;
    io.mc = MOTOR_OFF;
    io.lamp = LAMP_OFF;

    for (;;)
    {
        if(config.cnt_TCA >= TIME_CA)
        {
            return ESTADO_CERRANDO;
        }
    }

}

