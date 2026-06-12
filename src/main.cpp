/*
 * =====================================================
 *  🚑 AMBULANS SİSTEMİ v2 — ESP32-S3
 * =====================================================
 *  MODLAR (butona her basışta bir sonraki moda geçer):
 *
 *  MOD 0 — KAPALI
 *    LED kapalı, ses yok
 *
 *  MOD 1 — KLASİK AMBULANS
 *    Sol yarı mavi / sağ yarı kırmızı hızlı yanıp söner
 *    Ses: Wee-Woo sweep (700Hz ↔ 1200Hz)
 *
 *  MOD 2 — FULL RENK DÖNGÜSÜ
 *    Tüm LEDler → tam MAVİ ... tam KIRMIZI (hızlı dönüşüm)
 *    Ses: Çift bip (iki farklı ton art arda)
 *
 *  MOD 3 — YAVAŞ FLAŞ
 *    3 sn KIRMIZI → 3 sn MAVİ (sert geçiş)
 *    Ses: Uzun iniş-çıkış (ağır sirn tonu)
 *
 *  Bağlantılar:
 *    Joystick : SW→GPIO3 | X→GPIO1 | Y→GPIO2 | 3V3→3V3 | GND→GND
 *    Hoparlör : SD→GPIO4 | IN+→GPIO5 | GND→GND
 *    LED Ring : DIN→GPIO6 | VCC→5V | GND→GND
 * =====================================================
 */

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// ── Pin Tanımları ─────────────────────────────────
#define JOY_SW_PIN      3
#define SPEAKER_SD_PIN  4
#define SPEAKER_PWM_PIN 5
#define LED_PIN         6
#define LED_COUNT       8
#define RGB_LED_PIN     48   // ESP32-S3 dahili RGB

// ── PWM ───────────────────────────────────────────
#define PWM_CHANNEL     0
#define PWM_RESOLUTION  8

// ── Nesneler ──────────────────────────────────────
Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ── Mod ───────────────────────────────────────────
int  currentMode   = 0;      // 0=Kapalı, 1=Klasik, 2=FullRenk, 3=YavaşFlaş
#define MAX_MODES   4

// ── Buton debounce ────────────────────────────────
bool          lastBtnState  = HIGH;
bool          debouncedBtn  = HIGH;
unsigned long lastDebounce  = 0;
#define DEBOUNCE_MS 50

// ── LED zamanlaması ───────────────────────────────
unsigned long lastLedUpdate = 0;
bool          flashState    = false;  // mavi mi kırmızı mı
bool          ledIsOn       = true;

// ── Ses zamanlaması ───────────────────────────────
unsigned long lastSoundUpdate = 0;
int  currentFreq    = 700;
int  sirenDir       = +1;

// Mod2: çift bip durumu
int  bipPhase       = 0;       // 0=bip1, 1=bekleme1, 2=bip2, 3=bekleme2
unsigned long bipTimer = 0;

// Mod3: 3 sn blok sayacı
unsigned long blockStart = 0;
bool          blockRed   = true;  // true=kırmızı blok

// ── Yardımcı ──────────────────────────────────────
void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  neopixelWrite(RGB_LED_PIN, r, g, b);
}

void speakerOn(int freq) {
  if (freq < 20) freq = 20;
  ledcWriteTone(PWM_CHANNEL, freq);
}

void speakerOff() {
  ledcWrite(PWM_CHANNEL, 0);
}

void allLed(uint8_t r, uint8_t g, uint8_t b) {
  ring.fill(ring.Color(r, g, b));
  ring.show();
}

void ledOff() {
  ring.clear();
  ring.show();
  setRGB(0, 0, 0);
}

// ── Mod geçişinde sıfırla ─────────────────────────
void enterMode(int mode) {
  currentMode  = mode;
  lastLedUpdate  = millis();
  lastSoundUpdate = millis();
  flashState   = false;
  ledIsOn      = true;
  currentFreq  = 700;
  sirenDir     = +1;
  bipPhase     = 0;
  bipTimer     = millis();
  blockStart   = millis();
  blockRed     = true;

  if (mode == 0) {
    speakerOff();
    digitalWrite(SPEAKER_SD_PIN, LOW);
    ledOff();
    Serial.println("MOD 0 — Kapalı");
  } else {
    digitalWrite(SPEAKER_SD_PIN, HIGH);
    Serial.printf("MOD %d aktif\n", mode);
  }
}

// ── MOD 1: Klasik Ambulans ────────────────────────
//    Sol yarı mavi / sağ yarı kırmızı — hızlı blink
//    Ses: 700↔1200 sweep
void updateMode1(unsigned long now) {
  // LED: 120ms'de bir renk/taraf değişimi
  if (now - lastLedUpdate >= 120) {
    lastLedUpdate = now;
    ring.clear();
    for (int i = 0; i < LED_COUNT / 2; i++) {
      ring.setPixelColor(i,               flashState ? ring.Color(255,0,0) : ring.Color(0,0,255));
      ring.setPixelColor(i + LED_COUNT/2, flashState ? ring.Color(0,0,255) : ring.Color(255,0,0));
    }
    ring.show();
    setRGB(flashState ? 50 : 0, 0, flashState ? 0 : 50);
    flashState = !flashState;
  }

  // Ses: sweep 700↔1200
  if (now - lastSoundUpdate >= 8) {
    lastSoundUpdate = now;
    currentFreq += sirenDir * 14;
    if (currentFreq >= 1200) { currentFreq = 1200; sirenDir = -1; }
    if (currentFreq <= 700)  { currentFreq = 700;  sirenDir = +1; }
    speakerOn(currentFreq);
  }
}

