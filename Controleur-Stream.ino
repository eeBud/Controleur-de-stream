#include "MIDIUSB.h"
#include "Keyboard.h"
#include <Adafruit_SSD1306.h>

/////////////////////////
/*VARIABLES UTILISATEUR*/
/////////////////////////

#define Bouton_1 15
#define Bouton_2 14
#define Bouton_3 9
#define Bouton_4 4
#define Bouton_5 10
#define Bouton_6 16
#define Bouton_7 8
#define Bouton_8 7
#define Bouton_9 5
#define Bouton_10 6

#define Timout 1000


//Paramètres des potentiomètres MIDI
#define Nombre_Potentiometres 4
const byte Pins_Potentiometres[Nombre_Potentiometres] = { A0, A1, A2, A3 };
const char *Noms_Potentiometres[Nombre_Potentiometres] = { "Micro", "Cable", "Tchat", "Gen" };
const bool Aimanter[Nombre_Potentiometres] = { true, true, true, true };
const int Intensite_Aimant[Nombre_Potentiometres] = { 3, 3, 3, 3 };
const int Position_Aimant[Nombre_Potentiometres] = { 127, 127, 127, 127 };
const int Sensibilite = 2;

//Paramètres des boutons MIDI
#define Nombre_Boutons_MIDI 6
const byte Pins_Boutons_MIDI[Nombre_Boutons_MIDI] = { Bouton_1, Bouton_2, Bouton_7, Bouton_8, Bouton_9, Bouton_10 };
const char *Noms_Boutons_MIDI[Nombre_Boutons_MIDI] = { "Casque", "HP", "Micro", "Cable", "Tchat", "Diffuser" };
const char *Noms_Boutons_MIDI_0[Nombre_Boutons_MIDI] = { "On", "On", "On", "On", "On", "Off" };
const char *Noms_Boutons_MIDI_127[Nombre_Boutons_MIDI] = { "Mute", "Mute", "Mute", "Mute", "Mute", "On" };

//Paramètres des boutons de raccourcis
#define Nombre_Boutons_R 4
const byte Pins_Boutons_R[Nombre_Boutons_R] = { Bouton_3, Bouton_4, Bouton_5, Bouton_6 };
const char Raccourcis_Boutons_R[Nombre_Boutons_R] = { '1', '2', '3', '4' };  //Utiliser '' et pas ""
const char *Noms_Boutons_R[Nombre_Boutons_R] = { "Scene 1", "Scene 2", "Scene 3", "Scene 4" };
const bool CTRL = true;
const bool ALT = false;

///////////////////////
/*VARIABLES PROGRAMME*/
///////////////////////

//Ecran
#define nombreDePixelsEnLargeur 128
#define nombreDePixelsEnHauteur 64
#define brocheResetOLED -1
#define adresseI2CecranOLED 0x3C
Adafruit_SSD1306 oled(nombreDePixelsEnLargeur, nombreDePixelsEnHauteur, &Wire, -1);

//Vars Boutons MIDI
bool Etat_Bouton_MIDI[Nombre_Boutons_MIDI] = { false };
bool Mem_Bouton_MIDI[Nombre_Boutons_MIDI] = { false };
bool Action_Bouton_MIDI = false;

//Vars Boutons de raccourcis
bool Etat_Bouton_R[Nombre_Boutons_R] = { false };
bool Mem_Bouton_R[Nombre_Boutons_R] = { false };
bool Action_Bouton_R = false;

//Vars Pots
int Etat_Potentiometre[Nombre_Potentiometres] = {};
int Mem_Etat_Potentiometre[Nombre_Potentiometres] = {};
bool Action_Potentiometre[Nombre_Potentiometres] = {};
unsigned long Memoire_Timer_F = 0;
unsigned long Memoire_Timer_P = 0;
unsigned long Memoire_Timer = 0;
unsigned long Present_Timer = 0;
int Difference_Potentiometre = 0;

//Vars Feedback
bool Feed_Boutons_MIDI[Nombre_Boutons_MIDI] = { false };
bool Feed = false;

