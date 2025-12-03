# FM Radio with SI4703 on Arduino UNO

This project implements an FM radio using the **SI4703** chip on an **Arduino UNO** board. Control is provided via five buttons on the **ADKeyboard Module V3**. The radio supports **RDS (Radio Data System)** and signal strength measurement through `si4703_getRSSI()`. Output is displayed on a **128×64 OLED, 1.3" I2C**.

---

## 1. Hardware

### Components
- **Arduino UNO** – main microcontroller  
- **SI4703 FM receiver** – I2C tuner chip  
- **OLED 128×64, 1.3" I2C** – displays frequency, volume, tuning mode (AUT/MAN), RDS text  
- **ADKeyboard Module V3** – five-button input on a single analog pin  
- **Power supply** – 5 V via USB or external source  

### Connections
- **SI4703**  
  - SDA → A4  
  - SCL → A5  
  - GND → GND  
  - VCC → 3.3 V  

- **OLED (I2C)**  
  - SDA → A4  
  - SCL → A5  
  - GND → GND  
  - VCC → 5 V  

- **ADKeyboard Module V3**  
  - Analog out → A0  
  - VCC → 5 V  
  - GND → GND  

---

## 2. Button Control

### Functions
- **S1** – increase frequency / next station  
- **S2** – decrease frequency / previous station  
- **S3** – volume down  
- **S4** – volume up  
- **S5** – toggle tuning mode (AUT ↔ MAN)  

### Typical ADC Values
| Button | ADC Value |
|--------|-----------|
| S1     | ~100      |
| S2     | ~250      |
| S3     | ~400      |
| S4     | ~600      |
| S5     | ~800      |
| None   | ~1023     |

*Note: Values vary with supply voltage and resistor tolerances. Use threshold ranges in software.*

---

## 3. Radio Functions

### Automatic Mode (AUT)
- Default at startup  
- Scans FM band, evaluates signal via `si4703_getRSSI()`  
- **Threshold: 19** (0–127); stations ≥19 saved to list  
- **S1/S2** cycle through list (wrap-around enabled)  

### Manual Mode (MAN)
- Activated by **S5**  
- **S1**: +0.1 MHz, **S2**: –0.1 MHz  
- Display shows **AUT** or **MAN** in top-right corner  

### Volume
- **S3**: down, **S4**: up  
- Value shown on OLED top line with mode and frequency  

### RDS
- Displays station text if available  
- Updates dynamically; if unavailable, shows last valid text or `---`  

---

## 4. Program Workflow

1. **Initialization**  
   - Configure I2C (SI4703 + OLED)  
   - Start automatic scan  

2. **Scanning**  
   - Sweep FM band  
   - Save stations with RSSI ≥19  

3. **Main Loop**  
   - Read button input (ADC)  
   - Adjust frequency or station list (AUT/MAN)  
   - Update OLED (frequency, mode, volume, RDS)  
   - Monitor signal strength  

---

## 5. Display Layout

- **Top line**: mode (AUT/MAN), frequency, volume  
- **Lower section**: RDS text  

---

## 6. Notes

- **RSSI threshold** fixed at 19  
- **RDS latency**: keep last valid text until new data arrives  
- **ADKeyboard**: distinguish buttons by ADC voltage mapping  
- **Debouncing** recommended in software  
- **Memory**: station list in RAM; EEPROM optional  
