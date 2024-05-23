#include <SPI.h>
#include <RadioHead.h>
#include "../RadioHead/RH_NRF24.h"
#include "../RadioHead/RHMesh.h"

#define CLIENT_ADDRESS 1
#define SERVER1_ADDRESS 2
#define SERVER2_ADDRESS 3

#define CURR_ADDR CLIENT_ADDRESS

RH_NRF24 driver(32, 15);
// RH_NRF24 driver(8, 53);
RHMesh manager(driver, CURR_ADDR);

int prev_mod = 0;
int led_state = 0;


uint8_t buffer_bytes[5523];

void setup() {
  // Initialize the serial communication at 115200 baud rate
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  //   prev_mod = millis() % 500;

  Serial.println("Paso");

  delay(1000);


  if (!manager.init())
    Serial.println("RF22 init failed");


  Serial.println("Entre setchannel e init");

  if (!driver.setChannel(100)) Serial.println("Setchannel fallido");

  Serial.println("Acabo");
}

int semaforo_llegada = true;

uint8_t buff_llegada[100];


uint8_t from;
uint8_t len_buff_salida = sizeof(buff_llegada);

void loop() {
  // Check if at least two bytes are available to read
  if (Serial.available() > 2) {
    while (Serial.available() > 0) {
      Serial.read();
    }
  }

  if (Serial.available() == 2 && semaforo_llegada) {


    // Serial.print("Disponible: ");
    // Serial.println(Serial.available());
    // Read the first byte (Direccion)
    uint8_t direccion = Serial.read();

    // Read the second byte (Contenido)
    uint8_t contenido = Serial.read();

    uint8_t datos_a_enviar[1] = { contenido };



    // Print the received bytes in binary format
    Serial.print("Direccion: 0b");
    for (int i = 7; i >= 0; i--) {
      Serial.print((direccion >> i) & 0x01);
    }

    Serial.print("\tContenido: 0b");
    for (int i = 7; i >= 0; i--) {
      Serial.print((contenido >> i) & 0x01);
    }
    Serial.println("\nEnviando");
    uint8_t error = manager.sendtoWait(datos_a_enviar, sizeof(datos_a_enviar), direccion);
    if (error == RH_ROUTER_ERROR_NONE) {
        
      if (contenido < 0b01000000) {
        Serial.println("Enviado correctamente");
      }

    } else {
      Serial.print("Error al enviar. Error: ");
      Serial.println(direccion);
    }

    // Serial.println();
  }




  //     while (manager.available()) {
  len_buff_salida = sizeof(buff_llegada);
  if (manager.recvfromAck(buff_llegada, &len_buff_salida, &from)) {
    Serial.print("Mensaje recibido de: ");
    Serial.println(from);
    // len_buff_salida = sizeof(buff_llegada);
    // Serial.println(len_buff_salida);
    // Serial.println((char*)buff_llegada);
    //     for (int i = 0; i < len_buff_salida; i++) {
    //         printf("\\x%02x", buff_llegada[i]);
    //         Serial.println();
    //     }
    // }
    int curr_indice = 0;
    semaforo_llegada = false;
    for (int i = 0; curr_indice < len_buff_salida; i++) {
      if (buff_llegada[curr_indice] == 1) {
        char datos_humedad[7];
        strncpy(datos_humedad, (char *)(buff_llegada + curr_indice + 1), 6);
        datos_humedad[6] = '\0';

        Serial.print("Humedad: ");
        Serial.println(datos_humedad);
        curr_indice += 7;
      } else if (buff_llegada[curr_indice] == 2) {
        char datos_temperatura[7];
        strncpy(datos_temperatura, (char *)(buff_llegada + curr_indice + 1), 6);
        datos_temperatura[6] = '\0';

        Serial.print("Temperatura: ");
        Serial.println(datos_temperatura);
        curr_indice += 7;
      } else if (buff_llegada[curr_indice] == 3) {
        uint32_t ruido2 = (buff_llegada[curr_indice + 1] << 24) | (buff_llegada[curr_indice + 2] << 16) | (buff_llegada[curr_indice + 3] << 8) | buff_llegada[curr_indice + 4];

        // float ruido = *(float *)&ruido2;

        Serial.print("Ruido: ");
        Serial.println(ruido2);
        curr_indice += 5;
      } else if (buff_llegada[curr_indice] == 4) {
        buff_llegada[curr_indice] = 0;
        uint32_t longitud = (buff_llegada[curr_indice + 1] << 24) | (buff_llegada[curr_indice + 2] << 16) | (buff_llegada[curr_indice + 3] << 8) | buff_llegada[curr_indice + 4];

        Serial.print("Foto: ");
        Serial.print(longitud);
        Serial.print(", ciclos: ");
        int ciclos = ceil(longitud / 21) + 1;
        int offset_rx = 0;
        Serial.println(ciclos);
        for (int j = 0; j < ciclos; j++) {
          // Serial.println();

          // Serial.println(j);
          int error = 0;
          len_buff_salida = sizeof(buffer_bytes);

          while (!manager.recvfromAckTimeout(buffer_bytes + offset_rx, &len_buff_salida, 3000, &from)) {
            len_buff_salida = sizeof(buffer_bytes);
            // if (++error >= 3) break;
          }
          //   Serial.print(j);
          //   Serial.print(" ");
          //   Serial.println(len_buff_salida);
          //   int ultimo = 0;
          //   for (int k = 0; k < len_buff_salida; k++) {
          //     ultimo += sprintf(buffer_bytes + ultimo, "%02x", buff_llegada[k]);
          //   }
          if (offset_rx + len_buff_salida >= 5000 || j >= ciclos - 1) {

            //   buffer_bytes[offset_rx+len_buff_salida] = '\;
            int aux = Serial.write((char *)buffer_bytes, offset_rx + len_buff_salida);
            // Serial.println();
            // Serial.print("Len escrito: ");
            // Serial.println(aux);
            // delay(50);
            offset_rx = 0;
          } else {
            offset_rx += len_buff_salida;
          }
          //   delayMicroseconds(10);
        }
        break;
        // Serial.println("Fin_foto");
      } else {
        break;
      }
    }
    while (Serial.available() > 0) {
      Serial.read();
    }

    delay(10);
    while (Serial.available() > 0) {
      Serial.read();
    }
    semaforo_llegada = true;
  }







  // Main code can do other things here
  // For demonstration purposes, we'll just toggle an LED connected to pin 13
  // every 500 milliseconds
  int modulo = millis() % 500;
  if (modulo < prev_mod) {
    led_state = !led_state;
    digitalWrite(2, led_state);
  }
  prev_mod = modulo;
}
