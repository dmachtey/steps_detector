idf_component_register(
    SRCS
        # Archivo Principal
        "main.c"

        # Lógica de la Aplicación (WiFi, NVS, Colas)
        "app_core.c"

        # Gestores de Hardware (BSP)
        "hardware.c"
        "hw_display.c"
        "hw_imu.c"
        "hw_mic.c"
        "hw_sd.c"
        "hw_rtc.c"

        # Motor y Pantallas de la Interfaz (LVGL)
        "gui_hw.c"
        "gui.c"
        "gui_main.c"
        "gui_alarma.c"
        "gui_config.c"
        "gui_pasos.c"
        "gui_wifi.c"

    INCLUDE_DIRS "."
)

# Opciones de compilación para los demos de LVGL (mantenidas de tu archivo original)
set_source_files_properties(
    ${LV_DEMOS_SOURCES}
    PROPERTIES COMPILE_OPTIONS
    -DLV_LVGL_H_INCLUDE_SIMPLE
)
