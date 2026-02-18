clear; clc;

c = 299792458;      % Velocidade da luz (m/s)


%% 1) LEITURA DOS ARQUIVOS

raw_AB = readmatrix("data/deca/par_AB.csv");
raw_BC = readmatrix("data/deca/par_BC.csv");
raw_CA = readmatrix("data/deca/par_AC.csv");

% A segunda coluna contém a distância medida (m)
M_AB = mean(raw_AB(:,2));
M_BC = mean(raw_BC(:,2));
M_CA = mean(raw_CA(:,2));


%% 2) DISTÂNCIAS REAIS

Dist_Real_AB = 5.000;
Dist_Real_BC = 5.000;
Dist_Real_CA = 5.000;


%% 3) EDM ACTUAL E MEASURED

EDM_actual = [ 0           Dist_Real_AB   Dist_Real_CA;
               Dist_Real_AB   0           Dist_Real_BC;
               Dist_Real_CA Dist_Real_BC     0         ];

EDM_measured = [ 0    M_AB   M_CA;
                 M_AB  0     M_BC;
                 M_CA M_BC    0   ];


%% 4) CONVERTE PARA TOF (s)

ToF_actual   = EDM_actual   ./ c;
ToF_measured = EDM_measured ./ c;


%% 5) PARÂMETROS DO ALGORITMO

num_candidates = 1000;     % tamanho do conjunto inicial
num_iterations = 100;      % número total de iterações
keep_ratio     = 0.25;     % 25% melhores candidatos
perturb_limit  = 0.2e-9;   % ±0.2 ns = 0.2e-9 s

initial_delay = 513e-9;    % 513 ns → valor sugerido no APS014
spread_init   = 6e-9;      % ±6 ns para distribuição inicial

%% 6) GERAR CANDIDATOS INICIAIS

delays = initial_delay + (rand(num_candidates,3)*2 - 1) * spread_init;


%% 7) LOOP PRINCIPAL (100 ITERAÇÕES)

for iter = 1:num_iterations


    % Calcula o erro (norma) de cada candidato

    errors = zeros(num_candidates,1);

    for k = 1:num_candidates
        D1 = delays(k,1);
        D2 = delays(k,2);
        D3 = delays(k,3);

        % Matriz ToF_candidate (fórmula da Figura 5)
        ToF_candidate = zeros(3,3);

        % AB
        ToF_candidate(1,2) = (2*D1 + 2*D2 + 4*ToF_measured(1,2)) / 4;
        ToF_candidate(2,1) = ToF_candidate(1,2);

        % BC
        ToF_candidate(2,3) = (2*D2 + 2*D3 + 4*ToF_measured(2,3)) / 4;
        ToF_candidate(3,2) = ToF_candidate(2,3);

        % CA
        ToF_candidate(1,3) = (2*D1 + 2*D3 + 4*ToF_measured(1,3)) / 4;
        ToF_candidate(3,1) = ToF_candidate(1,3);

        % Calcula norma do erro (APS014)
        diffM = ToF_actual - ToF_candidate;
        errors(k) = norm(diffM,"fro");
    end


    % Seleciona os melhores 25%
  
    [~, idx] = sort(errors);
    num_keep = round(num_candidates * keep_ratio);

    best = delays(idx(1:num_keep), :);

    
    % Perturba esses melhores candidatos
    
    perturbed = best + (rand(num_keep,3)*2 - 1) * perturb_limit;

    
    % Constrói o novo conjunto
    
    delays = [best; perturbed];

    % Expande novamente até o tamanho total
    if size(delays,1) < num_candidates
        extra = initial_delay + (rand(num_candidates-size(delays,1),3)*2 - 1)*spread_init;
        delays = [delays; extra];
    end

    
    % Reduz a perturbação a cada 20 iterações
    
    if mod(iter,20) == 0
        perturb_limit = perturb_limit / 2;
    end

    fprintf("Iteração %d concluída (melhor erro: %g)\n", iter, errors(idx(1)));
end


%% 8) RESULTADO FINAL

best_delays_sec = delays(idx(1), :);        % segundos
best_delays_ns  = best_delays_sec * 1e9;   % nanossegundos

disp("========================================");
disp("AGGREGATE ANTENNA DELAYS (ns) – APS014");
disp("========================================");
fprintf("Dispositivo A: %.4f ns\n", best_delays_ns(1));
fprintf("Dispositivo B: %.4f ns\n", best_delays_ns(2));
fprintf("Dispositivo C: %.4f ns\n", best_delays_ns(3));
disp("========================================");

lsb_ps = 15.65e-3; % ns

Adelay_int = round(best_delays_ns ./ lsb_ps);

fprintf("\n===== VALORES PARA O FIRMWARE TWR (pizzo00) =====\n");
fprintf("Adelay A = %d\n", Adelay_int(1));
fprintf("Adelay B = %d\n", Adelay_int(2));
fprintf("Adelay C = %d\n", Adelay_int(3));
fprintf("==============================================\n");

%% 9) SEPARAÇÃO DE DELAY TX E RX (RECOMENDADO PARA SISTEMAS TDoA)

TX_fraction = 0.44;
RX_fraction = 0.56;

TX_delays_ns = best_delays_ns * TX_fraction;
RX_delays_ns = best_delays_ns * RX_fraction;

% Conversão para registrador DW1000 (LSB = 15.65 ps = 0.01565 ns)
lsb_ns = 0.01565;

TX_delays_int = round(TX_delays_ns ./ lsb_ns);
RX_delays_int = round(RX_delays_ns ./ lsb_ns);

fprintf("\n=========== DELAYS PARA TDoA (TX / RX) ===========\n");
fprintf("Dispositivo A: TX = %.3f ns (%d), RX = %.3f ns (%d)\n", ...
    TX_delays_ns(1), TX_delays_int(1), RX_delays_ns(1), RX_delays_int(1));

fprintf("Dispositivo B: TX = %.3f ns (%d), RX = %.3f ns (%d)\n", ...
    TX_delays_ns(2), TX_delays_int(2), RX_delays_ns(2), RX_delays_int(2));

fprintf("Dispositivo C: TX = %.3f ns (%d), RX = %.3f ns (%d)\n", ...
    TX_delays_ns(3), TX_delays_int(3), RX_delays_ns(3), RX_delays_int(3));

fprintf("===================================================\n");
