# UWB Data Collection, Analysis and Calibration

Projeto para coleta e análise de medidas UWB (ESP32 → MQTT), com ferramentas de
processamento em Python e scripts MATLAB para calibração (método DecaWave / DW1000).

---

**Resumo rápido**

- Coleta: cliente MQTT em Python (salva CSVs por tópico).
- Análise: scripts Python e MATLAB para visualização e estatísticas.
- Calibração: algoritmo DecaWave implementado em `data/matlab/calibration.m` para estimar
  delays agregados (antenna/device delays) do DW1000.

**Estrutura atual do repositório**

No workspace atual os arquivos estão organizados da seguinte forma (paths reais):

```
./data_analysis/                         # pasta do projeto (esta pasta)
├── config/config.yaml                    # configuração usada pelo collector
├── .gitignore
├── requirements.txt
├── data_collection/                      # collector (Python, MQTT)
│   └── src/
│       ├── main.py
│       ├── mqtt/
│       │   └── mqtt_client.py
│       └── process/process.py
├── data_analysis/                        # scripts de análise (MATLAB)
│   ├── calibration.m
│   ├── data_analysis.m
│   └── data/                             # dados de exemplo (subpastas por distância/anchor)
│       ├── 1meter/...
│       └── 4meters/...
└── README.md
```

Observação: aqui `data_analysis/data/` já contém exemplos de CSVs organizados por
condição (`1meter`, `4meters`) e por `anchor`. O collector grava em
`data_collection/src/` por padrão; você pode configurar `config/config.yaml` para
salvar diretamente em `data_analysis/data/` (veja seção Configuração).

---

**Como funciona (fluxo)**

1. `data_collection/src/main.py` conecta ao broker MQTT e armazena mensagens.
2. Cada tópico MQTT gera um CSV em `data_collection/data/` (nome: tópico com `/` → `_`).
3. Copie ou configure o collector para salvar em `data/data_processed.csv` (ou em
   `data/` como arquivos por tópico) para que os scripts de análise encontrem os dados.
4. Rode os scripts de análise em Python ou MATLAB.
5. Calibração: execute `data/matlab/calibration.m` no MATLAB (ou use a conversão
   Python caso disponível).

---

## Configuração (exemplo)

O collector usa `config/config.yaml`. Exemplo mínimo:

```yaml
mqtt:
	broker: "131.255.82.115"
	port: 1883
	topic: "argos/twr/anchor1/data"
	client_id: "uwb-validation-client"
	keepalive: 60
	username: ""
	password: ""

data:
	save_raw: true
	# Caminho final onde os CSVs devem ser colocados (pode conter variável substituível)
	data_processed: "${DATA_ANALYSIS}/data"

uwb:
	unit: "meters"
	type: "TWR"

analysis:
	save_interval_messages: 1
```

---

## Executando (Python)

1. Criar e ativar venv dentro de `data_collection/`:

```bash
cd data_collection
python -m venv .venv
source .venv/bin/activate    # Linux/macOS
\.venv\Scripts\activate    # Windows (cmd)
pip install -r requirements.txt
```

2. Executar o collector:

```bash
cd data_collection
python src/main.py
```
- Pressione Ctrl + Enter para interromper a coleta mqtt.
- Pressione `q` + Enter para salvar os CSVs e finalizar o programa.
- Os CSVs por tópico serão salvos no diretório configurado em `config.yaml`.

3. Executar analise de dados:

Abra o MATLAB e execute `data/matlab/data_analysis.m`, ou em linha de comando:

```bash
matlab -batch "run('$(pwd)/data/matlab/calibration.m')"
```

- Modifique a linha 6 para a comparação da distancia do arquivo .csv com a distância real medida.
- Modifique o caminho na linha 4 para a leitura do arquivo .csv desejado.

4.1 Calibração Ponto a Ponto:

4.2 Executar calibração (MATLAB):

Abra o MATLAB e execute `data/matlab/calibration.m`, ou em linha de comando:

```bash
matlab -batch "run('$(pwd)/data/matlab/calibration.m')"
```

---

## Sobre a calibração (DecaWave / DW1000)

O script `data/matlab/calibration.m` ainda está na fase de testes. Implementa um procedimento baseado nas notas
e exemplos da DecaWave (DW1000) para estimar atrasos agregados (antenna/device
delays). Resumidamente:

- Calcula médias das medidas entre pares (AB, BC, CA).
- Converte distâncias para ToF usando a velocidade da luz.
- Realiza uma otimização estocástica (conjunto de candidatos + seleção e perturbação)
  para minimizar a diferença (norma Frobenius) entre a matriz ToF real e a candidata.
- Retorna atrasos ótimos em ns e valores inteiros para registro no firmware DW1000
  (LSB = 15.65 ps).

Consulte o cabeçalho do `calibration.m` para parâmetros ajustáveis (número de
candidatos, iterações, limites de perturbação, fração TX/RX etc.).

---
