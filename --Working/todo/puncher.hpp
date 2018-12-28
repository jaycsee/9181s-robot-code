#ifndef _PUNCHER_HPP_
#define _PUNCHER_HPP_

#include "main.h"

class Puncher {
  private:
    static pros::Task * task;
    static bool stopped;

    static pros::Mutex * lock;
    static pros::Motor * motor;

    static void _task(void * param);

  public:
    static void start(pros::Mutex * lock, pros::Motor * motor);

    static void prime();

    static void shoot();

    static void stop();

};

#endif