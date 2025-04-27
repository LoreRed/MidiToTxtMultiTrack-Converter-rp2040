#ifndef MIDITOTXTMULTITRACK_H
#define MIDITOTXTMULTITRACK_H

#include <Arduino.h>  // Include le librerie base di Arduino
#include <SD.h>  // Include la libreria per l'interfacciamento con la SD

#define BUFFER_SIZE 1024 // Numero massimo di eventi che il buffer può contenere

// Struttura che rappresenta un evento MIDI
struct MidiEvent {
    uint32_t timeMs;  // Tempo dell'evento in millisecondi
    uint8_t note;     // Numero della nota MIDI (ad esempio 60 per il Do centrale)
    bool onOff;       // Stato della nota: true per "Note ON", false per "Note OFF"
};

class MidiToTxtMultiTrack {
public:
    // Funzione principale per convertire il file MIDI in un file di testo
    bool convert(const char* midiFilename, const char* txtFilename);

private:
    File midiFile;      // Oggetto File per accedere al file MIDI sulla SD
    File txtFile;       // Oggetto File per scrivere il file di testo sulla SD
    uint16_t division;  // Divisione del file MIDI, rappresenta la risoluzione dei ticks
    float tickDurationUs = 0.0f;  // Durata di un tick in microsecondi
    uint32_t tempo = 500000; // Tempo predefinito (500000 microsecondi = 120 BPM)

    MidiEvent eventBuffer[BUFFER_SIZE]; // Buffer per memorizzare gli eventi MIDI prima di scriverli su file
    uint16_t bufferIndex = 0;  // Indice che tiene traccia di quante voci ci sono nel buffer

    // Funzioni private per la lettura dei dati dal file MIDI
    uint8_t readByte();        // Legge un byte dal file MIDI
    uint32_t readVarLen();     // Legge una lunghezza variabile (VarLen) dal file MIDI
    uint16_t read16();         // Legge un intero a 16 bit (2 byte)
    uint32_t read32();         // Legge un intero a 32 bit (4 byte)

    // Funzione per analizzare una traccia MIDI e ottenere gli eventi da essa
    bool parseTrack(uint32_t trackEnd, uint32_t& absoluteTicks);

    // Funzioni per gestire gli eventi nel buffer
    void addEventToBuffer(uint32_t timeMs, uint8_t note, bool onOff);  // Aggiunge un evento al buffer
    void flushBuffer();  // Scrive gli eventi contenuti nel buffer su file di testo e resetta il buffer
};

#endif // MIDITOTXTMULTITRACK_H
