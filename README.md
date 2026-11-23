Mi Paint open gl

1. Datos del autor:

Estudiante: Fabian Esneider Chuscano Sierra 

Código estudiantil: 6014537

2. Resumen del proyecto:

es un software de ilustración digital programado en C++ apoyado en la libreria ImGui.
El programa brinda a los usuarios un entorno interactivo para diseñar trazos libres y
composiciones visuales mediante utilidades de mano alzada (lápiz y borrador), cubeta de
relleno, herramientas de selección y transformación (rotación), además del trazado automático
de primitivas geométricas (segmentos de recta, rectángulos, circunferencias, triángulos y pentágonos).

El sistema incorpora una interfaz de usuario intuitiva que agiliza la elección de pinceles,
paletas cromáticas y modos de manipulación directa sobre el lienzo.

3. Estructura:

El código fuente se divide en módulos desacoplados que operan de forma sincronizada para transformar
los eventos del usuario en gráficos renderizados:

-Interfaz de usuario (ImGUI): Implementada con Dear ImGui para desplegar paneles de control, 
selectores de primitivas, paletas de color y parámetros de edición en tiempo real.

-Control de eventos e interacción: Captura e interpreta las coordenadas del cursor y las pulsaciones 
del ratón para traducirlas en acciones dentro del área de trabajo.

-Núcleo de rasterización y geometría: Ejecuta el algoritmo de Bresenham para el cálculo eficiente de líneas
y emplea fórmulas matemáticas para generar polígonos.

-Manipulación y edición: Administra las operaciones de modificación sobre los elementos trazados, 
incluyendo borrado, rellenado de áreas, selección de objetos y rotación geométrica.

-Motor de renderizado: Emplea OpenGL para dibujar en pantalla la rejilla de referencia, los vértices almacenados
y las figuras resultantes.

-Contexto y gestión de ventana: Utiliza GLFW para inicializar y administrar el ciclo de vida de la ventana, 
junto con GLAD para cargar dinámicamente los punteros a las funciones modernas de OpenGL.

4. Arquitectura del Sistema

El proyecto sigue una arquitectura **Monolítica Modular basada en un Bucle de Renderizado en Tiempo Real (Game Loop)** bajo un paradigma **Procedural y Orientado a Datos**. En cada fotograma, el sistema ejecuta un flujo unidireccional de **Entrada $\rightarrow$ Estado $\rightarrow$ Rasterización $\rightarrow$ Renderizado**.

### Diagrama de Flujo Arquitectónico

```mermaid
flowchart TD
    subgraph Plataforma ["1. Capa de Plataforma y Ventana"]
        GLFW["GLFW (Ventana, V-Sync, Eventos)"]
        GLAD["GLAD (Carga de API OpenGL)"]
    end

    subgraph Interaccion ["2. Capa de Interacción"]
        Raton["ProcesarRaton()\nMáquina de Estados (modo 0, 1, 2)"]
        UI["DibujarInterfaz()\nPanel Contextual Dear ImGui"]
    end

    subgraph Estado ["3. Estado Central en Memoria"]
        Vector["std::vector&lt;Figura&gt; figuras\n(Entidades paramétricas + Historial)"]
        Globales["Variables de Control\n(herramientaActual, seleccion, tamGrilla)"]
    end

    subgraph Nucleo ["4. Núcleo de Rasterización"]
        Calc["ObtenerCeldasFigura()\nBresenham | Punto Medio | Rotación 2D | Scanline"]
        Sets["std::set&lt;Celda&gt;\nceldasBorde y celdasRelleno"]
    end

    subgraph Render ["5. Motor de Renderizado (OpenGL 2D)"]
        Capas["Renderizado por Capas\n1. Rellenos → 2. Bordes → 3. Vista Previa → 4. Grilla → 5. Cursor"]
    end

    GLFW -->|Eventos Mouse/Teclado| Interaccion
    Raton -->|Crea / Mueve / Escala| Estado
    UI -->|Modifica propiedades| Estado
    Estado -->|Lee parámetros geométricos| Calc
    Calc -->|Discretiza en coordenadas| Sets
    Sets -->|Proyección glOrtho| Capas
    Capas -->|Buffer Swap| GLFW
