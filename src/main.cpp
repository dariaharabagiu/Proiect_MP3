// ============================================================
// Mini MP3 Player - ATmega328P Xplained Mini
//
// Componente folosite:
//   - Modul MicroSD (SPI): citire fisiere .wav
//   - TMRpcm: redare audio prin PWM pe Pin 9
//   - Display OLED SSD1306 128x64 (I2C): afisare melodie
//   - Potentiometru slider HW-233 (ADC): control volum
//   - 4 LED-uri VU meter (GPIO): vizualizare volum
//   - 3 butoane tactile (GPIO + PULLUP): PREV, PLAY/PAUSE, NEXT
//   - Modul audio SC8002B: amplificare semnal PWM
//
// Laboratoare acoperite:
//   - Lab 0: GPIO (LED-uri, butoane cu INPUT_PULLUP)
//   - Lab 3: Timere & PWM (TMRpcm foloseste Timer1 Fast PWM)
//   - Lab 4: ADC (potentiometru slider pe A0)
//   - Lab 5: SPI (card microSD pe pinii 10-13)
//   - Lab 6: I2C (display OLED pe A4/SDA si A5/SCL)
// ============================================================

#include <Arduino.h>
#include <SPI.h>       // Protocol SPI pentru cardul microSD
#include <SD.h>        // Biblioteca pentru citirea cardului SD
#include <TMRpcm.h>    // Biblioteca pentru redare audio WAV prin PWM
#include <Wire.h>      // Protocol I2C pentru display OLED
#include <U8g2lib.h>   // Biblioteca pentru controlul display-ului OLED

// ── DISPLAY OLED SSD1306 128x64 prin I2C ─────────────────
// U8X8 = mod fara buffer, scrie direct pe display
// Nu blocheaza intreruperile Timer1 folosite de TMRpcm
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);

// ── AUDIO & SD ────────────────────────────────────────────
TMRpcm audio;                  // Obiect pentru redarea audio
const int chipSelect = 10;     // Pin CS pentru modulul microSD (SPI)

// ── VU METER - 4 LED-uri ──────────────────────────────────
// Verde x2 (D3, D4), Galben (D5), Rosu (D7)
// Pin 6 evitat - Timer0, interferente cu audio
const int vuPins[] = {3, 4, 5, 7};
const int numarLED = 4;

// ── POTENTIOMETRU SLIDER HW-233 ───────────────────────────
// Citit prin ADC pe pinul A0 (0-1023) -> mapat la volum (0-3)
const int potPin = A0;

// ── BUTOANE TACTILE cu INPUT_PULLUP intern ─────────────────
// Apasare = LOW (buton trage pinul la GND)
const int BTN_PREV = A1;   // Melodia anterioara
const int BTN_PLAY = A2;   // Play / Pause toggle
const int BTN_NEXT = A3;   // Melodia urmatoare

// ── LISTA DE MELODII PE CARDUL SD ────────────────────────
// Numele fisierelor WAV (format 8.3, litere mici, fara spatii)
const char* melodii[] = {
  "test1.wav",
  "test2.wav",
  "test3.wav"
};
const int numarMelodii = 3;

// ── STARE PLAYER ──────────────────────────────────────────
int melodieCurenta = 0;    // Indexul melodiei in curs de redare
bool isPaused = false;     // True = player in pauza

// ── DEBOUNCE BUTOANE ──────────────────────────────────────
// Evita citiri multiple la o singura apasare
unsigned long lastPressTime = 0;
const int DEBOUNCE_MS = 300;

// ─────────────────────────────────────────────────────────
// Actualizeaza display-ul OLED cu informatii despre melodia curenta
// Afiseaza: titlu player, nume melodie (fara .wav), status
// ─────────────────────────────────────────────────────────
void afiseazaOLED() {
  // Extragem numele melodiei fara extensia .wav
  char nume[13];
  strncpy(nume, melodii[melodieCurenta], 12);
  nume[12] = '\0';
  char* dot = strchr(nume, '.');
  if (dot) *dot = '\0';

  // Scriem pe display linie cu linie
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 0, "Mini MP3 Player");

  // Linia 2: eticheta
  u8x8.drawString(0, 2, "Now playing:");

  // Linia 4: numarul melodiei curente din total
  char nrBuf[16];
  snprintf(nrBuf, sizeof(nrBuf), "Track %d/%d",
           melodieCurenta + 1, numarMelodii);
  u8x8.drawString(0, 4, nrBuf);

  // Linia 7: status play sau pauza
  u8x8.drawString(0, 7, isPaused ? "|| PAUSED      " : "> PLAYING      ");
}

