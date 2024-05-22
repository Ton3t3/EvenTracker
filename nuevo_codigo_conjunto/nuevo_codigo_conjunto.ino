#include <DHT.h>
#include <Notes.h>
// #include <ToneESP32.h>
#include <FastLED.h>
// #define RH_ESP32_USE_HSPIç
#include <RadioHead.h>
#include <SPI.h>
// SPIClass hspi = SPIClass(2); //HSPI
// #include <RHSoftwareSPI.h>

// RHSoftwareSPI spi;

#include <stdint.h>

#include "../RadioHead/RHMesh.h"
#include "../RadioHead/RH_NRF24.h"
#include "esp_camera.h"

#define MAX_LENGTH_ANTONIO 21

typedef struct fsm_t fsm_t;

typedef int (*fsm_input_func_t)(fsm_t *);
typedef void (*fsm_output_func_t)(fsm_t *);

typedef struct fsm_trans_t {
  int orig_state;
  fsm_input_func_t in;
  int dest_state;
  fsm_output_func_t out;
} fsm_trans_t;

struct fsm_t {
  int current_state;
  fsm_trans_t *tt;
};

void fsm_init(fsm_t *that, fsm_trans_t *tt) {
  that->tt = tt;
  that->current_state = tt[0].orig_state;
}

fsm_t *fsm_new(fsm_trans_t *tt) {
  fsm_t *that = (fsm_t *)malloc(sizeof(fsm_t));
  fsm_init(that, tt);
  return that;
}

void fsm_fire(fsm_t *that) {
  fsm_trans_t *t;
  for (t = that->tt; t->orig_state >= 0; ++t) {
    if ((that->current_state == t->orig_state) && t->in(that)) {
      that->current_state = t->dest_state;
      if (t->out) t->out(that);
      break;
    }
  }
}

#define MAX9814_PIN 33
#define MAX4466_PIN 33
#define DHT22_PIN 33
#define DHT11_PIN 33
#define DHTTYPE1 DHT22
#define DHTTYPE2 DHT11
DHT dht1(DHT11_PIN, DHT22);

#define CLIENT_ADDRESS 1
#define SERVER1_ADDRESS 2
#define SERVER2_ADDRESS 3
#define SERVER3_ADDRESS 4
#define SERVER4_ADDRESS 5
#define CURR_ADDR SERVER1_ADDRESS
#define MAX_LENGTH_ANTONIO 21

char buff_salida[RH_MESH_MAX_MESSAGE_LEN];
uint8_t len_buff_salida = 0;

#if defined(__AVR_ATmega2560__)
RH_NRF24 driver(8, 53);
#elif defined(ESP32)
RH_NRF24 driver(32, 15);
#endif
RHMesh manager(driver, CURR_ADDR);

uint8_t data[] = "to you from server1";
uint8_t buffer_final[10000];
uint8_t buffer_camara[5];
uint8_t buffer_sensores[50];

// Dont put that on the stack:
uint8_t buf[100];

fsm_t *fsmcantar;
fsm_t *fsmgeneral;

uint32_t longitud_final;
uint32_t longitud_camara = 0;
uint32_t contador_idx_respuesta = 0;

uint8_t len = 100;
uint8_t from;
uint8_t header;
int prev_modulo_leer_energia = 0;
int prev_modulo_reset_energia = 0;
uint32_t registro_energia = 0;
uint32_t energia = 0;

#define ZUMBADOR_PIN 33



const uint32_t libreria_luces[2][64] = {
  /*IMAGEN 0 => FUEGO*/
  { 0x000000, 0x000000, 0xff0000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0xff0000, 0xff0000, 0x000000, 0xff0000, 0x000000, 0x000000,
    0x000000, 0x000000, 0xff0000, 0xff0000, 0xff0000, 0xff0000, 0x000000, 0x000000,
    0x000000, 0xff0000, 0xff0000, 0xff5e00, 0xff0000, 0xff0000, 0x000000, 0x000000,
    0x000000, 0xff0000, 0xff5e00, 0xfff200, 0xff0000, 0xff0000, 0xff0000, 0x000000,
    0xff0000, 0xff0000, 0xff5e00, 0xfff200, 0xfff200, 0xff5e00, 0xff0000, 0x000000,
    0xff0000, 0xff5e00, 0xfff200, 0xfff200, 0xfff200, 0xfff200, 0xff5e00, 0xff0000,
    0xff0000, 0xff5e00, 0xfff200, 0xfff200, 0xfff200, 0xfff200, 0xff5e00, 0xff0000 },
  /*IMAGEN 1 => PROHIBIDO*/
  { 0x000000, 0x000000, 0xff0000, 0xff0000, 0xff0000, 0xff0000, 0x000000, 0x000000,
    0x000000, 0xff0000, 0x000000, 0x000000, 0x000000, 0x000000, 0xff0000, 0x000000,
    0xff0000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0xff0000,
    0xff0000, 0x000000, 0xff0000, 0xff0000, 0xff0000, 0xff0000, 0x000000, 0xff0000,
    0xff0000, 0x000000, 0xff0000, 0xff0000, 0xff0000, 0xff0000, 0x000000, 0xff0000,
    0xff0000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0xff0000,
    0x000000, 0xff0000, 0x000000, 0x000000, 0x000000, 0x000000, 0xff0000, 0x000000,
    0x000000, 0x000000, 0xff0000, 0xff0000, 0xff0000, 0xff0000, 0x000000, 0x000000 }
};


