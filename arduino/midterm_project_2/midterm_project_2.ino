/***************************************************************************/
// File       [final_project.ino]
// Author     [Erik Kuo]
// Synopsis   [Code for managing main process]
// Functions  [setup, loop, Search_Mode, Hault_Mode, SetState]
// Modify     [2020/03/27 Erik Kuo]
/***************************************************************************/

#define DEBUG  // debug flag

// for RFID
#include <MFRC522.h>
#include <SPI.h>

// header files
#include "bluetooth.h"
#include "RFID.h"
#include "track.h"
#include "node.h"

/*===========================define pin & create module object================================*/
const int MotorR_I1 = 7;     // Define A1 pin (right)
const int MotorR_I2 = 6;     // Define A2 pin (right)
const int MotorR_PWMR = 10;  // Define ENA (PWM speed control) pin
const int MotorL_I3 = 8;     // Define B1 pin (left)
const int MotorL_I4 = 9;     // Define B2 pin (left)
const int MotorL_PWML = 11;  // Define ENB (PWM speed control) pin

const int IRpin_LL = A7;
const int IRpin_L = A6;
const int IRpin_M = A5;
const int IRpin_R = A4;
const int IRpin_RR = A3;
// RFID, set pins according to your own wiring
#define RST_PIN 3                 // Reset pin for card reader
#define SS_PIN 2                  // Chip select pin
MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 object
// BT
#define CUSTOM_NAME "sixcar" // Max length is 12 characters

// Bluetooth initialization variables
long baudRates[] = {9600, 19200, 38400, 57600, 115200, 4800, 2400, 1200, 230400};
bool moduleReady = false;

/*===========================initialize variables===========================*/
int l2 = 0, l1 = 0, m0 = 0, r1 = 0, r2 = 0;  // 紅外線模組的讀值(0->white,1->black)
bool state = false;     // set state to false to halt the car, set state to true to activate the car
// BT_CMD _cmd = NOTHING;  // enum for bluetooth message, reference in bluetooth.h line 2
BT_CMD _cmd_queue[3] = {NOTHING, NOTHING, NOTHING};
bool initial_commands_received = false; // Flag to track if we've received the initial 3 commands
bool prev_node_state = false; // Track previous node state to detect transitions

// ===== 新增：START 狀態機 =====
bool waiting_start_cmds = false;
byte start_cmd_count = 0;

// ===== 新增：RFID debounce / dedup =====
unsigned long last_rfid_ms = 0;
const unsigned long RFID_COOLDOWN_MS = 500;   // 可調：300~800ms 常見
byte last_uid[10];                            // MFRC522 UID 最長 10 bytes
byte last_uid_size = 0;

/*============setup============*/
void setup() {
    Serial.begin(9600);
    // RFID initial
    SPI.begin();
    mfrc522.PCD_Init();
    // TB6612 pin
    pinMode(MotorR_I1, OUTPUT);
    pinMode(MotorR_I2, OUTPUT);
    pinMode(MotorL_I3, OUTPUT);
    pinMode(MotorL_I4, OUTPUT);
    pinMode(MotorL_PWML, OUTPUT);
    pinMode(MotorR_PWMR, OUTPUT);
    // tracking pin
    pinMode(IRpin_LL, INPUT);
    pinMode(IRpin_L, INPUT);
    pinMode(IRpin_M, INPUT);
    pinMode(IRpin_R, INPUT);
    pinMode(IRpin_RR, INPUT);

    Serial.begin(115200);
    while (!Serial);
    Serial.println("Initializing HM-10...");
    

    // 1. Automatic Baud Rate Detection
    for (int i = 0; i < 9; i++) {
        Serial.print("Testing baud rate: ");
        Serial.println(baudRates[i]);
        
        Serial3.begin(baudRates[i]);
        Serial3.setTimeout(100);
        delay(100);

        // 2. Force Disconnection
        // Sending "AT" while connected forces the module to disconnect [2].
        Serial3.print("AT"); 
        
        if (waitForResponse("OK", 800)) {
        Serial.println("HM-10 detected and ready.");
        moduleReady = true;
        break; 
        } else {
        Serial3.end();
        delay(100);
        }
    }

    if (!moduleReady) {
        Serial.println("Failed to detect HM-10. Check 3.3V VCC and wiring.");
        return;
    }

    // 3. Restore Factory Defaults
    Serial.println("Restoring factory defaults...");
    sendATCommand("AT+RENEW"); // Restores all setup values
    delay(500);

    // 4. Set Custom Name via Macro
    Serial.print("Setting name to: ");
    Serial.println(CUSTOM_NAME);
    String nameCmd = "AT+NAME" + String(CUSTOM_NAME);
    sendATCommand(nameCmd.c_str()); // Max length is 12
    
    // 5. Enable Connection Notifications
    Serial.println("Enabling notifications...");
    sendATCommand("AT+NOTI1"); // Notify when link is established/lost

    // 6. Get the Bluetooth MAC Address
    Serial.println("Querying Bluetooth Address");
    sendATCommand("AT+ADDR?");

    // 7. Restart the module to apply changes
    Serial.println("Restarting module...");
    sendATCommand("AT+RESET"); // Restart the module
    delay(1000);
    Serial3.begin(9600); // Now the module would use baudrate 9600
    
    Serial.println("Initialization Complete.");

#ifdef DEBUG
    Serial.println("Start!");
#endif
}
/*============setup============*/

/*===========================declare function prototypes===========================*/
void Search();    // search graph
void execute_command(BT_CMD cmd); // Execute a command
bool receive_command();
void shift_queue_after_execute();
bool enqueue_command(BT_CMD cmd);
bool same_uid(byte* a, byte sizeA, byte* b, byte sizeB);
/*===========================declare function prototypes===========================*/

