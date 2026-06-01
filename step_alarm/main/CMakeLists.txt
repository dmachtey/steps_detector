idf_component_register(
    SRCS
        "main.c"
        "core/app_core.c"
        "core/app_wifi.c"
        "services/logger.c"
        "simulation/mock_sensors.c"
        "hardware/hardware.c"
        "hardware/hw_display.c"
        "hardware/hw_imu.c"
        "hardware/hw_mic.c"
        "hardware/hw_rtc.c"
        "hardware/hw_sd.c"

        # --- ARCHIVOS DE LA GUI ---
        "gui/gui.c"
        "gui/gui_alarma.c"
        "gui/gui_config.c"
        "gui/gui_hostname.c"
        "gui/gui_hw.c"
        "gui/gui_main.c"
        "gui/gui_pasos.c"
        "gui/gui_wifi.c"

        # --- CEREBRO ---
        "alarm_logic/alarm_core.c"
        "alarm_logic/Classifier.c"
        "alarm_logic/dsp_audio.c"

    INCLUDE_DIRS
        "."
        "core"
        "services"
        "simulation"
        "hardware"
        "gui"
        "alarm_logic"
)

# Opciones de compilación para los demos de LVGL
set_source_files_properties(
    ${LV_DEMOS_SOURCES}
    PROPERTIES COMPILE_OPTIONS
    -DLV_LVGL_H_INCLUDE_SIMPLE
)
