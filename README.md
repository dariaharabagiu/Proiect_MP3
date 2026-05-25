# Mini MP3 Player

Player audio portabil construit pe microcontrolerul ATmega328P Xplained Mini,
care reda fisiere .wav stocate pe un card microSD, cu afisaj OLED si control
prin butoane fizice.

## Componente Hardware

| Componenta | Rol | Pini |
|---|---|---|
| ATmega328P Xplained Mini | Microcontroller principal | — |
| Modul MicroSD | Stocare fisiere .wav | D10(CS), D11(MOSI), D12(MISO), D13(SCK) |
| Modul audio SC8002B | Amplificare semnal audio | D9(semnal), 5V, GND |
| Display OLED SSD1306 0.96" | Afisare melodie si status | A4(SDA), A5(SCL) |
| Potentiometru slider HW-233 | Control volum | A0 |
| LED Verde x2, Galben x1, Rosu x1 | VU meter vizual | D3, D4, D5, D7 |
| Butoane tactile x3 | PREV, PLAY/PAUSE, NEXT | A1, A2, A3 |
| Rezistente 220Ω x4 | Protectie LED-uri | — |

## Conexiuni

- **SPI** (SD card): MISO=D12, MOSI=D11, SCK=D13, CS=D10
- **I2C** (OLED): SDA=A4, SCL=A5
- **PWM Audio**: Pin D9 (Timer1 OC1A) → SC8002B
- **ADC**: A0 → slider potentiometru
- **GPIO Input**: A1, A2, A3 → butoane (INPUT_PULLUP)
- **GPIO Output**: D3, D4, D5, D7 → LED-uri VU meter

## Software

### Mediu de dezvoltare
- VS Code + PlatformIO
- Framework: Arduino
- Board: ATmega328P Xplained Mini

### Biblioteci folosite
- `SD` - citirea cardului microSD
- `TMRpcm` - redare audio WAV prin Timer1 PWM
- `U8g2` (U8X8) - control display OLED fara buffer
- `Wire` - protocol I2C
- `SPI` - protocol SPI

### Instalare
1. Cloneaza repository-ul
2. Deschide in VS Code cu extensia PlatformIO
3. Copiaza fisierele `.wav` pe cardul SD (format 8.3, litere mici)
4. Actualizeaza array-ul `melodii[]` din `main.cpp` cu numele fisierelor
5. Conecteaza hardware conform tabelului de mai sus
6. `PlatformIO: Upload`

### Formate audio suportate
- WAV, PCM nesemnat, 8-bit, mono
- Sample rate recomandat: 16000 Hz sau 22050 Hz

## Laboratoare PM acoperite

- **Lab 0 - GPIO**: LED-uri cu rezistente, butoane cu INPUT_PULLUP intern
- **Lab 3 - Timere & PWM**: TMRpcm foloseste Timer1 in mod Fast PWM pentru audio
- **Lab 4 - ADC**: Potentiometru slider citit prin analogRead() pe A0
- **Lab 5 - SPI**: Card microSD comunicat prin protocolul SPI
- **Lab 6 - I2C**: Display OLED SSD1306 comunicat prin protocolul I2C

## Autor

Daria Harabagiu - Proiect PM 2026
