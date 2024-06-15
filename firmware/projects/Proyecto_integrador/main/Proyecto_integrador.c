/*! @mainpage Proyecto final de Electronica Programable
 *
 * @section genDesc General Description
 *
 *  Este programa implementa conexión BLE entre la ESP-EDU y una aplicación en el celular 
 * que permite controlar el encendido, apagado y pausa de un Robot de guiado automatico, 
 * asi como visualizar y recibir alertas sobre el nivel de batería del mismo.
 * El Robot de guiado automatico, implementa sensores de ultrasonido y motores DC para seguir una línea 
 * negra en el piso.
 *
 * @section hardConn Hardware Connection
 *
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	B-1A	 	| 	GPIO_16		|
 * | 	B-1B	 	| 	GPIO_17		|
 * | 	A-1A	 	| 	GPIO_22		|
 * | 	A-1B	 	| 	GPIO_23		|
 * | 	VCC	 		| 	+5V			|
 * | 	GND	 		| 	GND			|
 * 
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	OUT	 		| 	GPIO_18		|
 * | 	VCC	 		| 	+5V			|
 * | 	GND	 		| 	GND			|
 * 
 * |    Peripheral  |   ESP32   	|
 * |:--------------:|:--------------|
 * | 	OUT	 		| 	GPIO_19		|
 * | 	VCC	 		| 	+5V			|
 * | 	GND		 	| 	GND			|
 *
 *
 * @section changelog Changelog
 *
 * |   Date	    | Description                                    |
 * |:----------:|:-----------------------------------------------|
 * | 16/05/2024 | Document creation		                         |
 *
 * @author Sacks Irina
 *
 */
/*==================[inclusions]=============================================*/
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "led.h"
#include "ble_mcu.h"
#include "pwm_mcu.h"
#include "gpio_mcu.h"
#include "timer_mcu.h"
#include "analog_io_mcu.h"
/*==================[macros and definitions]=================================*/
#define CONFIG_BLINK_PERIOD 500
#define DETECTOR_IZQ GPIO_18 //CABLE AZUL
#define DETECTOR_DER GPIO_19 //CABLE AMARILLO
#define CONFIG_PERIOD_DET 150000 // 0,25 seg, capaz deberia ser mas chico el tiempo
#define CONFIG_PERIOD_BAT 20000000 //esta en microseg asi que seria 30 seg, esto es para probar, pero cada cuanto deberia ser 30 000 000

//Motor izquierdo B
#define MIA GPIO_16 //Cable amarillo
#define	MIB GPIO_22 //Cable blanco

//Motor derecho A
#define	MDA GPIO_17 //Cable violeta
#define MDB	GPIO_23 //Cable gris
/*==================[internal data definition]===============================*/
typedef struct 
{   gpio_t pin;			/*!< GPIO pin number */
	io_t dir;			/*!< GPIO digection '0' IN;  '1' OUT*/
} gpioConf_t;

TaskHandle_t detectar_linea_handle = NULL;
TaskHandle_t control_bateria_handle = NULL;
TaskHandle_t mostrar_bateria_handle = NULL;

char direccion_robot; //Creo que es innecesaria
gpioConf_t gpio_detector_linea[2]; //NUEVA LINEA PROBAR !! SI NO FUNCIONA DESCOMENTAR ABAJO LA LINEA CON EL MISMO CODIGO
pwm_out_t pwm_control_motores[2]; //IDEM ARRIBA

bool encendido, pausa = false;
char niv_bateria[50];
uint16_t nivel_bat_dig;
bool alerta=false;
/*==================[internal functions declaration]=========================*/
void FuncTimerA_Detector (void* param){
	vTaskNotifyGiveFromISR(detectar_linea_handle, pdFALSE);
}

void FuncTimerB_AD (void* param){
	vTaskNotifyGiveFromISR(control_bateria_handle, pdFALSE);
	vTaskNotifyGiveFromISR(mostrar_bateria_handle, pdFALSE);
}

