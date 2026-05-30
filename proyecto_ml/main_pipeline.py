import os
import numpy as np
from modulos.data_prep import segmentar_multimodal

# --- CONFIGURACIÓN GLOBAL ---
RAW_DATA_DIR = '../raw-data'
PROCESSED_DATA_DIR = './datos_procesados'
WINDOW_SEC = 3.0
OVERLAP_SEC = 0.5
FS_ACCEL = 50
FS_AUDIO = 8000

def main():
    print("=== Iniciando Pipeline TinyML: ESP32 Steps Detector ===")

    # ---------------------------------------------------------
    # FASE 1: Preparación Sincrónica de Datos
    # ---------------------------------------------------------
    print(f"\n[1] Segmentando CSVs y WAVs en ventanas de {WINDOW_SEC}s (Overlap: {OVERLAP_SEC}s)...")
    X_accel, X_audio, y_labels = segmentar_multimodal(
        raw_folder=RAW_DATA_DIR,
        window_sec=WINDOW_SEC,
        overlap_sec=OVERLAP_SEC,
        fs_accel=FS_ACCEL,
        fs_audio=FS_AUDIO
    )

    if len(X_accel) == 0:
        print(f"¡Error! No se encontraron pares válidos en: {RAW_DATA_DIR}")
        return

    print(f" -> Total de pares generados: {len(X_accel)}")
    print(f" -> Tensor Acelerómetro: {X_accel.shape} (Ventanas, Muestras, Ejes)")
    print(f" -> Tensor Audio:        {X_audio.shape} (Ventanas, Muestras)")

    clases, conteos = np.unique(y_labels, return_counts=True)
    print(" -> Balance de clases:")
    for clase, count in zip(clases, conteos):
        print(f"    - {clase}: {count} ventanas")

    # ---------------------------------------------------------
    # FASE 2: Guardar dataset multimodal en disco (.npz)
    # ---------------------------------------------------------
    print("\n[2] Guardando dataset completo para entrenamiento...")
    os.makedirs(PROCESSED_DATA_DIR, exist_ok=True)
    archivo_salida = os.path.join(PROCESSED_DATA_DIR, 'dataset_multimodal.npz')

    # Guardamos todo el paquete junto
    np.savez_compressed(archivo_salida, X_accel=X_accel, X_audio=X_audio, y=y_labels)

    print(f" -> Datos guardados exitosamente en: {archivo_salida}")

if __name__ == "__main__":
    main()