/*===========================define function===========================*/
void loop() {
    receive_command();
    if (!state)
        MotorWriting(0, 0);
    else {
        Search();
        // tracking(irState);
    }
    // static unsigned long t = 0;
    // if (millis() - t > 200) {
    //     t = millis();
    //     Serial3.print("[LOOP] state=");
    //     Serial3.println(state);
    // }
}

void Search() {
    // TODO: let your car search graph(maze) according to bluetooth command from computer(python code)
    // 若目前正在執行前一個動作，持續更新就好
    if (motion_busy) {
        update_motion();
        return;
    }

    // 若目前沒有可執行指令，先持續循線，不要停住
    if (_cmd_queue[0] == NOTHING) {
        tracking(irState);
        prev_node_state = is_node();
        return;
    }


    bool current_node_state = is_node();

//     // 1. Check RFID
//     byte idSize = 0;
//     byte* id = rfid(idSize);

//     if (id != 0) {
//         unsigned long now = millis();
//         bool in_cooldown = (now - last_rfid_ms) < RFID_COOLDOWN_MS;
//         bool is_same_card = same_uid(id, idSize, last_uid, last_uid_size);

//         // 條件：不在 cooldown，或是不同卡 -> 允許觸發
//         if (!in_cooldown || !is_same_card) {
//             // send "i" + UID + '\n'
//             Serial3.print("i");
//             for (byte i = 0; i < idSize; i++) {
//                 if (id[i] < 0x10) Serial3.print("0");
//                 Serial3.print(id[i], HEX);
//             }
//             Serial3.println();

// #ifdef DEBUG
//             Serial.print("RFID detected, sent: i");
//             for (byte i = 0; i < idSize; i++) {
//                 if (id[i] < 0x10) Serial.print("0");
//                 Serial.print(id[i], HEX);
//             }
//             Serial.println();
// #endif

//             // like node event: execute + shift queue
//             execute_command(_cmd_queue[0]);
//             shift_queue_after_execute();

//             // update dedup states
//             last_rfid_ms = now;
//             last_uid_size = idSize;
//             for (byte i = 0; i < idSize; i++) last_uid[i] = id[i];

//             prev_node_state = current_node_state;
//             return;
//         }
//     }
    
    // 2. Check if we just enter a node
    if (current_node_state && !prev_node_state) {
        
        // Notify Python about the node and request next command
        Serial3.println("n");
        
        // Execute the current command from the queue
        execute_command(_cmd_queue[0]);
        shift_queue_after_execute();
    }

    tracking(irState);
    
    // Update previous node state
    prev_node_state = current_node_state;
}

bool receive_command() {
    BT_CMD cmd = ask_BT();
    if (cmd == NOTHING) return false;

    // 1) HALT 最高優先
    if (cmd == HALT) {
        state = false;
        waiting_start_cmds = false;
        start_cmd_count = 0;
        initial_commands_received = false;

        motion_busy = false;
        motion_type = MOTION_NONE;
        motion_stage = STAGE_IDLE;

        _cmd_queue[0] = NOTHING;
        _cmd_queue[1] = NOTHING;
        _cmd_queue[2] = NOTHING;

        MotorWriting(0, 0);
        return true;   // 不回傳任何額外字串，避免污染
    }

    // 2) START：進入收前三筆
    if (cmd == START) {
        state = true;
        waiting_start_cmds = true;
        start_cmd_count = 0;
        initial_commands_received = false;
        prev_node_state = false;

        _cmd_queue[0] = NOTHING;
        _cmd_queue[1] = NOTHING;
        _cmd_queue[2] = NOTHING;

        Serial3.println("s");   // 協議 ack
        return true;
    }

    // 3) START 階段：收滿三筆
    if (waiting_start_cmds) {
        if (!enqueue_command(cmd)) {
            Serial3.println("q");
            return true;
        }

        start_cmd_count++;
        if (start_cmd_count >= 3) {
            waiting_start_cmds = false;
            initial_commands_received = true;
            Serial3.println("g");  // ready
        }
        return true;
    }

    // 4) 一般運行：塞 queue
    if (!enqueue_command(cmd)) {
        Serial3.println("q");
    }
    return true;
}



bool same_uid(byte* a, byte sizeA, byte* b, byte sizeB) {
    if (sizeA != sizeB) return false;
    for (byte i = 0; i < sizeA; i++) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

void shift_queue_after_execute() {
    _cmd_queue[0] = _cmd_queue[1];
    _cmd_queue[1] = _cmd_queue[2];
    _cmd_queue[2] = NOTHING;
}

bool enqueue_command(BT_CMD cmd) {
    // 依序補空位，不是只看 _cmd_queue[2]
    for (byte i = 0; i < 3; i++) {
        if (_cmd_queue[i] == NOTHING) {
            _cmd_queue[i] = cmd;
            return true;
        }
    }
    return false; // full
}


// Execute a command based on its type
void execute_command(BT_CMD cmd) {
#ifdef DEBUG
    Serial.print("Executing command: ");
    Serial.println(int(cmd));
#endif
    start_motion(cmd);   // 不再直接呼叫 left_turn()/right_turn()...
}


void sendATCommand(const char* command) {
  Serial3.print(command);
  waitForResponse("", 1000); 
}

/**
 * Helper to check response for specific substrings
 */
bool waitForResponse(const char* expected, unsigned long timeout) {
  unsigned long start = millis();
  Serial3.setTimeout(timeout);
  String response = Serial3.readString();
  if (response.length() > 0) {
    Serial.print("HM10 Response: ");
    Serial.println(response);
  }
  return (response.indexOf(expected) != -1);
}
/*===========================define function===========================*/
