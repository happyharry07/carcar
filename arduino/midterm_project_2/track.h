/*===========================import variable===========================*/
// External references
extern const int MotorR_I1, MotorR_I2, MotorR_PWMR;
extern const int MotorL_I3, MotorL_I4, MotorL_PWML;
extern const int IRpin_LL, IRpin_L, IRpin_M, IRpin_R, IRpin_RR;

// Define motor pins
#define AIN1 MotorL_I3
#define AIN2 MotorL_I4
#define PWMA MotorL_PWML
#define BIN1 MotorR_I1
#define BIN2 MotorR_I2
#define PWMB MotorR_PWMR
/*===========================import variable===========================*/
//
int ir_thre = 150; 
int irState[5];
// PID state variables
double lastError = 0;
double sumError = 0;
int num_of_sum = 0;
int straight_count = 0;

// PID gain parameters
double Tp = 200.0;
double Kp = 60.0;
double Ki = 40.0;
double Kd = 20.0;

void IRupdate(int* arr) {
    arr[0] = analogRead(IRpin_LL);
    arr[1] = analogRead(IRpin_L);
    arr[2] = analogRead(IRpin_M);
    arr[3] = analogRead(IRpin_R);
    arr[4] = analogRead(IRpin_RR);
}

double IRvalue(int* arr) {
    bool s0 = arr[0] > ir_thre;
    bool s1 = arr[1] > ir_thre;
    bool s2 = arr[2] > ir_thre;
    bool s3 = arr[3] > ir_thre;
    bool s4 = arr[4] > ir_thre;

    if (s0 && !s1 && !s2 && !s3 && !s4) return -1.0;   // 10000
    if (s0 && s1 && !s2 && !s3 && !s4) return -0.8;    // 11000
    if (!s0 && s1 && !s2 && !s3 && !s4) return -0.4;   // 01000
    if (!s0 && s1 && s2 && !s3 && !s4) return -0.2;    // 01100
    if (!s0 && !s1 && s2 && !s3 && !s4) return 0.0;    // 00100
    if (!s0 && !s1 && s2 && s3 && !s4) return 0.2;     // 00110
    if (!s0 && !s1 && !s2 && s3 && !s4) return 0.4;    // 00010
    if (!s0 && !s1 && !s2 && s3 && s4) return 0.8;     // 00011
    if (!s0 && !s1 && !s2 && !s3 && s4) return 1.0;    // 00001
    
    return 0.0; // Default value to prevent returning garbage when no conditions match
}
// Write the voltage to motor.
void MotorWriting(double vL, double vR) {
    // TODO: use TB6612 to control motor voltage & direction
    if (vR >= 0) {
      digitalWrite(BIN1, LOW);
      digitalWrite(BIN2, HIGH);
    }
    else if (vR < 0) {
      digitalWrite(BIN1, HIGH);
      digitalWrite(BIN2, LOW);
      vR = -vR;
    }
    if (vL >= 0) {
      digitalWrite(AIN1, LOW);
      digitalWrite(AIN2, HIGH);
    }
    else if (vL < 0) {
      digitalWrite(AIN1, HIGH);
      digitalWrite(AIN2, LOW);
      vL = -vL;
    }
    analogWrite(PWMA, vL);
    analogWrite(PWMB, vR);
}

// P control Tracking (Proportional control only)
void trackingP(int* arr) {
    IRupdate(irState);
    int vL, vR;
    
    // Get current error value
    double error = IRvalue(irState);
    
    // Calculate power correction using only proportional term
    int powerCorrection = (double) Kp * error;
    
    // Apply correction to motor speeds
    vR = Tp - powerCorrection;       
    vL = Tp + powerCorrection;
    
    // Limit values to valid range
    if(vR > 255) vR = 255;
    if(vL > 255) vL = 255;
    if(vR < -255) vR = -255;
    if(vL < -255) vL = -255;
    
    // Write values to motors
    MotorWriting(vL, vR);
}

// PID control Tracking
void tracking(int* arr) {
    IRupdate(irState);
    // TODO: find your own parameters!
    double error = IRvalue(irState);
    double dError = error - lastError;
    double powerCorrection;
    
    // Calculate motor speed corrections
    // When error is positive (right deviation), left wheel should speed up or right wheel slow down to correct left
    double vL;
    double vR;

    // TODO: complete your P/PID tracking code
    if (error == 0) {
        if (straight_count >= 5) {
          sumError = 0;
          num_of_sum = 0;
        }
        else straight_count++;
      }
      else {
        straight_count = 0;
      }
      sumError = (sumError * num_of_sum + error) / (num_of_sum + 1);
      powerCorrection = Kp*error + Kd*dError + Ki*sumError;
      vR = Tp - powerCorrection;       
      vL = Tp + powerCorrection;
      if(vR>255) vR = 255;
      if(vL>255) vL = 255;
      if(vR<-255) vR = -255;
      if(vL<-255) vL = -255;
      lastError = error;
      ++num_of_sum;
    // }
    MotorWriting(vL, vR);
    // end TODO
}  // tracking