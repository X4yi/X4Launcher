# Packaging Linux tar.gz / Windows ZIP for X4Launcher

Este repositorio usa Qt6 y CMake. El objetivo es una distribución portable con la estructura:

- `bin/` — ejecutable principal
- `lib/` — bibliotecas compartidas Qt y dependencias externas
- `plugins/` — plugins Qt (platforms, imageformats, iconengines, styles, platforminputcontexts)
- `data/` — datos de la aplicación o recursos opcionales

No se usa AppImage, Flatpak ni Snap.

## Flujo recomendado (Automatizado)

Se recomienda usar el script principal `distribute.sh` en la raíz del proyecto para automatizar todo el proceso:

```bash
# Para Linux
./distribute.sh --linux

# Para Windows (cross-compile)
./distribute.sh --windows
```

Este script utiliza internamente los scripts detallados a continuación.

## Flujo manual (Paso a paso)

## 1. Compilar Release con CMake

```
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ..
cmake --build . -j$(nproc)
cmake --install . --prefix /usr
```

El script `packaging/build_release.sh` automatiza esto y usa `DESTDIR` para crear un árbol de staging en `build/package`.

### Por qué usar `DESTDIR` y `install_prefix=/usr`

- `cmake --install` escribe archivos bajo `DESTDIR/usr/...` en lugar de instalar en el sistema.
- El paquete distribuido queda desacoplado del sistema donde se compiló.

## 2. Recolectar dependencias necesarias

`packaging/collect_deps.sh` copia:

- el ejecutable `bin/X4Launcher`
- bibliotecas necesarias desde `ldd`
- todos los subdirectorios de plugins Qt disponibles en el Qt6 `plugin` dir
- un `bin/qt.conf` para que Qt busque plugins en rutas relativas

### Qué evita este script

- copiar `glibc`, `libm`, `libpthread`, `ld-linux`, `libdl`, `libstdc++.so`, `libgcc_s.so`.
- dejar bibliotecas en rutas absolutas del sistema como `/usr/lib`.

### Qué recopila

- bibliotecas Qt usadas por el ejecutable
- dependencias transitivas externas de esas bibliotecas
- plugins Qt necesarios para arranque y formatos de imagen

## 3. Rutas relativas y `qt.conf`

`packaging/collect_deps.sh` genera:

```ini
[Paths]
Plugins=../plugins
Libraries=../lib
```

Eso indica a Qt que busque plugins y bibliotecas en el ZIP, no en `/usr/lib`.

## 4. Asegurar Qt no dependa de rutas del sistema

Qt sigue buscando plugins relativos a su `qt.conf`. Sin embargo, el ejecutable y las bibliotecas también deben resolver dependencias en `dist/lib`.

`packaging/fix_rpath.sh` usa `patchelf` para fijar `RPATH` a:

- `$ORIGIN/../lib`

Esto hace que el ejecutable y las bibliotecas carguen dependencias desde `dist/lib` en tiempo de ejecución.

## 5. Verificación con `ldd`

`packaging/check_ldd.sh dist` muestra las dependencias de:

- `dist/bin/X4Launcher`
- todos los plugins Qt ELF en `dist/plugins`

Debe salir siempre sin líneas `=> not found`.

También puedes ejecutar manualmente:

```bash
ldd dist/bin/X4Launcher
find dist/plugins -type f | while read plugin; do file "$plugin" | grep -q ELF && ldd "$plugin"; done
```

## 6. Probar en un entorno limpio

`packaging/test_env.sh dist` lanza el ejecutable con `env -i` y variables mínimas:

- `PATH=/usr/bin:/bin`
- `LD_LIBRARY_PATH=dist/lib`
- `QT_PLUGIN_PATH=dist/plugins`

Si no hay servidor gráfico disponible, usa `QT_QPA_PLATFORM=minimal` o `QT_QPA_PLATFORM=offscreen` para validar la carga de plugins sin intentar conectarse a X11/Wayland.

Si falta algo, el fallo aparecerá inmediatamente.

### Nota de compatibilidad

En un sistema limpio, también puede ser útil definir:

```bash
XDG_RUNTIME_DIR=/tmp
QT_QPA_PLATFORM=xcb
```

## 7. Empaquetar en Linux y Windows

Para Linux crea un `tar.gz`:

```bash
bash packaging/package_linux.sh dist X4Launcher-linux.tar.gz
```

Para Windows crea un `zip`:

```bash
bash packaging/package_windows.sh dist X4Launcher-windows.zip
```

Ambos archivos preservan la estructura de `dist` sin rutas absolutas.

## Errores comunes y cómo evitarlos

- No usar `LD_LIBRARY_PATH` como único mecanismo de despliegue. El ejecutable debe tener `RPATH` relativo.
- Copiar `glibc`, `ld-linux` u otros componentes del sistema. Eso rompe en casi todas las distros.
- No incluir plugins Qt `platforms` o `imageformats`.
- No generar `qt.conf` o dejar rutas absolutas dentro de Qt.
- No verificar con `ldd` antes de empaquetar.

## Mejora de compatibilidad entre distros

### Build en un entorno más antiguo

Para que el binario sea compatible con distros más antiguas, compila en un contenedor o chroot con una versión antigua de glibc y Qt.

Recomendación práctica:

- usa una imagen Docker basada en Debian 12 / Ubuntu 22.04 si el destino puede ser distros modernas.
- si necesitas compatibilidad mayor, elige una base más antigua aún, pero asegúrate de tener Qt6.

### No copies glibc

Nunca copies `libc.so.6` ni `ld-linux-x86-64.so.2`. Es mejor que el sistema destino use su propia glibc compatible.

## Compilación Cruzada para Windows (desde Linux)

El proceso para compilar la versión de Windows desde Linux utiliza el toolchain de **MinGW-w64**. 
Para ejecutar `./distribute.sh --windows`, necesitas instalar el entorno de cross-compiling en tu sistema.

En **Arch Linux**, necesitas instalar los paquetes desde el AUR:
- `mingw-w64-gcc`
- `mingw-w64-cmake`
- `mingw-w64-qt6-base` (proporciona la herramienta mágica `x86_64-w64-mingw32-windeployqt`)
- *Opcional:* cualquier otro módulo de Qt6 que uses como `mingw-w64-qt6-declarative`.

El script usará `windeployqt` de MinGW para recolectar todas las DLLs de Qt necesarias (incluyendo `platforms/qwindows.dll`, `imageformats`, etc.) y las librerías base de C++ (`libstdc++-6.dll`, `libgcc_s_seh-1.dll`) de forma totalmente automatizada y segura.

No uses soluciones congeladas para Windows si buscas portabilidad simple.

## Archivos en `packaging/`

- `build_release.sh` — compila y prepara staging
- `collect_deps.sh` — copia el ejecutable, Qt libs y plugins
- `strip_binaries.sh` — reduce tamaño eliminando símbolos no necesarios
- `fix_rpath.sh` — fija RPATH relativo en el ejecutable y librerías
- `check_ldd.sh` — valida dependencias con `ldd`
- `test_env.sh` — prueba en entorno mínimo
- `package_linux.sh` — crea el tar.gz para Linux
- `package_windows.sh` — crea el zip para Windows
- `package_zip.sh` — alias genérico ZIP (opcional)
