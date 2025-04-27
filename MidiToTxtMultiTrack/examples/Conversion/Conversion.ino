#include <SD.h>
#include <SPI.h>
#include "MidiToTxtMultiTrack.h"

// Pin ChipSelect SD card
const int chipSelect = 10;

MidiToTxtMultiTrack midiConverter;

void setup() {
    Serial.begin(9600);

    // Inizializza la SD card
    if (!SD.begin(chipSelect)) {
        Serial.println("Errore nell'inizializzazione della SD card.");
        return;
    }
    Serial.println("SD card inizializzata.");

    // Nomina i file da usare
    const char* midiFilename = "example.mid";  // Nome del file MIDI da leggere
    const char* txtFilename = "output.txt";    // Nome del file di testo da scrivere

    // Converte il file MIDI in un file di testo
    bool success = midiConverter.convert(midiFilename, txtFilename);
    if (success) {
        Serial.println("Conversione completata con successo.");
    } else {
        Serial.println("Errore durante la conversione.");
    }
}

void loop() {
}
