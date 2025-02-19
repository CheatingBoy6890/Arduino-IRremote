/*
 * ir_MilesTag2.cpp
 *
 * Implementierung des MilesTag2-Protokolls für Laser-Tag-Spiele.
 * Basierend auf einer Portierung der IRremoteESP8266-Implementierung.
 */

#ifndef _IR_MILESTAG2_HPP
#define _IR_MILESTAG2_HPP

// MilesTag2-Konstanten
#define MILESTAG2_SHOT_BITS 14
#define MILESTAG2_MSG_BITS 24

#define MILESTAG_UNIT 600 // The smalles pulse width is 600us or approx. 23 periods with 38khz modulation
#define MILESTAG2_KHZ 38

#define MILESTAG2_RECEIVE_MULTIPLIER 1.25 // The Hardware tends to record both too long pulses and spaces. We need to compensate for that.

#ifdef MILESTAG2_RECEIVE_MULTIPLIER
#warning "MILESTAG2_RECEIVE_MULTIPLIER is defined. This is a really bad way of compensating for hardware issues. Be aware."
#endif

#define MILESTAG2_SHOT_MASK (1 << (MILESTAG2_SHOT_BITS - 1))
#define MILESTAG2_MSG_MASK (1 << (MILESTAG2_MSG_BITS - 1))
#define MILESTAG2_MSG_TERMINATOR 0xE8

#define MILESTAG2_HEADER_MARK (4 * MILESTAG_UNIT)
#define MILESTAG2_SPACE MILESTAG_UNIT
#define MILESTAG2_ONE_MARK (2 * MILESTAG_UNIT)
#define MILESTAG2_ZERO_MARK MILESTAG_UNIT
#define MILESTAG2_REPEATT_LENGTH 32000

struct PulseDistanceWidthProtocolConstants MilesTag2ProtocolSendConstants =
    {MILESTAG2,
     MILESTAG2_KHZ,
     {MILESTAG2_HEADER_MARK,
      MILESTAG2_SPACE,
      MILESTAG2_ONE_MARK,
      MILESTAG2_SPACE,
      MILESTAG2_ZERO_MARK,
      MILESTAG2_SPACE},
     PROTOCOL_IS_MSB_FIRST | SUPPRESS_STOP_BIT,
     (MILESTAG2_REPEATT_LENGTH / MICROS_IN_ONE_MILLI),
     NULL};

struct PulseDistanceWidthProtocolConstants MilesTag2ProtocolReceiveConstants =
    {MILESTAG2,
     MILESTAG2_KHZ,
     {MILESTAG2_HEADER_MARK * MILESTAG2_RECEIVE_MULTIPLIER,
      MILESTAG2_SPACE *MILESTAG2_RECEIVE_MULTIPLIER,
      MILESTAG2_ONE_MARK *MILESTAG2_RECEIVE_MULTIPLIER,
      MILESTAG2_SPACE *MILESTAG2_RECEIVE_MULTIPLIER,
      MILESTAG2_ZERO_MARK *MILESTAG2_RECEIVE_MULTIPLIER,
      MILESTAG2_SPACE *MILESTAG2_RECEIVE_MULTIPLIER},
     PROTOCOL_IS_MSB_FIRST | SUPPRESS_STOP_BIT,
     (MILESTAG2_REPEATT_LENGTH / MICROS_IN_ONE_MILLI),
     NULL};

// Send a MilesTag2 Packet ( 14bit for shot and 24 bit for message )
void IRsend::sendMilesTag2(uint32_t data, uint8_t nbits, uint16_t repeat)
{
    sendPulseDistanceWidth(&MilesTag2ProtocolSendConstants, data, nbits, repeat);
}

// Decodierung eines MilesTag2-Pakets
bool IRrecv::decodeMilesTag2()
{
    if (!checkHeader(&MilesTag2ProtocolReceiveConstants))
        return false;

    if (decodedIRData.rawlen != (2 * MILESTAG2_SHOT_BITS) + 2 && decodedIRData.rawlen != (2 * MILESTAG2_MSG_BITS) + 2)
    {
        IR_DEBUG_PRINT(F("Milestag2: "));
        IR_DEBUG_PRINT(F("Data length="));
        IR_DEBUG_PRINT(decodedIRData.rawlen);
        IR_DEBUG_PRINTLN(F(" is not 14 or 24"));
        return false;
    }

    if (!decodePulseDistanceWidthData(&MilesTag2ProtocolReceiveConstants, (decodedIRData.rawlen - 1) / 2))
    {
        IR_DEBUG_PRINT(F("Milestag2: "));
        IR_DEBUG_PRINT(F("Data length="));
        IR_DEBUG_PRINT(decodedIRData.rawlen);
        IR_DEBUG_PRINTLN(F(" is not 14 or 24"));
        return false;
    }

    decodedIRData.numberOfBits = (decodedIRData.rawlen - 1) / 2;
    decodedIRData.protocol = MILESTAG2;
    decodedIRData.flags = MilesTag2ProtocolReceiveConstants.Flags;
    uint32_t data = decodedIRData.decodedRawData;

    // Prüfen auf SHOT oder MSG Paket
    if ((decodedIRData.rawlen - 1) / 2 == MILESTAG2_SHOT_BITS)
    {
        if (data & MILESTAG2_SHOT_MASK)      // Check if the MSB is set
            return false;                    // SHOT-Pakete müssen `0` am MSB haben.
        decodedIRData.command = data & 0x3F; // Team & Damage
        decodedIRData.address = data >> 6;   // Player ID
    }
    else if ((decodedIRData.rawlen - 1) / 2 == MILESTAG2_MSG_BITS)
    {
        if (!(data & MILESTAG2_MSG_MASK) || ((data & 0xFF) != MILESTAG2_MSG_TERMINATOR))
            return false;
        decodedIRData.command = (data >> 8) & 0xFF;  // MSG Daten
        decodedIRData.address = (data >> 16) & 0x7F; // MSG ID
    }

    checkForRepeatSpaceTicksAndSetFlag(MILESTAG2_REPEATT_LENGTH / MICROS_PER_TICK);
    return true;
}

#endif // _IR_MILESTAG2_HPP
