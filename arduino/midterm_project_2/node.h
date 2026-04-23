// node.h (non-blocking version)

// Define constants
const int ir_threshold = 150;   // Threshold for IR sensors
const int turn_delay = 300;     // Delay before turning (ms)
const int turn_speed = 150;
const int fwd_speed = 255;

// ---- command enum comes from bluetooth.h ----
// FRONT, LEFT, RIGHT, BACK

// Helper functions
bool IRblank(int* arr) {
  for (int i = 0; i < 5; i++) {
    if (arr[i] > ir_threshold) return false;
  }
  return true;
}

bool IRblack(int* arr) {
  for (int i = 0; i < 5; i++) {
    if (arr[i] < ir_threshold) return false;
  }
  return true;
}

// Node detection
bool is_node() {
  IRupdate(irState);
  return IRblack(irState);
}

// ================= Non-blocking motion FSM =================
enum MotionType {
  MOTION_NONE,
  MOTION_FRONT,
  MOTION_LEFT,
  MOTION_RIGHT,
  MOTION_BACK
};

enum MotionStage {
  STAGE_IDLE,

  // FRONT
  STAGE_FRONT_EXIT_NODE,

  // LEFT
  STAGE_LEFT_FORWARD_DELAY,
  STAGE_LEFT_WAIT_ALL_WHITE,
  STAGE_LEFT_WAIT_BACK_TO_LINE,

  // RIGHT
  STAGE_RIGHT_FORWARD_DELAY,
  STAGE_RIGHT_WAIT_ALL_WHITE,
  STAGE_RIGHT_WAIT_BACK_TO_LINE,

  // BACK
  STAGE_BACK_WAIT_ALL_WHITE,
  STAGE_BACK_WAIT_BACK_TO_LINE
};

MotionType motion_type = MOTION_NONE;
MotionStage motion_stage = STAGE_IDLE;
bool motion_busy = false;
unsigned long motion_stage_start_ms = 0;

// call when you want to start one turning/forward action
void start_motion(BT_CMD cmd) {
  if (motion_busy) return;  // already running

  IRupdate(irState);

  switch (cmd) {
    case FRONT:
      motion_type = MOTION_FRONT;
      motion_stage = STAGE_FRONT_EXIT_NODE;
      motion_busy = true;
      break;

    case LEFT:
      motion_type = MOTION_LEFT;
      motion_stage = STAGE_LEFT_FORWARD_DELAY;
      motion_stage_start_ms = millis();
      motion_busy = true;
      break;

    case RIGHT:
      motion_type = MOTION_RIGHT;
      motion_stage = STAGE_RIGHT_FORWARD_DELAY;
      motion_stage_start_ms = millis();
      motion_busy = true;
      break;

    case BACK:
      motion_type = MOTION_BACK;
      motion_stage = STAGE_BACK_WAIT_ALL_WHITE;
      motion_busy = true;
      break;

    default:
      // NOTHING / START / HALT => no motion
      motion_type = MOTION_NONE;
      motion_stage = STAGE_IDLE;
      motion_busy = false;
      break;
  }
}

// call in every loop while motion_busy == true
void update_motion() {
  if (!motion_busy) return;

  IRupdate(irState);

  switch (motion_stage) {
    // ---------- FRONT ----------
    case STAGE_FRONT_EXIT_NODE:
      MotorWriting(fwd_speed, fwd_speed);
      if (!IRblack(irState)) {
        motion_busy = false;
        motion_type = MOTION_NONE;
        motion_stage = STAGE_IDLE;
      }
      break;

    // ---------- LEFT ----------
    case STAGE_LEFT_FORWARD_DELAY:
      MotorWriting(fwd_speed, fwd_speed);
      if (millis() - motion_stage_start_ms >= (unsigned long)turn_delay) {
        motion_stage = STAGE_LEFT_WAIT_ALL_WHITE;
      }
      break;

    case STAGE_LEFT_WAIT_ALL_WHITE:
      MotorWriting(0, turn_speed);  // pivot left
      if (IRblank(irState)) {
        motion_stage = STAGE_LEFT_WAIT_BACK_TO_LINE;
      }
      break;

    case STAGE_LEFT_WAIT_BACK_TO_LINE:
      MotorWriting(-turn_speed, turn_speed);
      if (!IRblank(irState)) {
        motion_busy = false;
        motion_type = MOTION_NONE;
        motion_stage = STAGE_IDLE;
      }
      break;

    // ---------- RIGHT ----------
    case STAGE_RIGHT_FORWARD_DELAY:
      MotorWriting(fwd_speed, fwd_speed);
      if (millis() - motion_stage_start_ms >= (unsigned long)turn_delay) {
        motion_stage = STAGE_RIGHT_WAIT_ALL_WHITE;
      }
      break;

    case STAGE_RIGHT_WAIT_ALL_WHITE:
      MotorWriting(turn_speed, 0);  // pivot right
      if (IRblank(irState)) {
        motion_stage = STAGE_RIGHT_WAIT_BACK_TO_LINE;
      }
      break;

    case STAGE_RIGHT_WAIT_BACK_TO_LINE:
      MotorWriting(turn_speed, -turn_speed);
      if (!IRblank(irState)) {
        motion_busy = false;
        motion_type = MOTION_NONE;
        motion_stage = STAGE_IDLE;
      }
      break;

    // ---------- BACK ----------
    case STAGE_BACK_WAIT_ALL_WHITE:
      MotorWriting(-150, 130);
      if (IRblank(irState)) {
        motion_stage = STAGE_BACK_WAIT_BACK_TO_LINE;
      }
      break;

    case STAGE_BACK_WAIT_BACK_TO_LINE:
      MotorWriting(-150, 130);
      if (!IRblank(irState)) {
        motion_busy = false;
        motion_type = MOTION_NONE;
        motion_stage = STAGE_IDLE;
      }
      break;

    default:
      motion_busy = false;
      motion_type = MOTION_NONE;
      motion_stage = STAGE_IDLE;
      break;
  }
}