void read_data(uint8_t * data, uint8_t length){
	//Si el boton esta en On ('O') el robot debe andar (encendido es True)
	//Si el boton esta en Off ('o') el robot no debe andar (encendido es False)
	//Al apretar boton de pausa ('P') el robot debe frenar (pausa = true)
	//Si el boton pausa es soltado ('p') el robot debe continuar normal (pausa = false)
	switch(data[0]){
        case 'O':
			//char nivel_bateria[50]="5";
            encendido = true;
			BleSendString("RR0G0B0*"); //Apaga el led rojo en la app del celular
            BleSendString("AR0G0B0*"); //Apaga el led amarillo en la app del celular
			BleSendString("*VR0G255B0*"); //Enciende led verde en la app del celular
			BleSendString("*MRobot en marcha\n*"); //Envio mensaje al monitor del estado robot en la app del celular

		break;
        case 'o':
            encendido = false;
			BleSendString("VR0G0B0*"); //Apaga el led verde en la app del celular
            BleSendString("AR0G0B0*"); //Apaga el led amarillo en la app del celular
			BleSendString("*RR255G0B0*"); //Enciende led rojo en la app del celular
			BleSendString("*MRobot estacionado\n*"); //Envio mensaje al monitor del estado robot en la app del celular
		break;
		//El mecanismo de la pausa deberia funcionar solo si el robot fue encendido, deberia ir abajo creo
		case 'P':
            if(encendido == true){
			    pausa = true;
			    BleSendString("*AR222G206B42*"); //Enciende led amarillo en la app del celular
			    BleSendString("*MRobot pausado\n*"); //Envio mensaje al monitor del estado robot en la app del celular
			    //Control_motores('F'); //Freno el robot
            }
		break;
		case 'p':
            if(encendido == true){
			    pausa = false;
			    BleSendString("AR0G0B0*"); //Apaga el led amarillo en la app del celular
			    //Aca no deberia de llamar a ninguna funcion porque deberian reanudarse el control de los detectores de linea
				BleSendString("*MRobot reanudado\n*");
			}
        break;
		
		default:
		break;
    }
}

void ControlMotores(char direccion_robot){ 
//Supuestamente va para adelante con pines 22 y 23 en ALTO - Osea las entradas B van en ALTO
	switch(direccion_robot){
		case 'A':
			//Defino A1B y B1B como HIGH para que vaya para adelante
			GPIOOn(MIB);
			GPIOOn(MDB);
			
			//Defino que tienen 
			PWMOn(pwm_control_motores[0]);
			PWMOn(pwm_control_motores[1]);
			
			//Seteo la velocidad
			PWMSetDutyCycle(pwm_control_motores[0], 80);
			PWMSetDutyCycle(pwm_control_motores[1], 80);
			break;
		case 'I':
			//Defino A1B y B1B como HIGH para que vaya para adelante
			GPIOOn(MIB);
			GPIOOn(MDB);
			
			//Defino que tienen 
			PWMOn(pwm_control_motores[0]);
			PWMOn(pwm_control_motores[1]);
			
			//Seteo la velocidad, el motor izquierdo tiene que girar mas lento y el derecho mas rapido
			// y asi logra girar a la izquierda
			PWMSetDutyCycle(pwm_control_motores[0], 90); //rueda izq
			PWMSetDutyCycle(pwm_control_motores[1], 30); //rueda der	
			break;
		case 'D':
			//Defino A1B y B1B como HIGH para que vaya para adelante
			GPIOOn(MIB);
			GPIOOn(MDB);
			
			//Defino que tienen 
			PWMOn(pwm_control_motores[0]);
			PWMOn(pwm_control_motores[1]);
			
			//Seteo la velocidad, el motor izquierdo tiene que girar mas rapido y el derecho mas lento
			// y asi logra girar a la derecha
			PWMSetDutyCycle(pwm_control_motores[0], 30); //rueda izq
			PWMSetDutyCycle(pwm_control_motores[1], 90); //rueda der	
			break;
		case 'F':
			//Defino A1B y B1B como HIGH para que vaya para adelante
			GPIOOn(MIB);
			GPIOOn(MDB);
			
			PWMSetDutyCycle(pwm_control_motores[0], 100); //rueda izq
			PWMSetDutyCycle(pwm_control_motores[1], 100); //rueda der	
            
			break;
		default:
			break;
	}
}