#define VOLUMEN 10

struct Nota_beep {
  int nota;
  int duration;
};

// Canción Star Wars
int cantar_nota_max_star_wars = 72;
Nota_beep cancion_star_wars[] = {
  { NOTE_A4, 500 }, { NOTE_A4, 500 }, { NOTE_A4, 500 }, { NOTE_F4, 350 }, { NOTE_C5, 150 }, { NOTE_A4, 500 }, { NOTE_F4, 350 }, { NOTE_C5, 150 }, { NOTE_A4, 650 }, { 0, 500 }, { NOTE_E5, 500 }, { NOTE_E5, 500 }, { NOTE_E5, 500 }, { NOTE_F5, 350 }, { NOTE_C5, 150 }, { NOTE_GS4, 500 }, { NOTE_F4, 350 }, { NOTE_C5, 150 }, { NOTE_A4, 650 }, { 0, 500 }, { NOTE_A5, 500 }, { NOTE_A4, 300 }, { NOTE_A4, 150 }, { NOTE_A5, 500 }, { NOTE_GSH, 325 }, { NOTE_G5, 175 }, { NOTE_FS5, 125 }, { NOTE_F5, 125 }, { NOTE_FS5, 250 }, { 0, 325 }, { NOTE_AS, 250 }, { NOTE_DS5, 500 }, { NOTE_D5, 325 }, { NOTE_CS5, 175 }, { NOTE_C5, 125 }, { NOTE_AS4, 125 }, { NOTE_C5, 250 }, { 0, 350 }, { NOTE_F4, 250 }, { NOTE_GS4, 500 }, { NOTE_F4, 350 }, { NOTE_A4, 125 }, { NOTE_C5, 500 }, { NOTE_A4, 375 }, { NOTE_C5, 125 }, { NOTE_E5, 650 }, { 0, 500 }, { NOTE_A5, 500 }, { NOTE_A4, 300 }, { NOTE_A4, 150 }, { NOTE_A5, 500 }, { NOTE_GSH, 325 }, { NOTE_G5, 175 }, { NOTE_FS5, 125 }, { NOTE_F5, 125 }, { NOTE_FS5, 250 }, { 0, 325 }, { NOTE_AS, 250 }, { NOTE_DS5, 500 }, { NOTE_D5, 325 }, { NOTE_CS5, 175 }, { NOTE_C5, 125 }, { NOTE_AS4, 125 }, { NOTE_C5, 250 }, { 0, 350 }, { NOTE_F4, 250 }, { NOTE_GS4, 500 }, { NOTE_F4, 375 }, { NOTE_C5, 125 }, { NOTE_A4, 500 }, { NOTE_F4, 375 }, { NOTE_C5, 125 }, { NOTE_A4, 650 }
};

// Canción Game of Thrones
int cantar_nota_max_game_of_thrones = 78;
Nota_beep cancion_game_of_thrones[] = {
  { NOTE_G4, 500 }, { NOTE_C4, 500 }, { NOTE_DS4, 250 }, { NOTE_F4, 250 }, { NOTE_G4, 500 }, { NOTE_C4, 500 }, { NOTE_DS4, 250 }, { NOTE_F4, 250 }, { NOTE_G4, 500 }, { NOTE_C4, 500 }, { NOTE_DS4, 250 }, { NOTE_F4, 250 }, { NOTE_G4, 500 }, { NOTE_C4, 500 }, { NOTE_E4, 250 }, { NOTE_F4, 250 }, { NOTE_G4, 500 }, { NOTE_C4, 500 }, { NOTE_E4, 250 }, { NOTE_F4, 250 }, { NOTE_G4, 500 }, { NOTE_C4, 500 }, { NOTE_E4, 250 }, { NOTE_F4, 250 }, { NOTE_G4, 500 }, { NOTE_C4, 500 }, { NOTE_DS4, 250 }, { NOTE_F4, 250 }, { NOTE_D4, 500 }, { NOTE_G3, 500 }, { NOTE_AS3, 250 }, { NOTE_C4, 250 }, { NOTE_D4, 500 }, { NOTE_G3, 500 }, { NOTE_AS3, 250 }, { NOTE_C4, 250 }, { NOTE_D4, 500 }, { NOTE_G3, 500 }, { NOTE_AS3, 250 }, { NOTE_C4, 250 }, { NOTE_D4, 1000 }, { NOTE_F4, 1000 }, { NOTE_AS3, 1000 }, { NOTE_DS4, 250 }, { NOTE_D4, 250 }, { NOTE_F4, 1000 }, { NOTE_AS3, 1000 }, { NOTE_DS4, 250 }, { NOTE_D4, 250 }, { NOTE_C4, 500 }, { NOTE_GS3, 250 }, { NOTE_AS3, 250 }, { NOTE_C4, 500 }, { NOTE_F3, 500 }, { NOTE_GS3, 250 }, { NOTE_AS3, 250 }, { NOTE_C4, 500 }, { NOTE_F3, 500 }, { NOTE_G4, 1000 }, { NOTE_C4, 1000 }, { NOTE_DS4, 250 }, { NOTE_F4, 250 }, { NOTE_G4, 1000 }, { NOTE_C4, 1000 }, { NOTE_DS4, 250 }, { NOTE_F4, 250 }, { NOTE_D4, 500 }, { NOTE_G3, 500 }, { NOTE_AS3, 250 }, { NOTE_C4, 250 }, { NOTE_D4, 500 }, { NOTE_G3, 500 }, { NOTE_AS3, 250 }, { NOTE_C4, 250 }, { NOTE_D4, 500 }, { NOTE_G3, 500 }, { NOTE_AS3, 250 }, { NOTE_C4, 250 }, { NOTE_D4, 500 }
};