// ─────────────────────────────────────────────────────────
// Porneste redarea melodiei de la indexul dat
// Reseteaza starea de pauza si actualizeaza display-ul
// ─────────────────────────────────────────────────────────
void pornesteMelodie(int index) {
  melodieCurenta = index;
  isPaused = false;
  audio.stopPlayback();      // ← opreste piesa curenta inainte
  delay(50);                 // ← mica pauza pentru stabilitate
  audio.play(melodii[melodieCurenta]);
  afiseazaOLED();
  Serial.print(F("Redau: "));
  Serial.println(melodii[melodieCurenta]);
}

// ─────────────────────────────────────────────────────────
// SETUP - ruleaza o singura data la pornire
// ─────────────────────────────────────────────────────────
void setup() {
  Serial.begin(57600);
  delay(2000);  // Asteptam stabilizarea tensiunii la pornire

  // Initializare pini LED ca OUTPUT, initial stinse
  for (int i = 0; i < numarLED; i++) {
    pinMode(vuPins[i], OUTPUT);
    digitalWrite(vuPins[i], LOW);
  }

  // Initializare butoane cu rezistenta pull-up interna
  // Pinul citeste HIGH cand butonul e liber, LOW cand e apasat
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_PLAY, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);

  // Initializare audio - Pin 9 = OC1A = iesirea Timer1 PWM
  audio.speakerPin = 9;
  pinMode(SS, OUTPUT);   // Pin SS necesar pentru SPI

  // Initializare card SD prin SPI
  if (!SD.begin(chipSelect)) {
    Serial.println(F("EROARE: Card SD negasit!"));
    return;
  }
  Serial.println(F("SD OK!"));

  // Setam volumul initial si pornim prima melodie
  audio.setVolume(5);

  // Initializare display OLED prin I2C
  // Wire.begin() activeaza magistrala I2C (A4=SDA, A5=SCL)
  Wire.begin();
  u8x8.begin();
  Serial.println(F("OLED OK!"));

  // Reda melodia + afisam informatiile primei melodii
  pornesteMelodie(0);
}

// ─────────────────────────────────────────────────────────
// LOOP - ruleaza continuu
// ─────────────────────────────────────────────────────────
void loop() {
  unsigned long acum = millis();

  // ── Citire butoane cu debounce ────────────────────────
  if (acum - lastPressTime > DEBOUNCE_MS) {

    // Buton NEXT: trece la melodia urmatoare (circular)
    if (digitalRead(BTN_NEXT) == LOW) {
      lastPressTime = acum;
      melodieCurenta = (melodieCurenta + 1) % numarMelodii;
      pornesteMelodie(melodieCurenta);
    }
    // Buton PREV: revine la melodia anterioara (circular)
    else if (digitalRead(BTN_PREV) == LOW) {
      lastPressTime = acum;
      melodieCurenta = (melodieCurenta - 1 + numarMelodii) % numarMelodii;
      pornesteMelodie(melodieCurenta);
    }
    // Buton PLAY/PAUSE: toggle intre redare si pauza
    else if (digitalRead(BTN_PLAY) == LOW) {
      lastPressTime = acum;
      audio.pause();           // TMRpcm: apel repetat = toggle
      isPaused = !isPaused;
      afiseazaOLED();          // Actualizam statusul pe display
      Serial.println(isPaused ? F("PAUZAT") : F("REDARE"));
    }
  }

  // ── Potentiometru -> Volum & VU Meter ─────────────────
  // Citim valoarea ADC (0-1023) de pe sliderul de volum
  int valPot = analogRead(potPin);

  // Mapam la intervalul de volum TMRpcm (0-3)
  audio.setVolume(map(valPot, 0, 1023, 0, 3));

  // Mapam la numarul de LED-uri aprinse (VU meter vizual)
  int leduriAprinse = map(valPot, 0, 1023, 0, numarLED);
  for (int i = 0; i < numarLED; i++) {
    digitalWrite(vuPins[i], i < leduriAprinse ? HIGH : LOW);
  }

  delay(50);  // Pauza scurta pentru stabilitate citiri ADC
}