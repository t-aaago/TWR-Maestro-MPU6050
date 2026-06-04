import os
import subprocess
import sys
import time
import serial.tools.list_ports

def execute_subprocess(command):
    print(f"Executando: {' '.join(command)}")
    result = subprocess.run(command)
    return result.returncode == 0

def get_serial_port():
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("Erro: Nenhuma porta serial detectada. Verifique a conexão USB.")
        sys.exit(1)

    if len(ports) == 1:
        print(f"Porta detectada automaticamente: {ports[0].device} ({ports[0].description})")
        return ports[0].device

    print("Múltiplas portas seriais disponíveis:")
    for i, port in enumerate(ports):
        print(f"[{i}] {port.device} - {port.description}")

    while True:
        try:
            selection = int(input("Selecione o índice da porta desejada: "))
            if 0 <= selection < len(ports):
                return ports[selection].device
            print("Índice fora do intervalo.")
        except ValueError:
            print("Entrada inválida. Digite um número inteiro.")

def build_all_only():
    for anchor in range(1, 10):
        print(f"\n{'='*50}")
        print(f"(build_all_only) INICIANDO PROCESSO: ÂNCORA {anchor}")
        print(f"{'='*50}")

        build_dir = f"build_anchor_{anchor}"
        build_cmd = [
            "idf.py",
            "-B", build_dir,
            f"-DANCHOR_NUMBER={anchor}",
            "build"
        ]

        if not execute_subprocess(build_cmd):
            print(f"(build_all_only) Erro Fatal: Falha na compilação da ancora {anchor}")
            sys.exit(1)

    print(f"Sucesso: Todas as âncoras foram compiladas")

def build_flash_all():
    for anchor in range(1, 10):
        print(f"\n{'='*50}")
        print(f"(main) INICIANDO PROCESSO: ÂNCORA {anchor}")
        print(f"{'='*50}")

        build_dir = f"build_anchor_{anchor}"
        build_cmd = [
            "idf.py",
            "-B", build_dir,
            f"-DANCHOR_NUMBER={anchor}",
            "build"
        ]

        if not execute_subprocess(build_cmd):
            print(f"(main) Erro Fatal: Falha na compilação da ancora {anchor}")
            sys.exit(1)
        
        port = get_serial_port()

        input(f"\n(main) [AÇÃO REQUERIDA] Conecte o hardware para a Âncora {anchor} na porta {port} e pressione ENTER...")

        flash_cmd = [
            "idf.py",
            "-B", build_dir,
            "-p", port,
            "flash"
        ]

        if not execute_subprocess(flash_cmd):
            print(f"Erro fatal: Falha durante o flash da Âncora {anchor}.")
            sys.exit(1)
        
        print(f"\n-> SUCESSO: Âncora {anchor} gravada e verificada.")
        time.sleep(1)

def flash_all_only():
    for anchor in range(1, 10):
        print(f"\n{'='*50}")
        print(f"(flash_all_only) INICIANDO PROCESSO: ÂNCORA {anchor}")
        print(f"{'='*50}")

        build_dir = f"build_anchor_{anchor}"
        
        # Verificação de segurança: aborta se os binários não existirem
        if not os.path.isdir(build_dir):
            print(f"(flash_all_only) Erro Fatal: Diretório {build_dir} não encontrado. Execute a compilação (Build) primeiro.")
            sys.exit(1)
        
        port = get_serial_port()

        input(f"\n(flash_all_only) [AÇÃO REQUERIDA] Conecte o hardware para a Âncora {anchor} na porta {port} e pressione ENTER...")

        flash_cmd = [
            "idf.py",
            "-B", build_dir,
            "-p", port,
            "flash"
        ]

        if not execute_subprocess(flash_cmd):
            print(f"Erro fatal: Falha durante o flash da Âncora {anchor}.")
            sys.exit(1)
        
        print(f"\n-> SUCESSO: Âncora {anchor} gravada e verificada.")
        time.sleep(1)

def main():
    while True:
        print(f"\n{'='*50}")
        print(" MENU DE AUTOMAÇÃO DE GRAVAÇÃO UWB ")
        print(f"{'='*50}")
        print("[1] Compilar todas as âncoras (Apenas Build)")
        print("[2] Compilar e gravar sequencialmente (Build & Flash)")
        print("[3] Gravar todas as âncoras (Apenas Flash)")
        print("[0] Sair")
        
        escolha = input("\nSelecione a operação desejada: ").strip()
        
        if escolha == '1':
            build_all_only()
        elif escolha == '2':
            build_flash_all()
        elif escolha == '3':
            flash_all_only()
        elif escolha == '0':
            print("Encerrando o programa.")
            sys.exit(0)
        else:
            print("Entrada inválida. Por favor, insira 0, 1, 2 ou 3.")

if __name__ == "__main__":
    main()