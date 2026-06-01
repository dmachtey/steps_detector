import os
import numpy as np
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import confusion_matrix
# Si usás otra librería para exportar a C (como m2cgen), cambiala acá:
import m2cgen as m2c


# --- CONFIGURACIÓN GLOBAL ---
DATASET_FILE = './datos_procesados/dataset_multimodal.npz'
EXPORT_DIR = '../step_alarm/main/alarm_logic' # Ajustá esta ruta si es necesario

DIV_IMU = 800.0
DIV_AUDIO = 2000.0

def main():
    print("=== Pipeline Optimizado: Random Forest + Feature Extraction (15 Variables) ===")

    # ---------------------------------------------------------
    # 1. CARGAR DATOS CRUDOS
    # ---------------------------------------------------------
    print("\n[1] Cargando dataset multimodal masivo...")
    if not os.path.exists(DATASET_FILE):
        print(f"Error: No se encontró el dataset en {DATASET_FILE}")
        return

    datos = np.load(DATASET_FILE)
    X_accel_raw = datos['X_accel'] # Forma esperada: (N, 150, 3)
    X_audio_raw = datos['X_audio'] # Forma esperada: (N, 24000)
    y_labels = datos['y']

    # Convertir etiquetas a números (none = 0, steps = 1)
    # Ajustá la validación según cómo se llamen tus clases exactamente
    y = np.array([0 if label == 'none' else 1 for label in y_labels])
    N_ventanas = len(y)

    # ---------------------------------------------------------
    # 2. PROCESAMIENTO IMU (DC Blocker + Normalización + 12 Features)
    # ---------------------------------------------------------
    print(f"\n[2] Procesando IMU y extrayendo 12 características estadísticas...")
    X_imu_features = np.zeros((N_ventanas, 12))

    for i in range(N_ventanas):
        for eje in range(3): # 0: X, 1: Y, 2: Z
            datos_eje = X_accel_raw[i, :, eje]

            # A. Filtro DC Blocker (Restar la media para matar gravedad)
            datos_limpios = datos_eje - np.mean(datos_eje)

            # B. Normalización y CLAMP (Hard-clipping a [-1.0, 1.0])
            datos_norm = np.clip(datos_limpios / DIV_IMU, -1.0, 1.0)

            # C. Extracción de Características
            idx = eje * 4
            X_imu_features[i, idx]   = np.sqrt(np.mean(datos_norm**2)) # 1. RMS (Energía)
            X_imu_features[i, idx+1] = np.max(datos_norm)              # 2. Max (Impacto)
            X_imu_features[i, idx+2] = np.min(datos_norm)              # 3. Min (Rebote)
            X_imu_features[i, idx+3] = np.var(datos_norm)              # 4. Varianza (Agitación)

    # ---------------------------------------------------------
    # 3. PROCESAMIENTO AUDIO (Normalización + 3 Features)
    # ---------------------------------------------------------
    print("[3] Procesando Audio y extrayendo 3 características...")
    X_audio_features = np.zeros((N_ventanas, 3))

    for i in range(N_ventanas):
        # Recuperar formato original y normalizar con DIV_AUDIO
        audio_chunk = X_audio_raw[i, :] * 32768.0
        audio_norm = np.clip(audio_chunk / DIV_AUDIO, -1.0, 1.0)

        # A. RMS (Energía Acústica)
        rms_audio = np.sqrt(np.mean(audio_norm**2))

        # B. ZCR (Cruces por cero - aproximación de frecuencia/ruido)
        zcr_audio = np.mean(np.diff(np.sign(audio_norm)) != 0)

        # C. Frecuencia Pico (simplificada usando FFT)
        espectro = np.abs(np.fft.rfft(audio_norm))
        freq_pico = np.argmax(espectro) / len(espectro) # Frecuencia normalizada

        X_audio_features[i, 0] = rms_audio
        X_audio_features[i, 1] = zcr_audio
        X_audio_features[i, 2] = freq_pico

    # ---------------------------------------------------------
    # 4. CONSTRUCCIÓN DEL DATASET FINAL Y SPLIT
    # ---------------------------------------------------------
    X_final = np.hstack((X_imu_features, X_audio_features))
    print(f" -> Forma final de los datos: {X_final.shape} (Ventanas, Características)")

    # Separamos en 85% para estudiar y 15% para el examen final
    X_train, X_test, y_train, y_test = train_test_split(
        X_final, y, test_size=0.15, random_state=42, stratify=y
    )

    # ---------------------------------------------------------
    # 5. ENTRENAMIENTO DEL MODELO
    # ---------------------------------------------------------
    print("\n[4] Entrenando Random Forest...")
    # Achicamos la cantidad de árboles (n_estimators) porque ahora el problema es más fácil.
    # Esto hace que el código en C sea muchísimo más liviano y rápido.
    clf = RandomForestClassifier(n_estimators=25, max_depth=8, random_state=42)
    clf.fit(X_train, y_train)

    # Imprimir Importancia
    importances = clf.feature_importances_
    print("\n--- RANKING DE IMPORTANCIA ---")
    print("IMU (Movimiento):")
    print(f"  Eje X (Lateral):  {sum(importances[0:4])*100:.2f}%")
    print(f"  Eje Y (Frontal):  {sum(importances[4:8])*100:.2f}%")
    print(f"  Eje Z (Vertical): {sum(importances[8:12])*100:.2f}%")
    print("Audio (Micrófono):")
    print(f"  RMS:  {importances[12]*100:.2f}%")
    print(f"  ZCR:  {importances[13]*100:.2f}%")
    print(f"  Freq: {importances[14]*100:.2f}%")

    # ---------------------------------------------------------
    # 6. EVALUACIÓN Y EXAMEN FINAL
    # ---------------------------------------------------------
    y_pred = clf.predict(X_test)
    cm = confusion_matrix(y_test, y_pred)

    # Dependiendo de qué clase sea 0 y 1, extraemos TN, FP, FN, TP
    # Asumimos: 0 es Reposo (TN, FP), 1 es Pasos (FN, TP)
    tn, fp, fn, tp = cm.ravel()

    print("\n[5] Evaluando Resultados del Examen...")
    print("\n--- REPORTE DE RENDIMIENTO ---")
    print(f"Total de ventanas de prueba evaluadas: {len(y_test)}")
    print(f"✔️  Pasos detectados (TP): {tp}")
    print(f"✔️  Reposo detectado (TN): {tn}")
    print(f"❌ Falsas Alarmas (FP):   {fp}")
    print(f"❌ Pasos Ignorados (FN):  {fn}")
    print("------------------------------")

