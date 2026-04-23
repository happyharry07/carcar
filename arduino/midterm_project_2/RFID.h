/***************************************************************************/
// File       [RFID.h]
// Author     [Erik Kuo]
// Synopsis   [Code for getting UID from RFID card]
// Functions  [rfid]
// Modify     [2020/03/27 Erik Kuo]
/***************************************************************************/

/*===========================don't change anything in this file===========================*/

#include <MFRC522.h>  // Include library
#include <SPI.h>

extern MFRC522 mfrc522;
/* pin---- SDA:9 SCK:13 MOSI:11 MISO:12 GND:GND RST:define on your own  */

byte* rfid(byte& idSize) {
    // Check if there's a new card
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
        byte* id = mfrc522.uid.uidByte;  // Get the card's UID
        idSize = mfrc522.uid.size;       // Get the UID length

// Serial.print("PICC type: ");      // Display card type
//  Determine card type based on the SAK value (mfrc522.uid.sak)
// MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
// Serial.println(mfrc522.PICC_GetTypeName(piccType));
#ifdef DEBUG
        Serial.print("UID Size: ");  // Display the card's UID length
        Serial.println(idSize);
        for (byte i = 0; i < idSize; i++) {  // Display each UID byte
            Serial.print("id[");
            Serial.print(i);
            Serial.print("]: ");
            Serial.println(id[i], HEX);  // Display UID value in hexadecimal
        }
        Serial.println();
#endif
        mfrc522.PICC_HaltA();  // Put the card in halt mode
        return id;
    }
    return 0;
}