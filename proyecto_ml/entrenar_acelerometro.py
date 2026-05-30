#!/usr/bin/env python3
import os
import numpy as np
import tensorflow as tf
from tensorflow.keras import layers, models
from sklearn.model_selection import train_test_split
from sklearn.metrics import confusion_matrix, classification_report

# --- CONFIGURACIÓN DE RUTAS ---
DATA_PATH = './datos_procesados/dataset_multimodal.npz'
MODEL_DIR = './modelos'
# ¡Ajustá esta ruta a la carpeta 'main' de tu proyecto en C!
ESP32_MAIN_DIR = '../data-logger/main'

def exportar_a_c(tflite_model, output_path):
    """Convierte el binario de TFLite a un array de C y lo guarda en el disco."""
    hex_lines = []
    # Convertimos cada byte del modelo a formato hexadecimal (0x00)
    for i in range(0, len(tflite_model), 12):
        chunk = tflite_model[i:i+12]
        hex_line = ', '.join([f'0x{b:02x}' for b in chunk])
        hex_lines.append(hex_line)

    hex_array = ',\n    '.join(hex_lines)

    c_code = f"""// Archivo autogenerado por el Pipeline de Python
// No editar manualmente.

#ifndef TFLITE_MODEL_H
#define TFLITE_MODEL_H

const unsigned int model_tflite_len = {len(tflite_model)};
const unsigned char model_tflite[] = {{
    {hex_array}
}};

#endif // TFLITE_MODEL_H
"""
    with open(output_path, 'w') as f:
        f.write(c_code)

def main():
    print("=== Pipeline TinyML: Entrenamiento y Exportación ESP32 ===")

    # 1. Cargar y Balancear los datos
    print("\n[1] Preparando Datos...")
    datos = np.load(DATA_PATH)
    X = datos['X_accel']
    y = np.array([1 if label == 'steps' else 0 for label in datos['y']])

    idx_none = np.where(y == 0)[0]
    idx_steps = np.where(y == 1)[0]
    min_muestras = min(len(idx_none), len(idx_steps))

    idx_none_bal = np.random.choice(idx_none, min_muestras, replace=False)
    idx_steps_bal = np.random.choice(idx_steps, min_muestras, replace=False)
    indices = np.concatenate([idx_none_bal, idx_steps_bal])
    np.random.shuffle(indices)

    X_bal = X[indices]
    y_bal = y[indices]

    X_train, X_test, y_train, y_test = train_test_split(
        X_bal, y_bal, test_size=0.15, random_state=42, stratify=y_bal # Subí a 15% para ver mejores métricas
    )

    # 2. Diseñar y Entrenar la Red Neuronal (CNN 1D)
    print("\n[2] Entrenando Red Neuronal...")
    modelo = models.Sequential([
        layers.Conv1D(16, 3, activation='relu', input_shape=(X_train.shape[1], X_train.shape[2])),
        layers.MaxPooling1D(2),
        layers.Conv1D(32, 3, activation='relu'),
        layers.MaxPooling1D(2),
        layers.Flatten(),
        layers.Dense(16, activation='relu'),
        layers.Dense(1, activation='sigmoid')
    ])

    modelo.compile(optimizer='adam', loss='binary_crossentropy', metrics=['accuracy'])

    # Silenciamos el output largo del entrenamiento (verbose=0)
    modelo.fit(X_train, y_train, epochs=30, batch_size=16, validation_split=0.1, verbose=0)

    # 3. Evaluación Detallada
    print("\n[3] Evaluando Resultados del Examen...")
    y_pred_prob = modelo.predict(X_test, verbose=0)
    y_pred = (y_pred_prob > 0.5).astype(int).flatten()

    cm = confusion_matrix(y_test, y_pred)
    # cm[0][0] = True Negatives (Correcto: Reposo)
    # cm[0][1] = False Positives (Falsa Alarma: Dijo Paso pero era Reposo)
    # cm[1][0] = False Negatives (Paso Perdido: Dijo Reposo pero era Paso)
    # cm[1][1] = True Positives (Correcto: Paso)

    print("\n--- REPORTE DE RENDIMIENTO ---")
    print(f"Total de ventanas evaluadas: {len(y_test)}")
    print(f"✔️  Pasos detectados correctamente (True Positives):  {cm[1][1]}")
    print(f"✔️  Reposo detectado correctamente (True Negatives):  {cm[0][0]}")
    print(f"❌ Falsas Alarmas (False Positives): {cm[0][1]} (El modelo inventó un paso)")
    print(f"❌ Pasos Ignorados (False Negatives): {cm[1][0]} (El modelo no vio el paso)")
    print("------------------------------")


# 4. Exportar a TensorFlow Lite
    print("\n[4] Convirtiendo a TensorFlow Lite (TinyML)...")
    os.makedirs(MODEL_DIR, exist_ok=True)
    ruta_export = os.path.join(MODEL_DIR, 'modelo_guardado')

    # 1. Guardamos el modelo en el nuevo formato nativo de Keras 3
    modelo.export(ruta_export)

    # 2. Lo cargamos en el conversor desde el disco en lugar de la memoria
    converter = tf.lite.TFLiteConverter.from_saved_model(ruta_export)
    tflite_model = converter.convert()

    ruta_tflite = os.path.join(MODEL_DIR, 'modelo_acelerometro.tflite')
    with open(ruta_tflite, 'wb') as f:
        f.write(tflite_model)
    print(f" -> Modelo TFLite generado ({len(tflite_model)} bytes).")


if __name__ == "__main__":
    main()