//Divers
const char _en[83] = " =qwertyuiopasdfghjkl;zxcvbnQWERTYUIOPASDFGHJKL:ZXCVBNm,./M<>?1234567890!@#$%^&*()";
const char _fr[83] = " =azertyuiopqsdfghjklmwxcvbnAZERTYUIOPQSDFGHJKLMWXCVBN,;:!?./ & \"'(- _  1234567890";

char fr(char CS) {
  char ret = 0;
  for (int i = 0; i < 82; i++) {
    if (_fr[i] == CS) {
      ret = _en[i];
      break;
    }
    delay(2);
  }
  return ret;
}


void setup() {
  delay(500);

  for (int i = 0; i < Nombre_Boutons_MIDI; i++) {
    pinMode(Pins_Boutons_MIDI[i], INPUT_PULLUP);
    delay(2);
  }

  for (int i = 0; i < Nombre_Boutons_R; i++) {
    pinMode(Pins_Boutons_R[i], INPUT_PULLUP);
    delay(2);
  }

  Keyboard.begin();

  //Ecran
  if (!oled.begin(SSD1306_SWITCHCAPVCC, adresseI2CecranOLED)) {
    while (1)
      ;
  }

  OledWrite("Bonjour", "");
  delay(1000);
}


void OledWrite(const char *Pot, const char *buf) {
  int16_t x1, y1;
  uint16_t w, h;
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(2);
  oled.getTextBounds(Pot, nombreDePixelsEnLargeur, nombreDePixelsEnHauteur, &x1, &y1, &w, &h);
  oled.setCursor((nombreDePixelsEnLargeur - w) / 2, 0);
  oled.print(Pot);
  oled.setTextSize(3);
  oled.getTextBounds(buf, nombreDePixelsEnLargeur, nombreDePixelsEnHauteur, &x1, &y1, &w, &h);
  oled.setCursor((nombreDePixelsEnLargeur - w) / 2, 20);
  oled.print(buf);
  oled.display();
}

int larg = 0;
int pas = 0;
int j = 0;
void OledMenu() {
  oled.clearDisplay();
  OledCase();
  oled.display();
}

void OledCase() {
  if (larg == 0) {
    larg = (nombreDePixelsEnLargeur / Nombre_Boutons_MIDI) - 4;
    pas = (nombreDePixelsEnLargeur / Nombre_Boutons_MIDI);
    if (larg > 28) larg = 28;
    j = (pas / 2) - (larg / 2);
  }
  for (int i = 0; i < Nombre_Boutons_MIDI; i++) {
    if (Feed_Boutons_MIDI[i]) {
      oled.fillRoundRect(j, 16, larg, larg, 5, WHITE);
    } else {
      oled.drawRoundRect(j, 16, larg, larg, 5, WHITE);
    }
    j = j + pas;
    delay(2);
  }
  j = (pas / 2) - (larg / 2);
}


void loop() {
  Potentiometres_MIDI();
  Boutons_MIDI();
  Feedback_MIDI();
  Timer_MIDI();
  Boutons_R();
}


void Boutons_R() {
  for (int i = 0; i < Nombre_Boutons_R; i++) {
    Etat_Bouton_R[i] = digitalRead(Pins_Boutons_R[i]);
    if (Etat_Bouton_R[i] != Mem_Bouton_R[i]) {
      
      if (Etat_Bouton_R[i] == false) {
        if (CTRL) Keyboard.press(KEY_LEFT_CTRL);
        if (ALT) Keyboard.press(KEY_LEFT_ALT);
        Keyboard.press(fr(Raccourcis_Boutons_R[i]));
        OledWrite(Noms_Boutons_R[i], "");
      } else {
        Keyboard.release(fr(Raccourcis_Boutons_R[i]));
        if (ALT) Keyboard.release(KEY_LEFT_ALT);
        if (CTRL) Keyboard.release(KEY_LEFT_CTRL);
        OledMenu();
      }
      Mem_Bouton_R[i] = Etat_Bouton_R[i];
      delay(Timout);
    }
    delay(2);
  }
}

