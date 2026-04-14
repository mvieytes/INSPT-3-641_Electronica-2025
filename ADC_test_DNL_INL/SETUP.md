# Instrucciones para compartir y preparar este proyecto

Este documento explica qué debe tener otra persona en su máquina y los pasos para clonar, preparar el entorno Python, compilar y grabar la Raspberry Pi Pico desde este repositorio.

**Requisitos (mínimos recomendados)**
- **Python:** 3.10 o 3.11 instalada en el sistema.
- **Herramientas de build:** `cmake` (>= 3.13), `ninja`.
- **Toolchain para RP2040:** `arm-none-eabi-gcc` (GNU Arm Embedded toolchain) instalado y en `PATH`.
- **Pico SDK y utilidades de flasheo:** `picotool` (opcional para UF2) y/o `openocd` (para flasheo por SWD). Drivers para tu probe (CMSIS-DAP, etc.).
- **Git:** para clonar el repositorio.

Nota: las versiones exactas del SDK/toolchain deben ser compatibles con la versión del Pico SDK usada en el proyecto.

Preparación local (receta)

1) Clonar el repo

```bash
git clone <url-del-repo>
cd <repo>
```

2) Entorno Python aislado (recomendado)

- Crear el entorno (si prefieres recrearlo en lugar de usar uno incluido):

```powershell
python -m venv .venv
```

- Activar (PowerShell):

```powershell
. .venv\Scripts\Activate.ps1
```

- Activar (CMD):

```cmd
.venv\Scripts\activate.bat
```

- Instalar dependencias Python (si existe `requirements.txt`):

```powershell
pip install -r requirements.txt
```

3) Generar `requirements.txt` (si tú lo proporcionas desde tu entorno)

Si quieres exportar las dependencias que usas aquí para que otros las instalen:

```powershell
# activa la `.venv` primero
pip freeze > requirements.txt
```

4) Build del proyecto (ejemplo común para Pico con Ninja)

```powershell
mkdir build
cd build
cmake -G "Ninja" .. -DPICO_SDK_PATH=../pico-sdk
ninja
```

Adaptá `-DPICO_SDK_PATH` si el SDK está en otra ruta.

5) Flashear la Pico

- Con `picotool` (uf2):

```powershell
picotool load build\<proyecto>.uf2 -fx
```

- Con `openocd` (ejemplo genérico):

```powershell
openocd -s <scripts_path> -f interface/cmsis-dap.cfg -f target/<target>.cfg -c "adapter speed 5000; program \"build/<proyecto>.elf\" verify reset exit"
```

6) Integración con VSCode

- Seleccionar el intérprete Python dentro de `.venv` (palette: `Python: Select Interpreter`) para que la terminal integrada use ese entorno.
- En la terminal integrada, activa `.venv` (comandos anteriores) antes de ejecutar scripts.
- Si usas las tareas de VSCode, ajusta rutas a tus herramientas (`picotool`, `openocd`, `ninja`) si no están en `PATH`.

7) Medición de DNL e INL con rampa lenta

- El firmware actual captura `200000` muestras del `ADC0` (`GPIO26`) usando DMA y acumula un histograma de `4096` bins en la Pico antes de transmitirlo por UART.
- La captura esta alineada a `4` periodos de una rampa sawtooth de `1 Hz`, usando `50000` muestras/s, para que el histograma cubra un numero entero de barridos sin cargar tanto al sistema durante la adquisicion.
- La etapa `# state=capture` tarda alrededor de `4 s` y durante ese intervalo no se imprime nada por UART; eso es esperable mientras la Pico esta adquiriendo y armando el histograma.
- La salida USB del stdio esta deshabilitada. El proyecto usa UART a `460800` baud para priorizar integridad de datos durante la transferencia.
- Despues de transmitir `BEGIN_HIST 4096 200000`, todos los bins del histograma y `END`, el firmware queda detenido en `while (true) { tight_loop_contents(); }`, de modo que no se reinicia al terminar la captura.
- Bajo debugger, pausas breves del probe o vistas que lean variables/perifericos pueden frenar momentaneamente el core; por eso el ring buffer DMA se dejo mas grande para tolerar mejor ese escenario, pero un halt largo sigue pudiendo arruinar la adquisicion.
- Para este ensayo debes inyectar una rampa lenta, idealmente de `0 V` a `3.3 V` a alrededor de `1 Hz`, para que el histograma de codigos permita estimar DNL e INL.

