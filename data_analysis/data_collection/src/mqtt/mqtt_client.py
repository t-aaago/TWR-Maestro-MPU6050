import asyncio
import gmqtt
import yaml
import json
from datetime import datetime
from pathlib import Path
from collections import defaultdict # Importante para criar listas automaticamente

class MQTTClient:
    def __init__(self):
        with open("config/config.yaml", "r") as file:
            self.config = yaml.safe_load(file)

        self.broker = self.config["mqtt"]["broker"]
        self.port = self.config["mqtt"]["port"]
        self.base_topic = self.config["mqtt"]["topic"] # Tópico base
        self.username = self.config["mqtt"].get("username", "")
        self.password = self.config["mqtt"].get("password", "")

        # Assumindo que o path no yaml é um diretório ou arquivo base
        self.output_path = self.config["data"]["data_processed"]
        
        self.collecting = True
        
        # MUDANÇA: Agora é um dicionário de listas, não apenas uma lista
        self.data = defaultdict(list) 
        self.client = None

    async def connect(self):
        print(f"Conectando ao broker {self.broker}:{self.port}...")
        self.client = gmqtt.Client(self.config["mqtt"]["client_id"])
        self.client.on_connect = self.on_connected
        self.client.on_message = self.on_message
        self.client.on_disconnect = self.on_disconnect
        
        if self.username and self.password:
            self.client.set_username_password(self.username, self.password)

        try:
            await self.client.connect(self.broker, self.port, ssl=False)
        except Exception as e:
            print(f"Erro na conexão: {e}")
            raise

    def on_connected(self, client, flags, rc, properties):
        print(f"Conectado ao broker: {self.broker}")
        
        # MUDANÇA: Adiciona '/#' para pegar todos os subtópicos
        # Ex: se base_topic for 'sensores', ele assina 'sensores/#'
        topic_to_subscribe = f"{self.base_topic}/#"
        client.subscribe(topic_to_subscribe)
        print(f"Coletando dados em '{topic_to_subscribe}'... (pressione 'q' e Enter para parar)")

    def on_message(self, client, topic, payload, qos, properties):
        if not self.collecting:
            return

        try:
            data = json.loads(payload.decode())
            # Ajuste conforme seu JSON. Se for só {distance: 10}, ok.
            # Se o JSON variar por tópico, talvez precise de try/except específicos.
            distance = float(data.get("distance", 0)) 
            timestamp = datetime.now().isoformat()

            # MUDANÇA: Salva na lista específica daquele tópico
            self.data[topic].append((timestamp, distance))
            
            # Printa qual tópico recebeu a mensagem
            print(f"[{topic}] {timestamp} -> {distance} m")

        except Exception as e:
            print(f"Erro ao processar mensagem do tópico {topic}: {e}")

    def on_disconnect(self, client, packet, exc=None):
        print("Desconectado do broker MQTT")

    async def stop_collection(self):
        print("\nColeta finalizada.")
        self.collecting = False
        if self.client:
            await self.client.disconnect()

    async def run(self):
        await self.connect()
        while self.collecting:
            await asyncio.sleep(1)