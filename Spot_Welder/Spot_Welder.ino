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

enum PackType {
  PT_A,
  PT_B
};

bool stopped = false;
PackType packType = PT_A;
unsigned long lastPrint = 0;

HorizontalAxis x(xEnablePin, xDirPin, xStepPin, false);
HorizontalAxis y(yEnablePin, yDirPin, yStepPin, true, 2.0f);
ZAxis z(zEnablePin, zDirPin, zStepPin, true);

const int X_ZERO = 620;
const int Y_ZERO = 1940;
const int Z_ZERO = 825;
const float X_STEPOVER = 25*25.4/2*sqrt(3);
const float Y_STEPOVER = 1016;
const float Z_STEPDOWN = 200;
const float WELD_SPACE = 1016.0 * 0.08;

int row = 0, cell = 0, side = 0;
uint8_t welded[16][24] = {0};

void moveToCell(int mRow, int mCell, int mSide, PackType packType, bool retract = true);

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
  x.attachHome(xHomePin);
  x.setStepover(X_STEPOVER); // 550 = sqrt(3)/2 in

  // Configure Y Axis Settings
  y.setMaxSpeed(2000);
  y.setAcceleration(5000);
  pinMode(yHomePin, INPUT_PULLUP);
  y.attachHome(yHomePin);
  y.setStepover(Y_STEPOVER); // 1016 = 1 in

  // Configure Z Axis Settings
  z.setMaxSpeed(1000);
  z.setAcceleration(5000);
  pinMode(zHomePin, INPUT_PULLUP);
  z.attachHome(zHomePin);
  z.setStepdown(Z_STEPDOWN); // 200 = 6.25mm

  pinMode(eStopPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(eStopPin), eStop, RISING);
  delay(10);
  x.resetEStop();
  y.resetEStop();
  z.resetEStop();
}

// Weld the current cell
void zWeld() {
  Serial.print("R");
  Serial.print(row); // Row count
  Serial.print(" ");
  Serial.print(side); // Pass count
  Serial.print(" ");
  Serial.println(cell); // Cell count
  z.stepdownCycle(WELD_TIME);
  welded[row][cell] |= (1 << side);
}

// Run the script to weld a series of 24 cells
// Returns true if success, false if stopped early
bool runSeries(int passes = 2, int cells = 24, bool manual = false) {
  stopped = false;
  for (int i = 0; i < passes && !stopped; i++) {
    side = i;
    for (int j = 0; j < cells && !stopped; j++) {
      cell = j;
      if (welded[row][cell] & (1 << side))
        continue;
      moveToCell(row, cell, side, packType, false);
      delay(100);
      zWeld();
      delay(100);
      while (Serial.available() || manual) {
        String cmd = Serial.readString();
        cmd.trim();
        Serial.println("# " + cmd);
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
        if (manual && cmd == "next")
          break;
        delay(20);
      }
    }
    if (stopped)
      break;
    y.stepoverBlockingCustom(80, false);
    y.stepoverBlockingCustom(y.getStepover() * (cells - 1), true);
    delay(1000);
  }
  return !stopped;
}

// Run a full pack of 16 serieses
void runPack(int passes = 2, PackType packType = PT_A, int seriesCells = 24) {
  bool close = !packType;
  for (int i = 0; i < 16; i++) {
    row = i;
    if (!runSeries(passes, seriesCells, false))
      return;
    y.stepoverBlockingCustom(80*passes, true);
    y.stepoverHalfBlocking(!close);
    close = !close;
    x.stepoverBlocking();
    delay(3000);
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
      Serial.println(cmd);
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

// Align the welder to the first cell
void align(PackType packType) {
  z.moveTo(0);
  while (z.isRunning()) {
    z.run();
  }
  if (packType == PT_A) {
    x.moveTo(X_ZERO);
    y.moveTo(Y_ZERO);
  }
  else if (packType == PT_B) {
    x.moveTo(X_ZERO);
    y.moveTo(Y_ZERO + (y.getStepover()/2));
  }
  while (x.isRunning() || y.isRunning()) {
    x.run();
    y.run();
  }
  z.moveTo(Z_ZERO);
  while (z.isRunning()) {
    z.run();
  }
}

// Move the welder to a specific cell
void moveToCell(int mRow, int mCell, int mSide, PackType packType, bool retract = true) {
  row = mRow;
  cell = mCell;
  side = mSide;
  if (retract) {
    z.moveTo(0);
    while (z.isRunning()) {
      z.run();
    }
  }
  if (packType == PT_A) {
    x.moveTo(X_ZERO + mRow * X_STEPOVER);
    y.moveTo(Y_ZERO + ((mRow + 1) % 2) * Y_STEPOVER / 2 + mCell * Y_STEPOVER + (mSide ? (WELD_SPACE / 2) : (-WELD_SPACE / 2)));
  }
  else if (packType == PT_B) {
    x.moveTo(X_ZERO + mRow * X_STEPOVER);
    y.moveTo(Y_ZERO + (mRow % 2) * Y_STEPOVER / 2 + mCell * Y_STEPOVER + (mSide ? (WELD_SPACE / 2) : (-WELD_SPACE / 2)));
  }
  while (x.isRunning() || y.isRunning()) {
    x.run();
    y.run();
  }
  z.moveTo(Z_ZERO);
  while (z.isRunning()) {
    z.run();
  }
}

void parseCommand(String cmd, String cmd2 = "") {
  Serial.println("# " + cmd + " " + cmd2);
  if (cmd == "runSeries") {
    runSeries(2);
  }
  else if (cmd == "runSeries18650") {
    int cells = cmd2.toInt();
    runSeries18650(cells, 2);
  }
  else if (cmd == "runPack") {
    runPack(2, packType);
  }
  else if (cmd == "packType") {
    if (cmd2 == "A")
      packType = PT_A;
    else if (cmd2 == "B")
      packType = PT_B;
    else
      Serial.println("# Unknown pack type " + cmd2);
  }
  else if (cmd == "align") {
    align(packType);
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
    z.stepdownCycle(WELD_TIME);
  }
  else if (cmd == "zWeld") {
    zWeld();
  }
  else if (cmd == "resetWelds") {
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 24; j++) {
        welded[i][j] = 0;
      }
    }
  }
  else if (cmd == "xHome") {
    x.home(500);
  }
  else if (cmd == "yHome") {
    y.home(900);
  }
  else if (cmd == "zHome") {
    z.home(500);
  }
  else if (cmd == "homeAll") {
    z.home();
    x.home();
    y.home();
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
  else if (cmd == "moveToCell") {
    String row = cmd2.substring(0, cmd2.indexOf("_"));
    String cell = cmd2.substring(cmd2.indexOf("_") + 1);
    String side = cell.substring(cell.indexOf("_") + 1);
    cell = cell.substring(0, cell.indexOf("_"));
    Serial.println("# " + row + " " + cell + " " + side);
    moveToCell(row.toInt(), cell.toInt(), side.toInt(), packType);
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
    Serial.println("# Unknown command " + cmd + ".");
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
