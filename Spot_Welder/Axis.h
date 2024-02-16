#include "AccelStepper.h"

//////////////////////////////////////////////////////
// Defines a 1-Axis Control for our Battery CNC Controller
// This class assumes that a stepper controller is used
//////////////////////////////////////////////////////
class Axis {
public:
  // Constructor takes in EN, DIR, and STEP pins, as well as whether the stepper is inverted
  Axis(int EN, int DIR, int STEP, bool invertedIn = false, float multiplier = 1.0f): stepper(AccelStepper::DRIVER, STEP, DIR), mHomePin(-1), inverted(invertedIn), ESTOPPED(false), mMultiplier(multiplier) {
    stepper.setEnablePin(EN);
  };

  // Set the move speed of the stepper
  void setMaxSpeed(float speed) {
    stepper.setMaxSpeed(speed);
  }

  // Set the acceleration of the stepper
  void setAcceleration(float acceleration) {
    stepper.setAcceleration(acceleration);
  }

  // Invert the stepper
  void setInverted(bool invertedIn) {
    inverted = invertedIn;
  }

  // Set a home button. Assumes the pin is configured active low.
  void attachHome(int pin) {
    mHomePin = pin;
  }

  // Set the stepper at a speed indefinitely
  void setSpeed(float speed) {
    stepper.setSpeed(speed);
  }

  // Move by a given amount
  void move(float distance) {
    stepper.move((inverted ? -distance : distance) * mMultiplier);
  }

  // Set to exact position
  void moveTo(float position) {
    stepper.moveTo((inverted ? -position : position) * mMultiplier);
  }

  // Stop the stepper as soon as acceleration allows
  void stop() {
    stepper.stop();
  }

  // Get if the stepper is running
  bool isRunning() {
    return stepper.isRunning();
  }

  // Get the current position of the stepper
  float getPosition() {
    return (inverted ? -stepper.currentPosition() : stepper.currentPosition()) * mMultiplier;
  }

  // Get the target position of the stepper
  float getTargetPosition() {
    return (inverted ? -stepper.targetPosition() : stepper.targetPosition()) * mMultiplier;
  }

  // Get the distance to go
  float getDistanceToGo() {
    return (inverted ? -stepper.distanceToGo() : stepper.distanceToGo()) * mMultiplier;
  }

  // Run the stepper according to active program. Will not process past home.
  void run() {
    if (ESTOPPED) {
      stop();
      return;
    }
    if (mHomePin != -1) {
      if (digitalRead(mHomePin) == LOW) {
        stepper.stop();
      }
    }
    stepper.run();
  }

  // Home the current axis. Returns immediately if no home is set.
  void home(float homeSpeed = 500) {
    delay(5);
    if (mHomePin == -1) {
      stepper.setCurrentPosition(0);
      return;
    }
    delay(5);
    Serial.println("# Homing");
    stepper.setSpeed(inverted ? homeSpeed : -homeSpeed);
    Serial.println(ESTOPPED);
    while (digitalRead(mHomePin) == HIGH && !ESTOPPED) {
      stepper.runSpeed();
      // delay(5);
    }
    Serial.println(ESTOPPED);
    stepper.stop();
    delay(50);
    move(100);
    while (getDistanceToGo() != 0 && !ESTOPPED) {
      stepper.run();
    }
    stepper.stop();
    stepper.setCurrentPosition(0);
  }

  // Emergency stop
  void eStop() {
    Serial.println("# EMERGENCY STOP");
    ESTOPPED = true;
    stepper.stop();
    stepper.run();
  }

  // Reset emergency stop
  void resetEStop() {
    ESTOPPED = false;
  }

protected:
  AccelStepper stepper;
  bool inverted;
  bool ESTOPPED;
  int mHomePin;
  float mMultiplier;
};


/////////////////////////////////////////////////////////////////////////////////////
class HorizontalAxis : public Axis {
public:
  // Constructor
  HorizontalAxis(int EN, int DIR, int STEP, bool invertedIn = false, float multiplier = 1.0f): Axis(EN, DIR, STEP, invertedIn, multiplier) {}

  // Set the stepover distance
  void setStepover(float stepover) {
    mStepover = stepover;
  }

  // Get the stepover distance
  float getStepover() {
    return mStepover;
  }

  // Stepover by the set distance
  void stepover(bool backwards = false) {
    move((backwards) ? -mStepover : mStepover);
  }

  // Stepover by half the set distance
  void stepoverHalf(bool backwards = false) {
    move((backwards) ? -mStepover / 2 : mStepover / 2);
  }

  // Stepover by a custom distance
  void stepoverCustom(float stepSize, bool backwards = false) {
    move((backwards) ? -stepSize : stepSize);
  }

  // Run a stepover, pausing other operations
  void stepoverBlocking(bool backwards = false) {
    stepover(backwards);
    while (stepper.distanceToGo() != 0 && !ESTOPPED) {
      run();
    }
    stepper.stop();
  }

  // Run a half stepover, pausing other operations
  void stepoverHalfBlocking(bool backwards = false) {
    stepoverHalf(backwards);
    while (stepper.distanceToGo() != 0 && !ESTOPPED) {
      run();
    }
    stepper.stop();
  }

  // Run a stepover, pausing other operations
  void stepoverBlockingCustom(float stepSize, bool backwards = false) {
    stepoverCustom(stepSize, backwards);
    while (stepper.distanceToGo() != 0 && !ESTOPPED) {
      run();
    }
    stepper.stop();
  }

private:
  float mStepover;
};


/////////////////////////////////////////////////////////////////////////////////////
class ZAxis : public Axis {
public:
  // Constructor
  // Note: defaults to inverted
  ZAxis(int EN, int DIR, int STEP, bool invertedIn = false, float multiplier = 1.0f): Axis(EN, DIR, STEP, invertedIn, multiplier) {}
  
  // Set the stepdown distance
  void setStepdown(float stepSize) {
    mStepdown = stepSize;
  }

  // Get the stepdown distance
  float getStepdown() {
    return mStepdown;
  }

  // Stepdown by the set distance
  void stepdown() {
    move(mStepdown);
  }

  // Stepdown by a custom distance
  void stepdownCustom(float stepSize) {
    move(stepSize);
  }

  // Stepup by the set distance
  void stepup() {
    move(-mStepdown);
  }

  // Stepup by a custom distance
  void stepupCustom(float stepSize) {
    move(-stepSize);
  }

  // Run a stepdown cycle, pausing other operations
  // It pauses at the bottom the time specified in milliseconds
  void stepdownCycle(int pause) {
    stepdown();
    while (stepper.distanceToGo() != 0 && !ESTOPPED) {
      run();
    }
    stop();
    delay(pause);
    stepup();
    while (stepper.distanceToGo() != 0 && !ESTOPPED) {
      run();
    }
    stop();
  }

private:
  float mStepdown;
};