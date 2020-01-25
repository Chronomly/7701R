#include "generalFunctions.h"
#include "graphics.h"
#include "Settings.h"


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

//controller
extern pros::Controller master, partner;

//motor declaration (declaraton is in initialize.cpp)
  //wheels
 extern GEAH::Motor left_mtr_back;
 extern GEAH::Motor right_mtr_back;

 extern GEAH::Motor left_mtr_front;
 extern GEAH::Motor right_mtr_front;


 extern GEAH::Motor ramp_mtr;
 extern GEAH::Motor intake_lift_mtr;
 extern GEAH::Motor left_intake;
 extern GEAH::Motor right_intake;

bool potDrivetrainSelect = false;

GEAH::driver driver = getDriver();

int drive;

unsigned long loops = 0;

HEEA_Graphics::RotatableShape square = makeShape();

void vibrateController(const char*rumble_pattern) {
  master.rumble(rumble_pattern);
}

std::vector<pros::Controller> controllers = {master,partner};
namespace GEAH {
  bool buttonIsPressed(GEAH::controllerButton button) {
    if (button.name == "notAssigned") return false; //if the button is not assigned return false

    if (not (controllers[button.controller].get_digital(button.baseButton))) {
      return false; //return false if the basebutton is not pressed
    }
    if (button.shift == controllers[button.controller].get_digital(getDriver().shiftButton.baseButton)) {

      return true;   //if the button is a shifted button and the shift key is pressed return true!
    }else{
      return false;
    }
  }

  bool buttonIsPressedNoShift(GEAH::controllerButton button) {
    if (button.name == "notAssigned") return false; //if the button is not assigned return false

    if ( (controllers[button.controller].get_digital(button.baseButton))) {
      return true;
    }else{
      return false;
    }
  }
}

void runOpControl();


void opcontrol() {

  initializeLVGL();


  setDriveTrainPIDIsActivated(false);

  while (true) {
    runOpControl();
  }
}