// Canción Game of Thrones
int cantar_nota_max_happy_bday = 24;
Nota_beep cancion_happy_bday[] = {
  { NOTE_G3, 200 }, { NOTE_G3, 200 }, { NOTE_A3, 500 }, { NOTE_G3, 500 }, { NOTE_C4, 500 }, { NOTE_B3, 1000 }, { NOTE_G3, 200 }, { NOTE_G3, 200 }, { NOTE_A3, 500 }, { NOTE_G3, 500 }, { NOTE_D4, 500 }, { NOTE_C4, 1000 }, { NOTE_G3, 200 }, { NOTE_G3, 200 }, { NOTE_G4, 500 }, { NOTE_E4, 500 }, { NOTE_C4, 500 }, { NOTE_B3, 500 }, { NOTE_A3, 750 }, { NOTE_F4, 200 }, { NOTE_F4, 200 }, { NOTE_E4, 500 }, { NOTE_C4, 500 }, { NOTE_D4, 500 }, { NOTE_C4, 1000 }
};

int cantar_nota_max_porompompo = 29;
Nota_beep cancion_porompompo[] = {
  { NOTE_B4, 150 }, { NOTE_G5, 250 }, { NOTE_FS5, 250 }, { NOTE_E5, 500 }, { 0, 250 }, { NOTE_E5, 250 }, { NOTE_E5, 150 }, { NOTE_E5, 250 }, { NOTE_E5, 150 }, { NOTE_FS5, 250 }, { NOTE_E5, 250 }, { NOTE_D5, 150 }, { NOTE_D5, 250 }, { NOTE_CS5, 150 }, { NOTE_D5, 500 }, { NOTE_D5, 150 }, { NOTE_D5, 250 }, { NOTE_D5, 150 }, { NOTE_E5, 250 }, { NOTE_D5, 250 }, { NOTE_C5, 150 }, { NOTE_C5, 250 }, { NOTE_B4, 150 }, { NOTE_C5, 500 }, { NOTE_C5, 150 }, { NOTE_C5, 250 }, { NOTE_C5, 150 }, { NOTE_D5, 250 }, { NOTE_C5, 250 }, { NOTE_B4, 1000 }
};

#define LED_PIN 33
#define COLOR_ORDER GRB
#define CHIPSET WS2812B
uint8_t BRIGHTNESS = 50;
const uint8_t kMatrixWidth = 8;
const uint8_t kMatrixHeight = 8;
const bool kMatrixVertical = false;
#define NUM_LEDS 64
CRGB leds_plus_safety_pixel[NUM_LEDS + 1];
CRGB *const leds(leds_plus_safety_pixel + 1);

// uint8_t* buf_camara_aux;
camera_fb_t *fb;

uint32_t longitud_buf_camara;
camera_config_t config;

void setup_camera() {
  // camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 4;
  config.pin_d1 = 5;
  config.pin_d2 = 18;
  config.pin_d3 = 19;
  config.pin_d4 = 36;
  config.pin_d5 = 39;
  config.pin_d6 = 34;
  config.pin_d7 = 35;
  config.pin_xclk = 21;
  config.pin_pclk = 22;
  config.pin_vsync = 25;
  config.pin_href = 23;
  config.pin_sccb_sda = 26;
  config.pin_sccb_scl = 27;
  config.pin_pwdn = -1;
  config.pin_reset = -1;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_VGA;    // FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  // config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_DRAM;
  config.jpeg_quality = 20;
  config.fb_count = 1;

  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
    //   Serial.println("Hay PSRAM");
      config.jpeg_quality = 20;
      config.fb_count = 1;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_VGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 1;
#endif
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);

  //   return esp_camera_fb_get();
}

