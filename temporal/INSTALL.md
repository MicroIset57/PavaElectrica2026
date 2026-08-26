# 🔧 Guía de Instalación - ESP-IDF para Windows, Linux y macOS

## Windows

### Opción 1: Instalador Oficial (Recomendado)

1. Descarga el instalador ESP-IDF desde:
   https://github.com/espressif/idf-installer/releases

2. Ejecuta el instalador `.exe`

3. Sigue los pasos del asistente de instalación

4. El instalador configurará automáticamente las variables de entorno

5. Reinicia PowerShell/CMD después de instalar

### Opción 2: Instalación Manual

1. Abre PowerShell como Administrador

2. Crea una carpeta para ESP-IDF:
   ```powershell
   mkdir $env:USERPROFILE\esp
   cd $env:USERPROFILE\esp
   ```

3. Clona el repositorio:
   ```powershell
   git clone --recursive https://github.com/espressif/esp-idf.git
   cd esp-idf
   ```

4. Ejecuta el script de instalación:
   ```powershell
   .\install.ps1
   ```

5. Carga el entorno en tu sesión actual:
   ```powershell
   .\export.ps1
   ```

6. Para futuras sesiones, agrega esto a tu perfil de PowerShell:
   ```powershell
   & $env:USERPROFILE\esp\esp-idf\export.ps1
   ```

### Verificar instalación en Windows

```powershell
idf.py --version
```

Debe mostrar algo como: `ESP-IDF v5.1.2`

---

## Linux (Ubuntu/Debian)

### Instalación

1. Instala dependencias:
   ```bash
   sudo apt-get install git wget flex bison gperf python3 python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
   ```

2. Crea la carpeta de ESP-IDF:
   ```bash
   mkdir -p ~/esp
   cd ~/esp
   ```

3. Clona el repositorio:
   ```bash
   git clone --recursive https://github.com/espressif/esp-idf.git
   cd esp-idf
   ```

4. Ejecuta el script de instalación:
   ```bash
   ./install.sh esp32
   ```

5. Carga el entorno:
   ```bash
   source export.sh
   ```

6. Para futuras sesiones, agrega a `~/.bashrc`:
   ```bash
   alias idf='. ~/esp/esp-idf/export.sh'
   ```

   Luego: `source ~/.bashrc`

### Verificar instalación en Linux

```bash
idf.py --version
```

---

## macOS

### Instalación

1. Instala Homebrew (si no lo tienes):
   ```bash
   /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
   ```

2. Instala dependencias:
   ```bash
   brew install python3 cmake ninja ccache dfu-util libusb
   ```

3. Crea la carpeta de ESP-IDF:
   ```bash
   mkdir -p ~/esp
   cd ~/esp
   ```

4. Clona el repositorio:
   ```bash
   git clone --recursive https://github.com/espressif/esp-idf.git
   cd esp-idf
   ```

5. Ejecuta el script de instalación:
   ```bash
   ./install.sh esp32
   ```

6. Carga el entorno:
   ```bash
   source export.sh
   ```

7. Para futuras sesiones, agrega a `~/.zprofile` (o `~/.bash_profile` para bash):
   ```bash
   alias idf='. ~/esp/esp-idf/export.sh'
   ```

   Luego: `source ~/.zprofile`

### Verificar instalación en macOS

```bash
idf.py --version
```

---

## Verificación Final

Después de instalar, verifica que puedas ejecutar:

```bash
# Mostrar versión
idf.py --version

# Crear un nuevo proyecto
idf.py create-project-from-template get-started/sample_project

# Ir al proyecto
cd sample_project

# Compilar
idf.py build

# Flashear (con ESP32 conectado)
idf.py -p COM3 flash
```

---

## Solución de Problemas

### Problema: `idf.py: command not found`

**Solución:**
```bash
# Linux/macOS
source ~/esp/esp-idf/export.sh

# Windows PowerShell
& $env:USERPROFILE\esp\esp-idf\export.ps1
```

### Problema: Python no encontrado

**Solución:**
```bash
# Verificar que Python 3.7+ está instalado
python3 --version

# Si no está instalado:
# Windows: descargar de https://www.python.org/
# Linux: sudo apt install python3
# macOS: brew install python3
```

### Problema: Error de permisos en Linux/macOS

**Solución:**
```bash
# Dar permisos de escritura
chmod -R u+w ~/esp/esp-idf
```

### Problema: CMake no encontrado

**Solución:**
```bash
# Windows PowerShell
pip install cmake

# Linux
sudo apt install cmake

# macOS
brew install cmake
```

---

## Recursos Adicionales

- Documentación oficial: https://docs.espressif.com/projects/esp-idf/
- Repositorio GitHub: https://github.com/espressif/esp-idf
- Foros: https://esp32.com/

---

¡Una vez completada la instalación, estás listo para compilar y flashear tu ESP32! 🚀
