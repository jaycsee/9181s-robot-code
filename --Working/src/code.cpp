#include "main.h"
#include "code.hpp"

void flags::aliveAll() {
  flags::generalAlive = true;
  flags::opcontrolAlive = true;
  flags::watchdogAlive = true;
}

void flags::killAll() {
  flags::generalAlive = false;
  flags::opcontrolAlive = false;
  flags::watchdogAlive = false;
}

Unused::Unused() {}

Unused::Unused(int i) {};

void ports::init() {
  ports::opcontrolTaskWatcher = &(Watchdog::watch("Operator control", TASK_OPCONTROL_HZ));
  ports::ballLauncherTaskWatcher = &(Watchdog::watch("Ball launcher", TASK_BALLLAUNCHER_HZ));

  ports::brainBattery = new BrainBattery();
  ports::controllerMainBattery = new ControllerBattery(CONTROLLER_MAIN);
  ports::controllerPartnerBattery = new ControllerBattery(CONTROLLER_PARTNER);

  ports::controllerMain = new RecordedController(CONTROLLER_MAIN);
  ports::controllerPartner = new RecordedController(CONTROLLER_PARTNER);

  ports::leftDriveMotor = new pros::Motor(1, GEARSET_200, FWD, ENCODER_DEGREES);
  ports::rightDriveMotor = new pros::Motor(10, GEARSET_200, REV, ENCODER_DEGREES);
  ports::elasticLaunchMotor = new pros::Motor(9, GEARSET_100, FWD, ENCODER_DEGREES);

  ports::driveLock = new pros::Mutex();
  ports::launcherLock = new pros::Mutex();
  ports::intakeLock = new pros::Mutex();
  ports::liftLock = new pros::Mutex();

  ports::driveControl = new DriveControl(*ports::driveLock, *ports::leftDriveMotor, *ports::rightDriveMotor);
  ports::ballLauncher = new ElasticLauncher(ports::ballLauncherTaskWatcher, *ports::launcherLock, *ports::elasticLaunchMotor);

  Watchdog::watch("Brain battery", *ports::brainBattery);
  Watchdog::watch("Main controller battery", *ports::controllerMainBattery);
  Watchdog::watch("Partner controller battery", *ports::controllerPartnerBattery);

  Watchdog::watch("Main controller", *ports::controllerMain);
  Watchdog::watch("Partner controller", *ports::controllerPartner);

  Watchdog::watch("Left drive motor", *ports::driveLock, *ports::leftDriveMotor);
  Watchdog::watch("Right drive motor", *ports::driveLock, *ports::rightDriveMotor);
  Watchdog::watch("Launcher motor", *ports::launcherLock, *ports::elasticLaunchMotor);

  // Watchdog::watchCompetition();
}

/**
 * Runs initialization code. This occurs as soon as the program is started.
 *
 * All other competition modes are blocked by initialize; it is recommended
 * to keep execution time for this mode under a few seconds.
 */
void initialize() {
  flags::aliveAll();

  ports::init();

  Logger::init("/ser/sout");
  if (SD_INSERTED)
	  Logger::init("/usd/logs/" + emath::timestamp() + ".txt");

  Watchdog::start();

	pros::lcd::initialize();
	pros::lcd::set_text(1, "Hello PROS User!");
}

using namespace ports;

/**
 * Runs after initialize(), and before autonomous when connected to the Field
 * Management System or the VEX Competition Switch. This is intended for
 * competition-specific initialization routines, such as an autonomous selector
 * on the LCD.
 *
 * This task will exit when the robot is enabled and autonomous or opcontrol
 * starts.
 */
void competition_initialize() {}


/**
 * Runs the user autonomous code. This function will be started in its own task
 * with the default priority and stack size whenever the robot is enabled via
 * the Field Management System or the VEX Competition Switch in the autonomous
 * mode. Alternatively, this function may be called in initialize or opcontrol
 * for non-competition testing purposes.
 *
 * If the robot is disabled or communications is lost, the autonomous task
 * will be stopped. Re-enabling the robot will restart the task, not re-start it
 * from where it left off.
 */
void autonomous() {
  autonomousSafe();
}

void autonomousSafe() {}


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
	operatorControlSafe();
}

void operatorControlSafe() {
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

	while (flags::opcontrolAlive) {
    opcontrolTaskWatcher->notify();

		driveControl->run(controllerMain->get_analog(STICK_LEFT_X), controllerMain->get_analog(STICK_LEFT_Y), false, true, 1, 1);

		pros::c::delay(1000 / TASK_OPCONTROL_HZ);
	}
}


/**
 * Runs while the robot is in the disabled state of Field Management System or
 * the VEX Competition Switch, following either autonomous or opcontrol. When
 * the robot is enabled, this task will exit.
 */
void disabled() {}