void setup_LEDs() {
  quitar_buzzer(LOW, LOW);

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(BRIGHTNESS);
  DrawDarkFrame();
}

void setup_MAX9814() {
  pinMode(MAX9814_PIN, INPUT);
}

void setup_MAX4466() {
  pinMode(MAX4466_PIN, INPUT);
}

void setup_DHT(DHT *dht) {
  dht->begin();
}

int read_microphone(int pin) {

  quitar_buzzer(LOW, HIGH);

  int aux = analogRead(pin);
  pinMode(pin, OUTPUT);

  volver_a_poner_buzzer();

  return abs(aux - 2048);
}
float read_humidity(DHT *dht) {
  //   static uint8_t humidity = 10;
  //   return humidity++;
  quitar_buzzer(HIGH, LOW);
  //   pinMode(33, INPUT);
  float aux = dht->readHumidity();

  volver_a_poner_buzzer();

  return aux;
}

float read_temperature(DHT *dht) {
  //   static uint8_t temp = 20;
  //   return temp++;
  quitar_buzzer(HIGH, LOW);

  float aux = dht->readTemperature();

  volver_a_poner_buzzer();


  return aux;
}

float read_heatIndex(DHT *dht, float temp, float humid) {
  return dht->computeHeatIndex(temp, humid, false);
}


uint16_t XY(uint8_t x, uint8_t y) {
  uint16_t i;
  if (kMatrixVertical == false) {
    i = (y * kMatrixWidth) + x;
  } else {
    i = kMatrixHeight * (kMatrixWidth - (x + 1)) + y;
  }
  return i;
}
uint16_t XYsafe(uint8_t x, uint8_t y) {
  if (x >= kMatrixWidth) return -1;
  if (y >= kMatrixHeight) return -1;
  return XY(x, y);
}

void DrawOneFrame(const uint32_t *frame)  // Para dibujar lo que sea
{
  quitar_buzzer(LOW, LOW);

  for (uint8_t y = 0; y < kMatrixHeight; y++) {
    for (uint8_t x = 0; x < kMatrixWidth; x++) {
      leds[XY(x, y)] = frame[x + y * 8];
    }
  }
  FastLED.show();

  volver_a_poner_buzzer();
}
void DrawDarkFrame() {  // Para apagar los leds
  quitar_buzzer(LOW, LOW);

  for (uint8_t y = 0; y < kMatrixHeight; y++) {
    for (uint8_t x = 0; x < kMatrixWidth; x++) {
      leds[XY(x, y)] = 0x000000;
    }
  }
  FastLED.show();

  volver_a_poner_buzzer();
}

void quitar_buzzer(int pin_0_level, int pin_2_level) {
  ledcDetachPin(33);
  digitalWrite(0, pin_0_level);
  digitalWrite(2, pin_2_level);
}

void volver_a_poner_buzzer() {

  digitalWrite(0, HIGH);
  digitalWrite(2, HIGH);
  ledcAttachPin(33, 0);
}


void dar_la_nota(uint8_t canal, uint32_t nota, uint32_t duty) {
  ledcWriteTone(canal, nota);
  ledcWrite(canal, duty);
}







enum { REPOSO,
       NOTA };



uint8_t flag_cantar = 0;

unsigned long cantar_now;
unsigned long cantar_next;

uint32_t cantar_nota_actual = cantar_nota_max_porompompo - 4;
uint32_t cantar_nota_max = cantar_nota_max_porompompo;

Nota_beep *cancion_actual = cancion_porompompo;

// COMPROBACIONES
int comprobar_flag_cantar_inactiva(fsm_t *that) {
  return flag_cantar == 0;
}

int comprobar_flag_cantar_activa(fsm_t *that) {
  return flag_cantar != 0;
}

int comprobar_delay_todavia_no(fsm_t *that) {
  return millis() < cantar_next;
}

int comprobar_delay_pasado_no_ultima_nota(fsm_t *that) {
  return (millis() >= cantar_next) && cantar_nota_actual != cantar_nota_max;
}

int comprobar_delay_pasado_ultima_nota(fsm_t *that) {
  return (millis() >= cantar_next) && cantar_nota_actual == cantar_nota_max;
}

// ACCIONES
void empezar_a_cantar(fsm_t *that) {
//   Serial.print("Empieza a cantar con la cancion: ");
  int cancion = (header & 0x38) >> 3;
//   Serial.println(cancion);

  if (cancion == 1) {
    // cantar_now = millis();
    // cantar_next = cancion_star_wars[0].duration;
    cancion_actual = cancion_star_wars;
    cantar_nota_max = cantar_nota_max_star_wars;
  } else if (cancion == 2) {
    cancion_actual = cancion_game_of_thrones;
    // cantar_now = millis();
    // cantar_next = cancion_game_of_thrones[0].duration;
    cantar_nota_max = cantar_nota_max_game_of_thrones;
  } else if (cancion == 3) {
    cancion_actual = cancion_happy_bday;
    // cantar_now = millis();
    // cantar_next = cancion_happy_bday[0].duration;
    cantar_nota_max = cantar_nota_max_happy_bday;
  } else if(cancion == 4) {
      cancion_actual = cancion_porompompo;
      cantar_nota_max = cantar_nota_max_porompompo;
  }

  // cantar_now = millis();
  cantar_next = millis() + cancion_actual[0].duration;

  cantar_nota_actual = 0;
  dar_la_nota(0, cancion_actual[0].nota, VOLUMEN);
}

