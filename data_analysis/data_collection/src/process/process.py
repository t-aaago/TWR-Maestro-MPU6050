import pandas as pd
from pathlib import Path

def save_to_csv(data_dict, output_path_config):
    """
    Salva os dados coletados separando por tópico.
    data_dict: Dicionário {'topico/sub': [(ts, dist), ...]}
    output_path_config: Caminho base vindo do config.yaml
    """

    if not data_dict:
        print("Nenhum dado coletado.")
        return

    # Tratamento do caminho de saída
    base_path = Path(output_path_config)
    
    # Se o config apontava para um arquivo (ex: dados.csv), pegamos apenas a pasta pai
    if base_path.suffix: 
        output_dir = base_path.parent
    else:
        output_dir = base_path

    output_dir.mkdir(parents=True, exist_ok=True)

    print("\n--- Gerando arquivos CSV ---")

    # Itera sobre cada tópico coletado
    for topic, records in data_dict.items():
        if not records:
            continue

        df = pd.DataFrame(records, columns=["Timestamp", "Distance (m)"])
        
        # Sanitiza o nome do tópico para virar nome de arquivo
        # Ex: "sensores/quarto/distancia" vira "sensores_quarto_distancia.csv"
        safe_filename = topic.replace('/', '_').replace('\\', '_') + ".csv"
        final_path = output_dir / safe_filename

        try:
            df.to_csv(final_path, index=False)
            print(f"📄 Arquivo gerado: {final_path} ({len(df)} registros)")
        except Exception as e:
            print(f" Erro ao salvar {safe_filename}: {e}")