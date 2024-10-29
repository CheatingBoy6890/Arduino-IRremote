/*
 * ir_MilesTag2.cpp
 *
 * Implementierung des MilesTag2-Protokolls für Laser-Tag-Spiele.
 * Basierend auf einer Portierung der IRremoteESP8266-Implementierung.
 */

#ifndef _IR_MILESTAG2_HPP
#define _IR_MILESTAG2_HPP

#include <Arduino.h>
#include "IRremote.h"

// MilesTag2-Konstanten
#define MILESTAG2_SHOT_BITS 14
#define MILESTAG2_MSG_BITS 24

#define MILESTAG2_SHOT_MASK (1 << (MILESTAG2_SHOT_BITS - 1))
#define MILESTAG2_MSG_MASK (1 << (MILESTAG2_MSG_BITS - 1))
#define MILESTAG2_MSG_TERMINATOR 0xE8

#define MILESTAG2_HEADER_MARK 2400
#define MILESTAG2_SPACE 600
#define MILESTAG2_ONE_MARK 1200
#define MILESTAG2_ZERO_MARK 600
#define MILESTAG2_REPEATT_LENGTH 32000
#define MILESTAG2_KHZ 38
#define MILESTAG2_MESSAGE_TERMINATOR 0xE

struct PulseDistanceWidthProtocolConstants MilesTag2ProtocolConstants =
    {MILESTAG2,
     MILESTAG2_KHZ,
     {MILESTAG2_HEADER_MARK,
      MILESTAG2_SPACE,
      MILESTAG2_ONE_MARK,
      MILESTAG2_SPACE,
      MILESTAG2_ZERO_MARK,
      MILESTAG2_SPACE},
     PROTOCOL_IS_MSB_FIRST,
     MILESTAG2_REPEATT_LENGTH,
     NULL};

// Send a MilesTag2 Packet ( 14bit for shot and 24 bit for message )
void IRsend::sendMilesTag2(uint32_t data, uint8_t nbits, uint16_t repeat)
{
    sendPulseDistanceWidth(&MilesTag2ProtocolConstants, data, nbits, repeat);
}

// Decodierung eines MilesTag2-Pakets
bool IRrecv::decodeMilesTag2()
{
    if (!checkHeader(&MilesTag2ProtocolConstants))
        return false;

    // Bestimmen der Anzahl der Bits
    uint16_t nbits = (decodedIRData.rawlen > 30) ? MILESTAG2_MSG_BITS : MILESTAG2_SHOT_BITS;

    if (!decodePulseDistanceWidthData(&MilesTag2ProtocolConstants, nbits))
        return false;

    decodedIRData.numberOfBits = nbits;
    decodedIRData.protocol = MILESTAG2;
    uint32_t data = decodedIRData.decodedRawData;

    // Prüfen auf SHOT oder MSG Paket
    if (nbits == MILESTAG2_SHOT_BITS)
    {
        if (data & MILESTAG2_SHOT_MASK)
            return false;                    // SHOT-Pakete müssen `0` am MSB haben.
        decodedIRData.command = data & 0x3F; // Team & Damage
        decodedIRData.address = data >> 6;   // Player ID
    }
    else if (nbits == MILESTAG2_MSG_BITS)
    {
        if (!(data & MILESTAG2_MSG_MASK) || ((data & 0xFF) != MILESTAG2_MSG_TERMINATOR))
            return false;
        decodedIRData.command = (data >> 8) & 0xFF;  // MSG Daten
        decodedIRData.address = (data >> 16) & 0x7F; // MSG ID
    }
    else
    {
        return false; // Ungültige Bitlänge
    }

    checkForRepeatSpaceTicksAndSetFlag(MILESTAG2_REPEATT_LENGTH / MICROS_PER_TICK);
    return true;
}

#endif // _IR_MILESTAG2_HPP
