# Circuit Diagram

## Components
- NodeMCU ESP8266 (ESP-12E)
- 5V DC Fan
- 2N3904 NPN Transistor
- GY-68 BMP180 Temperature / Pressure Sensor
- 1kΩ Resistor

---

## Pin Connections

### Fan + Transistor (2N3904)
| Component    | Pin       | Connected To  |
|--------------|-----------|---------------|
| NodeMCU      | D1        | 1kΩ Resistor  |
| 1kΩ Resistor | other end | 2N3904 Base   |    <img width="268" height="421" alt="image" src="https://github.com/user-attachments/assets/57439ca4-8307-44e4-a66e-4c7d9fe2940f" />
| 2N3904       | Collector | Fan (-)       |
| 2N3904       | Emitter   | GND           |
| Fan          | (+)       | 5V            |

### BMP180 Sensor (GY-68)
| Component | Pin | Connected To |
|-----------|-----|--------------|
| BMP180    | VCC | 3.3V         |
| BMP180    | GND | GND          |
| BMP180    | SDA | D2           |
| BMP180    | SCL | D5           |

---

## 2N3904 Pinout (flat face toward you, left to right)
| Pin | Number |
|-----|--------|
| Emitter   | 1 |
| Base      | 2 |
| Collector | 3 |

---

## Wiring Diagram (Text)

```
5V ──────────────────────────── Fan (+)
                                Fan (-) ── 2N3904 Collector
NodeMCU D1 ── 1kΩ ── 2N3904 Base
                      2N3904 Emitter ── GND

NodeMCU 3.3V ── BMP180 VCC
NodeMCU GND  ── BMP180 GND
NodeMCU D2   ── BMP180 SDA
NodeMCU D5   ── BMP180 SCL
```
