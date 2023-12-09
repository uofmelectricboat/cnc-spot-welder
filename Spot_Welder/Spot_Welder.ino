#include <AccelStepper.h>
#include "Axis.h"

const int eStopPin = 2;
const int xStepPin = 3;
const int yStepPin = 4;
const int zStepPin = 5;
const int xDirPin = 6;
const int yDirPin = 7;
const int zDirPin = 8;
const int xEnablePin = 9;
const int yEnablePin = 10;
const int zEnablePin = 11;
const int xHomePin = A0;
const int yHomePin = A1;
const int zHomePin = A2;

bool stopped = false;

HorizontalAxis x(xStepPin, xDirPin, xEnablePin, false);
HorizontalAxis y(yStepPin, yDirPin, yEnablePin, false);
ZAxis z(zStepPin, zDirPin, zEnablePin, false);

void setup() {
  // Configure X Axis Settings
  x.setMaxSpeed(1000);
  x.setAcceleration(50000);
  pinMode(xHomePin, INPUT_PULLUP);
  x.attachHome(xHomePin);
  x.setStepover(50);

  // Configure Y Axis Settings
  y.setMaxSpeed(1000);
  y.setAcceleration(50000);
  pinMode(yHomePin, INPUT_PULLUP);
  y.attachHome(yHomePin);
  y.setStepover(50);

  // Configure Z Axis Settings
  z.setMaxSpeed(1000);
  z.setAcceleration(50000);
  pinMode(zHomePin, INPUT_PULLUP);
  z.attachHome(zHomePin);
  z.setStepdown(20);

  pinMode(eStopPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(eStopPin), []() {
    y.eStop(); z.eStop(); stopped = true;
  }, FALLING);

  Serial.begin(9600);
}

// Run the script to weld a series of 24 cells
void runSeries(int passes = 1) {
  stopped = false;
  for (int i = 0; i < passes && !stopped; i++) {
    z.stepdownCycle(1000);
    for (int j = 0; j < 23 && !stopped; i++) {
      y.stepoverBlocking();
      delay(100);
      z.stepdownCycle(500);
      delay(100);
      while (Serial.available()) {
        String cmd = Serial.readString();
        if (cmd == "stop") {
          stopped = true;
          break;
        }
        while (cmd == "pause") {
          while (!Serial.available()) {
            delay(100);
          }
          cmd = Serial.readString();
          if (cmd == "stop") {
            stopped = true;
            break;
          }
        }
      }
    }
    y.stepoverBlockingCustom(10);
    delay(1000);
  }
  Serial.println("finished");
}

void parseCommand(String cmd) {
  if (cmd == "runSeries") {
    runSeries(2);
  }
  else if (cmd == "ySetStepSize") {
    float stepover = Serial.parseFloat();
    y.setStepover(stepover);
  }
  else if (cmd == "zSetStepSize") {
    float stepdown = Serial.parseFloat();
    z.setStepdown(stepdown);
  }
  else if (cmd == "yStepover") {
    y.stepover();
  }
  else if (cmd == "yStepback") {
    y.stepover(true);
  }
  else if (cmd == "zStepdown") {
    z.stepdown();
  }
  else if (cmd == "zStepup") {
    z.stepup();
  }
  else if (cmd == "yHome") {
    y.home();
  }
  else if (cmd == "zHome") {
    z.home();
  }
  else if (cmd == "homeAll") {
    x.home();
    y.home();
    z.home();
  }
  else if (cmd == "yMove") {
    float distance = Serial.parseFloat();
    y.move(distance);
  }
  else if (cmd == "zMove") {
    float distance = Serial.parseFloat();
    z.move(distance);
  }
  else if (cmd == "yMoveTo") {
    float position = Serial.parseFloat();
    y.moveTo(position);
  }
  else if (cmd == "zMoveTo") {
    float position = Serial.parseFloat();
    z.moveTo(position);
  }
  else if (cmd == "yRunMs") {
    float targTime = millis() + Serial.parseFloat();
    while (millis() < targTime) {
      y.run();
    }
  }
  else if (cmd == "zRunMs") {
    float targTime = millis() + Serial.parseFloat();
    while (millis() < targTime) {
      z.run();
    }
  }
  else if (cmd == "yStop") {
    y.stop();
  }
  else if (cmd == "zStop") {
    z.stop();
  }
  else if (cmd == "yIsRunning") {
    Serial.println(y.isRunning());
  }
  else if (cmd == "zIsRunning") {
    Serial.println(z.isRunning());
  }
  else if (cmd == "yGetPosition") {
    Serial.println(y.getPosition());
  }
  else if (cmd == "zGetPosition") {
    Serial.println(z.getPosition());
  }
  else if (cmd == "yGetTargetPosition") {
    Serial.println(y.getTargetPosition());
  }
  else if (cmd == "zGetTargetPosition") {
    Serial.println(z.getTargetPosition());
  }
  else if (cmd == "yGetDistanceToGo") {
    Serial.println(y.getDistanceToGo());
  }
  else if (cmd == "zGetDistanceToGo") {
    Serial.println(z.getDistanceToGo());
  }
  else if (cmd == "ySetMaxSpeed") {
    float speed = Serial.parseFloat();
    y.setMaxSpeed(speed);
  }
  else if (cmd == "ySetMaxSpeed") {
    float speed = Serial.parseFloat();
    y.setMaxSpeed(speed);
  }
  else if (cmd == "zSetMaxSpeed") {
    float speed = Serial.parseFloat();
    z.setMaxSpeed(speed);
  }
  else if (cmd == "ping") {
    Serial.println("pong");
  }
  else {
    Serial.println("Unknown command");
  }
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readString();
    cmd.trim();
    parseCommand(cmd);
  }
  y.run();
  z.run();
}
