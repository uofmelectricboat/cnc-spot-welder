#include <AccelStepper.h>
#include "Axis.h"

const int eStopPin = 2;
const int xStepPin = 3;
const int yStepPin = 5;
const int zStepPin = 6;
const int xDirPin = 4;
const int yDirPin = 7;
const int zDirPin = 8;
const int xEnablePin = 9;
const int yEnablePin = 10;
const int zEnablePin = 11;
const int xHomePin = A0;
const int yHomePin = A1;
const int zHomePin = A2;

#define WELD_TIME 200

bool stopped = false;
unsigned long lastPrint = 0;

HorizontalAxis x(xEnablePin, xDirPin, xStepPin, false);
HorizontalAxis y(yEnablePin, yDirPin, yStepPin, true);
ZAxis z(zEnablePin, zDirPin, zStepPin, true);

void eStop() {
  x.eStop();
  y.eStop();
  z.eStop();
  stopped = true;
  Serial.println("ESTOP");
}

void setup() {
  Serial.begin(9600);

  // Configure X Axis Settings
  x.setMaxSpeed(1000);
  x.setAcceleration(5000);
  pinMode(xHomePin, INPUT_PULLUP);
  // x.attachHome(xHomePin);
  x.setStepover(1016 / sqrt(2)); // 1016 = 1/sqrt(2) in

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
  x.resetEStop();
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
    y.stepoverBlockingCustom(80);
    y.stepoverBlockingCustom(y.getStepover() * 23, true);
    delay(1000);
  }
  Serial.println("finished");
}

// Run a full pack of 16 serieses
void runPack(int passes = 1, bool startClose = false) {
  bool close = startClose;
  for (int i = 0; i < 16; i++) {
    runSeries(passes);
    if (close)
      y.stepoverHalfBlocking();
    else
      y.stepoverHalfBlocking(true);
    close = !close;
    x.stepoverBlocking();
    delay(3000);
  }
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
  else if (cmd == "runSeries18650") {
    int cells = cmd2.toInt();
    runSeries18650(cells, 2);
  }
  else if (cmd == "runPack") {
    Serial.println("# runPack");
    runPack(2);
  }
  else if (cmd == "xSetStepSize") {
    float stepover = cmd2.toFloat();
    x.setStepover(stepover);
  }
  else if (cmd == "ySetStepSize") {
    float stepover = cmd2.toFloat();
    y.setStepover(stepover);
  }
  else if (cmd == "zSetStepSize") {
    float stepdown = cmd2.toFloat();
    z.setStepdown(stepdown);
  }
  else if (cmd == "xStepover") {
    x.stepover();
  }
  else if (cmd == "xStepback") {
    x.stepover(true);
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
  else if (cmd == "xHome") {
    x.home();
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
  else if (cmd == "xMove") {
    float distance = cmd2.toFloat();
    Serial.print("# xMove ");
    Serial.println(distance);
    x.move(distance);
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
  else if (cmd == "xMoveTo") {
    float position = cmd2.toFloat();
    x.moveTo(position);
  }
  else if (cmd == "yMoveTo") {
    float position = cmd2.toFloat();
    y.moveTo(position);
  }
  else if (cmd == "zMoveTo") {
    float position = cmd2.toFloat();
    z.moveTo(position);
  }
  else if (cmd == "xRunMs") {
    float targTime = millis() + cmd2.toFloat();
    while (millis() < targTime) {
      x.run();
    }
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
  else if (cmd == "xStop") {
    x.stop();
  }
  else if (cmd == "yStop") {
    y.stop();
  }
  else if (cmd == "zStop") {
    z.stop();
  }
  else if (cmd == "eStop") {
    x.eStop();
    y.eStop();
    z.eStop();
  }
  else if (cmd == "resetEStop") {
    x.resetEStop();
    y.resetEStop();
    z.resetEStop();
  }
  else if (cmd == "xIsRunning") {
    Serial.println(x.isRunning());
  }
  else if (cmd == "yIsRunning") {
    Serial.println(y.isRunning());
  }
  else if (cmd == "zIsRunning") {
    Serial.println(z.isRunning());
  }
  else if (cmd == "xGetPosition") {
    Serial.println(x.getPosition());
  }
  else if (cmd == "yGetPosition") {
    Serial.println(y.getPosition());
  }
  else if (cmd == "zGetPosition") {
    Serial.println(z.getPosition());
  }
  else if (cmd == "xGetTargetPosition") {
    Serial.println(x.getTargetPosition());
  }
  else if (cmd == "yGetTargetPosition") {
    Serial.println(y.getTargetPosition());
  }
  else if (cmd == "zGetTargetPosition") {
    Serial.println(z.getTargetPosition());
  }
  else if (cmd == "xGetDistanceToGo") {
    Serial.println(x.getDistanceToGo());
  }
  else if (cmd == "yGetDistanceToGo") {
    Serial.println(y.getDistanceToGo());
  }
  else if (cmd == "zGetDistanceToGo") {
    Serial.println(z.getDistanceToGo());
  }
  else if (cmd == "xSetMaxSpeed") {
    float speed = cmd2.toFloat();
    x.setMaxSpeed(speed);
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
  x.run();
  y.run();
  z.run();
  if (millis() > lastPrint + 1000) {
    if (x.getDistanceToGo() == 0 && y.getDistanceToGo() == 0 && z.getDistanceToGo() == 0)
      Serial.println("idle");
    else
      Serial.println("moving");
    lastPrint = millis();
  }
}