Flujo sugerido:

1. Compilar y grabar el firmware en la Pico.
2. Conectar la rampa al pin `GPIO26`.
3. Iniciar la captura desde la PC:

```powershell
python capture_uart_samples.py COM5 --baudrate 460800 --output samples.txt
```

4. Ejecutar el analisis de DNL/INL:

```powershell
python analyze_dnl_inl.py samples.txt --show
```

Por defecto el script informa dos resultados en una sola corrida:

- `Escala completa`, usando toda la ventana barrida.
- `Ventana interna 0.05 V a 3.25 V`, para comparar contra una zona util menos sensible a saturacion en bordes.

Si quieres analizar solo la ventana util de una rampa que va aproximadamente de `0.1 V` a `3.2 V`, puedes pedirla directamente en volts:

```powershell
python analyze_dnl_inl.py samples.txt --first-voltage 0.1 --last-voltage 3.2 --show
```

El script convierte esa ventana a codigos ADC usando `3.3 V` como fondo de escala. Si necesitas otra referencia, puedes ajustar `--full-scale-voltage`.

Si quieres cambiar la ventana interna comparativa por defecto, puedes usar:

```powershell
python analyze_dnl_inl.py samples.txt --compare-first-voltage 0.05 --compare-last-voltage 3.25
```

Si prefieres desactivar ese segundo reporte automatico:

```powershell
python analyze_dnl_inl.py samples.txt --no-compare-window
```

Por defecto tambien se guardan dos figuras en disco con los nombres `<archivo>_full_dnl_inl.png` y `<archivo>_window_dnl_inl.png`. Si quieres elegir el prefijo manualmente:

```powershell
python analyze_dnl_inl.py samples.txt --save-prefix adc_rampa
```

Métricas reportadas por el script:

- Rango de codigos realmente barrido por la rampa.
- Conteo ideal promedio por codigo dentro de la ventana util.
- DNL maximo, minimo y absoluto maximo en `LSB`.
- INL maximo, minimo y absoluto maximo en `LSB`.

Notas practicas:

- El analisis excluye automaticamente el primer y el ultimo codigo activos, porque los extremos de la rampa suelen quedar contaminados por errores de sobre-recorrido o por el tiempo que la señal pasa cerca de los rieles.
- Si la captura contiene mas de un barrido o un numero no entero de periodos, el analisis corrige automaticamente la cobertura por codigo para no inflar artificialmente el INL.
- Si necesitas forzar la ventana de analisis, usa `--first-code` y `--last-code` en `analize_dnl_inl.py`.
- Alternativamente puedes fijar la ventana en volts con `--first-voltage` y `--last-voltage`, por ejemplo para evitar los bordes si la rampa real no llega a cubrir toda la escala de manera limpia.

Buenas prácticas de repositorio

- NO subir el directorio ` .venv` al repositorio. Añadí `/.venv` a tu `.gitignore`.
- Sí subir `requirements.txt` para que otros puedan recrear el entorno Python fácilmente.
- Documentar la versión de Python y las herramientas en este archivo (`SETUP.md`) o en `README.md`.

Problemas comunes
- Si alguien intenta usar el `.venv` incluido en el repo y su sistema es diferente (Windows vs Linux, distinta versión de Python), pueden aparecer errores binarios: por eso es preferible recrear el entorno.
- Asegurate de que `arm-none-eabi-gcc`, `cmake` y `ninja` estén en `PATH` o documentá la ruta.

Si querés, puedo:
- generar `requirements.txt` desde el `.venv` que está en este repo,
- añadir `/.venv` al `.gitignore` del proyecto,
- o crear un `README.md` más extenso adaptado a Windows/WSL/macOS.

Fin.
