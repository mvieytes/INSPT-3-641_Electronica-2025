# 3-641 Microcontroladores y Microprocesadores

## Proyectos en este directorio

### 00_template

Simple "New C/C++ Project" para una placa Raspberry Pi Pico 2, donde solo se editó el launch.json agregando el liveWatch para poder hacer debug y ver variables globales en tiempo real.  
Se puede copiar para tener como base de otros proyectos. También es necesario, antes de compilar, cambiar a la placa que usaremos en caso que no sea una Pico 2W.

### 01_parpadeo

Con base en el proyecto anterior, en este proyecto se configura el GPIO22 como salida digital donde se conecta un LED con R limitadora y se provoca su encendido y apagado cada 500mS.
Se utiliza como demora una función del SDK bloqueante (sleep()), práctica NO recomendada para programar, pero simple para esta sencilla demostración.


### 02_puls_led_1

Agregando al anterior, en este proyecto se describe la conexión del hardware en un header (hardware.h) como ejemplo de uso de esos archivos en el proyecto.
Se conecta un pulsador en el GPIO15 (a GND y con una R de pullup externa) de forma que el GPIO se mantiene en alto si NO está presionado el pulsador.
EL LED enciende si el pulsador está presionado y se apaga si se libera el pulsador.


### 03_puls_led_2

Igual que el anterior, solo que en este proyecto se agregan un pulsador y un LED, de modo que ahora cada pulsador controla un LED.
Se utilizaron, para inicializar el hardware, las funciones del SDK "masked" para ejemplificar su uso, aplicando en cada caso, la máscara correspondiente.


### 04_puls_led_3

En este proyecto, basado en el anterior, solo usamos un pulsador y un LED para que, cada vez que se presiona el pulsador, el LED cambie de estado (encendido o apagado). Aquí se agregó una variable "flag" auxiliar para discriminar cuando se produce un flanco descendente en el GPIO del pulsador y una variable "demora" para determinar el momento en que dicho evento se produce y poder, luego de pasado un tiempo prudencial antirebote (30mS) y si el pulsador sigue preionado, cambiar el estado del LED. En el código los comentarios aclaran que se ejecuta en cada parte del mismo.


### 05_puls_led_int_1

Igual al proyecto anterior, solo que en este caso se detecta el evento de flanco descendente en la interrupción de GPIO, donde se inicializa la variable "demora" si no está inicializada, y luego, en el lazo principal, se determina al expirar dicha demora, si el pulsador está presionado y se habilita realizar la función necesaria, en este caso, cambiar el estado de un LED.
