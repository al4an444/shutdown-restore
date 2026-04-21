# Shutdown Restore Service

Este programa es un Servicio de Windows escrito en C++ que se ejecuta automÃ¡ticamente en segundo plano. EstÃ¡ diseÃ±ado para interceptar la seÃ±al de apagado (`PRESHUTDOWN`), crear un nuevo Punto de RestauraciÃ³n del Sistema, y borrar el punto de restauraciÃ³n mÃ¡s antiguo, garantizando que siempre tengas una copia de seguridad reciente antes de apagar el ordenador en caso de que algo salga mal.

## CaracterÃ­sticas
* **Completamente invisible**: Se ejecuta en segundo plano sin interfaces grÃ¡ficas molestas.
* **IntegraciÃ³n nativa**: Utiliza las APIs nativas de Windows `SRSetRestorePoint` y WMI para interactuar con los Puntos de RestauraciÃ³n.
* **Bypass de LÃ­mite de 24 horas**: Windows por defecto sÃ³lo permite crear un punto de restauraciÃ³n al dÃ­a automÃ¡ticamente. Este servicio puentea ese lÃ­mite modificando el registro (`SystemRestorePointCreationFrequency = 0`).
* **RotaciÃ³n automÃ¡tica**: Borra el punto de restauraciÃ³n mÃ¡s antiguo antes de crear el nuevo para no llenar tu disco duro.

## Requisitos
* Windows 10 o Windows 11.
* Puntos de RestauraciÃ³n (ProtecciÃ³n del sistema) **activados** en tu disco C:.
* CMake y Visual Studio Build Tools (C++) para compilarlo.

## CÃ³mo Compilar
Simplemente haz doble clic en el archivo `build.bat`. 
Esto crearÃ¡ una carpeta `build/` y compilarÃ¡ la versiÃ³n `Release` del programa.

## InstalaciÃ³n y Uso

**âš ï¸ IMPORTANTE**: Los comandos de instalaciÃ³n requieren permisos de Administrador.

1. Abre una ventana de **CMD** o **PowerShell** como **Administrador**.
2. Navega a la carpeta donde se compilÃ³ el programa:
   ```cmd
   cd C:\Ruta\A\Tu\Carpeta\build\Release
   ```
3. Instala e inicia el servicio con el siguiente comando:
   ```cmd
   ShutdownRestoreService.exe --install
   ```

Â¡Ya estÃ¡! A partir de ahora, cada vez que le des a apagar a tu ordenador, el servicio detendrÃ¡ temporalmente el apagado (hasta un mÃ¡ximo de 3 minutos, aunque suele tardar unos segundos), crearÃ¡ tu punto de restauraciÃ³n y borrarÃ¡ el viejo.

## Otros Comandos

Puedes probar que funciona sin tener que apagar el ordenador utilizando el modo de prueba (recuerda abrir la consola como Administrador):

```cmd
ShutdownRestoreService.exe --test
```

Si en algÃºn momento quieres desinstalar el servicio:

```cmd
ShutdownRestoreService.exe --uninstall
```

## Logs / Registros
Como es un servicio de Windows no tiene consola. Si quieres ver quÃ© estÃ¡ haciendo o si ha habido algÃºn error, el programa crea un archivo llamado `ShutdownRestore.log` en la misma carpeta donde estÃ¡ el ejecutable `ShutdownRestoreService.exe`.
