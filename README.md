# 3-641 Microcontroladores y Microprocesadores

## Proyectos en este directorio

### 00_template

Simple "New C/C++ Project" para una placa Raspberry Pi Pico 2, donde solo se editó el launch.json agregando el liveWatch para poder hacer debug y ver variables globales en tiempo real.  
Se puede copiar para tener como base de otros proyectos. También es necesario, antes de compilar, cambiar a la placa que usaremos en caso que no sea una Pico 2W.

### 01_parpadeo

En este proyecto se configura el GPIO22 como salida digital donde se conecta un LED con R limitadora y se provoca su encendido y apagado cada 500mS.
Se utiliza como demora una función del SDK bloqueante, práctica NO recomendada para programar, pero simple para esta sencilla demostración.


### 02_puls_led_1

En este proyecto se describe la conexión del hardware en un header (hardware.h) como ejemplo de uso de esos archivos en el proyecto.
Se conecta un pulsador en el GPIO15 (a GND y con una R de pullup externa) de forma que el GPIO se mantiene en alto si NO está presionado el pulsador.
EL LED enciende si el pulsador está presionado y se apaga si se libera el pulsador.


### 03_puls_led_2

Igual que el anterior, solo que en este proyecto se agregan un pulsador y un LED, de modo que ahorac cada pulsador controla un LED.
Se utilizaron, para inicializar el hardware, las funciones del SDK "masked" para ejemplificar su uso, aplicando en cada caso, la máscara correspondiente.