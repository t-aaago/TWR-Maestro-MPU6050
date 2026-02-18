clear; clc; close all;

%% === CONFIGURAÇÕES ===
file_measured = 'data/1meter/anchor2/argos_twr_anchor2_data.csv'; 

distancia_real = 0.707; % DISTÂNCIA REAL (em metros)

%% === LEITURA E EXTRAÇÃO ===

if ~isfile(file_measured)
    error('Arquivo %s não encontrado.', file_measured);
end

data = readtable(file_measured);

y_measured = data.Distance_m_;
x_samples = (1:length(y_measured))';

%% === CÁLCULO DO ERRO ===

error_vals = y_measured - distancia_real; % Erro com sinal 
error_abs = abs(error_vals);              % Erro absoluto 

%% === CÁLCULO DO R95 (PRECISÃO) ===
% 1. Ordena os erros absolutos do menor para o maior
sorted_errors = sort(error_abs);

% 2. Cria o eixo de probabilidade (CDF)
cdf_prob = (1:length(sorted_errors))' / length(sorted_errors);

% 3. Encontra o índice de 95% (Corrigido com ceil)
idx_r95 = ceil(0.95 * length(sorted_errors));
if idx_r95 < 1, idx_r95 = 1; end % Proteção

val_r95 = sorted_errors(idx_r95);

% Estatísticas adicionais
mean_error = mean(error_abs);
bias_error = mean(error_vals); % Se positivo, sensor mede a mais; negativo, a menos
std_dev = std(error_vals);
rmse = sqrt(mean(error_vals.^2));
median_error = median(error_abs);

fprintf('\n=== RESULTADOS (Real = %.2fm) ===\n', distancia_real);
fprintf('Viés (Erro Médio com sinal): %.4f m\n', bias_error);
fprintf('Erro Médio Absoluto (MAE):   %.4f m\n', mean_error);
fprintf('Desvio Padrão (Precisão):    %.4f m\n', std_dev);
fprintf('R95 (95%% confiabilidade):    %.4f m\n', val_r95);
fprintf('RMSE:                        %.4f m\n', rmse);
fprintf('Mediana do Erro Absoluto:   %.4f m\n', median_error);

%% === GRÁFICO 1: MEDIÇÕES VS REAL (FIXO) ===
figure('Name', 'Analise Estática UWB', 'Color', 'w');
hold on;

% Medidas
stairs(x_samples, y_measured, '--', 'LineWidth', 1.2, ...
       'DisplayName', 'Amostras (TWR)');

% Linha de referência
yline(distancia_real,'-g', 'LineWidth', 2.2, ...
      'DisplayName', sprintf('Distância real: %.4fm', distancia_real));

% Regressão linear
p = polyfit(x_samples, y_measured, 1);
y_reg = polyval(p, x_samples);

plot(x_samples, y_reg, ...
     'LineWidth', 1.8, ...
     'Marker', 's', ...
     'MarkerSize', 6, ...
     'MarkerIndices', 1:round(length(x_samples)/25):length(x_samples), ...
     'DisplayName', sprintf('Regressão: y = %.6fx + %.6f', p(1), p(2)));

grid on; 
box on;

set(gca, 'FontSize', 14);

xlabel('Número da Amostra', 'FontSize', 16);
ylabel('Distância [m]', 'FontSize', 16);
title(sprintf('Estabilidade da Medição (Real = %.4fm)', distancia_real), ...
      'FontSize', 18, 'FontWeight', 'bold');

legend('Location', 'best', 'FontSize', 13);

xlim([1 length(x_samples)]);

% PASSO DE 0.025 NO EIXO Y (pedido seu)
yl = ylim;
%yticks( floor(yl(1)/0.025)*0.025 : 0.025 : ceil(yl(2)/0.025)*0.025 );

hold off;


%% === GRÁFICO 2: HISTOGRAMA E CDF (PRECISÃO) ===
figure('Name', 'Distribuição do Erro', 'Color', 'w');

% Subplot 1: Histograma do Erro
subplot(2,1,1);
histogram(error_vals, 100, 'FaceAlpha', 0.8, 'DisplayName', 'Amostra dos erros');
xline(0, 'k--', 'LineWidth', 1.8, 'DisplayName', sprintf('Erro nulo'));
xline(bias_error, 'r-', 'LineWidth', 1.8, 'DisplayName', sprintf('Viés(erro médio com sinal): %.5f', bias_error));

set(gca, 'FontSize', 16);
%xticks(min(error_vals) : 0.5 : max(error_vals));

grid on;
title('Histograma do Erro (Medido - Real)', 'FontSize', 18, 'FontWeight', 'bold');
xlabel('Erro [m]', 'FontSize', 16);
ylabel('Contagem', 'FontSize', 16);
legend('Location', 'best', 'FontSize', 14);

% Subplot 2: CDF (R95)
subplot(2,1,2);
plot(sorted_errors, cdf_prob * 100, ...
    'LineWidth', 2, ...
    'Marker', 's', ...                 % Quadrados
    'MarkerSize', 6, ...
    'MarkerIndices', 1:round(length(sorted_errors)/20):length(sorted_errors));

hold on;

% Linha em 95%
yline(95, '--r', '95%', ...
    'LineWidth', 1.5, ...
    'LabelHorizontalAlignment', 'left',...
    'FontSize', 14);

% Linha do R95
xline(val_r95, '--k', sprintf('R95 = %.3fm', val_r95), ...
    'LineWidth', 1.5, ...
    'LabelVerticalAlignment', 'bottom', ...
    'FontSize', 14);

grid on;
set(gca, 'FontSize', 14);

title('CDF do Erro Absoluto', 'FontSize', 18, 'FontWeight', 'bold');
xlabel('Erro Absoluto [m]', 'FontSize', 16);
ylabel('Probabilidade [%]', 'FontSize', 16);

ylim([0 105]);
yticks(0 : 10 : 100);

xlim([0 max(sorted_errors)]);
%xticks(0 : 0.025 : max(sorted_errors));

hold off;
