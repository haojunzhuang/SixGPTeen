// adapted from turning lab

#define LEFT_MOTOR                  6
#define LEFT_ENCODER                3
#define RIGHT_MOTOR                 5
#define RIGHT_ENCODER               2

#define RXLED  17
#define TXLED  30 

#define SAMPLING_INTERVAL           100

#define MODE_LISTEN                 0
#define MODE_DRIVE                  1

#define NUM_COMMANDS                4
#define DRIVE_STRAIGHT              0
#define DRIVE_LEFT                  1
#define DRIVE_RIGHT                 2
#define STOP                        3

#define JOLT_STEPS                  2

boolean loop_mode = MODE_DRIVE;
int drive_mode;
int program_count;
int sample_lens[NUM_COMMANDS] = {0};

int step_num = 0;

volatile int left_count = 0;
volatile int right_count = 0;

/*---------------------------*/
/*      CODE BLOCK CON1      */
/*    From closed_loop.ino   */
/*---------------------------*/
float theta_left = 0.2682;
float theta_right = 0.2968;
float beta_left = -25.04;
float beta_right = -23.15;
float v_star = 63.9;

// PWM inputs to jolt the car straight
int left_jolt = 240;
int right_jolt = 210;

// Control gains
float f_left = 1;
float f_right = 1;
/*---------------------------*/
/*---------------------------*/
/*---------------------------*/

/*---------------------------*/
/*      CODE BLOCK CON2      */
/*    From closed_loop.ino   */
/*---------------------------*/
float driveStraight_left(float delta) {
  return 1/theta_left*(v_star-f_left*delta+beta_left);
}

float driveStraight_right(float delta) {
  return 1/theta_right*(v_star+f_right*delta+beta_right);
}
/*---------------------------*/
/*---------------------------*/
/*---------------------------*/

/*---------------------------*/
/*      CODE BLOCK CON3      */
/*    From closed_loop.ino   */
/*---------------------------*/
float delta_ss = 6;
/*---------------------------*/
/*---------------------------*/
/*---------------------------*/

/*---------------------------*/
/*      CODE BLOCK CON4      */
/*---------------------------*/
#define CAR_WIDTH                   15.0 // in cm
#define TURN_RADIUS                 91 // in cm - 6 feet diameter
// #define TURN_RADIUS                 60 // in cm - 4 feet diameter
/*---------------------------*/
/*---------------------------*/
/*---------------------------*/

/*---------------------------*/
/*    PREPROGRAMMED PATH     */
/*---------------------------*/
// int run_times[NUM_COMMANDS] = {7000, 5000, 2500, 5000}; // length of commands roughly in ms
// int drive_modes[NUM_COMMANDS] = {DRIVE_STRAIGHT, DRIVE_LEFT, DRIVE_STRAIGHT, DRIVE_RIGHT}; // commands: [DRIVE_STRAIGHT, DRIVE_LEFT, DRIVE_RIGHT]
/*---------------------------*/
/*    NOT PREPROGRAMMED PATH     */
/*---------------------------*/
int run_times[4];   
int drive_modes[4];


/*---------------------------*/
/*      CODE BLOCK CON5      */
/*---------------------------*/
float delta_reference(int i) {
  // YOUR CODE HERE
  float r = TURN_RADIUS;
  float l = CAR_WIDTH;
  // Remember to divide the v* value you use in this function by 5 because of sampling interval differences!
  if (drive_mode == DRIVE_RIGHT) { // Return a NEGATIVE expression
    return -v_star*i*l/(5*r);
  }
  else if (drive_mode == DRIVE_LEFT) { // Return a POSITIVE expression
    return v_star*i*l/(5*r);
  }
  else { // DRIVE_STRAIGHT
    return 0;
  }
}
/*---------------------------*/
/*---------------------------*/
/*---------------------------*/


/*---------------------------*/
/*      CODE BLOCK CON6      */
/*---------------------------*/
#define INFINITY                    (3.4e+38)
#define STRAIGHT_RADIUS             INFINITY

float straight_correction(int i) {
  // YOUR CODE HERE
  return 0; // Replace this line
}

/*---------------------------*/
/*---------------------------*/
/*---------------------------*/