# ---------------------------------------------------------
    # 7. EXPORTACIÓN A C PURO (m2cgen)
    # ---------------------------------------------------------
    print("\n[6] Generando código ANSI C puro con m2cgen...")
    os.makedirs(EXPORT_DIR, exist_ok=True)

    # 1. Obtenemos el modelo en C puro
    c_code = m2c.export_to_c(clf)

    # 2. Creamos nuestro wrapper para que encaje perfecto con alarm_core.c
    wrapper_c = """
// --- Wrapper para integrar con el firmware ---
#include "Classifier.h"

int predict(float *features) {
    double input[15];
    double output[2]; // 0: Reposo, 1: Pasos

    // Convertir de float a double para m2cgen
    for(int i = 0; i < 15; i++) {
        input[i] = (double)features[i];
    }

    // score() es la red generada por m2cgen arriba
    score(input, output);

    // Retornamos 1 si la votación por "Pasos" gana
    return (output[1] > output[0]) ? 1 : 0;
}
"""

    # Escribimos el .c
    archivo_c = os.path.join(EXPORT_DIR, 'Classifier.c')
    with open(archivo_c, 'w') as f:
        f.write(c_code)
        f.write("\n")
        f.write(wrapper_c)

    # Escribimos el .h automático
    archivo_h = os.path.join(EXPORT_DIR, 'Classifier.h')
    with open(archivo_h, 'w') as f:
        f.write("#pragma once\n\n")
        f.write("int predict(float *features);\n")

    print(f" -> ¡ÉXITO! Classifier.c y Classifier.h exportados a: {EXPORT_DIR}")

# ---------------------------------------------------------
    # 7. EXPORTACIÓN A C PURO (m2cgen)
    # ---------------------------------------------------------
    print("\n[6] Generando código ANSI C puro con m2cgen...")
    os.makedirs(EXPORT_DIR, exist_ok=True)

    # 1. Obtenemos el modelo en C puro
    c_code = m2c.export_to_c(clf)

    # 2. Creamos nuestro wrapper para que encaje perfecto con alarm_core.c
    wrapper_c = """
// --- Wrapper para integrar con el firmware ---
#include "Classifier.h"

int predict(float *features) {
    double input[15];
    double output[2]; // 0: Reposo, 1: Pasos

    // Convertir de float a double para m2cgen
    for(int i = 0; i < 15; i++) {
        input[i] = (double)features[i];
    }

    // score() es la red generada por m2cgen arriba
    score(input, output);

    // Retornamos 1 si la votación por "Pasos" gana
    return (output[1] > output[0]) ? 1 : 0;
}
"""

    # Escribimos el .c
    archivo_c = os.path.join(EXPORT_DIR, 'Classifier.c')
    with open(archivo_c, 'w') as f:
        f.write(c_code)
        f.write("\n")
        f.write(wrapper_c)

    # Escribimos el .h automático
    archivo_h = os.path.join(EXPORT_DIR, 'Classifier.h')
    with open(archivo_h, 'w') as f:
        f.write("#pragma once\n\n")
        f.write("int predict(float *features);\n")

    print(f" -> ¡ÉXITO! Classifier.c y Classifier.h exportados a: {EXPORT_DIR}")


if __name__ == "__main__":
    main()
