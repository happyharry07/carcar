 /***************************************************************************/
// File			  [bluetooth.h]
// Author		  [Erik Kuo]
// Synopsis		hhhhhhh[Code for bluetooth communication]
// Functions  [ask_BT, send_msg, send_byte]
// Modify		  [2020/03/27 Erik Kuo]
/***************************************************************************/

/*if you have no idea how to start*/
/*check out what you have learned from week 2*/

enum BT_CMD {
    NOTHING,  // No command or invalid command
    START,    // Start the car
    FRONT,    // Go forward at a node
    LEFT,     // Turn left at a node
    RIGHT,    // Turn right at a node
    BACK,     // Turn around at a node
    HALT      // Stop the car
};

BT_CMD ask_BT() {
    BT_CMD message = NOTHING;
    
    if (Serial3.available()) {
        // TODO:
        // 1. get cmd from Serial3(bluetooth serial)
        // 2. link bluetooth message to your own command type
        char cmd = Serial3.read();
        if (cmd == '\n' || cmd == '\r' || cmd == ' ') return NOTHING;
        
        // Map received characters to commands
        switch(cmd) {
            case 's':
                message = START;
                break;
            case 'f':
                message = FRONT;
                break;
            case 'l':
                message = LEFT;
                break;
            case 'r':
                message = RIGHT;
                break;
            case 'b':
                message = BACK;
                break;
            case 'h':
                message = HALT;
                break;
        }

#ifdef DEBUG
        Serial.print("cmd : ");
        Serial.println(cmd);
#endif
    }
    return message;
}  // ask_BT

// send msg back through Serial1(bluetooth serial)
// can use send_byte alternatively to send msg back
// (but need to convert to byte type)
void send_msg(const char& msg) {
    // TODO:
    Serial3.println(msg);

#ifdef DEBUG
    Serial.print("Sent message: ");
    Serial.println(msg);
#endif
}  // send_msg

// send UID back through Serial3(bluetooth serial)
void send_byte(byte* id, byte& idSize) {
    for (byte i = 0; i < idSize; i++) {  // Send UID consequently.
        Serial3.print(id[i]);
    }
#ifdef DEBUG
    Serial.print("Sent id: ");
    for (byte i = 0; i < idSize; i++) {  // Show UID consequently.
        Serial.print(id[i], HEX);
    }
    Serial.println();
#endif
}  // send_byte