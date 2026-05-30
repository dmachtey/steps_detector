import os
import glob
import pandas as pd
import numpy as np

def get_label_from_filename(filename):
    basename = os.path.basename(filename).lower()
    if basename.startswith('sin_caminando'):
        return 'none'
    elif basename.startswith('caminando'):
        return 'steps'
    return 'unknown'

def segmentar_acelerometro(raw_folder, window_sec=3.0, overlap_sec=1.5, fs=50):
    """
    Lee todos los CSVs, extrae [X, Y, Z] y los pica en ventanas de tamaño fijo.
    """
    window_samples = int(window_sec * fs)          # Ej: 3.0s * 50Hz = 150 muestras
    step_samples = int((window_sec - overlap_sec) * fs) # Cuánto avanza la ventana
    
    X_data = []
    y_labels = []
    
    # Buscar todos los CSVs del acelerómetro
    csv_pattern = os.path.join(raw_folder, '*_acelerometro.csv')
    csv_files = glob.glob(csv_pattern)
    
    for file in csv_files:
        label = get_label_from_filename(file)
        if label == 'unknown':
            print(f"Ignorando archivo sin etiqueta clara: {os.path.basename(file)}")
            continue
            
        try:
            # Leer el CSV. pandas detecta automáticamente la cabecera.
            df = pd.read_csv(file)
            
            # Extraemos solo las columnas de aceleración (asumiendo que son X, Y, Z)
            # Ignoramos la columna 0 (timestamp_ms)
            data = df[['accel_x', 'accel_y', 'accel_z']].values 
            
        except Exception as e:
            print(f"Error procesando {os.path.basename(file)}: {e}")
            continue
            
        # Picar los datos en ventanas
        n_samples = len(data)
        for start in range(0, n_samples - window_samples + 1, step_samples):
            window = data[start : start + window_samples]
            
            # Opcional pero recomendado: Normalizar la ventana si los valores varían mucho
            # window = window / np.max(np.abs(window), axis=0) # (Lo dejamos comentado por ahora)
            
            X_data.append(window)
            y_labels.append(label)
            
    return np.array(X_data), np.array(y_labels)