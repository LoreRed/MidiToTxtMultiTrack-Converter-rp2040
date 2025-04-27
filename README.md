# MidiToTxtMultiTrack

Una libreria per rp2040 che converte file MIDI in file di testo leggibile, consentendo di gestire tracce multiple con informazioni sulle note e il tempo.

## Caratteristiche
- Legge file MIDI.
- Estrae gli eventi di nota (on/off) e li converte in un formato di testo.
- Gestisce più tracce contemporaneamente.
- Funziona con la memoria SD.

## Installazione

1. Apri l'IDE di Arduino.
2. Vai su **Sketch** -> **Include Library** -> **Add .ZIP Library...**.
3. Seleziona il file `.zip` della libreria.

## Esempio di utilizzo

```cpp
#include <MidiToTxtMultiTrack.h>

MidiToTxtMultiTrack midiConverter;

void setup() {
  Serial.begin(9600);
  if (midiConverter.convert("/test.midi", "/output.txt")) {
    Serial.println("Conversione completata!");
  } else {
    Serial.println("Errore nella conversione.");
  }
}

void loop() {
  // Aggiungi eventuali altre operazioni
}
