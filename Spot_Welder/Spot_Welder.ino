#include <AccelStepper.h>
#include "Axis.h"

const int eStopPin = 2;
const int yStepPin = 5;
const int zStepPin = 6;
const int yDirPin = 7;
const int zDirPin = 8;
const int yEnablePin = 10;
const int zEnablePin = 11;
const int yHomePin = A0;
const int zHomePin = A1;

#define WELD_TIME 200

bool stopped = false;
unsigned long lastPrint = 0;

HorizontalAxis y(yEnablePin, yDirPin, yStepPin, true);
ZAxis z(zEnablePin, zDirPin, zStepPin, true);

void eStop() {
  y.eStop();
  z.eStop();
  stopped = true;
  Serial.println("ESTOP");
}

void setup() {
  Serial.begin(9600);

  // Configure Y Axis Settings
  y.setMaxSpeed(1000);
  y.setAcceleration(5000);
  pinMode(yHomePin, INPUT_PULLUP);
  y.attachHome(yHomePin);
  y.setStepover(1016); // 1016 = 1 in

  // Configure Z Axis Settings
  z.setMaxSpeed(1000);
  z.setAcceleration(5000);
  pinMode(zHomePin, INPUT_PULLUP);
  z.attachHome(zHomePin);
  z.setStepdown(125); // 125 = 5mm

  pinMode(eStopPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(eStopPin), eStop, FALLING);
  delay(10);
  y.resetEStop();
  z.resetEStop();
}

// Run the script to weld a series of 24 cells
void runSeries(int passes = 1) {
  stopped = false;
  for (int i = 0; i < passes && !stopped; i++) {
    Serial.print("R");
    Serial.print(i); // Pass count
    Serial.print(" ");
    Serial.println(0); // Cell count
    z.stepdownCycle(WELD_TIME);
    for (int j = 0; j < 23 && !stopped; j++) {
      Serial.print("R");
      Serial.print(i); // Pass count
      Serial.print(" ");
      Serial.println(j + 1); // Cell count
      y.stepoverBlocking();
      delay(100);
      z.stepdownCycle(WELD_TIME);
      delay(100);
      while (Serial.available()) {
        String cmd = Serial.readString();
        cmd.trim();
        if (cmd == "stop") {
          stopped = true;
          break;
        }
        while (cmd == "pause") {
          while (!Serial.available()) {
            Serial.println("paused");
            delay(100);
          }
          cmd = Serial.readString();
          cmd.trim();
          if (cmd == "stop") {
            stopped = true;
            break;
          }
        }
      }
    }
    if (stopped)
      break;
    y.stepoverBlockingCustom(10);
    y.stepoverBlockingCustom(y.getStepover() * 23, true);
    delay(1000);
  }
  Serial.println("finished");
}


// Run the script to weld a series of N 18650 cells
void runSeries18650(int cells, int passes = 1) {
  y.setStepover(889);
  z.setStepdown(250);
  stopped = false;
  for (int i = 0; i < passes && !stopped; i++) {
    Serial.print("R");
    Serial.print(i); // Pass count
    Serial.print(" ");
    Serial.println(0); // Cell count
    z.stepdownCycle(WELD_TIME);
    for (int j = 0; j < cells && !stopped; j++) {
      Serial.print("R");
      Serial.print(i); // Pass count
      Serial.print(" ");
      Serial.println(j + 1); // Cell count
      y.stepoverBlocking(true);
      delay(100);
      z.stepdownCycle(WELD_TIME);
      delay(100);
      while (Serial.available()) {
        String cmd = Serial.readString();
        cmd.trim();
        if (cmd == "stop") {
          stopped = true;
          break;
        }
        while (cmd == "pause") {
          while (!Serial.available()) {
            Serial.println("paused");
            delay(100);
          }
          cmd = Serial.readString();
          cmd.trim();
          if (cmd == "stop") {
            stopped = true;
            break;
          }
        }
      }
    }
    if (stopped)
      break;
    y.stepoverBlockingCustom(10);
    y.stepoverBlockingCustom(y.getStepover() * 6, true);
    delay(1000);
    while (Serial.available()) {
      String cmd = Serial.readString();
      cmd.trim();
      if (cmd == "stop") {
        stopped = true;
        break;
      }
      while (cmd == "pause") {
        while (!Serial.available()) {
          Serial.println("paused");
          delay(100);
        }
        cmd = Serial.readString();
        cmd.trim();
        if (cmd == "stop") {
          stopped = true;
          break;
        }
      }
    }
  }
  Serial.println("finished");
}

void parseCommand(String cmd, String cmd2 = "") {
  if (cmd == "runSeries") {
    Serial.println("# runSeries");
    runSeries(2);
  }
  else if (cmd == "ySetStepSize") {
    float stepover = cmd2.toFloat();
    y.setStepover(stepover);
  }
  else if (cmd == "zSetStepSize") {
    float stepdown = cmd2.toFloat();
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
  else if (cmd == "zStepCycle") {
    z.stepdownCycle(300);
  }
  else if (cmd == "yHome") {
    y.home();
  }
  else if (cmd == "zHome") {
    z.home();
  }
  else if (cmd == "homeAll") {
    y.home();
    z.home();
  }
  else if (cmd == "yMove") {
    float distance = cmd2.toFloat();
    Serial.print("# yMove ");
    Serial.println(distance);
    y.move(distance);
  }
  else if (cmd == "zMove") {
    float distance = cmd2.toFloat();
    Serial.print("# zMove ");
    Serial.println(distance);
    z.move(distance);
  }
  else if (cmd == "yMoveTo") {
    float position = cmd2.toFloat();
    y.moveTo(position);
  }
  else if (cmd == "zMoveTo") {
    float position = cmd2.toFloat();
    z.moveTo(position);
  }
  else if (cmd == "yRunMs") {
    float targTime = millis() + cmd2.toFloat();
    while (millis() < targTime) {
      y.run();
    }
  }
  else if (cmd == "zRunMs") {
    float targTime = millis() + cmd2.toFloat();
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
  else if (cmd == "eStop") {
    y.eStop();
    z.eStop();
  }
  else if (cmd == "resetEStop") {
    y.resetEStop();
    z.resetEStop();
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
    float speed = cmd2.toFloat();
    y.setMaxSpeed(speed);
  }
  else if (cmd == "ySetMaxSpeed") {
    float speed = cmd2.toFloat();
    y.setMaxSpeed(speed);
  }
  else if (cmd == "zSetMaxSpeed") {
    float speed = cmd2.toFloat();
    z.setMaxSpeed(speed);
  }
  else if (cmd == "ping") {
    Serial.println("pong");
  }
  else {
    Serial.println("Unknown command " + cmd + ".");
  }
}

void loop() {
  if (Serial.available()) {
    String cmd = Serial.readString();
    cmd.trim();
    String cmd2;
    if (cmd.indexOf(" ") != -1) {
      cmd2 = cmd.substring(cmd.indexOf(" ") + 1);
      cmd = cmd.substring(0, cmd.indexOf(" "));
    }
    parseCommand(cmd, cmd2);
  }
  y.run();
  z.run();
  if (millis() > lastPrint + 1000) {
    if (y.getDistanceToGo() == 0 && z.getDistanceToGo() == 0)
      Serial.println("idle");
    else
      Serial.println("moving");
    lastPrint = millis();
  }
}
