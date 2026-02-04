# 📡 Comunicación LoRa P2P - Heltec WiFi LoRa 32 V3

![Platform](https://img.shields.io/badge/Hardware-Heltec_V3_SX1262-blue)
![Library](https://img.shields.io/badge/Library-RadioLib-orange)
![IDE](https://img.shields.io/badge/Arduino_IDE-2.X-teal)

Este repositorio contiene la adaptación del protocolo de comunicación LoRa Punto a Punto (P2P) para las asignaturas de: **Sistemas de Sensores** (Maestría en IoT), **Tec. Inalámbricas** e **Internet de las Cosas** para ITSOEH, diseñado específicamente para la nueva arquitectura de la placa **Heltec V3**.

## ⚠️ Diferencias Críticas: V2 vs. V3

Si vienes de usar la versión V2 de Heltec, es fundamental entender que **el hardware ha cambiado drásticamente**, por lo que el código antiguo NO funcionará.

| Característica | Heltec V2 | Heltec V3 |
| :--- | :--- | :--- |
| **Microcontrolador** | ESP32 (Xtensa) | **ESP32-S3** (Más potente, USB nativo) |
| **Chip de Radio** | SX1276 / SX1278 | **SX1262** (Requiere librerías modernas) |
| **Librería Standard** | `LoRa.h` (Sandeep Mistry) | **`RadioLib`** (Jan Gromeš) |
| **Método de Recepción** | Polling (Preguntar en el loop) | **Interrupciones** (Evento de hardware) |

---

## 🛠️ Requisitos de Instalación (Arduino IDE 2.X)

Para compilar este proyecto, asegúrate de tener el entorno configurado correctamente:

### 1. Gestor de Tarjetas (Board Manager)
* **Paquete:** Heltec ESP32 Dev-Boards.
* **Versión:** 3.0.0 o superior.
* **Tarjeta a seleccionar:** `Heltec WiFi LoRa 32(V3) / Wireless stick Lite(V3)`.

### 2. Librerías Requeridas
Instalar desde el Gestor de Librerías (Ctrl+Shift+I):
* **RadioLib** (por Jan Gromeš): Manejo del chip SX1262.
* **Heltec ESP32 Dev-Boards** (Built-in): Incluye `HT_SSD1306Wire` para la pantalla.

---

## 🧠 Conceptos Clave del Código

Este código introduce conceptos más avanzados de programación de sistemas embebidos respecto a la versión anterior.

### 1. Definición Explícita de Pines
A diferencia de la V2, en la V3 debemos "mapear" manualmente los pines del chip de radio SX1262.
```cpp
// NSS: Chip Select | DIO1: Interrupción | RST: Reset | BUSY: Estado
SX1262 radio = new Module(8, 14, 12, 13);

```markdown
### 2. Transmisión por Buffer (Arrays)
La librería `LoRa.h` antigua funcionaba como un `Serial.print` (stream). En la V3 con RadioLib, usamos un **Buffer de Memoria**.

* **Concepto:** Creamos un "paquete" (array de bytes) en memoria, lo llenamos casilla por casilla y lo enviamos de golpe.
* **Ventaja:** Permite visualizar mejor la estructura de una trama de red (`Header` + `Payload`) y es más eficiente para el chip SX1262.

### 3. Eficiencia: Interrupciones vs. Polling
* **Método Antiguo (Polling):** El procesador preguntaba constantemente `¿Llegó mensaje?` dentro del `loop()`. Esto desperdicia ciclos de CPU y energía.
* **Método Nuevo (Interrupción):**
    1.  El procesador ignora la radio y se dedica a leer sensores u otras tareas.
    2.  Cuando llega un mensaje, el chip LoRa envía una señal eléctrica al **Pin 14 (DIO1)**.
    3.  Se activa una función especial (`setFlag`) que avisa al `loop()` que hay trabajo pendiente.
    * *Analogía:* Es como esperar a que suene el timbre de la puerta en lugar de estar abriendo la puerta cada 10 segundos para ver si hay alguien.

---

## 🔌 Pinout de Referencia (Heltec V3)

| Componente | Pin GPIO | Función |
| :--- | :--- | :--- |
| **Radio NSS** | 8 | Chip Select LoRa |
| **Radio DIO1** | 14 | IRQ (Interrupción) |
| **Radio RST** | 12 | Reset LoRa |
| **Radio BUSY** | 13 | Indicador de ocupado |
| **OLED SDA** | 17 | Datos Pantalla |
| **OLED SCL** | 18 | Reloj Pantalla |
| **OLED RST** | 21 | Reset Pantalla |
| **LED** | 35 | LED Integrado (Usuario) |
| **User Button**| 0 | Botón PRG |

---

## 🚀 Guía de Uso para la Práctica

1.  **Configurar Direcciones:**
    Asigna `dir_local` y `dir_destino` en el código para crear parejas de comunicación (Ej: 0xC1 <-> 0xD3).
2.  **Cargar el Código:**
    Conecta la Heltec V3 vía USB-C. Asegúrate de que el puerto COM sea reconocido como "USB Serial Device" o "Heltec V3".
3.  **Monitor Serial:**
    Configura la velocidad a **115200 baudios**.
4.  **Interpretación:**
    * **TX:** El LED parpadeará largo (100ms) al enviar.
    * **RX:** El LED parpadeará corto (50ms) al recibir un paquete válido.
    * **Pantalla:** Mostrará el ID del remitente, el mensaje decodificado y el RSSI (Potencia de señal).

---

## 👤 Créditos

**Autor:** MGTI. Saúl Isaí Soto Ortiz  
**Asignatura:** Sistemas de Sensores - Maestría en IoT  

> *Este código es educativo y utiliza la banda ISM de 915MHz. Asegúrese de conectar la antena antes de energizar la placa para evitar daños en el chip de radio.*
