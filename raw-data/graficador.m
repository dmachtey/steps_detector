function graficador(basename)
    % =========================================================================
    % Graficador de Datos: Proyecto Detección de Pasos en Estructura Flotante
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
    x = accel_data(:, 2);
    y = accel_data(:, 3);
    z = accel_data(:, 4);

    fs_accel = 50;
    t_accel = (0:length(x)-1)' / fs_accel;

    % --- 3. Normalización Robusta (Filtro Percentil 99%) ---
    disp('Filtrando picos espurios y normalizando...');

    % Ordenar valores absolutos
    sx = sort(abs(x)); sy = sort(abs(y)); sz = sort(abs(z)); sa = sort(abs(audio_data));

    % Obtener el índice del 99% (ignorando el 1% de picos más altos)
    idx_x = max(1, round(length(sx) * 0.99));
    idx_y = max(1, round(length(sy) * 0.99));
    idx_z = max(1, round(length(sz) * 0.99));
    idx_a = max(1, round(length(sa) * 0.99));

    % Normalizar usando ese percentil como "máximo real"
    if sx(idx_x) ~= 0, x = x ./ sx(idx_x); end
    if sy(idx_y) ~= 0, y = y ./ sy(idx_y); end
    if sz(idx_z) ~= 0, z = z ./ sz(idx_z); end
    if sa(idx_a) ~= 0, audio_data = audio_data ./ sa(idx_a); end

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
    disp('Generando gráficas enfocadas en la banda de energía...');
    screen_size = get(0, 'ScreenSize');
    figure('Name', ['Análisis de Pasos - Muestra: ', basename], 'Position', screen_size);


    limites_y = [-5 5]; % Aleja la vista vertical (hace que las ondas se vean más chicas)


    subplot(4, 1, 1);
    plot(t_audio, audio_data, 'k');
    title('Señal Acústica (Micrófono)');
    ylabel('Amplitud'); grid on; xlim([0 t_util]); ylim(limites_y);


    limites_y = [-1.5 1.5]; % Forzamos el zoom en la banda útil
    subplot(4, 1, 2);
    plot(t_accel, x, 'r', 'LineWidth', 1.2);
    title('Movimiento Estructural - Eje X');
    ylabel('Accel (Norm)'); grid on; xlim([0 t_util]); ylim(limites_y);

    subplot(4, 1, 3);
    plot(t_accel, y, 'b', 'LineWidth', 1.2);
    title('Movimiento Estructural - Eje Y');
    ylabel('Accel (Norm)'); grid on; xlim([0 t_util]); ylim(limites_y);

    subplot(4, 1, 4);
    plot(t_accel, z, 'g', 'LineWidth', 1.2);
    title('Movimiento Estructural - Eje Z');
    xlabel('Tiempo (Segundos)');
    ylabel('Accel (Norm)'); grid on; xlim([0 t_util]); ylim(limites_y);

    linkaxes(findall(gcf, 'type', 'axes'), 'x');
    disp('¡Gráficas individuales generadas con éxito!');
end
