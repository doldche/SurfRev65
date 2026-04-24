# SURF REV-65 — Spring Reverb VST3

Plugin de reverb de resorte vintage compilado automáticamente en la nube con GitHub Actions.
**No necesitás instalar nada en tu PC.**

---

## Cómo obtener el plugin instalable

### Paso 1 — Crear cuenta en GitHub
Ir a **https://github.com** → Sign up (es gratis)

### Paso 2 — Crear repositorio nuevo
1. Click en **"New repository"** (botón verde)
2. Nombre: `SurfRev65`
3. Marcar **"Public"**
4. Click **"Create repository"**

### Paso 3 — Subir los archivos
En la página del repositorio vacío:
1. Click en **"uploading an existing file"**
2. Arrastrar TODOS los archivos de esta carpeta:
   - `CMakeLists.txt`
   - Carpeta `Source/` con los 4 archivos .h y .cpp
   - Carpeta `.github/` con `workflows/build.yml`
3. Click **"Commit changes"**

### Paso 4 — Esperar la compilación (15-20 min)
1. Ir a la pestaña **"Actions"** del repositorio
2. Ver el workflow **"Compilar SURF REV-65"** corriendo
3. Esperar que el círculo pase de amarillo ⏳ a verde ✅

### Paso 5 — Descargar el plugin
1. Click en el workflow completado ✅
2. Bajar hasta la sección **"Artifacts"**
3. Click en **"SURF-REV65-Windows"** → se baja un ZIP
4. Descomprimirlo — adentro está:
   - `SURF REV-65.vst3` — el plugin para el DAW
   - `SURF_REV65.exe` — standalone sin DAW

### Paso 6 — Instalar el VST3 en Windows
Copiar la carpeta `SURF REV-65.vst3` a:
```
C:\Program Files\Common Files\VST3\
```
Luego en tu DAW escanear plugins → aparece "SURF REV-65"

---

## Estructura del repositorio

```
SurfRev65/
├── .github/
│   └── workflows/
│       └── build.yml       ← Script de compilacion en la nube
├── Source/
│   ├── PluginProcessor.h   ← Cabecera DSP
│   ├── PluginProcessor.cpp ← Algoritmo spring reverb
│   ├── PluginEditor.h      ← Cabecera UI
│   └── PluginEditor.cpp    ← Interfaz grafica del pedal
├── CMakeLists.txt          ← Configuracion de compilacion
└── README.md               ← Este archivo
```

---

## Si algo falla

En la pestaña Actions, click en el workflow fallido → ver los logs en rojo.
Los errores más comunes y su solución:

| Error | Solución |
|-------|----------|
| `CMake not found` | Ya está incluido en el workflow, no hacer nada |
| `JUCE download failed` | Re-ejecutar el workflow (click "Re-run jobs") |
| `Compiler error` | Abrir un Issue en este repo con el log |

---

## Volver a compilar

Cada vez que modificás cualquier archivo y hacés commit,
GitHub Actions compila automáticamente una versión nueva.

También podés compilar manualmente:
1. Pestaña **Actions**
2. Click en **"Compilar SURF REV-65"**
3. Click en **"Run workflow"** → **"Run workflow"**
