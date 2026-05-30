import os
import glob
import pandas as pd
import numpy as np
from scipy.io import wavfile

def get_label_from_filename(filename):
    basename = os.path.basename(filename).lower()
    if basename.startswith('sin_caminando'):
        return 'none'
    elif basename.startswith('caminando'):
        return 'steps'
    return 'unknown'

def segmentar_multimodal(raw_folder, window_sec=3.0, overlap_sec=0.5, fs_accel=50, fs_audio=8000):
    """
    Lee pares de archivos (CSV y WAV) y los segmenta sincrónicamente.
    """
    # Cálculos para el acelerómetro
    window_accel = int(window_sec * fs_accel)          # 150
    step_accel = int((window_sec - overlap_sec) * fs_accel) # 125

    # Cálculos para el audio
    window_audio = int(window_sec * fs_audio)          # 24000
    step_audio = int((window_sec - overlap_sec) * fs_audio) # 20000

    X_accel_list = []
    X_audio_list = []
    y_labels = []

    # Buscar todos los CSVs
    csv_files = glob.glob(os.path.join(raw_folder, '*_acelerometro.csv'))

    for csv_file in csv_files:
        # Deducir el nombre del archivo de audio correspondiente
        base_name = csv_file.replace('_acelerometro.csv', '')
        wav_file = base_name + '_audio.wav'

        label = get_label_from_filename(csv_file)
        if label == 'unknown':
            continue

        if not os.path.exists(wav_file):
            print(f"Aviso: Falta el archivo de audio para {os.path.basename(csv_file)}. Se ignora el par.")
            continue

        try:
            # 1. Leer Acelerómetro
            df = pd.read_csv(csv_file)
            data_accel = df[['accel_x', 'accel_y', 'accel_z']].values

            # 2. Leer Audio
            fs_wav, data_audio = wavfile.read(wav_file)

            # Si el audio quedó guardado como estéreo (por el driver crudo), lo forzamos a mono
            if len(data_audio.shape) > 1:
                data_audio = data_audio[:, 0]

        except Exception as e:
            print(f"Error procesando el par {os.path.basename(base_name)}: {e}")
            continue

        # Determinar cuántas ventanas podemos sacar (limitado por el archivo que haya quedado más corto)
        n_windows_accel = (len(data_accel) - window_accel) // step_accel + 1
        n_windows_audio = (len(data_audio) - window_audio) // step_audio + 1
        n_windows = min(n_windows_accel, n_windows_audio)

        # Extraer las ventanas sincrónicamente
        for i in range(n_windows):
            start_a = i * step_accel
            start_m = i * step_audio

            win_accel = data_accel[start_a : start_a + window_accel]
            win_audio = data_audio[start_m : start_m + window_audio]

            X_accel_list.append(win_accel)
            X_audio_list.append(win_audio)
            y_labels.append(label)

    return np.array(X_accel_list), np.array(X_audio_list), np.array(y_labels)
