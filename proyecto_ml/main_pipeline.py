import numpy as np
from modulos.data_prep import segmentar_acelerometro

# --- CONFIGURACIÓN GLOBAL ---
# Subimos un nivel ("..") para salir de proyecto_ml y entrar a raw_data
RAW_DATA_DIR = '../raw_data' 
WINDOW_SEC = 3.0
OVERLAP_SEC = 1.0
FS_ACCEL = 50

def main():
    print("=== Iniciando Pipeline TinyML: ESP32 Steps Detector ===")
    
    # ---------------------------------------------------------
    # FASE 1: Preparación de Datos (Acelerómetro)
    # ---------------------------------------------------------
    print(f"\n[1] Segmentando CSVs en ventanas de {WINDOW_SEC}s...")
    X_accel, y_labels = segmentar_acelerometro(
        raw_folder=RAW_DATA_DIR, 
        window_sec=WINDOW_SEC, 
        overlap_sec=OVERLAP_SEC, 
        fs=FS_ACCEL
    )
    
    if len(X_accel) == 0:
        print(f"¡Error! No se encontraron datos válidos en: {RAW_DATA_DIR}")
        return

    print(f" -> Total de ventanas generadas: {len(X_accel)}")
    print(f" -> Forma del Tensor de entrada: {X_accel.shape} (Ventanas, Muestras, Ejes)")
    
    clases, conteos = np.unique(y_labels, return_counts=True)
    print(" -> Balance de clases:")
    for clase, count in zip(clases, conteos):
        print(f"    - {clase}: {count} ventanas")

    # ---------------------------------------------------------
    # FASE 2: Preparación de Datos (Audio) -> PENDIENTE
    # ---------------------------------------------------------
    
    # ---------------------------------------------------------
    # FASE 3: Construcción y Entrenamiento del Modelo -> PENDIENTE
    # ---------------------------------------------------------
    
    # ---------------------------------------------------------
    # FASE 4: Exportación a C (TensorFlow Lite Micro) -> PENDIENTE
    # ---------------------------------------------------------

if __name__ == "__main__":
    main()