/*
 * ---------------------------------------------------------------------------
 * ASIGNATURA: SISTEMAS DE SENSORES | MAESTRÍA IOT
 * PLATAFORMA: Heltec WiFi LoRa 32 V3 (ESP32-S3 + SX1262)
 * LIBRERÍA: RadioLib (Estándar para SX1262)
 * ---------------------------------------------------------------------------
 */

#include <RadioLib.h>
#include "Wire.h"
#include "HT_SSD1306Wire.h" // Librería de pantalla incluida en el paquete Heltec

// ==========================================
// 1. DEFINICIÓN DE PINES (ESPECÍFICO HELTEC V3)
// ==========================================
// La V3 usa el chip SX1262 y tiene un pineado diferente a la V2
#define LORA_NSS    8
#define LORA_DIO1   14
#define LORA_RST    12
#define LORA_BUSY   13

// Pines Pantalla OLED V3
#define OLED_RST    21
#define OLED_SDA    17
#define OLED_SCL    18

// Instancia del módulo de radio (SX1262)
SX1262 radio = new Module(LORA_NSS, LORA_DIO1, LORA_RST, LORA_BUSY);

// Instancia de la pantalla (Dirección 0x3c)
SSD1306Wire display(0x3c, 500000, OLED_SDA, OLED_SCL, GEOMETRY_128_64, OLED_RST);

// ==========================================
// 2. CONFIGURACIÓN DE RED
// ==========================================
float frecuencia = 915.0; // MHz
float ancho_banda = 125.0; // kHz
uint8_t spreading_factor = 8;
uint8_t coding_rate = 5; // 4/5

// ==========================================
// 3. IDENTIDAD Y DIRECCIONAMIENTO
// ==========================================
byte dir_local   = 0xC1; 
byte dir_destino = 0xD3; 

byte id_msjLoRa  = 0;   
String sensorEstado = "ON";

// Variables de Recepción
byte dir_envio  = 0;
byte dir_remite = 0;
String paqueteRcb = "";
byte paqRcb_ID  = 0;
byte paqRcb_Estado = 0; // 0:Nada, 1:OK, 2:Error Len, 3:Error Dir

// Variables de Tiempo (Multitarea)
long tiempo_antes = 0;
long tiempo_intervalo = 6000;
long tiempo_espera = tiempo_intervalo + random(3000);

#define LED_PIN 35 // El LED en la V3 suele ser el pin 35 o LED_BUILTIN

// Bandera de interrupción para recepción
volatile bool paquete_recibido = false;

// Función que se ejecuta cuando llega un mensaje (Interrupción)
#if defined(ESP8266) || defined(ESP32)
  ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  paquete_recibido = true;
}

void setup() {
  Serial.begin(115200);

  // --- INICIO PANTALLA ---
  display.init();
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Iniciando Heltec V3...");
  display.display();
  
  pinMode(LED_PIN, OUTPUT);

  // --- INICIO RADIO (SX1262) ---
  Serial.print(F("[SX1262] Iniciando... "));
  // Inicializamos con la frecuencia base
  int state = radio.begin(frecuencia, ancho_banda, spreading_factor, coding_rate);
  
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("¡Éxito!"));
    display.drawString(0, 10, "Radio: OK (915MHz)");
  } else {
    Serial.print(F("Falló, código: ")); Serial.println(state);
    display.drawString(0, 10, "Radio: FALLO");
    display.display();
    while (true); // Detener si no hay radio
  }

  // Configurar interrupción de recepción (Es más eficiente que el polling antiguo)
  radio.setDio1Action(setFlag);
  
  // Comenzar a escuchar (Modo Recepción RX)
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("[SX1262] Escuchando..."));
  }
  
  display.display();
}

