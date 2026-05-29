function graficador(basename)
    % =========================================================================
    % Graficador de Datos: Proyecto Detección de Pasos en Estructura Flotante
    % =========================================================================
    % Uso en consola: graficador('nombre_del_archivo_sin_extensiones')

    % 1. Definir los nombres de los archivos
    archivo_csv = [basename, '_acelerometro.csv'];
    archivo_wav = [basename, '_audio.wav'];

    % -------------------------------------------------------------------------
    % 2. Procesamiento del Audio (.wav)
    % -------------------------------------------------------------------------
    disp('Cargando datos de audio...');
    [audio_data, fs_audio] = audioread(archivo_wav);
    t_audio = (0:length(audio_data)-1)' / fs_audio;

    % Imprimimos info de depuración del audio
    fprintf(' -> Audio cargado: %.2f segundos a %d Hz\n', max(t_audio), fs_audio);
    fprintf(' -> Amplitud máxima del micro: %.4f\n', max(abs(audio_data)));

    % -------------------------------------------------------------------------
    % 3. Procesamiento del Acelerómetro (.csv)
    % -------------------------------------------------------------------------
    disp('Cargando datos del acelerómetro...');

    % Usar dlmread para saltar la primera fila (cabecera)
    accel_data = dlmread(archivo_csv, ',', 1, 0);

    % Extraer columnas de aceleración (IGNORAMOS LA COLUMNA 1 DE TIEMPO)
    x = accel_data(:, 2);
    y = accel_data(:, 3);
    z = accel_data(:, 4);

    % Normalización Segura
    if max(abs(x)) ~= 0, x = x ./ max(abs(x)); end
    if max(abs(y)) ~= 0, y = y ./ max(abs(y)); end
    if max(abs(z)) ~= 0, z = z ./ max(abs(z)); end

    % [NUEVO]: Forzamos la creación del tiempo matemáticamente a 50Hz
    fs_accel = 50;
    t_accel = (0:length(x)-1)' / fs_accel;

    fprintf(' -> Acelerómetro cargado: %.2f segundos (asumiendo %d Hz)\n', max(t_accel), fs_accel);

    % -------------------------------------------------------------------------
    % 4. DETECCIÓN Y RECORTE DE DATOS ÚTILES
    % -------------------------------------------------------------------------
    t_max_audio = max(t_audio);
    t_max_accel = max(t_accel);

    t_util = min(t_max_audio, t_max_accel);

    % Medida de seguridad extra por si los archivos están vacíos
    if isempty(t_util) || t_util <= 0
        t_util = 0.1;
    end

    if abs(t_max_audio - t_max_accel) > 0.1
        fprintf('\n--- AVISO DE SINCRONIZACIÓN ---\n');
        fprintf('Se detectó un corte asimétrico en la recolección de datos.\n');
        fprintf('Recortando ambos archivos a la parte útil sincronizada: %.2f seg\n\n', t_util);
    end

    % Recortamos los arreglos usando indexación lógica
    idx_audio = t_audio <= t_util;
    t_audio = t_audio(idx_audio);
    audio_data = audio_data(idx_audio);

    idx_accel = t_accel <= t_util;
    t_accel = t_accel(idx_accel);
    x = x(idx_accel);
    y = y(idx_accel);
    z = z(idx_accel);

    % -------------------------------------------------------------------------
    % 5. Graficación Sincronizada (4 subplots independientes)
    % -------------------------------------------------------------------------
    disp('Generando gráficas...');
    screen_size = get(0, 'ScreenSize');
    figure('Name', ['Análisis de Pasos - Muestra: ', basename], 'Position', screen_size);

    % --- Subplot 1: Señal de Audio ---
    subplot(4, 1, 1);
    plot(t_audio, audio_data, 'k');
    title('Señal Acústica (Micrófono) - Golpes de pasos / Ambiente');
    ylabel('Amplitud');
    grid on;
    xlim([0 t_util]);

    % --- Subplot 2: Eje X del Acelerómetro ---
    subplot(4, 1, 2);
    plot(t_accel, x, 'r', 'LineWidth', 1.2);
    title('Movimiento Estructural - Eje X');
    ylabel('Aceleración (Norm)');
    grid on;
    xlim([0 t_util]);

    % --- Subplot 3: Eje Y del Acelerómetro ---
    subplot(4, 1, 3);
    plot(t_accel, y, 'b', 'LineWidth', 1.2);
    title('Movimiento Estructural - Eje Y');
    ylabel('Aceleración (Norm)');
    grid on;
    xlim([0 t_util]);

    % --- Subplot 4: Eje Z del Acelerómetro ---
    subplot(4, 1, 4);
    plot(t_accel, z, 'g', 'LineWidth', 1.2);
    title('Movimiento Estructural - Eje Z');
    xlabel('Tiempo (Segundos)');
    ylabel('Aceleración (Norm)');
    grid on;
    xlim([0 t_util]);

    % Vincular los ejes X de las 4 gráficas de manera simultánea
    linkaxes(findall(gcf, 'type', 'axes'), 'x');

    disp('¡Gráficas individuales generadas con éxito!');
end