void Timer_MIDI() {
  bool AP = false;
  Present_Timer = millis();

  if (Present_Timer - Memoire_Timer_P > Timout) {
    for (int i = 0; i < Nombre_Potentiometres; i++) {
      if (Action_Potentiometre[i]) {
        Action_Potentiometre[i] = false;
        AP = true;
        OledMenu();
      }
      delay(2);
    }
  }

  if (Present_Timer - Memoire_Timer > Timout && Action_Bouton_MIDI) {
    Action_Bouton_MIDI = false;
    OledMenu();
  }

  if (Present_Timer - Memoire_Timer_F > Timout && Feed && !Action_Bouton_MIDI && !AP) {
    Feed = false;
    OledMenu();
  }
}
void Feedback_MIDI() {
  midiEventPacket_t rx;

  rx = MidiUSB.read();
  if (rx.header != 0) {
    if (rx.header == 9) {  // Boutons MIDI
      Feed = true;
      Memoire_Timer_F = millis();

      if (rx.byte3 == 0) {
        OledWrite(Noms_Boutons_MIDI[rx.byte2], Noms_Boutons_MIDI_0[rx.byte2]);
        Feed_Boutons_MIDI[rx.byte2] = false;
      } else {
        OledWrite(Noms_Boutons_MIDI[rx.byte2], Noms_Boutons_MIDI_127[rx.byte2]);
        Feed_Boutons_MIDI[rx.byte2] = true;
      }
    } else if (rx.header == 11) {  // Potentiomètres
      Feed = true;
      Memoire_Timer_F = millis();
      float Value = 0;

      Value = rx.byte3;
      char result[8];
      dtostrf(Value, 2, 0, result);

      OledWrite(Noms_Potentiometres[rx.byte2], result);
    }
  }
}


void Boutons_MIDI() {
  for (int i = 0; i < Nombre_Boutons_MIDI; i++) {

    Etat_Bouton_MIDI[i] = digitalRead(Pins_Boutons_MIDI[i]);
    if (Etat_Bouton_MIDI[i] != Mem_Bouton_MIDI[i]) {
      //Serial.println("BoutonMIDI");
      Action_Bouton_MIDI = true;
      Memoire_Timer = millis();
      if (Etat_Bouton_MIDI[i] == false) {
        noteOn(0, i, 100);
        MidiUSB.flush();
      } else {
        noteOff(0, i, 100);
        MidiUSB.flush();
      }
      Mem_Bouton_MIDI[i] = Etat_Bouton_MIDI[i];
      delay(200);
    }
    delay(2);
  }
}

void Potentiometres_MIDI() {
  for (int i = 0; i < Nombre_Potentiometres; i++) {
    int State = analogRead(Pins_Potentiometres[i]);
    Etat_Potentiometre[i] = map(State, 0, 1023, 0, 127);
    if (Position_Aimant[i] - Intensite_Aimant[i] < Etat_Potentiometre[i] && Etat_Potentiometre[i] < Position_Aimant[i] + Intensite_Aimant[i] && Aimanter[i]) Etat_Potentiometre[i] = Position_Aimant[i];
    Difference_Potentiometre = abs(Etat_Potentiometre[i] - Mem_Etat_Potentiometre[i]);
    if (Difference_Potentiometre > Sensibilite) {
      Memoire_Timer_P = millis();
      Action_Potentiometre[i] = true;
    }
    if (Action_Potentiometre[i] == true) {  //&& Difference_Potentiometre > Sensibilite) {
      if (Etat_Potentiometre[i] != Mem_Etat_Potentiometre[i]) {
        //Serial.println("PotMidi");
        controlChange(0, i, Etat_Potentiometre[i]);
        Mem_Etat_Potentiometre[i] = Etat_Potentiometre[i];
      }
    }
    delay(2);
  }
}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = { 0x09, 0x90 | channel, pitch, velocity };
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = { 0x08, 0x80 | channel, pitch, velocity };
  MidiUSB.sendMIDI(noteOff);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = { 0x0B, 0xB0 | channel, control, value };
  MidiUSB.sendMIDI(event);
  delay(2);
  MidiUSB.flush();
}