void pasar_a_siguiente_nota(fsm_t *that) {
//   Serial.println("Pasa a la siguiente nota");
  cantar_nota_actual += 1;
  cantar_next = millis() + cancion_actual[cantar_nota_actual].duration;
  dar_la_nota(0, cancion_actual[cantar_nota_actual].nota, VOLUMEN);
}

void resetear_flag_cantar(fsm_t *that) {
//   Serial.println("Reset de cantar");
  flag_cantar = 0;
  ledcWriteTone(0, 0);
  dar_la_nota(0, 0, 0);
}

// FSM

fsm_t *fsm_new_cantar(void) {
  static fsm_trans_t tt[] = {
    { NOTA, comprobar_delay_todavia_no, NOTA, NULL },
    { NOTA, comprobar_delay_pasado_no_ultima_nota, NOTA, pasar_a_siguiente_nota },
    { NOTA, comprobar_delay_pasado_ultima_nota, REPOSO, resetear_flag_cantar },
    { REPOSO, comprobar_flag_cantar_inactiva, REPOSO, NULL },
    { REPOSO, comprobar_flag_cantar_activa, NOTA, empezar_a_cantar },

    // { 1, comprueba_condicion, 0, procesa_accion2 },
    { -1, NULL, -1, NULL }
  };
  return fsm_new(tt);
}

enum {
  WAIT_RECEPCION,
  COMPROBAR_SENSORES,  // HECHO: HAY QUE UNIFICARLOS EN UN COMPROBAR_SENSORES
  COMPROBAR_CAMARA,
  COMPROBAR_CANCION,
  COMPROBAR_DIBUJAR,
  COMPROBAR_RESPONDER_FOTO,
  ESTADO_RESPONDER_FOTO,
  COMPROBAR_RESPONDER_SENSORES,
  ESTADO_RESPONDER_SENSORES,

};

bool hacer_recvack_solo_una_vez;
// COMPROBACIONES
int comprobar_hay_recepcion_no_broadcast(fsm_t *that) {
  bool aux = hacer_recvack_solo_una_vez;

  if (aux && buf[0] == 0) {
    // Serial.println("Hay recepcion broadcast");
  }
  if(aux && buf[0] != 0);// Serial.println("Passasassa");
  return aux && buf[0] != 0;
}

int comprobar_hay_recepcion_broadcast(fsm_t *that) {
  bool aux = hacer_recvack_solo_una_vez;
  if (aux && buf[0] == 0) {
    // Serial.println("Hay recepcion broadcast");
  }
  if(aux && buf[0] != 0);// Serial.println("Pcassasassa");
  return aux && buf[0] == 0;
}

int comprobar_no_hay_recepcion(fsm_t *that) {
  // Serial.println("Paso popr aqui");    
  hacer_recvack_solo_una_vez = manager.recvfromAck(buf, &len, &from);
  return !hacer_recvack_solo_una_vez;
}

int comprobar_quiere_cantar(fsm_t *that) {
  return (header & 0x38) != (0x00);
}

int comprobar_no_quiere_cantar(fsm_t *that) {
  return (header & 0x38) == (0x00);
}

int comprobar_quiere_dibujar(fsm_t *that) {
  return (header & 0x07) != (0x00);
}

int comprobar_no_quiere_dibujar(fsm_t *that) {
  return (header & 0x07) == (0x00);
}

int comprobar_quiere_responder_sensores(fsm_t *that) {
  return (header >> 6) % 2 == 1;
}

int comprobar_no_quiere_responder_sensores(fsm_t *that) {
  return (header >> 6) % 2 != 1;
}

int comprobar_quiere_responder_foto(fsm_t *that) {
  return (header >> 7) % 2 == 1;
}

int comprobar_no_quiere_responder_foto(fsm_t *that) {
  return (header >> 7) % 2 != 1;
}

int comprobar_quiere_seguir_enviando(fsm_t *that) {
  return contador_idx_respuesta < longitud_final;
}

int comprobar_todo_ha_sido_enviado(fsm_t *that) {
  return contador_idx_respuesta >= longitud_final;
}

// ACCIONES
void printear_cosas_recibidas(fsm_t *that) {
//   Serial.print("got request from : 0x");
//   Serial.print(from, HEX);
//   Serial.print(": ");
  // Serial.println((char *)buf);
  header = buf[0];
}

