idf_component_register(
    SRCS
        "main.c"
        "app_core.c"
        "logger.c"              # <--- ¡AGREGAR ESTA LÍNEA!
        "hardware/hardware.c"
        "hardware/hw_display.c"
        "hardware/hw_imu.c"
        "hardware/hw_mic.c"
        "hardware/hw_sd.c"
        "hardware/hw_rtc.c"
        "gui/gui.c"
        "gui/gui_hw.c"
        "gui/gui_main.c"
        "gui/gui_alarma.c"
        "gui/gui_config.c"
        "gui/gui_pasos.c"
        "gui/gui_wifi.c"
        "gui/gui_hostname.c"

    INCLUDE_DIRS
        "."
        "hardware"
        "gui"
)

# Opciones de compilación para los demos de LVGL
set_source_files_properties(
    ${LV_DEMOS_SOURCES}
    PROPERTIES COMPILE_OPTIONS
    -DLV_LVGL_H_INCLUDE_SIMPLE
)
