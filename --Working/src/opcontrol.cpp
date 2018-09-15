/** @file opcontrol.c
 * @brief File for operator control code
 *
 * This file should contain the user operatorControl() function and any functions related to it.
 *
 * Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/
 *
 * PROS contains FreeRTOS (http://www.freertos.org) whose source code may be
 * obtained from http://sourceforge.net/projects/freertos/files/ or on request.
 */

#include "main.h"

/**
 * Runs the operator control code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the operator
 * control mode.
 *
 * If no competition control is connected, this function will run immediately
 * following initialize().
 *
 * If the robot is disabled or communications is lost, the
 * operator control task will be stopped. Re-enabling the robot will restart the
 * task, not resume it from where it left off.
 */
void operatorControl() {
	/*
	pros::Controller master(pros::E_CONTROLLER_MASTER);
	auto left_mtr = 1_mtr;
	pros::Motor right_mtr(2);
	while (true) {
	  pros::lcd::print(0, "%d %d %d", (pros::lcd::read_buttons() & LCD_BTN_LEFT) >> 2,
	                   (pros::lcd::read_buttons() & LCD_BTN_CENTER) >> 1,
	                   (pros::lcd::read_buttons() & LCD_BTN_RIGHT) >> 0);
	  int left = master.get_analog(ANALOG_LEFT_Y);
	  int right = master.get_analog(ANALOG_RIGHT_Y);

	  left_mtr = left;
	  right_mtr = right;
	  pros::delay(20);
	}
	*/

	Logger::init("/ser/sout");
	Logger::init("/usd/" + emath::timestamp() + ".txt");

	// Setup
	Battery battery;
	pros::Controller controller (CONTROLLER_MAIN);
	pros::Controller controllerPartner (CONTROLLER_PARTNER);
	pros::Motor leftFrontDriveMotor (1, GEARSET_200, REV, ENCODER_DEGREES);
	pros::Motor leftRearDriveMotor (11, GEARSET_200, FWD, ENCODER_DEGREES);
	pros::Motor rightFrontDriveMotor (10, GEARSET_200, REV, ENCODER_DEGREES);
	pros::Motor rightRearDriveMotor (20, GEARSET_200, REV, ENCODER_DEGREES);
	pros::Motor unused1 (4, GEARSET_200, FWD, ENCODER_DEGREES);
	pros::Motor unused2 (5, GEARSET_200, FWD, ENCODER_DEGREES);
	pros::Motor unused3 (6, GEARSET_200, FWD, ENCODER_DEGREES);
	pros::Motor unused4 (7, GEARSET_200, FWD, ENCODER_DEGREES);
	pros::Vision vision (8);

	pros::Mutex driveMotorsMutex;

	DriveControl drivecontrol (driveMotorsMutex, leftFrontDriveMotor, leftRearDriveMotor, rightFrontDriveMotor, rightRearDriveMotor);
/*
	pros::Mutex driveMotorsMutex;

	initPorts();

	DriveControl drivecontrol (driveMotorsMutex, *::leftFrontDriveMotor, *::leftRearDriveMotor, *::rightFrontDriveMotor, *::rightRearDriveMotor);
*/
	while (true) {

		drivecontrol.run(controller.get_analog(STICK_LEFT_X), controller.get_analog(STICK_LEFT_Y), false, true, 1, 1);

		pros::c::delay(1000 / TASK_OPCONTROL_HZ);
	}
}