static void DetectarLinea(void *pvParameter){ 
	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);     
		//bool det_izq  = GPIORead(DETECTOR_IZQ); //true GPIO input high
		//bool det_der  = GPIORead(DETECTOR_DER);	
		//NUEVA LINEA, SI NO FUNCIONA DESCOMENTAR LAS DOS DE ARRIBA !!
		bool det_izq = GPIORead(gpio_detector_linea[0].pin);
		bool det_der = GPIORead(gpio_detector_linea[1].pin);
		//Agregar la linea de abajo para que funcione si se encendio desde el bt
		if(encendido == true && pausa == false){
			if(det_izq==true && det_der==true){
				//si eso significa que estan en la linea blanca, entonces el auto sigue derecho
				direccion_robot = 'A';
				//LedsOffAll();
				//LedOn(LED_1);
			}
			else if(det_izq==true && det_der==false){
				//si eso significa que el detector izquierdo esta sobre linea blanca y el derecho en la negra
				//entonces llamar funcion que DOBLE A LA DERECHA (gira la rueda izquierda y la derecha mas o menos quieta)
				direccion_robot = 'D';
				//LedsOffAll();
				//LedOn(LED_2);
			}
			else if(det_izq==false && det_der==true){
				//si eso significa que el detector izquierdo esta sobre linea negra y el derecho en la blanca
				//entonces llamar funcion que DOBLE A LA IZQUIERDA (gira la rueda derecha y la izquierda mas o menos quieta)
				direccion_robot = 'I';
				//LedsOffAll();
				//LedOn(LED_3);
			}
			else if(det_izq==false && det_der==false){
				//si eso significa que el detector izquierdo y derecho estan sobre la linea negra
				//entonces llamar funcion que FRENE EL AUTO
				direccion_robot = 'F';
			}
			
			ControlMotores(direccion_robot);
		}else if(encendido==true && pausa==true){
			ControlMotores('F');
		}else if(encendido==false){
			ControlMotores('F');
		}
		
	}
}

static void ControlBateria(void *pvParameter){ 
	uint16_t nivel_bat_analog;//, nivel_bat_dig; 
	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		//Agregar linea cuando este con el codigo de bt
		if(encendido == true){
			AnalogInputReadSingle(CH1, &nivel_bat_analog);
			nivel_bat_dig = (AnalogRaw2mV(nivel_bat_analog))*2;
			nivel_bat_dig = (nivel_bat_dig*100/4000); 
			sprintf(niv_bateria, "%hu", nivel_bat_dig);
		}
	}		
}
static void MostrarBateria(void *pvParameter){ 
	char aux_bat[100];
	aux_bat[0] = '\0';
	while(1){
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		if(encendido == true){
			strcat(aux_bat, "*B");
			strcat(aux_bat, "El nivel de bateria es de: ");
			strcat(aux_bat, niv_bateria);
			if(nivel_bat_dig >=0 && nivel_bat_dig<=10){
				strcat(aux_bat, "% \n¡NIVEL BATERIA CRITICO! Cambie las baterias...");
			}else if(nivel_bat_dig >10 && nivel_bat_dig <=20){
				strcat(aux_bat, "% \n¡NIVEL DE BATERIA BAJO!");			
			}else if(nivel_bat_dig >20 && nivel_bat_dig<=50){
				strcat(aux_bat, "% \nNIVEL DE BATERIA MEDIO");	
			}else if(nivel_bat_dig>50 && nivel_bat_dig <=100){
				strcat(aux_bat, "% \nNIVEL DE BATERIA BUENO");	
			}
			strcat(aux_bat, "\n*");
			BleSendString(aux_bat); //Envio el nivel de la bateria en porcentaje al monitor en la app del celular
		}
	}		
}

