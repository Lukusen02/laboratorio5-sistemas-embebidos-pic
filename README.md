# Sistema Embebido con PIC16F877A - Control de Actuadores y LCD

## Descripción

Este proyecto implementa un sistema embebido basado en el microcontrolador **PIC16F877A**, capaz de controlar diferentes actuadores como una bomba, un motor paso a paso y un conjunto de LEDs mediante una interfaz de usuario.

La interacción se realiza a través de una pantalla **LCD 16x2** y tres botones físicos, permitiendo la navegación por un sistema de menús y submenús. El sistema también incluye un modo de reposo (IDLE) con desplazamiento de texto (scrolling).


## Características principales

- Interfaz de usuario con LCD 16x2 en modo 4 bits
- Sistema de menús y submenús interactivo
- Control de:
  - Bomba (encendido/apagado con diferentes velocidades)
  - Motor paso a paso (control de dirección y pulsos)
  - Secuencia de LEDs
- Texto en movimiento en modo inactivo (scroll)
- Máquina de estados (IDLE, MENU, SUBMENU)
- Temporización por software con control de inactividad
- Implementación en lenguaje C para CCS Compiler


## Hardware utilizado

- PIC16F877A
- LCD 16x2
- Botones (3)
- LEDs (5)
- Motor paso a paso + driver (ULN2003 o equivalente)
- Bomba o actuador DC
- Resistencias y fuente de alimentación


## Estructura del sistema

El sistema se basa en una **máquina de estados**, organizada de la siguiente forma:

- **IDLE:** Modo de espera con texto en movimiento
- **MENU:** Selección de módulos
- **SUBMENU:** Configuración de cada módulo


## Funcionamiento

1. El sistema inicia en modo IDLE mostrando un mensaje en scroll.
2. Al presionar SELECT se accede al menú principal.
3. Se navega con LEFT y RIGHT entre opciones.
4. SELECT entra a submenú o ejecuta configuración.
5. El sistema regresa automáticamente a IDLE tras inactividad (~30 s).


## Tiempo de inactividad

El sistema utiliza un contador de software configurado para generar aproximadamente **30 segundos de inactividad** antes de regresar al estado IDLE.


## Código fuente

El código fue desarrollado en lenguaje C usando el compilador CCS para PIC.

## Autor

- Juan Pablo González  
- Samuel Martínez  
- Universidad Pontificia Bolivariana


## Notas

Este proyecto fue desarrollado con fines académicos para el curso de sistemas embebidos / microcontroladores.
