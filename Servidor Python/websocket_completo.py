import asyncio
import websockets
import serial
import datetime
import json
import threading
import time

DEBUG = False  # Set to True to print instead of using actual serial communication

connected_clients = set()
serial_lock = threading.Lock()  # Lock for thread-safe serial communication
message_in_progress = False  # Flag to indicate if a message is being processed

# Initialize serial connection to Arduino
if not DEBUG:
    ser = serial.Serial('COM56', 115200)  # Update the serial port and baud rate

def send_serial_message(address, content_byte):
    global message_in_progress
    while message_in_progress:
        time.sleep(0.1)  # Wait for previous message to finish
    if not message_in_progress:
        message = bytearray([address, content_byte])
        message_in_progress = True
        if DEBUG:
            print(f"Debug: Sending to serial: {message}")
        else:
            with serial_lock:
                if ser.is_open:
                    ser.write(message)

def handle_serial_input(loop):
    asyncio.set_event_loop(loop)
    while True:
        if not DEBUG:
            with serial_lock:
                if ser.is_open and ser.in_waiting > 0:
                    line = ser.readline().decode('utf-8').strip()
                    process_serial_input(line, loop)

def process_serial_input(line, loop):
    global message_in_progress
    print("Linea: " + line)
    if "Error al enviar." in line:
        address = line.split()[-1]
        response = json.dumps({"address": int(address), "purpose": "Error"})
		
        print("Linea de error: " + str(response))
        asyncio.run_coroutine_threadsafe(broadcast_message(response), loop)
        message_in_progress = False
    elif "Mensaje recibido de:" in line:
        address = int(line.split()[-1])
        response = {"address": address}
        while True:
            line = ser.readline().decode('utf-8').strip()
            if line.startswith("Humedad:"):
                response["purpose"] = "sensors"
                response["humedad"] = float(line.split(": ")[1])
            elif line.startswith("Temperatura:"):
                response["temperature"] = float(line.split(": ")[1])
            elif line.startswith("Ruido:"):
                response["noise"] = int(line.split(": ")[1])
                asyncio.run_coroutine_threadsafe(broadcast_message(json.dumps(response)), loop)
                break
            elif "Foto:" in line:
                # Start receiving JPEG data
                print("Foto recibida")
                image_data = bytearray()
                while True:
                    byte = ser.read()
                    image_data.append(byte[0])
                    if image_data[-2:] == b'\xff\xd9':  # JPEG end marker
                        break
                print("Acabose")
                image_path = datetime.datetime.now().strftime("%Y-%m-%d-%H-%M-%S.jpeg")
                print("Guarde")
                with open("servidor_nuevo/" + image_path, 'wb') as img_file:
                    img_file.write(image_data)
                response = {
                    "address": address,
                    "purpose": "image",
                    "path": image_path
                }
                asyncio.run_coroutine_threadsafe(broadcast_message(json.dumps(response)), loop)
                break
        message_in_progress = False
    elif "Acabo" in line or "Enviado correctamente" in line:
        message_in_progress = False
      # Reset the flag when a response is received

async def broadcast_message(message):
    for client in connected_clients:
        await client.send(message)

async def handle_websocket(websocket, path):
    # Register new client
    connected_clients.add(websocket)
    try:
        async for message in websocket:
            print(f"Received message from client: {message}")

            # Parse the incoming message as JSON
            try:
                message_data = json.loads(message)
                address = message_data.get("address")
                content = message_data.get("content")
                choice = message_data.get("choice", 0)
                content_byte = 0

                if content == "photo":
                    content_byte = 0b10000000
                elif content == "sensors":
                    content_byte = 0b01000000
                elif content == "song":
                    content_byte = (choice & 0b111) << 3
                elif content == "image":
                    content_byte = choice & 0b111

                send_serial_message(address, content_byte)
            except (json.JSONDecodeError, KeyError):
                pass
    finally:
        # Unregister client when connection is closed
        connected_clients.remove(websocket)

async def push_updates():
    while True:
        # Get current date and time
        current_time = datetime.datetime.now().strftime("%H:%M:%S %d/%m/%Y")
        response = json.dumps({"type": "update", "content": current_time})

        if connected_clients:
            # Send current date and time to all connected clients
            await broadcast_message(response)

        # Wait for half a second before sending the next update
        await asyncio.sleep(0.5)

async def main():
    loop = asyncio.get_event_loop()
    start_server = await websockets.serve(handle_websocket, 'localhost', 8765)
    print(f"WebSocket server started on ws://localhost:8765")

    # Start pushing updates to clients
    asyncio.create_task(push_updates())

    # Start serial input handling in a separate thread
    if not DEBUG:
        serial_thread = threading.Thread(target=handle_serial_input, args=(loop,), daemon=True)
        serial_thread.start()

    # Keep the server running
    await asyncio.Event().wait()

if __name__ == "__main__":
    asyncio.run(main())
