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

    % Normalización Segura
    if max(abs(x)) ~= 0, x = x ./ max(abs(x)); end
    if max(abs(y)) ~= 0, y = y ./ max(abs(y)); end
    if max(abs(z)) ~= 0, z = z ./ max(abs(z)); end

    % SEPARACIÓN EN ALTURA (Offset)
    y = y + 1;
    z = z + 2;

    % Normalizar el tiempo del acelerómetro para que empiece en 0 segundos
    t_accel = (t_ms - t_ms(1)) / 1000;
    
    % -------------------------------------------------------------------------
    % 4. DETECCIÓN Y RECORTE DE DATOS ÚTILES (NUEVO)
    % -------------------------------------------------------------------------
    % Calculamos la longitud real de cada registro
    t_max_audio = max(t_audio);
    t_max_accel = max(t_accel);
    
    % La "parte buena" es el tiempo mínimo donde ambas señales existen
    t_util = min(t_max_audio, t_max_accel);
    
    % Si hay una diferencia mayor a 0.1 segundos, informamos al usuario
    if abs(t_max_audio - t_max_accel) > 0.1
        fprintf('\n--- AVISO DE SINCRONIZACIÓN ---\n');
        fprintf('Se detectó un corte asimétrico en la recolección de datos.\n');
        fprintf('Longitud Audio: %.2f seg | Longitud Acelerómetro: %.2f seg\n', t_max_audio, t_max_accel);
        fprintf('Recortando archivos a la parte útil y sincronizada: %.2f seg\n\n', t_util);
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
    % 5. Graficación Sincronizada
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
    xlim([0 t_util]); % Usamos t_util en lugar del antiguo t_max

    % --- Subplot 2: Acelerómetro Normalizado y Separado ---
    subplot(2, 1, 2);
    plot(t_accel, x, 'r', 'LineWidth', 1.2); hold on;
    plot(t_accel, y, 'b', 'LineWidth', 1.2);
    plot(t_accel, z, 'g', 'LineWidth', 1.2); hold off;
    
    title('Movimiento Estructural (Acelerómetro)');
    xlabel('Tiempo (Segundos)');
    
    yticks([0 1 2]);
    yticklabels({'Eje X', 'Eje Y', 'Eje Z'});
    grid on;
    xlim([0 t_util]); 

    % Vincular los ejes X de ambas gráficas
    linkaxes(findall(gcf,'type','axes'), 'x');

    disp('¡Gráficas generadas con éxito!');
end