void leer_datos_sensores(fsm_t *that) {
//   Serial.println("Leer sensores");
  float humedad = read_humidity(&dht1);
  uint32_t humedad2 = *(uint32_t *)&humedad;
  uint8_t hum_int_0 = (humedad2 & 0xff000000) >> 24;
  uint8_t hum_int_1 = (humedad2 & 0x00ff0000) >> 16;
  uint8_t hum_int_2 = (humedad2 & 0x0000ff00) >> 8;
  uint8_t hum_int_3 = (humedad2 & 0x000000ff);
  /*1. IDENTIFICAMOS QUE EL DATO ES DE HUMEDAD (1B)*/
  buffer_sensores[0] = 0x01;
  /*2. INSERTAMOS EL VALOR OBTENIDO POR LOS SENSORES (4B)*/
  sprintf((char *)(buffer_sensores + 1), "%3.2f", humedad);
  // buffer_sensores[1] = hum_int_0;
  // buffer_sensores[2] = hum_int_1;
  // buffer_sensores[3] = hum_int_2;
  // buffer_sensores[4] = hum_int_3;

  float temperatura = read_temperature(&dht1);
  uint32_t temperatura2 = *(uint32_t *)&temperatura;
  uint8_t tem_int_0 = (temperatura2 & 0xff000000) >> 24;
  uint8_t tem_int_1 = (temperatura2 & 0x00ff0000) >> 16;
  uint8_t tem_int_2 = (temperatura2 & 0x0000ff00) >> 8;
  uint8_t tem_int_3 = (temperatura2 & 0x000000ff);
  /*1. IDENTIFICAMOS QUE EL DATO ES DE TEMP (1B)*/
  buffer_sensores[7] = 0x02;
  /*2. INSERTAMOS EL VALOR OBTENIDO POR LOS SENSORES (4B)*/
  sprintf((char *)(buffer_sensores + 8), "%3.2f", temperatura);
  // buffer_sensores[6] = tem_int_0;
  // buffer_sensores[7] = tem_int_1;
  // buffer_sensores[8] = tem_int_2;
  // buffer_sensores[9] = tem_int_3;



  uint8_t ruido_int_0 = (registro_energia & 0xff000000) >> 24;
  uint8_t ruido_int_1 = (registro_energia & 0x00ff0000) >> 16;
  uint8_t ruido_int_2 = (registro_energia & 0x0000ff00) >> 8;
  uint8_t ruido_int_3 = (registro_energia & 0x000000ff);
  // energia = 0;
  /*1. IDENTIFICAMOS QUE EL DATO ES DE RUIDO (1B)*/
  buffer_sensores[14] = 0x03;
  /*2. INSERTAMOS EL VALOR OBTENIDO POR LOS SENSORES (4B)*/
  buffer_sensores[15] = ruido_int_0;
  buffer_sensores[16] = ruido_int_1;
  buffer_sensores[17] = ruido_int_2;
  buffer_sensores[18] = ruido_int_3;

  // Serial.print("Humedad: ");
  // Serial.println(humedad);
  // Serial.print("Temperatura: ");
  // Serial.println(temperatura);
  // Serial.print("Ruido: ");
  // Serial.println(registro_energia);
}

void accion_cantar(fsm_t *that) {
  // Serial.println("Cantar");
  if (flag_cantar == 0) {
    flag_cantar = (header & 0x38) >> 3;
  }
}

void accion_dibujar(fsm_t *that) {
//   Serial.println("Dibujar");
  switch ((header & 0x07)) {
    case (0x01):
      DrawOneFrame(libreria_luces[0]);
    //   Serial.println("Llega a pintar");
      break;
    case (0x02):
      DrawOneFrame(libreria_luces[1]);
      break;
    default:
      break;
  }
}

void leer_imagen(fsm_t *that) {
//   Serial.println("Leer imagenu");

  fb = esp_camera_fb_get();
  buffer_camara[0] = 0x04;
  /*2. INSERTAMOS LA LONGITUD DEL BUFFER DE IMAGEN (4B)*/
  longitud_camara = (uint32_t)(fb->len & 0x00000000ffffffff);
  uint8_t long_int_0 = (longitud_camara & 0xff000000) >> 24;
  uint8_t long_int_1 = (longitud_camara & 0x00ff0000) >> 16;
  uint8_t long_int_2 = (longitud_camara & 0x0000ff00) >> 8;
  uint8_t long_int_3 = (longitud_camara & 0x000000ff);

  buffer_camara[1] = long_int_0;
  buffer_camara[2] = long_int_1;
  buffer_camara[3] = long_int_2;
  buffer_camara[4] = long_int_3;

  
  // Serial.print("Longitud: ");
  // Serial.println(fb->len);
  // Serial.println(longitud_camara);

  //   fb = fb_local;
  // buf_camara_aux = fb->buf;


  /*3. INSERTAMOS EL VALOR OBTENIDO POR LOS SENSORES (XB)*/
  // for (int i = 0; i < longitud_camara; i++) {
  //     buffer_camara[5 + i] = fb->buf[i];
  // }
}

