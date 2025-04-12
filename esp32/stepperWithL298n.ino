#include <math.h>
const int IN1 = 4;
const int IN2 = 16;
const int IN3 = 17;
const int IN4 = 15;

// Half-step sequence (8 steps per cycle)
const int stepSequence[8][4] = {
  {1, 0, 0, 0},
  {1, 1, 0, 0},
  {0, 1, 0, 0},
  {0, 1, 1, 0},
  {0, 0, 1, 0},
  {0, 0, 1, 1},
  {0, 0, 0, 1},
  {1, 0, 0, 1}
};

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  
  //runStepper(2400);  // 200 steps/rev × 2 (half-step) × 6 revs = 2400 steps
}

void loop() {
  rotation(10);
  delay(1000);
}
void rotation(int turns){
int steps = turns * 200;
runStepper(steps);
}

void runStepper(int totalSteps) {
  for (int i = 0; i < totalSteps; i++) {
    int stepIndex = i % 8;
    digitalWrite(IN1, stepSequence[stepIndex][0]);
    digitalWrite(IN2, stepSequence[stepIndex][1]);
    digitalWrite(IN3, stepSequence[stepIndex][2]);
    digitalWrite(IN4, stepSequence[stepIndex][3]);
    delayMicroseconds(1000);  // = 1 millisecond → fast and smooth
  }

  // Turn off coils after move
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}
