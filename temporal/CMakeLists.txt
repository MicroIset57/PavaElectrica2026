cmake_minimum_required(VERSION 3.16)

project(ws2812_led_demo)

# Establecer la versión del firmware
set(PROJECT_VER "1.0.0")

# Incluir las herramientas de ESP-IDF
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Agregar el proyecto
project_info()