void loop() {
  // -------------------------------------------------------
  // BLOQUE 1: TRANSMISIÓN (TX)
  // -------------------------------------------------------
  if ((millis() - tiempo_antes) >= tiempo_espera) {
    
    sensor_revisa();
    
    // Enviar paquete usando nuestra función manual
    envia_lora_v3(dir_destino, dir_local, id_msjLoRa, sensorEstado);
    
    id_msjLoRa++;
    tiempo_antes = millis();
    tiempo_espera = tiempo_intervalo + random(3000);
    
    // Parpadeo LED
    digitalWrite(LED_PIN, HIGH); delay(100); digitalWrite(LED_PIN, LOW);
  }

  // -------------------------------------------------------
  // BLOQUE 2: RECEPCIÓN (RX)
  // -------------------------------------------------------
  // En RadioLib verificamos la bandera de la interrupción
  if (paquete_recibido) {
    paquete_recibido = false; // Reset bandera
    
    recibe_lora_v3(); // Procesar el paquete
    
    // Volver a poner la radio en modo escucha
    radio.startReceive();
    
    if (paqRcb_Estado == 1) {
       // Actualizar Pantalla solo si el mensaje es válido
       display.clear();
       display.drawString(0, 0, "RX de: 0x" + String(dir_remite, HEX));
       display.drawString(0, 15, "Msg: " + paqueteRcb);
       // RSSI (Potencia) y SNR (Ruido)
       display.drawString(0, 30, "RSSI: " + String(radio.getRSSI()) + " dBm");
       display.display();
       
       Serial.print("RX Válido: "); Serial.println(paqueteRcb);
       digitalWrite(LED_PIN, HIGH); delay(50); digitalWrite(LED_PIN, LOW);
    }
  }
  
  delay(10);
}

/*
 * ---------------------------------------------------------------------------
 * FUNCIÓN: envia_lora_v3 (Estilo RadioLib)
 * EXPLICACIÓN: RadioLib usa un array de bytes (buffer) para transmitir.
 * Aquí "encapsulamos" manualmente llenando ese array.
 * ---------------------------------------------------------------------------
 */
void envia_lora_v3(byte destino, byte remite, byte paqueteID, String mensaje) {
  
  Serial.print("Enviando paquete ID: "); Serial.println(paqueteID);

  int lenMensaje = mensaje.length();
  // El tamaño total = 4 bytes de cabecera + longitud del mensaje
  int packetLen = 4 + lenMensaje;
  
  // Creamos el buffer temporal (El "Sobre")
  byte buffer[packetLen];
  
  // --- LLENADO DEL BUFFER (ENCAPSULAMIENTO) ---
  buffer[0] = destino;      // Byte 0
  buffer[1] = remite;       // Byte 1
  buffer[2] = paqueteID;    // Byte 2
  buffer[3] = (byte)lenMensaje; // Byte 3
  
  // Copiamos el mensaje (String) al buffer de bytes
  for(int i=0; i<lenMensaje; i++){
    buffer[4+i] = mensaje[i];
  }

  // --- TRANSMISIÓN FÍSICA ---
  // RadioLib se encarga de enviarlo al aire
  int state = radio.transmit(buffer, packetLen);

  if (state == RADIOLIB_ERR_NONE) {
    // Éxito
  } else {
    Serial.print(F("Error enviando, código: ")); Serial.println(state);
  }
  
  // IMPORTANTE: Al terminar de enviar, RadioLib vuelve a standby.
  // Debemos reactivar la escucha en el loop principal o aquí mismo.
  radio.startReceive();
}

/*
 * ---------------------------------------------------------------------------
 * FUNCIÓN: recibe_lora_v3
 * EXPLICACIÓN: Lee el buffer recibido y lo "desencapsula".
 * ---------------------------------------------------------------------------
 */
void recibe_lora_v3() {
  // Leemos cuántos bytes llegaron
  int len = radio.getPacketLength();
  
  // Creamos un buffer para guardar lo recibido
  byte buffer[len];
  
  // Leemos los datos de la radio a nuestro buffer
  int state = radio.readData(buffer, len);

  if (state != RADIOLIB_ERR_NONE) {
    paqRcb_Estado = 0; // Error de lectura
    return;
  }

  // --- DESENCAPSULAMIENTO ---
  dir_envio  = buffer[0]; // Byte 0
  dir_remite = buffer[1]; // Byte 1
  paqRcb_ID  = buffer[2]; // Byte 2
  byte lenMsgDeclarada = buffer[3]; // Byte 3
  
  // Reconstruir el mensaje (Payload)
  paqueteRcb = "";
  for(int i=4; i<len; i++){
    paqueteRcb += (char)buffer[i];
  }

  // --- VALIDACIONES ---
  // 1. Integridad
  if (lenMsgDeclarada != paqueteRcb.length()) {
     Serial.println("Error: Longitud no coincide");
     paqRcb_Estado = 2; return;
  }
  
  // 2. Direccionamiento
  if (dir_envio != dir_local && dir_envio != 0xFF) {
     Serial.print("Ignorado. Era para: 0x"); Serial.println(dir_envio, HEX);
     paqRcb_Estado = 3; return;
  }

  paqRcb_Estado = 1; // Todo OK
}

void sensor_revisa() {
  sensorEstado = (sensorEstado == "ON") ? "OFF" : "ON";
}