void accion_responder_sensores(fsm_t *that) {
//   Serial.println("ACC responder");
  // if ((header >> 6) % 2 == 1) {                          // Mandamos info de sensores
  //     if ((header >> 7) % 2 == 1) {                      // Mandamos info de foto
  //         longitud_final = 3 * 5 + 5 + longitud_camara;  // 3*(1B + 4B) + 1B + 4B + len_camera;
  //         for (int i = 0; i < 21; i++) {
  //             buffer_final[i] = buffer_sensores[i];
  //         }
  //         for (int i = 0; i < (5 + longitud_camara); i++) {
  //             buffer_final[i + 21] = buffer_camara[i];
  //         }
  //     } else {
  //         longitud_final = 3 * 5;
  //         for (int i = 0; i < 15; i++) {
  //             buffer_final[i] = buffer_sensores[i];
  //         }
  //     }
  // } else if ((header >> 7) % 2 == 1) {
  //     longitud_final = 5 + longitud_camara;
  //     for (int i = 0; i < (5 + longitud_camara); i++) {
  //         buffer_final[i] = buffer_camara[i];
  //     }
  // }

  longitud_final = 2 * 7 + 5;
  contador_idx_respuesta = 0;
}

void accion_responder_foto(fsm_t *that) {
  longitud_final = fb->len;
  uint8_t error = manager.sendtoWait(buffer_camara, 5, from);
  contador_idx_respuesta = 0;
}

void accion_seguir_respondiendo_sensores(fsm_t *that) {
//   Serial.println("Accion seguir respondiendo");
  int longitud_mensaje = contador_idx_respuesta + RH_MESH_MAX_MESSAGE_LEN <= longitud_final
                           ? RH_MESH_MAX_MESSAGE_LEN
                           : longitud_final - contador_idx_respuesta;
  // Serial.println(longitud_mensaje);
  uint8_t error = manager.sendtoWait(buffer_sensores + contador_idx_respuesta, longitud_mensaje, from);
  if (error == RH_ROUTER_ERROR_NONE) {
    // Serial.println("Enviado correctamente");
    contador_idx_respuesta += longitud_mensaje;
  } else {
    // Serial.print("No se ha enviado o ha dado algún error. Error: ");
    // Serial.println(error);
  }
}

void accion_seguir_respondiendo_foto(fsm_t *that) {
  // static int longitud_maxima = 50;
//   Serial.println("Accion seguir respondiendo foto");
  int longitud_mensaje = contador_idx_respuesta + MAX_LENGTH_ANTONIO < longitud_final
                           ? MAX_LENGTH_ANTONIO
                           : longitud_final - contador_idx_respuesta;
  // Serial.println(longitud_mensaje);
  uint8_t error = manager.sendtoWait(fb->buf + contador_idx_respuesta, longitud_mensaje, from);
  if (error == RH_ROUTER_ERROR_NONE) {
    // Serial.println("Enviado correctamente");
    contador_idx_respuesta += longitud_mensaje;
  } else {
    // Serial.print("No se ha enviado o ha dado algún error. Error: ");
    // Serial.println(error);
    // Serial.print("longitud mensaje baja a: ");
    // Serial.println(--longitud_maxima);
  }
}

void reset_len_buf(fsm_t *that) {
//   Serial.println("Reset len buf");
  contador_idx_respuesta = 0;
  len = sizeof(buf);
  esp_camera_fb_return(fb);
}

void enviar_ack_broadcast(fsm_t *that) {
  // Serial.println("Enviar ack broadcast");
  uint8_t aux[1] = { 0 };
  uint8_t aux_error;
  do {
    aux_error = manager.sendtoWait(aux, 1, from);
  } while (aux_error == RH_ROUTER_ERROR_NONE);
}

// TRANSICIONES

