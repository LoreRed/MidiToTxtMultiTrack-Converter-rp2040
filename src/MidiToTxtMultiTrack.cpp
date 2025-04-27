#include "MidiToTxtMultiTrack.h"

// Funzione per leggere un byte dal file MIDI
uint8_t MidiToTxtMultiTrack::readByte() {
    int b = midiFile.read();  // Legge un byte dal file
    return (b == -1) ? 0 : (uint8_t)b;  // Se la lettura fallisce (EOF), restituisce 0, altrimenti restituisce il byte letto
}

// Funzione per leggere un intero a 16 bit (2 byte) dal file MIDI
uint16_t MidiToTxtMultiTrack::read16() {
    return (readByte() << 8) | readByte();  // Legge due byte e li combina in un intero a 16 bit
}

// Funzione per leggere un intero a 32 bit (4 byte) dal file MIDI
uint32_t MidiToTxtMultiTrack::read32() {
    return (readByte() << 24) | (readByte() << 16) | (readByte() << 8) | readByte();  // Legge quattro byte e li combina in un intero a 32 bit
}

// Funzione per leggere una lunghezza variabile (VarLen) dal file MIDI
uint32_t MidiToTxtMultiTrack::readVarLen() {
    uint32_t value = 0;
    uint8_t c;
    do {
        c = readByte();  // Legge un byte
        value = (value << 7) | (c & 0x7F);  // Estrae i 7 bit meno significativi e li aggiunge al valore
    } while (c & 0x80);  // Continua a leggere finché il bit più significativo non è zero
    return value;  // Restituisce il valore finale della lunghezza variabile
}

// Aggiunge un evento nel buffer (gli eventi saranno poi scritti nel file di testo)
void MidiToTxtMultiTrack::addEventToBuffer(uint32_t timeMs, uint8_t note, bool onOff) {
    if (bufferIndex < BUFFER_SIZE) {
        eventBuffer[bufferIndex++] = {timeMs, note, onOff};  // Aggiunge l'evento nel buffer se c'è spazio
    }

    // Se il buffer è pieno, scrive gli eventi sulla SD e svuota il buffer
    if (bufferIndex >= BUFFER_SIZE) {
        flushBuffer();
    }
}

// Scrive gli eventi dal buffer nel file di testo e resetta il buffer
void MidiToTxtMultiTrack::flushBuffer() {
    for (uint16_t i = 0; i < bufferIndex; ++i) {
        txtFile.printf("[%lu][%d][%d]\n", eventBuffer[i].timeMs, eventBuffer[i].note, eventBuffer[i].onOff ? 1 : 0);
    }
    bufferIndex = 0;  // Resetta l'indice del buffer
}

// Funzione principale per convertire il file MIDI in un file di testo
bool MidiToTxtMultiTrack::convert(const char* midiFilename, const char* txtFilename) {
    bufferIndex = 0;  // Resetta l'indice del buffer
    midiFile = SD.open(midiFilename);  // Apre il file MIDI dalla SD
    if (!midiFile) return false;  // Se il file non può essere aperto, restituisce false

    txtFile = SD.open(txtFilename, FILE_WRITE);  // Apre il file di testo sulla SD in modalità scrittura
    if (!txtFile) return false;  // Se il file di testo non può essere aperto, restituisce false

    if (read32() != 0x4D546864) return false;  // Verifica l'intestazione 'MThd' (header del file MIDI)
    read32();  // Legge la dimensione dell'header (non usata)
    uint16_t format = read16();  // Legge il formato del file MIDI (0, 1, o 2)
    uint16_t numTracks = read16();  // Legge il numero di tracce nel file MIDI
    division = read16();  // Legge la divisione (risoluzione per il tempo)

    tickDurationUs = (float)tempo / division;  // Calcola la durata di un tick in microsecondi

    // Cicla attraverso tutte le tracce e le analizza
    for (uint16_t i = 0; i < numTracks; ++i) {
        if (read32() != 0x4D54726B) break;  // Verifica l'intestazione 'MTrk' (inizio traccia)
        uint32_t trackLen = read32();  // Legge la lunghezza della traccia
        uint32_t absoluteTicks = 0;  // Inizializza il contatore dei ticks assoluti per la traccia
        if (!parseTrack(midiFile.position() + trackLen, absoluteTicks)) break;  // Analizza la traccia
    }

    // Scrive gli eventi rimasti nel buffer (se ce ne sono)
    if (bufferIndex > 0) {
        flushBuffer();
    }

    midiFile.close();  // Chiude il file MIDI
    txtFile.close();  // Chiude il file di testo
    return true;  // Restituisce true se la conversione è riuscita
}

