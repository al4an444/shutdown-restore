# 🔄 Shutdown Restore Service

A lightweight, native Windows Service written in C++ that automatically creates a System Restore Point every time you shut down your PC. It intercepts the shutdown signal (`PRESHUTDOWN`), bypasses the Windows 24-hour restriction, creates a new backup, and rotates old ones to save disk space.

---

## 🇪🇸 Descripción en Español

Este proyecto es un Servicio de Windows escrito en C++ que se ejecuta de manera silenciosa en segundo plano. Está diseñado para interceptar la señal de apagado del sistema, crear un nuevo Punto de Restauración y eliminar el más antiguo. Esto garantiza que siempre dispongas de una copia de seguridad reciente antes de apagar el equipo, por si surge algún problema en el siguiente arranque.

### ✨ Características Principales

* **Completamente invisible**: Se ejecuta en segundo plano sin interfaces gráficas ni interrupciones.
* **Integración nativa**: Utiliza las APIs nativas de Windows (`SRSetRestorePoint`) y WMI para gestionar los puntos de restauración.
* **Evasión del límite de 24 horas**: Por defecto, Windows solo permite crear un punto de restauración automático al día. Este servicio evade dicha restricción modificando el registro de forma segura (`SystemRestorePointCreationFrequency = 0`).
* **Rotación de copias (Rolling Backup)**: Elimina el punto de restauración más antiguo antes de crear uno nuevo, evitando así que el disco duro se llene.

### 📋 Requisitos

* **Sistema Operativo**: Windows 10 o Windows 11.
* **Protección del sistema**: Los Puntos de Restauración deben estar **activados** en tu unidad principal (normalmente `C:`).
* **Entorno de desarrollo**: CMake y Visual Studio Build Tools (C++) para la compilación.

---

## 🚀 Instalación y Uso

### 1. Compilación
Para compilar el proyecto, simplemente haz doble clic en el archivo `build.bat`. 
Este script generará automáticamente una carpeta `build/` y compilará la versión `Release` del programa utilizando CMake.

### 2. Instalación del Servicio
> **⚠️ IMPORTANTE**: Todos los comandos de instalación requieren permisos de Administrador.

1. Abre una ventana de **CMD** o **PowerShell** como Administrador.
2. Navega hasta el directorio donde se ha generado el ejecutable:

   cd C:\Ruta\Al\Directorio\build\Release

3. Instala e inicia el servicio ejecutando:
      
      ShutdownRestoreService.exe --install