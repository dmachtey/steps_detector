function graficador(basename)
    % =========================================================================
    % Graficador de Datos: Proyecto Detección de Pasos en Estructura Flotante
    % =========================================================================
    % Uso en consola: graficador('nombre_del_archivo_sin_extensiones')
    % Ejemplo: graficador('caminando_20260520_081915')

    % 1. Definir los nombres de los archivos usando el basename
    archivo_csv = [basename, '_acelerometro.csv'];
    archivo_wav = [basename, '_audio.wav'];

    % -------------------------------------------------------------------------
    % 2. Procesamiento del Audio (.wav)
    % -------------------------------------------------------------------------
    disp('Cargando datos de audio...');
    [audio_data, fs_audio] = audioread(archivo_wav);

    % Crear vector de tiempo para el audio (en segundos)
    t_audio = (0:length(audio_data)-1) / fs_audio;

    % -------------------------------------------------------------------------
    % 3. Procesamiento del Acelerómetro (.csv)
    % -------------------------------------------------------------------------
    disp('Cargando datos del acelerómetro...');
    accel_data = csvread(archivo_csv);

    % Limpieza de encabezados si existen
    if accel_data(1, 1) < 1000000
        accel_data(1, :) = []; 
    end

    % Extraer columnas
    t_ms = accel_data(:, 1);
    x = accel_data(:, 2);
    y = accel_data(:, 3);
    z = accel_data(:, 4);

    % Normalización Segura (Previene NaNs por división por cero)
    if max(abs(x)) ~= 0, x = x ./ max(abs(x)); end
    if max(abs(y)) ~= 0, y = y ./ max(abs(y)); end
    if max(abs(z)) ~= 0, z = z ./ max(abs(z)); end

    % SEPARACIÓN EN ALTURA (Offset)
    % El eje X se queda en 0. Sumamos 1 al eje Y, y 2 al eje Z.
    y = y + 1;
    z = z + 2;

    % Normalizar el tiempo del acelerómetro para que empiece en 0 segundos
    t_accel = (t_ms - t_ms(1)) / 1000;
    
    % Calcular el tiempo máximo para ajustar el tamaño de los gráficos
    t_max = max(max(t_audio), max(t_accel));

    % -------------------------------------------------------------------------
    % 4. Graficación Sincronizada
    % -------------------------------------------------------------------------
    disp('Generando gráficas...');
    figure('Name', ['Análisis de Pasos - Muestra: ', basename], 'Position', [100, 100, 800, 600]);

    % --- Subplot 1: Señal de Audio ---
    subplot(2, 1, 1);
    plot(t_audio, audio_data, 'k'); 
    title('Señal Acústica (Micrófono) - Golpes de pasos / Ambiente');
    xlabel('Tiempo (Segundos)');
    ylabel('Amplitud');
    grid on;
    xlim([0 t_max]); % Ajustar tamaño de la gráfica al máximo de la muestra

    % --- Subplot 2: Acelerómetro Normalizado y Separado ---
    subplot(2, 1, 2);
    plot(t_accel, x, 'r', 'LineWidth', 1.2); hold on;
    plot(t_accel, y, 'b', 'LineWidth', 1.2);
    plot(t_accel, z, 'g', 'LineWidth', 1.2); hold off;
    
    title('Movimiento Estructural (Acelerómetro)');
    xlabel('Tiempo (Segundos)');
    
    % Cambiar las etiquetas del eje Y para que muestren los ejes correspondientes en lugar de números
    yticks([0 1 2]);
    yticklabels({'Eje X', 'Eje Y', 'Eje Z'});
    
    % legend('Eje X', 'Eje Y', 'Eje Z', 'Location', 'northeast'); % Opcional, ya está claro con los yticks
    grid on;
    xlim([0 t_max]); % Ajustar tamaño de la gráfica al máximo de la muestra

    % Vincular los ejes X de ambas gráficas para hacer Zoom simultáneo
    linkaxes(findall(gcf,'type','axes'), 'x');

    disp('¡Gráficas generadas con éxito!');
end
