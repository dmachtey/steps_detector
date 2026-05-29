#!/usr/bin/env python3

#En algunos de los registros con el teléfono se
# cortó el registro del acelerómetro, este script borra y sincroniza
# la longitud (tiempo) del archivo de acelerométro y con su registro
# de audio

import os
import glob
import pandas as pd
from scipy.io import wavfile

def limpiar_csv(archivo_entrada, archivo_salida):
    """Elimina las líneas vacías o nulas al final del CSV."""
    with open(archivo_entrada, 'r', encoding='utf-8') as f:
        lineas = f.readlines()

    ultimo_indice = len(lineas) - 1
    while ultimo_indice >= 0:
        linea = lineas[ultimo_indice].strip()
        # Ignorar líneas vacías o llenas solo de comas
        if linea and linea.replace(',', '') != "":
            break
        ultimo_indice -= 1

    lineas_validas = lineas[:ultimo_indice + 1]

    with open(archivo_salida, 'w', encoding='utf-8') as f:
        f.writelines(lineas_validas)

    return archivo_salida


def calcular_duracion_csv(archivo_csv):
    """Calcula la duración, descartando saltos masivos de tiempo al final."""
    df = pd.read_csv(archivo_csv)
    col_tiempo = df.columns[0]

    # Asegurarnos de que sea numérico
    df[col_tiempo] = pd.to_numeric(df[col_tiempo], errors='coerce')
    df = df.dropna(subset=[col_tiempo]) # Quitar NaNs por si acaso

    # Calcular el tiempo entre cada muestra (delta)
    deltas = df[col_tiempo].diff()

    # Definir qué es un "salto masivo" (ej. si el salto es mayor al 5% del tiempo total, o un valor fijo)
    # Como vemos en tu gráfica un salto enorme, buscaremos el índice donde el delta se dispara
    # Si tu timestamp está en milisegundos, un salto de 1000ms (1 seg) es enorme para un acelerómetro.

    # Vamos a buscar el primer salto que rompa la continuidad (ajusta el valor según tu unidad)
    # Suponiendo que el salto anómalo es el mayor de todos:
    indice_corte = len(df) - 1

    # Si detectamos un salto anormalmente grande en el último 10% del archivo
    umbral_salto = deltas.median() * 50 # Si tarda 50 veces más que lo normal
    saltos_anomalos = deltas[deltas > umbral_salto]

    if not saltos_anomalos.empty:
        # Cortar el dataframe justo ANTES del primer salto anómalo
        indice_corte = saltos_anomalos.index[0] - 1
        print(f"    [!] Salto de tiempo detectado en la muestra {indice_corte+1}. Recortando datos corruptos.")

    inicio = df[col_tiempo].iloc[0]
    fin = df[col_tiempo].iloc[indice_corte]

    diferencia = fin - inicio

    # Conversión a segundos
    if diferencia > 1000000000: # Nanosegundos
        duracion_segundos = diferencia / 1e9
    elif diferencia > 1000000: # Microsegundos
        duracion_segundos = diferencia / 1e6
    elif diferencia > 1000: # Milisegundos
        duracion_segundos = diferencia / 1e3
    else: # Segundos
        duracion_segundos = diferencia

    # OPCIONAL: Sobreescribir el CSV para quitarle también la línea del salto
    # df.iloc[:indice_corte+1].to_csv(archivo_csv, index=False)

    return duracion_segundos

def procesar_directorio():
    # Buscar todos los archivos del acelerómetro en el directorio actual
    archivos_csv = glob.glob('*_acelerometro.csv')

    # Filtrar los que ya terminan en "_limpio.csv" por si corremos el script dos veces
    archivos_csv = [f for f in archivos_csv if not f.endswith('_limpio.csv')]

    if not archivos_csv:
        print("No se encontraron archivos de acelerómetro (.csv) en este directorio.")
        return

    for csv_file in archivos_csv:
        print(f"\nProcesando: {csv_file}")

        # Identificar el archivo de audio correspondiente
        wav_file = csv_file.replace('_acelerometro.csv', '_audio.wav')

        if not os.path.exists(wav_file):
            print(f"  [!] No se encontró el archivo de audio: {wav_file} - Saltando...")
            continue

        # Nombres para los archivos de salida
        csv_limpio = csv_file.replace('.csv', '_limpio.csv')
        wav_limpio = wav_file.replace('.wav', '_limpio.wav')

        try:
            # 1. Limpiar el CSV de los datos vacíos al final
            limpiar_csv(csv_file, csv_limpio)
            print(f"  [-] CSV limpiado de datos nulos al final.")

            # 2. Calcular la duración del registro válido en el CSV
            duracion_segundos = calcular_duracion_csv(csv_limpio)
            print(f"  [-] Duración calculada del acelerómetro: {duracion_segundos:.2f} segundos.")

            # 3. Leer y recortar el audio
            samplerate, data = wavfile.read(wav_file)
            frames_a_mantener = int(samplerate * duracion_segundos)

            # Recortar los datos de audio
            data_recortada = data[:frames_a_mantener]

            # 4. Guardar el audio recortado
            wavfile.write(wav_limpio, samplerate, data_recortada)
            print(f"  [-] Audio recortado y guardado como: {wav_limpio}")

        except Exception as e:
            print(f"  [!] Ocurrió un error al procesar {csv_file}: {e}")

if __name__ == '__main__':
    procesar_directorio()
