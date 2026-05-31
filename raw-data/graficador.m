function graficador(basename)
    % =========================================================================
    % Graficador de Datos: Proyecto Detección de Pasos en Estructura Flotante
    % (Normalización de Hardware Espejo a C y Python con Filtro DC Blocker)
    % =========================================================================

    archivo_csv = [basename, '_acelerometro.csv'];
    archivo_wav = [basename, '_audio.wav'];

    % --- 1. Cargar Audio ---
    disp('Cargando datos de audio...');
    [audio_data, fs_audio] = audioread(archivo_wav);
    t_audio = (0:length(audio_data)-1)' / fs_audio;

    % --- 2. Cargar Acelerómetro ---
    disp('Cargando datos del acelerómetro...');
    accel_data = dlmread(archivo_csv, ',', 1, 0);
    x_raw = accel_data(:, 2);
    y_raw = accel_data(:, 3);
    z_raw = accel_data(:, 4);

    fs_accel = 50;
    t_accel = (0:length(x_raw)-1)' / fs_accel;

    % --- 3. DC Blocker y Normalización ---
    disp('Aplicando Filtro de Gravedad, Ganancia y Recorte (Clipping)...');

    DIVISOR_IMU = 1000.0;
    DIVISOR_AUDIO = 7000.0;

    % Filtro DC Blocker para el IMU (Restamos la media para eliminar gravedad)
    x_limpio = x_raw - mean(x_raw);
    y_limpio = y_raw - mean(y_raw);
    z_limpio = z_raw - mean(z_raw);

    % Aplicamos Ganancia y CLAMP al IMU (El recorte a los límites -1.0 y 1.0)
    x = max(-1.0, min(1.0, x_limpio / DIVISOR_IMU));
    y = max(-1.0, min(1.0, y_limpio / DIVISOR_IMU));
    z = max(-1.0, min(1.0, z_limpio / DIVISOR_IMU));

    % audioread ya divide por 32768 por defecto.
    % Multiplicamos para recuperar el RAW original, y aplicamos nuestro divisor:
    audio_crudo = audio_data * 32768.0;
    audio_data = max(-1.0, min(1.0, audio_crudo / DIVISOR_AUDIO));

    % --- 4. Recorte de Sincronización ---
    t_util = min(max(t_audio), max(t_accel));
    if isempty(t_util) || t_util <= 0, t_util = 0.1; end

    idx_audio = t_audio <= t_util;
    t_audio = t_audio(idx_audio);
    audio_data = audio_data(idx_audio);

    idx_accel = t_accel <= t_util;
    t_accel = t_accel(idx_accel);
    x = x(idx_accel); y = y(idx_accel); z = z(idx_accel);

    % --- 5. Graficar ---
    disp('Generando gráficas en escalas reales...');
    screen_size = get(0, 'ScreenSize');
    figure('Name', ['Análisis de Pasos - Muestra: ', basename], 'Position', screen_size);

    % --- ESCALAS REALES ---
    % Como aplicamos CLAMP entre -1 y 1, fijamos los ejes en 1.2 para ver la saturación clara
    limites_y_audio = [-1.2 1.2];
    limites_y_imu = [-1.2 1.2];

    subplot(4, 1, 1);
    plot(t_audio, audio_data, 'k');
    title('Señal Acústica (Micrófono) [Normalizada y Recortada]');
    ylabel('Amplitud'); grid on; xlim([0 t_util]); ylim(limites_y_audio);

    subplot(4, 1, 2);
    plot(t_accel, x, 'r', 'LineWidth', 1.2);
    title('Movimiento - Eje X (Sin Gravedad, Normalizado)');
    ylabel('Amplitud (Norm)'); grid on; xlim([0 t_util]); ylim(limites_y_imu);

    subplot(4, 1, 3);
    plot(t_accel, y, 'b', 'LineWidth', 1.2);
    title('Movimiento - Eje Y (Sin Gravedad, Normalizado)');
    ylabel('Amplitud (Norm)'); grid on; xlim([0 t_util]); ylim(limites_y_imu);

    subplot(4, 1, 4);
    plot(t_accel, z, 'g', 'LineWidth', 1.2);
    title('Movimiento - Eje Z (Sin Gravedad, Normalizado)');
    xlabel('Tiempo (Segundos)');
    ylabel('Amplitud (Norm)'); grid on; xlim([0 t_util]); ylim(limites_y_imu);

    linkaxes(findall(gcf, 'type', 'axes'), 'x');
    disp('¡Gráficas individuales generadas con éxito!');
end
