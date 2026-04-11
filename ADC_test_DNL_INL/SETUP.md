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

- El firmware actual captura un unico bloque de `131072` muestras del `ADC0` (`GPIO26`) usando DMA y lo transmite por UART con `printf`.
- La salida USB del stdio esta deshabilitada. El proyecto usa UART a `921600` baud para reducir el tiempo de transferencia.
- Despues de transmitir `BEGIN N`, todas las muestras y `END`, el firmware queda detenido en `while (true) { tight_loop_contents(); }`, de modo que no se reinicia al terminar la captura.
- Para este ensayo debes inyectar una rampa lenta, idealmente de `0 V` a `3.3 V` a alrededor de `1 Hz`, para que el histograma de codigos permita estimar DNL e INL.

Flujo sugerido:

1. Compilar y grabar el firmware en la Pico.
2. Conectar la rampa al pin `GPIO26`.
3. Iniciar la captura desde la PC:

```powershell
python capture_uart_samples.py COM5 --baudrate 921600 --output samples.txt
```

4. Ejecutar el analisis de DNL/INL:

```powershell
python analize_dnl_inl.py samples.txt --show
```

Opcionalmente puedes guardar la figura en disco:

```powershell
python analize_dnl_inl.py samples.txt --save-prefix adc_rampa
```

Métricas reportadas por el script:

- Rango de codigos realmente barrido por la rampa.
- Conteo ideal promedio por codigo dentro de la ventana util.
- DNL maximo, minimo y absoluto maximo en `LSB`.
- INL maximo, minimo y absoluto maximo en `LSB`.

Notas practicas:

- El analisis excluye automaticamente el primer y el ultimo codigo activos, porque los extremos de la rampa suelen quedar contaminados por errores de sobre-recorrido o por el tiempo que la señal pasa cerca de los rieles.
- Si necesitas forzar la ventana de analisis, usa `--first-code` y `--last-code` en `analize_dnl_inl.py`.

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
