// Arduino FM Rádio Si4703

// připojení potřebných knihoven
#include <Si4703_Breakout.h>
#include <Wire.h>
// nastavení propojovacích pinů
#define resetPin 2
#define SDIO A4
#define SCLK A5
// inicializace modulu z knihovny
Si4703_Breakout radio(resetPin, SDIO, SCLK);
// proměnné pro běh programu
int frekvence;
int hlasitost;
char rdsBuffer[10];

void setup() {
  // zahájení komunikace po sériové lince
  // rychlostí 9600 baud
  Serial.begin(9600);
  // vytištění nápovědy
  Serial.println("\n\nSi4703 Radio");
  Serial.println("===========================");
  Serial.println("a b     Oblibene stanice nastavene v programu");
  Serial.println("+ -     hlasitost (max 15)");
  Serial.println("n d     Zmena frekvence nahoru/dolu");
  Serial.println("r       Nacteni RDS dat (15 sekund timeout)");
  Serial.println();
  // zahájení komunikace s modulem
  radio.powerOn();
  // nastavení hlasitosti na nulu
  radio.setVolume(0);
}
void loop() {
  // kontrola sériové linky na příchozí data
  if (Serial.available()) {
    // načteme přijatý znak do proměnné
    char ch = Serial.read();
    // dále pomocí if podmínek hledáme známé znaky pro ovládání činnosti,
    // při každé činnosti je zároveň zavolán podprogram zobrazInfo,
    // který vytiskne aktuální frekvenci a hlasitost po sériové lince
    
    // při znaku n spustíme hledání směrem nahoru
    if (ch == 'n') {
      frekvence = radio.seekUp();
        Serial.print("Frekvence:"); Serial.print(frekvence);
  Serial.print(" | Hlasitost:"); Serial.println(hlasitost);
    }
    // při znaku D spustíme hledání směrem dolů
    else if (ch == 'd') {
      frekvence = radio.seekDown();
        Serial.print("Frekvence:"); Serial.print(frekvence);
  Serial.print(" | Hlasitost:"); Serial.println(hlasitost);
    }
    // při znaku + přidáme hlasitost
    else if (ch == '+') {
      hlasitost ++;
      // maximální hodnota hlasitosti je 15,
      // při větší nastavíme pouze 15
      if (hlasitost == 16) hlasitost = 15;
      radio.setVolume(hlasitost);
        Serial.print("Frekvence:"); Serial.print(frekvence);
  Serial.print(" | Hlasitost:"); Serial.println(hlasitost);
    }
    // při znaku - snížíme hlasitost
    else if (ch == '-') {
      hlasitost --;
      // minimální hodnota hlasitosti je 0,
      // při záporné hodnotě nastavíme 0
      if (hlasitost < 0) hlasitost = 0;
      radio.setVolume(hlasitost);
        Serial.print("Frekvence:"); Serial.print(frekvence);
  Serial.print(" | Hlasitost:"); Serial.println(hlasitost);
    }
    // při znaku a nastavíme oblíbenou stanici,
    // pro kterou je nastavena frekvence níže
    else if (ch == 'a') {
      frekvence = 910; // Radio Beat
      radio.setChannel(frekvence);
        Serial.print("Frekvence:"); Serial.print(frekvence);
  Serial.print(" | Hlasitost:"); Serial.println(hlasitost);
    }
    // při znaku b nastavíme oblíbenou stanici,
    // pro kterou je nastavena frekvence níže
    else if (ch == 'b') {
      frekvence = 1055; // Radio Evropa 2
      radio.setChannel(frekvence);
        Serial.print("Frekvence:"); Serial.print(frekvence);
  Serial.print(" | Hlasitost:"); Serial.println(hlasitost);
    }
    // při znaku r spustíme načítání RDS dat,
    // které bude trvat 15 sekund - lze změnit níže
    else if (ch == 'r') {
      // pomocí funkce readRDS se pokusíme
      // načíst RDS data do proměnné
      // a následně je vytiskneme
      Serial.println("Nacteni RDS dat...");
      radio.readRDS(rdsBuffer, 15000);
      Serial.print("RDS data:");
      Serial.println(rdsBuffer);
    }
  }
}