// Funzione che analizza una traccia MIDI e estrae gli eventi
bool MidiToTxtMultiTrack::parseTrack(uint32_t trackEnd, uint32_t& absoluteTicks) {
    uint8_t runningStatus = 0;  // Stato di running status (se non è esplicitamente specificato, viene riutilizzato l'ultimo comando)
    while (midiFile.position() < trackEnd) {  // Cicla fino alla fine della traccia
        uint32_t delta = readVarLen();  // Legge il delta-time (tempo intercorso dall'ultimo evento)
        absoluteTicks += delta;  // Aggiunge il delta-time ai ticks assoluti
        uint32_t timeMs = (uint32_t)((float)absoluteTicks * tickDurationUs / 1000.0f);  // Calcola il tempo in millisecondi

        uint8_t b = readByte();  // Legge un byte che indica il tipo di evento
        if (b < 0x80) {  // Se il byte è inferiore a 0x80, è un running status
            midiFile.seek(midiFile.position() - 1);  // Torna indietro di un byte per rileggere il byte precedente
            b = runningStatus;  // Usa l'ultimo comando come running status
        } else {
            runningStatus = b;  // Aggiorna il running status
        }

        // Analizza gli eventi "Note ON" e "Note OFF" (comando 0x90 e 0x80)
        if ((b & 0xF0) == 0x90 || (b & 0xF0) == 0x80) {
            uint8_t note = readByte();  // Legge il numero della nota
            uint8_t velocity = readByte();  // Legge la velocità (intensità della nota)
            if ((b & 0xF0) == 0x90 && velocity > 0) {
                addEventToBuffer(timeMs, note, true);  // Aggiunge evento "Nota ON"
            } else {
                addEventToBuffer(timeMs, note, false);  // Aggiunge evento "Nota OFF"
            }
        } else if ((b & 0xF0) == 0xA0 || (b & 0xF0) == 0xB0 || (b & 0xF0) == 0xE0) {
            readByte(); readByte();  // Ignora i dati di controllo (ad esempio, i messaggi di modulação)
        } else if ((b & 0xF0) == 0xC0 || (b & 0xF0) == 0xD0) {
            readByte();  // Cambia il programma (ad esempio, cambiamento di timbro)
        } else if (b == 0xFF) {  // Meta-eventi (ad esempio, tempo o cambiamenti di tempo)
            uint8_t metaType = readByte();
            uint32_t len = readVarLen();
            if (metaType == 0x51 && len == 3) {  // Tempo (Meta-evento 0x51)
                tempo = (readByte() << 16) | (readByte() << 8) | readByte();
                tickDurationUs = (float)tempo / division;  // Ricalcola la durata del tick
            } else {
                for (uint32_t i = 0; i < len; ++i) readByte();  // Ignora altri metadati
            }
        } else if (b == 0xF0 || b == 0xF7) {  // SysEx (messaggi di sistema)
            uint32_t len = readVarLen();
            for (uint32_t i = 0; i < len; ++i) readByte();  // Ignora i dati SysEx
        }
    }
    return true;  // Restituisce true se la traccia è stata analizzata correttamente
}