fsm_t *fsm_new_general(void) {
  static fsm_trans_t tt[] = {
    { WAIT_RECEPCION, comprobar_no_hay_recepcion, WAIT_RECEPCION, NULL },
    { WAIT_RECEPCION, comprobar_hay_recepcion_broadcast, WAIT_RECEPCION, enviar_ack_broadcast },
    { WAIT_RECEPCION, comprobar_hay_recepcion_no_broadcast, COMPROBAR_SENSORES, printear_cosas_recibidas },
    { COMPROBAR_SENSORES, comprobar_quiere_responder_sensores, COMPROBAR_CAMARA,
      leer_datos_sensores },
    { COMPROBAR_SENSORES, comprobar_no_quiere_responder_sensores, COMPROBAR_CAMARA, NULL },
    { COMPROBAR_CAMARA, comprobar_quiere_responder_foto, COMPROBAR_CANCION, leer_imagen },
    { COMPROBAR_CAMARA, comprobar_no_quiere_responder_foto, COMPROBAR_CANCION, NULL },
    { COMPROBAR_CANCION, comprobar_quiere_cantar, COMPROBAR_DIBUJAR, accion_cantar },
    { COMPROBAR_CANCION, comprobar_no_quiere_cantar, COMPROBAR_DIBUJAR, NULL },
    { COMPROBAR_DIBUJAR, comprobar_quiere_dibujar, COMPROBAR_RESPONDER_SENSORES, accion_dibujar },
    { COMPROBAR_DIBUJAR, comprobar_no_quiere_dibujar, COMPROBAR_RESPONDER_SENSORES, NULL },
    { COMPROBAR_RESPONDER_SENSORES, comprobar_quiere_responder_sensores, ESTADO_RESPONDER_SENSORES, accion_responder_sensores },
    { COMPROBAR_RESPONDER_SENSORES, comprobar_no_quiere_responder_sensores, COMPROBAR_RESPONDER_FOTO, NULL },
    { ESTADO_RESPONDER_SENSORES, comprobar_quiere_seguir_enviando, ESTADO_RESPONDER_SENSORES, accion_seguir_respondiendo_sensores },
    { ESTADO_RESPONDER_SENSORES, comprobar_todo_ha_sido_enviado, COMPROBAR_RESPONDER_FOTO, NULL },
    { COMPROBAR_RESPONDER_FOTO, comprobar_quiere_responder_foto, ESTADO_RESPONDER_FOTO, accion_responder_foto },
    { COMPROBAR_RESPONDER_FOTO, comprobar_no_quiere_responder_foto, WAIT_RECEPCION, reset_len_buf },
    { ESTADO_RESPONDER_FOTO, comprobar_quiere_seguir_enviando, ESTADO_RESPONDER_FOTO, accion_seguir_respondiendo_foto },
    { ESTADO_RESPONDER_FOTO, comprobar_todo_ha_sido_enviado, WAIT_RECEPCION, reset_len_buf },

    // { 1, comprueba_condicion, 0, procesa_accion2 },
    { -1, NULL, -1, NULL }
  };
  return fsm_new(tt);
}





void setup() {
  Serial.begin(115200);

  pinMode(0, OUTPUT);  // GPIO0 => SW1
  pinMode(2, OUTPUT);  // GPIO2 => SW2

  pinMode(33, OUTPUT);  // GPIO33 => SIGNAL

  //   delay(1000);
//   Serial.println("Llega al setup");
  // hspi.begin(14, 12, 13, 15); //CLK, MISO, MOSI, CSN
  // spi.setPins(12, 13, 14); // MISO 12, MOSI 11, SCK  13
  // Serial.println("Pasa el spi");
  ledcSetup(0, 2000, 8);
  ledcAttachPin(33, 0);
  digitalWrite(0, HIGH);
  digitalWrite(2, HIGH);

  dar_la_nota(0, 0, VOLUMEN);
  // delay(4000);

  if (!manager.init()) Serial.println("RF22 init failed");

  if (!driver.setChannel(100)) Serial.println("Setchannel fallido");


#if defined(__AVR_ATmega2560__)
    // Serial.println("Es un arduino Mega");
#else
    // Serial.println("Es un ESP / No es un Arduino Mega");
#endif
  // Defaults after init are 434.0MHz, 0.05MHz AFC pull-in, modulation FSK_Rb2_4Fd36
  // Serial.print("Direccion: ");
  // Serial.println(CURR_ADDR);
  // delay(4000);

  setup_DHT(&dht1);
//   Serial.println("Llega alfffff setupxx");
  //   // setup_TIM1_custom();


  //   int now = millis();
  //   // next = now + 1000;
  //   next_leer_energia = now + 50;

  //   setup_MAX9814();
//   Serial.println("Llega al setupxddddx");
  // delay(4000);

  setup_camera();
//   Serial.println("Llega al setupxx");
  // delay(4000);

  fsmcantar = fsm_new_cantar();
  fsmgeneral = fsm_new_general();
  // delay(4000);

  setup_LEDs();
//   Serial.println("Llega al setupdadfsasdfaxx");
  Serial.println(CURR_ADDR);

  // ledcSetup(0, 2000, 8);
  // ledcAttachPin(ZUMBADOR_PIN, 0);
  //   prev_leer_energia = millis();
  //   next_reset_energia = millis();
  // delay(4000);
//   Serial.println("Se acaba el setup");
}

void loop() {
  // Serial.println("Estoy vivo");
  fsm_fire(fsmgeneral);
  fsm_fire(fsmcantar);

  int now = millis();
  int modulo_leer_energia = now % 50;
  int modulo_reset_energia = now % 1000;

  if (modulo_leer_energia < prev_modulo_leer_energia) {
    energia += read_microphone(33);
  }
  if (modulo_reset_energia < prev_modulo_reset_energia) {
    registro_energia = energia;
    energia = 0;
  }
  prev_modulo_leer_energia = modulo_leer_energia;
  prev_modulo_reset_energia = modulo_reset_energia;

}