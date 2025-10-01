# 3-641 Microcontroladoes y Microprocesadores

## Proyectos en este directorio

### 00_template

Simple "New C/C++ Project" para una placa Raspberry Pi Pico 2, donde solo se editó el launch.json agregando el liveWatch para poder hacer debug y ver variables globales en tiempo real.  
Se puede copiar para tener como base de otros proyectos. También es necesario, antes de compilar, cambiar a la placa que usaremos en caso que no sea una Pico 2W.

### 01_parpadeo

En este proyecto se configura un GPIO como salida digital y se controla el encendido y apagado de un LED cada 500mS.
Se utiliza como demora una función del SDK bloqueante, práctica NO recomendada para programar, pero simple para esta sencilla demostración.