void setup() {
  Serial1.begin(9600); // Serial1 is the Bluetooth module
  Serial.begin(9600); // Serial is for serial monitor debugging
  delay(2000);
  Serial.println("Test");
  Serial1.print("AA");

  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(TXLED, LOW);
  digitalWrite(RXLED, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(TXLED, HIGH);
  delay(1000);
  digitalWrite(RXLED, HIGH);
  delay(1000);

  // RECEIVE COMMANDS
  int count = 0;      

  // Wait to receive enough data
  while (Serial1.available() < 8) {
    Serial.print("Nothing");
    delay(1000);
  }

  // Read 8 bytes and store them in the respective arrays
  while (Serial1.available() > 0 && count < 8) {
    byte inByte = Serial1.read();  // Read a byte from Serial1

    if (count < 4) {
      // First four bytes go into drive_modes
      drive_modes[count] = inByte;
      Serial.print("Drive Mode [");
      Serial.print(count);
      Serial.print("]: ");
      Serial.println(drive_modes[count]);
    } else {
      // Next four bytes go into run_times
      run_times[count - 4] = inByte;
      run_times[count - 4] = run_times[count - 4] * 10; // adjustion
      Serial.print("Run Time [");
      Serial.print(count - 4);
      Serial.print("]: ");
      Serial.println(run_times[count - 4]);
    }
    
    count++; 
  }


  pinMode(LEFT_MOTOR, OUTPUT);
  pinMode(LEFT_ENCODER, INPUT);
  pinMode(RIGHT_MOTOR, OUTPUT);
  pinMode(RIGHT_ENCODER, INPUT);
  
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RXLED, OUTPUT);
  pinMode(TXLED, OUTPUT);
  delay(500);

  write_pwm(0, 0);
  digitalWrite(LED_BUILTIN, HIGH);
  digitalWrite(TXLED, LOW);
  digitalWrite(RXLED, LOW);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
  digitalWrite(TXLED, HIGH);
  delay(1000);
  digitalWrite(RXLED, HIGH);
  delay(1000);
  for (int j = 0; j < NUM_COMMANDS; j++) {
    sample_lens[j] = run_times[j] / SAMPLING_INTERVAL;
  }

  attachInterrupt(digitalPinToInterrupt(RIGHT_ENCODER), flag_right, CHANGE);
  attachInterrupt(digitalPinToInterrupt(LEFT_ENCODER), flag_left, CHANGE);

  program_count = 0;
  start_drive_mode();
}

void loop() {
  if (loop_mode == MODE_LISTEN) {
    // In the integration phase of the project, this section will listen
    // to the microphone and switch to the specified mode.
    // For now, we simply cycle through them.

    
    if (program_count < 4) {
      drive_mode = drive_modes[program_count];
      start_drive_mode();
    } else {
      digitalWrite(RXLED, HIGH);
      digitalWrite(TXLED, HIGH);
    }
  } else if (loop_mode == MODE_DRIVE) {
    if (step_num < JOLT_STEPS) {
      write_pwm(left_jolt, right_jolt);
    } else {
      // Save positions because _left_position and _right_position
      // can change in the middle of one loop.
      int left_position = left_count;
      int right_position = right_count;

       /*---------------------------*/
      /*      CODE BLOCK CON0      */
      /*---------------------------*/

      float delta = left_position - right_position + delta_ss;
      delta = delta + delta_reference(step_num) + straight_correction(step_num);

      // Drive using feedback
      // Compute the needed pwm values for each wheel using delta and v_star
      int left_cur_pwm = driveStraight_left(delta);
      int right_cur_pwm = driveStraight_right(delta);
      write_pwm(left_cur_pwm, right_cur_pwm);
      /*---------------------------*/
      /*---------------------------*/
      /*---------------------------*/
      
    }

    // Counter for how many times loop is executed since entering DRIVE MODE
    step_num++;
    digitalWrite(RXLED, (!(((drive_mode == DRIVE_STRAIGHT) || (drive_mode == DRIVE_RIGHT)) && ((step_num / 5) % 2))));
    digitalWrite(TXLED, (!(((drive_mode == DRIVE_STRAIGHT) || (drive_mode == DRIVE_LEFT)) && ((step_num / 5) % 2))));

    if (step_num == sample_lens[program_count]) {
      // Completely stop and go back to listen MODE after 3 seconds
      program_count += 1;
      digitalWrite(RXLED, HIGH);
      digitalWrite(TXLED, LOW);
      start_listen_mode();
    }
    delay(SAMPLING_INTERVAL);
  }
}

/*---------------------------*/
/*     Helper functions      */
/*---------------------------*/

void write_pwm(int pwm_left, int pwm_right) {
  analogWrite(LEFT_MOTOR, (int) min(max(0, pwm_left), 255));
  analogWrite(RIGHT_MOTOR, (int) min(max(0, pwm_right), 255));
}

void flag_left() {
  if(digitalRead(LEFT_ENCODER)) {
    left_count++;
  }
}

void flag_right() {
  if(digitalRead(RIGHT_ENCODER)) {
    right_count++;
  }
}

void start_drive_mode(void) {
  loop_mode = MODE_DRIVE;
  step_num = 0;
  left_count = 0;
  right_count = 0;
}

void start_listen_mode(void) {
  write_pwm(0, 0);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(2000);
  digitalWrite(LED_BUILTIN, LOW);
  loop_mode = MODE_LISTEN;
}