void runOpControl() {
    if (loops % 3 == 0) { //prioratizes actual robot functions over graphics
      //update graphics
      square.update(1);
      updateStyle();
    }

    driver = getDriver();


    drive = driver.drive_type;



		LEDs(false);

    int left, right, straif, s_correction;

    if(drive == 0) { //arcade drive
      int power = master.get_analog(ANALOG_LEFT_Y) - master.get_analog(ANALOG_RIGHT_Y);
      int turn = master.get_analog(ANALOG_LEFT_X) + master.get_analog(ANALOG_RIGHT_X);
      left = power + turn;
      right = power - turn;
      s_correction = 0; //(master.get_analog(ANALOG_RIGHT_Y));
      straif = 0; //(master.get_analog(ANALOG_RIGHT_X));
    }else if (drive == 1){ //tank drive
      left = master.get_analog(ANALOG_LEFT_Y);
      right = master.get_analog(ANALOG_RIGHT_Y);
      straif = (master.get_analog(ANALOG_RIGHT_X));
      s_correction = 0;
    }else if (drive == 2) { //drone drive
      int power = master.get_analog(ANALOG_RIGHT_Y);
      int turn = master.get_analog(ANALOG_LEFT_X);
      left = power + turn;
      right = power - turn;
      s_correction = 0;
      straif = (master.get_analog(ANALOG_RIGHT_X));
    }

		left_mtr_back.move(straif + left);
		right_mtr_back.move(-straif + right);
		left_mtr_front.move(-straif*.1*(0.02*s_correction + 1) + left);
		right_mtr_front.move(straif*.1*(0.02*s_correction + 1) + right);


		if( ( abs(left)+abs(right)+abs(2*straif) ) > 100) {
		 LEDs(true);
		}


    if (GEAH::buttonIsPressed(driver.LAUNCH_AUTON)) {
      pros::delay(500);
      autonomous();
    }




    //wheel lock
    if (GEAH::buttonIsPressed(driver.BREAKS_ON)) {
      consoleLogN("-Breaks on-");
      setLeftDriveTrainTarget(left_mtr_back.get_position(),200);
      setRightDriveTrainTarget(right_mtr_back.get_position(),200);
      setDriveTrainPIDIsActivated(true);
    }else if(GEAH::buttonIsPressed(driver.BREAKS_OFF)){
      consoleLogN("-Breaks off-");
      setDriveTrainPIDIsActivated(false);
    }



//arms
    bool autoTowerPressedAlready = false;
    int autoTowerSetting = 0; //0=down, 1=low, 2=high
    if (GEAH::buttonIsPressed(driver.intake_up)) {
      setAPIDIsActivated("intake_lift_PID", false);
      intake_lift_mtr.move(200);
    }else if (GEAH::buttonIsPressed(driver.intake_down)) {
      setAPIDIsActivated("intake_lift_PID", false);
      intake_lift_mtr.move(-200);
    }else if (GEAH::buttonIsPressed(driver.unloadReset)) {
      setAPIDIsActivated("intake_lift_PID", true);
      setAPIDTargetAndSpeed("intake_lift_PID",0, 255);
    }else if (GEAH::buttonIsPressed(driver.autoTowerUp)) {
      if ((! autoTowerPressedAlready) && (autoTowerSetting < 2)) {
        autoTowerSetting++;
      }
      autoTowerPressedAlready = true;
    }else if (GEAH::buttonIsPressed(driver.autoTowerDown)) {
      if ((! autoTowerPressedAlready) && (autoTowerSetting > 0)) {
        autoTowerSetting--;
      }
      autoTowerPressedAlready = true;
    }else{
      autoTowerPressedAlready = false;
      setAPIDIsActivated("intake_lift_PID", false);
      intake_lift_mtr.move(0);
    }

    //setting APID for autotower operation
    if ((autoTowerSetting == 0) && (autoTowerPressedAlready)) {
      setAPIDIsActivated("intake_lift_PID",true);
      setAPIDTarget("intake_lift_PID", 0);
    }else if ((autoTowerSetting == 1) && (autoTowerPressedAlready)) {
      //tower low
      setAPIDIsActivated("intake_lift_PID", true);
      setAPIDTargetAndSpeed("intake_lift_PID", 70*84/12, 255);
    }if ((autoTowerSetting == 2) && (autoTowerPressedAlready)) {
      //tower high
      setAPIDIsActivated("intake_lift_PID", true);
      setAPIDTargetAndSpeed("intake_lift_PID", 90*84/12, 255);
    }

    //LOCK WHEELS AND INTAKE
    if (GEAH::buttonIsPressed(driver.lockWheelsIntake)) {
      pros::delay(10);
      left_intake.move(1.1 * left_mtr_front.get_actual_velocity());
      right_intake.move(1.1 * left_mtr_front.get_actual_velocity());
      pros::delay(10);
    }else if (GEAH::buttonIsPressedNoShift(driver.intake_in)) {
      left_intake.move(255);
      right_intake.move(255);
    }else if (GEAH::buttonIsPressedNoShift(driver.intake_out)) {
      left_intake.move(-100);
      right_intake.move(-100);
    }else{
      left_intake.move(0);
      right_intake.move(0);
    }

//ramp stuff
    if (GEAH::buttonIsPressed(driver.ramp_up)) {
      setAPIDIsActivated("ramp_PID", false);
      ramp_mtr.move(100);
    }else if (GEAH::buttonIsPressed(driver.ramp_down)) {
      setAPIDIsActivated("ramp_PID", false);
      ramp_mtr.move(-100);
    }else if (GEAH::buttonIsPressed(driver.fine_ramp_up)) {
      setAPIDIsActivated("ramp_PID", false);
      ramp_mtr.move(50);
    }else if (GEAH::buttonIsPressed(driver.fine_ramp_down)) {
      setAPIDIsActivated("ramp_PID", false);
      ramp_mtr.move(-50);
    }else if (GEAH::buttonIsPressed(driver.unloadReset)) {
      setAPIDIsActivated("ramp_PID", true);
      setAPIDTargetAndSpeed("ramp_PID", 3, 255);
    }else{
      ramp_mtr.move(0);
    }

    //moving the ramp forwards when the arms are lifted
    if ((intake_lift_mtr.get_position() >  3*84/12) && (intake_lift_mtr.get_actual_velocity() > 0.1) ) {
      setAPIDIsActivated("ramp_PID", true);
      setAPIDTargetAndSpeed("ramp_PID", 105 * 84/12, 255);
    }else if (((intake_lift_mtr.get_position() <  3*84/12) && (intake_lift_mtr.get_actual_velocity() < -0.1) )) {
      setAPIDIsActivated("ramp_PID", true);
      setAPIDTargetAndSpeed("ramp_PID",0, 255);
    }


/*
    if(GEAH::buttonIsPressed(driver.auto_unload)){
    ramp_mtr.move(-15);

    left_mtr_back.move(-50);
    right_mtr_back.move(-50);
    left_mtr_front.move(-50);
    right_mtr_front.move(-50);

    left_intake.move(-50);
    right_intake.move(-50)

    ::pros.delay(2000);
    ramp_mtr.move(-15);

    ::pros.delay(2000);

    left_mtr_back.move(0);
    right_mtr_back.move(0);
    left_mtr_front.move(0);
    right_mtr_front.move(0);

    left_intake.move(0);
    right_intake.move(0);

    ramp_mtr.move(0);

  }
*/




    autoPilotController(loops);

    checkForStop();

		pros::delay(5);


		loops++;
}
