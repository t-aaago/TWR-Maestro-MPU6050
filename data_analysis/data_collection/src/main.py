import asyncio
import contextlib
import concurrent.futures
from mqtt.mqtt_client import MQTTClient
from process.process import save_to_csv

async def main():
    mqtt = MQTTClient()

    task = asyncio.create_task(mqtt.run())

    loop = asyncio.get_event_loop()
    executor = concurrent.futures.ThreadPoolExecutor(max_workers=1)
    try:
        # loop principal: lê input do usuário (q para parar)
        while mqtt.collecting:
            try:
                key = await asyncio.wait_for(
                    loop.run_in_executor(executor, input),
                    timeout=1.0
                )
                if key.strip().lower() == 'q':
                    await mqtt.stop_collection()
                    break
            except asyncio.TimeoutError:
                continue
    except (KeyboardInterrupt, asyncio.CancelledError):
        print("\nInterrompido pelo usuário")
        with contextlib.suppress(Exception):
            await mqtt.stop_collection()
    finally:
        # cancela a task de run se ainda estiver ativa
        if not task.done():
            task.cancel()
            with contextlib.suppress(asyncio.CancelledError):
                await task

        # salva os dados usando o caminho configurado no mqtt
        save_to_csv(mqtt.data, mqtt.output_path)
        print("Programa finalizado.")

        # garantir shutdown do executor antes de retornar
        executor.shutdown(wait=True)

        # retornar para encerrar normalmente 
        return

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        # proteção extra caso o usuário aperte Ctrl+C no momento exato do teardown
        pass