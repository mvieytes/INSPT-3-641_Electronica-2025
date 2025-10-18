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


### 06_puls_led_int_2

Al proyecto anterior se agrega un pulsador y un LED. Ahora con una segunda variable de demora, en la misma interrupción se discrimina en que pulsador se produce el evento e inicializa la demora correspondiente. Si el pulsador, pasada la demora, sigue presionado, se cambia el estado del LED asociado.


### 07_adc

Este proyecto implementa la lectura de una entrada analógica (potenciómetro conectado al GPIO26, canal 0 del ADC) cada 100mS, acumula 10 mediciones y realiza un promedio, obteniendo un valor cada 1 seg. Se muestra por consola el promedio obtenido y su conversión a tensión (suponiendo Vdd de alimentación de 3,3v). Se agrega en CMakeLists.txt la biblioteca de hardware ADC (en el campo target_link_libraries).


### 08_adc_filtros

Con base en el proyecto anterior, aquí se implementan 3 ejemplos de filtros sencillos, un promedio móvil, un filtro IIR y un filtro FIR (que implementa un tipo promedio móvil). Para todos, se toma una muestra ADC cada 10mS (100Hz) y se procesa esa misma muestra en cada caso, la cual se puede procesar como 12bits o, según compilación condicional, 10 u 8 bits. También se utiliza (condicionalmente) la extensión Teleplot para graficar los valores salidas que se muestran en consola.


### 09_pwm_dimmer_led

Implementa el control de un LED, dimerizando el encendido y apagado en ciclos de un segundo. Incrementa, cada 5mS el duty de un PWM desde 0% a 100% y viceversa.


### 10_pwm_servo_puls

Implementa un PWM configurado a una frecuencia de 50Hz y con un ancho de pulso variable de 0,5mS a 2,5mS para controlar la posición de un servomecanismo de 0º a 180º y viceversa, en 10 pasos de 18º c/u cada vez que se presiona el pulsador.


### 11_pwm_servo_adc

Igual al anterior, pero agrega la lectura de un potenciómetro para hacer la variación de 0º a 180º y viceversa con todos los valores intermeios. Se puede, según com pilación condicional, pasar al comportamiento del anterior.


### 60_LCD1602_I2C

Librería para utilizar un display LCD alfanumérico de 16x2 con interfaz I2C y programa básico de ejemplo de uso.


### 61_SSD1306_I2C

Librería para utilizar un display OLED SSD1306 con interfaz I2C y programa básico de ejemplo de uso.


### 70_simple_web_server

Implementa un sencillo servidor web que muestra una página con el estado de dos pulsadores y dos LEDs y permite enecnderlos o apagarlos a través de dichos pulsadores o desde la web con la acción de 4 botones.