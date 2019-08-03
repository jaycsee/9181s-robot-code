#include "main.h"
#include "xdrive.hpp"
#include <cmath>

void XDriveControl::runFrontLeftMotors(int voltage) {
  for (const auto & motor : XDriveControl::frontLeftMotors)
    motor.move(voltage);
}

void XDriveControl::runRearLeftMotors(int voltage) {
  for (const auto & motor : XDriveControl::rearLeftMotors)
    motor.move(voltage);
}

void XDriveControl::runFrontRightMotors(int voltage) {
  for (const auto & motor : XDriveControl::frontRightMotors)
    motor.move(voltage);
}

void XDriveControl::runRearRightMotors(int voltage) {
  for (const auto & motor : XDriveControl::rearRightMotors)
    motor.move(voltage);
}

void XDriveControl::setFrontLeftBrake(pros::motor_brake_mode_e_t mode) {
  for (const auto & motor : XDriveControl::frontLeftMotors)
    if (motor.get_brake_mode() != mode)
      motor.set_brake_mode(mode);
}

void XDriveControl::setFrontRightBrake(pros::motor_brake_mode_e_t mode) {
  for (const auto & motor : XDriveControl::frontRightMotors)
    if (motor.get_brake_mode() != mode)
      motor.set_brake_mode(mode);
}

void XDriveControl::setRearLeftBrake(pros::motor_brake_mode_e_t mode) {
  for (const auto & motor : XDriveControl::rearLeftMotors)
    if (motor.get_brake_mode() != mode)
      motor.set_brake_mode(mode);
}

void XDriveControl::setRearRightBrake(pros::motor_brake_mode_e_t mode) {
  for (const auto & motor : XDriveControl::rearRightMotors)
    if (motor.get_brake_mode() != mode)
      motor.set_brake_mode(mode);
}

XDriveControl::XDriveControl(MotorWatcher & motorLock, pros::Motor frontLeftMotor, pros::Motor rearLeftMotor, pros::Motor frontRightMotor, pros::Motor rearRightMotor) {
  XDriveControl::lock = &motorLock;
  XDriveControl::addFrontLeftMotor(frontLeftMotor);
  XDriveControl::addFrontRightMotor(frontRightMotor);
  XDriveControl::addRearLeftMotor(rearLeftMotor);
  XDriveControl::addRearRightMotor(rearRightMotor);
}

void XDriveControl::addFrontLeftMotor(pros::Motor motor) {
  XDriveControl::frontLeftMotors.push_back(motor);
}

void XDriveControl::addFrontRightMotor(pros::Motor motor) {
  XDriveControl::frontRightMotors.push_back(motor);
}

void XDriveControl::addRearLeftMotor(pros::Motor motor) {
  XDriveControl::rearLeftMotors.push_back(motor);
}

void XDriveControl::addRearRightMotor(pros::Motor motor) {
  XDriveControl::rearRightMotors.push_back(motor);
}
/* may be implemented in the future in conjunction with DriveControl::remove*Motor()
bool XDriveControl::removeFrontLeftMotor(pros::Motor motor) {

}

bool XDriveControl::removeFrontRightMotor(pros::Motor motor) {

}

bool XDriveControl::removeRearLeftMotor(pros::Motor motor) {

}

bool XDriveControl::removeRearRightMotor(pros::Motor motor) {

}
*/
void XDriveControl::clearMotors() {
  XDriveControl::clearFrontLeftMotors();
  XDriveControl::clearFrontRightMotors();
  XDriveControl::clearRearLeftMotors();
  XDriveControl::clearRearRightMotors();
}

void XDriveControl::clearFrontLeftMotors() {
  XDriveControl::frontLeftMotors.clear();
}

void XDriveControl::clearFrontRightMotors() {
  XDriveControl::frontRightMotors.clear();
}

void XDriveControl::clearRearLeftMotors() {
  XDriveControl::rearLeftMotors.clear();
}

void XDriveControl::clearRearRightMotors() {
  XDriveControl::rearRightMotors.clear();
}

void XDriveControl::run(double moveVoltage, double strafeVoltage, double turnVoltage, bool leftBrake, bool rightBrake) {
  XDriveControl::run(moveVoltage, strafeVoltage, turnVoltage, leftBrake, rightBrake, 1.0, 1.0, 1.0);
}

void XDriveControl::run(double moveVoltage, double strafeVoltage, double turnVoltage, bool leftBrake, bool rightBrake, double moveSensitivity, double strafeSensitivity, double turnSensitivity) {
  moveVoltage *= moveSensitivity;
  strafeVoltage *= strafeSensitivity;
  turnVoltage *= turnSensitivity;

  int frontLeftVoltage = util::limit127(moveVoltage + strafeVoltage);
  int frontRightVoltage = util::limit127(moveVoltage - strafeVoltage);
  int rearLeftVoltage = util::limit127(moveVoltage - strafeVoltage);
  int rearRightVoltage = util::limit127(moveVoltage + strafeVoltage);

  class nest {
    public:
      static int distribute(int & turn, int & frontVoltage, int & rearVoltage) {
        int overflow = 0;
        for (; turn != 0; turn = util::step0(turn)) {
          int ftemp = std::round(frontVoltage + turn - util::step0(turn));
          int rtemp = std::round(rearVoltage + turn - util::step0(turn));

          if (util::within127(ftemp))
            frontVoltage = ftemp;
          else {
            int rtt = rtemp + turn - util::step0(turn);
            if (util::within127(rtt))
              rtemp = rtt;
            else if (!util::within127(rtemp)) {
              overflow = overflow - turn + util::step0(turn);
              continue;
            }
          }

          if (util::within127(rtemp))
            rearVoltage = rtemp;
          else {
            int ftt = rtemp + turn - util::step0(turn);
            if (util::within127(ftt))
              frontVoltage = ftt;
          }
        }
        return overflow;
      }
  };

  int lt = std::round(turnVoltage / 2.0);
	int rt = std::round(turnVoltage / -2.0);

	double overflow = 0;

  overflow = nest::distribute(lt, frontLeftVoltage, rearLeftVoltage);

	rt += overflow;
	overflow = nest::distribute(rt, frontRightVoltage, rearRightVoltage);

	lt += overflow;
	overflow = nest::distribute(lt, frontLeftVoltage, rearLeftVoltage);

  if (lock->takeMutex("X-Drive control", MUTEX_WAIT_TIME)) {
    XDriveControl::setFrontLeftBrake(leftBrake ? BRAKE_BRAKE : BRAKE_HOLD);
    XDriveControl::setFrontRightBrake(rightBrake ? BRAKE_BRAKE : BRAKE_HOLD);
    XDriveControl::setRearLeftBrake(leftBrake ? BRAKE_BRAKE : BRAKE_HOLD);
    XDriveControl::setRearRightBrake(rightBrake ? BRAKE_BRAKE : BRAKE_HOLD);
    XDriveControl::runFrontLeftMotors(leftBrake ? 0 : frontLeftVoltage);
    XDriveControl::runFrontRightMotors(rightBrake ? 0 : frontRightVoltage);
    XDriveControl::runRearLeftMotors(leftBrake ? 0 : rearLeftVoltage);
    XDriveControl::runRearRightMotors(rightBrake ? 0 : rearRightVoltage);
    lock->giveMutex("X-Drive control");
  }

}