#ifndef _PORTS_HPP_
#define _PORTS_HPP_

#include "main.h"

namespace ports {

  // Controllers
  extern pros::Controller * controllerMain;
  extern pros::Controller * controllerPartner;

  // Motors
  extern pros::Motor * port1;
  extern pros::Motor * port2;
  extern pros::Motor * port3;
  extern pros::Motor * port4;
  extern pros::Motor * port5;
  extern pros::Motor * port6;
  extern pros::Motor * port7;
  extern pros::Motor * port8;
  extern pros::Motor * port9;
  extern pros::Motor * port10;
  extern pros::Motor * port11;
  extern pros::Motor * port12;
  extern pros::Motor * port13;
  extern pros::Motor * port14;
  extern pros::Motor * port15;
  extern pros::Motor * port16;
  extern pros::Motor * port17;
  extern pros::Motor * port18;
  extern pros::Motor * port19;
  extern pros::Motor * port20;
  extern pros::Motor * port21;

  // Port mapping
  extern pros::Motor * frontLeftDrive;
  extern pros::Motor * backLeftDrive;
  extern pros::Motor * frontRightDrive;
  extern pros::Motor * backRightDrive;

  // Vision

  // ADI (3-wire) ports

  // PID manager

}

// Selected autonomous routine
extern int selectedAutonomous;

#endif