// ── MOD 2: Full Renk Döngüsü ─────────────────────
//    Tüm ring: MAVİ → KIRMIZI loopу (200ms'de bir)
//    Ses: çift bip (900Hz + 600Hz, art arda, kısa bekleme)
void updateMode2(unsigned long now) {
  // LED: 200ms'de bir tam mavi ↔ tam kırmızı
  if (now - lastLedUpdate >= 200) {
    lastLedUpdate = now;
    if (!flashState) {
      allLed(0, 0, 255);       // Tam Mavi
      setRGB(0, 0, 60);
    } else {
      allLed(255, 0, 0);       // Tam Kırmızı
      setRGB(60, 0, 0);
    }
    flashState = !flashState;
  }

  // Ses: çift bip pattern
  // Faz 0: bip1 (900Hz, 150ms)
  // Faz 1: sessiz (80ms)
  // Faz 2: bip2 (600Hz, 150ms)
  // Faz 3: sessiz (400ms)
  const int BIP_DURATIONS[] = {150, 80, 150, 400};
  const int BIP_FREQS[]     = {900,   0, 600,   0};

  if (now - bipTimer >= (unsigned long)BIP_DURATIONS[bipPhase]) {
    bipTimer = now;
    bipPhase = (bipPhase + 1) % 4;
    if (BIP_FREQS[bipPhase] > 0) {
      speakerOn(BIP_FREQS[bipPhase]);
    } else {
      speakerOff();
    }
  }
}

// ── MOD 3: Yavaş Flaş ────────────────────────────
//    3 sn kırmızı, 3 sn mavi (sert geçiş)
//    Ses: yavaş sweep 500↔1000 Hz (ağır siren)
void updateMode3(unsigned long now) {
  // LED: 3 saniyede bir renk değişimi
  if (now - blockStart >= 3000) {
    blockStart = now;
    blockRed   = !blockRed;
  }

  // Rengi sürekli yaz (her loop)
  static bool lastBlockRed = !blockRed;
  if (lastBlockRed != blockRed) {
    lastBlockRed = blockRed;
    if (blockRed) {
      allLed(255, 0, 0);
      setRGB(60, 0, 0);
      Serial.println("  → KIRMIZI");
    } else {
      allLed(0, 0, 255);
      setRGB(0, 0, 60);
      Serial.println("  → MAVİ");
    }
  }

  // Ses: yavaş sweep 500↔1000 Hz
  if (now - lastSoundUpdate >= 18) {
    lastSoundUpdate = now;
    currentFreq += sirenDir * 6;
    if (currentFreq >= 1000) { currentFreq = 1000; sirenDir = -1; }
    if (currentFreq <= 500)  { currentFreq = 500;  sirenDir = +1; }
    speakerOn(currentFreq);
  }
}

// ── Buton okuma ───────────────────────────────────
bool readButtonPressed() {
  bool reading = digitalRead(JOY_SW_PIN);

  if (reading != lastBtnState) {
    lastDebounce = millis();
    lastBtnState = reading;
  }

  if ((millis() - lastDebounce) > DEBOUNCE_MS) {
    if (reading != debouncedBtn) {
      debouncedBtn = reading;
      if (debouncedBtn == LOW) {
        return true;  // Yeni basış!
      }
    }
  }
  return false;
}

// ── Setup ─────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n╔══════════════════════════════════════╗");
  Serial.println("║   🚑 Ambulans v2 — 3 Mod + Kapalı   ║");
  Serial.println("╠══════════════════════════════════════╣");
  Serial.println("║  Buton: MOD 0→1→2→3→0→...           ║");
  Serial.println("║  MOD 0: Kapalı                       ║");
  Serial.println("║  MOD 1: Klasik ambulans              ║");
  Serial.println("║  MOD 2: Tam mavi↔kırmızı + çift bip ║");
  Serial.println("║  MOD 3: 3sn kırmızı↔mavi + ağır ses ║");
  Serial.println("╚══════════════════════════════════════╝\n");

  pinMode(JOY_SW_PIN, INPUT_PULLUP);
  pinMode(SPEAKER_SD_PIN, OUTPUT);
  digitalWrite(SPEAKER_SD_PIN, LOW);

  // PWM başlat
  ledcSetup(PWM_CHANNEL, 2000, PWM_RESOLUTION);
  ledcAttachPin(SPEAKER_PWM_PIN, PWM_CHANNEL);
  speakerOff();

  // NeoPixel
  ring.begin();
  ring.setBrightness(130);
  ring.clear();
  ring.show();

  // Dahili RGB
  pinMode(RGB_LED_PIN, OUTPUT);
  setRGB(0, 0, 0);

  // Başlangıç animasyonu
  for (int i = 0; i < LED_COUNT; i++) {
    ring.setPixelColor(i, ring.Color(0, 0, 180));
    ring.show();
    delay(50);
  }
  delay(200);
  for (int i = 0; i < LED_COUNT; i++) {
    ring.setPixelColor(i, ring.Color(180, 0, 0));
    ring.show();
    delay(50);
  }
  delay(200);
  ring.clear();
  ring.show();

  setRGB(0, 20, 0);  // Hazır → yeşil
  delay(400);
  setRGB(0, 0, 0);

  Serial.println("Hazır! MOD 0 (Kapalı). Butona bas.");
}

// ── Loop ──────────────────────────────────────────
void loop() {
  unsigned long now = millis();

  // Buton basıldıysa bir sonraki moda geç
  if (readButtonPressed()) {
    int nextMode = (currentMode + 1) % MAX_MODES;
    enterMode(nextMode);
  }

  // Aktif modu çalıştır
  switch (currentMode) {
    case 1: updateMode1(now); break;
    case 2: updateMode2(now); break;
    case 3: updateMode3(now); break;
    default: break;  // MOD 0 = hiçbir şey yapma
  }

  delay(1);
}