/*==================[external functions definition]==========================*/
void app_main(void){
    //Inicializo los LEDS
    LedsInit();

    //Inicializo Bluetooth
    ble_config_t ble_configuration = {
        "ROBOT_LINEA_IRINA",
        read_data
    };
    BleInit(&ble_configuration);

	//GPIO PARA PUENTE H Y CONTROL MOTORES
    gpioConf_t gpio_control_motores[4]; 
	//MIB (B1B) y MDB (A1B) no son PWM porque el auto solo va para adelante - tienen que tener ambos LOW para ser asi-
    gpio_control_motores[0].pin=MIB; // GPIO_22; 
    gpio_control_motores[1].pin=MDB; // GPIO_23; 
	
	//GPIO a usar como PWM -para control motores-
	gpio_control_motores[2].pin=MIA; // GPIO_16 
	gpio_control_motores[3].pin=MDA; // GPIO_17 
	
    //DEFINO GPIO COMO SALIDA
    for(int i=0; i<4;i++){
		gpio_control_motores[i].dir=GPIO_OUTPUT; /*!< GPIO direction '0' IN;  '1' OUT*/
	}
	
    //INICIALIZO GPIO para control de motores por puente H
    for(int i=0; i<4;i++){
		GPIOInit(gpio_control_motores[i].pin, gpio_control_motores[i].dir);
	}

	//PWM PARA LAS ENTRADAS DEL PUENTE H PARA CONTROL MOTORES
	pwm_control_motores[0]=PWM_0;
	pwm_control_motores[1]=PWM_1;

	//Inicializo los PWM para control motores a una frecuencia de 50
	PWMInit(pwm_control_motores[0], MIA, 50);
	PWMInit(pwm_control_motores[1], MDA, 50);

	//GPIO PARA DETECTORES DE LINEA TCRT5000
    gpio_detector_linea[0].pin=DETECTOR_IZQ; 
    gpio_detector_linea[1].pin=DETECTOR_DER;
	
    //DEFINO GPIO COMO ENTRADA
    for(int i=0; i<2;i++){
		gpio_detector_linea[i].dir=GPIO_INPUT; /*!< GPIO digection '0' IN;  '1' OUT*/
	}
	
    //INICIALIZO GPIO para detectores linea
    for(int i=0; i<2;i++){
		GPIOInit(gpio_detector_linea[i].pin, gpio_detector_linea[i].dir);
	}

	analog_input_config_t senal_analogica_bat = {			
		.input= CH1,			
		.mode= ADC_SINGLE,		
		.func_p= NULL,			
		.param_p=NULL	
	};	
	AnalogInputInit(&senal_analogica_bat);

        //Inicializo el timer para los detectores de linea
    timer_config_t timer_detector = {
        .timer = TIMER_A,
        .period = CONFIG_PERIOD_DET,
        .func_p = FuncTimerA_Detector,
        .param_p = NULL  
    };
	// //Inicializo el timer para el control de la bateria y conversion a digital
    timer_config_t timer_bateria = {
        .timer = TIMER_B,
        .period = CONFIG_PERIOD_BAT,
        .func_p = FuncTimerB_AD,
        .param_p = NULL  
    };

	TimerInit(&timer_detector);
	//TimerInit(&timer_bateria);
	
	xTaskCreate(&DetectarLinea, "DetectarLinea", 2048, NULL, 5, &detectar_linea_handle); 
	xTaskCreate(&ControlBateria, "ControlBateria", 2048, NULL, 5, &control_bateria_handle); 
	xTaskCreate(&MostrarBateria, "MostrarBateria", 2048, NULL, 5, &mostrar_bateria_handle); 

	TimerStart(timer_detector.timer);
	TimerStart(timer_bateria.timer);

    //Leds cambian segun el estado del BLE: 
	//Conectado prende led verde, si se desconecta parpadea el led amarillo y si se apaga el led rojo
    while(1){
        vTaskDelay(CONFIG_BLINK_PERIOD / portTICK_PERIOD_MS);
        switch(BleStatus()){
            case BLE_OFF:
				LedOff(LED_1);
				LedOff(LED_2);
                LedOn(LED_3);
                encendido = false;
				pausa=false;
            break;
            case BLE_DISCONNECTED:
                LedOff(LED_1);
                LedToggle(LED_2);
				LedOff(LED_3);
                encendido = false;
				pausa=false;
            break;
            case BLE_CONNECTED:
				LedOn(LED_1);
                LedOff(LED_2);
                LedOff(LED_3);
            break;
        }
    }

}