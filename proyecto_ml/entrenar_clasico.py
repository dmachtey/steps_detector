import os
import shutil
import numpy as np
from sklearn.ensemble import RandomForestClassifier
from sklearn.model_selection import train_test_split
from sklearn.metrics import confusion_matrix, classification_report
from micromlgen import port
import process_code  # Importamos el script de tu profesor

# --- CONFIGURACIÓN ---
DATA_PATH = './datos_procesados/dataset_multimodal.npz'
ESP32_MAIN_DIR = '../step_alarm/main' # <-- Actualizado al nuevo nombre

def extraer_features_audio(audio_window, fs=8000):
    """ Extrae Energía, ZCR y Frecuencia Dominante de una ventana de audio """
    # Pasamos a float para evitar desbordamientos
    audio_f = audio_window.astype(np.float32) 
    
    # 1. Energía (RMS - Root Mean Square)
    rms = np.sqrt(np.mean(audio_f**2))
    
    # 2. Zero Crossing Rate (Cruces por cero)
    # Cuántas veces la señal pasa de positivo a negativo (ideal para golpes/pasos)
    cruces = np.sum(np.abs(np.diff(np.sign(audio_f)))) / (2 * len(audio_f))
    
    # 3. Frecuencia Dominante (usando FFT)
    espectro = np.abs(np.fft.rfft(audio_f))
    frecuencias = np.fft.rfftfreq(len(audio_f), 1/fs)
    freq_dominante = frecuencias[np.argmax(espectro)]
    
    return [rms, cruces, freq_dominante]

def main():
    print("=== Pipeline Clásico: Random Forest + Audio DSP ===")
    
    # 1. Cargar Datos
    print("\n[1] Cargando dataset multimodal...")
    datos = np.load(DATA_PATH)
    X_accel = datos['X_accel']  # (N, 150, 3)
    X_audio = datos['X_audio']  # (N, 24000)
    y_text = datos['y']
    y = np.array([1 if label == 'steps' else 0 for label in y_text])
    
    # 2. Procesamiento de Características (Feature Engineering)
    print("\n[2] Extrayendo características (Acelerómetro Raw + Audio DSP)...")
    
    # Acelerómetro: Aplanamos el cubo 3D (150x3) a un vector 1D de 450 valores
    X_accel_flat = X_accel.reshape(X_accel.shape[0], -1)
    
    # Audio: Calculamos RMS, ZCR y Freq para cada ventana
    X_audio_features = np.array([extraer_features_audio(w) for w in X_audio])
    
    # Juntamos Acelerómetro (450) + Audio (3) = 453 características
    X_final = np.hstack((X_accel_flat, X_audio_features))
    
    print(f" -> Forma final de los datos: {X_final.shape} (Ventanas, Características)")
    
    # 3. Balanceo y División
    idx_none = np.where(y == 0)[0]
    idx_steps = np.where(y == 1)[0]
    min_muestras = min(len(idx_none), len(idx_steps))
    
    idx_none_bal = np.random.choice(idx_none, min_muestras, replace=False)
    idx_steps_bal = np.random.choice(idx_steps, min_muestras, replace=False)
    indices = np.concatenate([idx_none_bal, idx_steps_bal])
    np.random.shuffle(indices)
    
    X_bal = X_final[indices]
    y_bal = y[indices]
    
    X_train, X_test, y_train, y_test = train_test_split(
        X_bal, y_bal, test_size=0.15, random_state=42, stratify=y_bal
    )
    
    # 4. Entrenamiento del Bosque Aleatorio
    print("\n[3] Entrenando Random Forest...")
    clf = RandomForestClassifier(n_estimators=15, max_depth=10, random_state=42)
    clf.fit(X_train, y_train)
    
    # 5. Evaluación
    print("\n[4] Evaluando Resultados del Examen...")
    y_pred = clf.predict(X_test)
    cm = confusion_matrix(y_test, y_pred)
    
    print("\n--- REPORTE DE RENDIMIENTO ---")
    print(f"Total de ventanas evaluadas: {len(y_test)}")
    print(f"✔️  Pasos detectados (TP): {cm[1][1]}")
    print(f"✔️  Reposo detectado (TN): {cm[0][0]}")
    print(f"❌ Falsas Alarmas (FP):  {cm[0][1]}")
    print(f"❌ Pasos Ignorados (FN): {cm[1][0]}")
    print("------------------------------")
    
    # 6. Exportación a C con el script de la Cátedra
    print("\n[5] Generando código ANSI C...")
    # port() genera el C++ (orientado a objetos)
    codigo_cpp = port(clf, classmap={0: 'none', 1: 'steps'})
    
    # Llamamos al script del profesor para pasarlo a ANSI C
    # process_classifier guarda "Classifier.c" (y asume que tenés tu Classifier.h)
    process_code.process_classifier(codigo_cpp, 'RandomForest')
    
    # Movemos el archivo generado a la carpeta del ESP32
    os.makedirs(ESP32_MAIN_DIR, exist_ok=True)
    shutil.move('Classifier.c', os.path.join(ESP32_MAIN_DIR, 'Classifier.c'))
    
    print(f" -> ¡ÉXITO! Classifier.c exportado a: {ESP32_MAIN_DIR}")
    print(" -> (Asegurate de tener el Classifier.h copiado en esa carpeta también).")

if __name__ == "__main__":
    main()