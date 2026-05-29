
import os
import glob
import numpy as np
import pandas as pd

def procesar_y_remuestrear_50hz(archivo_entrada, archivo_salida):
    # 1. Leer el CSV original
    df = pd.read_csv(archivo_entrada)

    # Asumimos que la primera columna siempre es el tiempo/timestamp
    col_tiempo = df.columns[0]

    # Asegurar que el tiempo sea numérico y limpiar filas rotas
    df[col_tiempo] = pd.to_numeric(df[col_tiempo], errors='coerce')
    df = df.dropna(subset=[col_tiempo]).reset_index(drop=True)

    if len(df) < 2:
        print(f"  [!] {archivo_entrada} no tiene suficientes datos válidos.")
        return

    # 2. Cortar el salto anómalo del final (basado en la gráfica de Octave)
    deltas = df[col_tiempo].diff()
    median_delta = deltas.median()

    # Si un paso de tiempo es 50 veces mayor que el promedio, es la anomalía de cierre
    umbral_salto = median_delta * 50
    saltos = deltas[deltas > umbral_salto]

    if not saltos.empty:
        indice_corte = saltos.index[0] - 1
        df = df.iloc[:indice_corte + 1]
        print(f"  [!] Anomalía detectada. Archivo recortado en la muestra {indice_corte}.")

    # 3. Normalizar el tiempo a segundos empezando en 0
    tiempos_originales = df[col_tiempo].values

    # Auto-detectar si está en milisegundos o segundos
    if median_delta > 5:  # Si el delta promedio es grande, son ms (ej: 20ms o 40ms)
        tiempos_segundos = (tiempos_originales - tiempos_originales[0]) / 1000.0
    else:
        tiempos_segundos = tiempos_originales - tiempos_originales[0]

    duracion_total = tiempos_segundos[-1]

    # 4. Crear la nueva grilla de tiempo a 50 Hz (1 / 50 = 0.02 segundos por paso)
    paso_50hz = 0.02
    t_50hz = np.arange(0, duracion_total, paso_50hz)

    # 5. Interpolar cada columna de datos sobre la nueva grilla de 50 Hz
    columnas_datos = df.columns[1:]  # Excluimos la columna de tiempo
    datos_resampleados = {}

    for col in columnas_datos:
        valores_originales = df[col].values
        # Interpolación lineal estándar
        datos_resampleados[col] = np.interp(t_50hz, tiempos_segundos, valores_originales)

    # 6. Crear el nuevo DataFrame SIN la columna de tiempo
    df_50hz = pd.DataFrame(datos_resampleados)

    # Guardar el resultado (index=False para no meter una columna de IDs)
    df_50hz.to_csv(archivo_salida, index=False)
    print(f"  [+] Remuestreado exitoso: {len(t_50hz)} filas a 50Hz escritas en {archivo_salida}")


def procesar_todo_el_directorio():
    # Buscar todos los archivos que terminen en _acelerometro.csv
    archivos = glob.glob('*_acelerometro.csv')

    # Evitar procesar archivos que ya hayan sido convertidos en una corrida previa
    archivos = [f for f in archivos if not f.endswith('_50Hz.csv')]

    if not archivos:
        print("No se encontraron archivos *_acelerometro.csv en el directorio.")
        return

    print(f"Se encontraron {len(archivos)} archivos para procesar.\n" + "-"*50)

    for archivo in archivos:
        print(f"Procesando: {archivo}")
        archivo_salida = archivo.replace('_acelerometro.csv', '_50Hz.csv')
        try:
            procesar_y_remuestrear_50hz(archivo, archivo_salida)
        except Exception as e:
            print(f"  [!] Error al procesar {archivo}: {e}")


if __name__ == '__main__':
    procesar_todo_el_directorio()
