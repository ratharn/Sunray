/*
 * robot messages
 *  01 : sensor data
 *  03 : map bitmap data  
 *  05 : perimeter outline data
 *  11 : tracking forever
 *  12 : start mapping
 *  13 : mowing lanes
 *  14 : mowing random
 *  15 : particles data
 *  16 : distribute particles on perimeter
 *  17 : robot motion data (distance, orientation) 
 *  70 : configure bluetooth  
 *  75 : erase microcontroller flash memory
 *  76 : eeprom data
 
 * battery messages
 *  88 : battery data
 
 * perimeter messages
 *  84 : perimeter settings
 
 * sonar messages
 *  87 : sonar data (verbose)
 
 * motor messages
 *  00 : stop immediately
 *  02 : set motor pwm (left, right)
 *  85 : travel angle distance (speed, distance, orientation)
 *  06 : travel line distance (speed, distance, orientation)
 *  07 : travel line time (speed, time, orientation) 
 *  08 : rotate angle (speed)
 *  09 : rotate time (speed) 
 *  74 : set mow motor pwm
 *  83 : motor settings 
 *  86 : motor controller data
 
 * ADC messages
 *  71 : calibrate ADC
 
 * IMU messages
 *  04 : IMU data (verbose)
 *  10 : calibrate gyro
 *  72 : calibrate compass 
 *  73 : toggle verbose
 *  78 : compass calibration data (centre X,Y,Z,radii X,Y,Z)
 *  79 : IMU self test
 *  80 : start compass calibration  
 *  81 : stop compass calibration  
 *  82 : IMU settings
 
 * ranging messages
 *  77 : ranging data (time, address, distance, power)
 
 
 */
      
#include "robot.h"
#include "i2c.h"
#include "modelrc.h"
#include "buzzer.h"
#include "bumper.h"
#include "config.h"
#include "remotectl.h"
#include "flashmem.h"
#include "helper.h"
#include "motor.h"
#include "map.h"
#include "battery.h"
#include "pinman.h"
#include "settings.h"
#include "bt.h"
#include "sonar.h"
#ifndef __AVR
  #include <Reset.h>
#endif


#define MAGIC 52

RobotClass Robot;



// RobotState {  STAT_IDLE, STAT_CAL_GYRO, STAT_TRACK, STAT_CREATE_MAP, STAT_MOW, STAT_RC };
const char* robotStateNames[] = { "IDLE", "GYRO", "TRAK", "MAP ", "MOW ", "R/C " };



RobotClass::RobotClass(){
}

void RobotClass::begin(){      
  Buzzer.begin();
  ROBOTMSG.begin(ROBOTMSG_BAUDRATE);    
//  receiveEEPROM_or_ERASE(); 
  I2C_reset();
  Wire.begin();            
	Settings.begin();
  PinMan.begin();  
  ADCMan.begin();      
  //RemoteCtl.begin();      
  
  //while (!Console) ; // required if using Due native port
  DEBUGLN(F("SETUP")); 

  // keep battery switched ON
  Battery.begin();    

  RANGING.begin(115200);

  Bumper.begin();	
  if (IMU_USE) IMU.begin();    
  Motor.begin();      
  Perimeter.begin(pinPerimeterLeft, pinPerimeterRight);          
	Sonar.begin();
    
  Map.begin();
  DEBUG(F("freeRam="));
  DEBUGLN(freeRam());
  RC.begin();
  
  state = STAT_IDLE;
  lastState = state;
  mowState = MOW_LINE;
  mowPattern = PATTERN_NONE;
  trackState = TRK_RUN;  
	trackClockwise = true;

	sensorTriggerStatus = 0;
  nextControlTime = 0;  
  nextInfoTime = 0;  
  nextIMUTime = 0;
  loopCounter = 0;
  loopsPerSec = 0;
  trackLineTimeout = 0;
  mowingAngle = 0;
  mowingDirection = 0;
  lastStartLineTime; 
	trackSpeedPerc = 0.5;
	trackRotationSpeedPerc = 0.3;
	rotationSpeedPerc = 0.3;
	reverseSpeedPerc = 0.3;

  ADCMan.printInfo();
  Buzzer.sound(SND_READY, true);   
}

void RobotClass::configureBluetooth(){
  BluetoothConfig bt; 
  bt.setParams(F("Ardumower"), 1234, 115200, true);
}

void RobotClass::printSensorData(){
/*ROBOTMSG.write(0xAA);
    ROBOTMSG.write(0xBB);
    ROBOTMSG.write(0x01);
    printInt(17);    
    printLong(millis());
    ROBOTMSG.write(Robot.loopsPerSec);
    printFloat(Motor.motorLeftRpmCurr);
    printFloat(Motor.motorRightRpmCurr);
    printFloat(IMU.getYaw());   */

    ROBOTMSG.print(F("!01"));     
    ROBOTMSG.print(F(","));       
    ROBOTMSG.print(millis());
    ROBOTMSG.print(F(","));      
    ROBOTMSG.print(state);   
    ROBOTMSG.print(F(","));      
    ROBOTMSG.print(Robot.loopsPerSec);     
    ROBOTMSG.print(F(","));      
    ROBOTMSG.print(Motor.motorLeftTicks);    
    ROBOTMSG.print(F(","));      
    ROBOTMSG.print(Motor.motorRightTicks);  
    ROBOTMSG.print(F(","));     
    ROBOTMSG.print(Motor.motorLeftRpmCurr, 2);   
    ROBOTMSG.print(F(","));      
    ROBOTMSG.print(Motor.motorRightRpmCurr, 2);     
    ROBOTMSG.print(F(","));     
    ROBOTMSG.print(Perimeter.getMagnitude(IDX_LEFT));     
    ROBOTMSG.print(F(","));      
    ROBOTMSG.print(Perimeter.getMagnitude(IDX_RIGHT));     
    ROBOTMSG.print(F(","));      
    //ROBOTMSG.print(IMU.getYaw(), 3);       
	  ROBOTMSG.print(Motor.angleRadCurr, 3);       
    ROBOTMSG.print(F(","));      
    ROBOTMSG.print(IMU.acc.x, 3);   
    ROBOTMSG.print(F(","));            
    ROBOTMSG.print(IMU.acc.y, 3);   
    ROBOTMSG.print(F(","));            
    ROBOTMSG.print(IMU.acc.z, 3);   
    ROBOTMSG.print(F(","));                
    ROBOTMSG.print(IMU.gravity.x, 3);        
    ROBOTMSG.print(F(","));          
    ROBOTMSG.print(IMU.gravity.y, 3);        
    ROBOTMSG.print(F(","));      
    ROBOTMSG.print(IMU.gravity.z, 3);    
    ROBOTMSG.print(F(","));          
    ROBOTMSG.print(IMU.comYaw, 3);     
    ROBOTMSG.print(F(","));          
    ROBOTMSG.print(IMU.ypr.pitch, 3);     
    ROBOTMSG.print(F(","));          
    ROBOTMSG.print(IMU.ypr.roll, 3);    
    ROBOTMSG.print(F(","));          
    ROBOTMSG.print(Map.robotState.x);    
    ROBOTMSG.print(F(","));       
    ROBOTMSG.print(Map.robotState.y);
    ROBOTMSG.print(F(","));       
    ROBOTMSG.print(Motor.motorLeftSense);
    ROBOTMSG.print(F(","));       
    ROBOTMSG.print(Motor.motorRightSense);    
    ROBOTMSG.print(F(","));       
    ROBOTMSG.print(Motor.motorMowSense);
    ROBOTMSG.print(F(","));           
    ROBOTMSG.print(Motor.motorLeftFriction);
    ROBOTMSG.print(F(","));               
    ROBOTMSG.print(Motor.motorRightFriction);
    ROBOTMSG.print(F(","));               
    ROBOTMSG.print(Map.overallProb);
    ROBOTMSG.print(F(","));               
    ROBOTMSG.print(Battery.batteryVoltage);                   
    ROBOTMSG.print(F(","));         
    ROBOTMSG.print(Motor.motion);   
    ROBOTMSG.print(F(","));             
    ROBOTMSG.print(IMU.state);       
    ROBOTMSG.print(F(","));             
    ROBOTMSG.print(Motor.distanceCmSet);   
    ROBOTMSG.print(F(","));             
    ROBOTMSG.print(Motor.angleRadSet);   
		ROBOTMSG.print(F(","));	
	  ROBOTMSG.print(sensorTriggerStatus);  
		ROBOTMSG.print(F(","));
		ROBOTMSG.print(Sonar.distanceLeft);
		ROBOTMSG.print(F(","));
		ROBOTMSG.print(Sonar.distanceCenter);
		ROBOTMSG.print(F(","));
		ROBOTMSG.print(Sonar.distanceRight);						
    ROBOTMSG.println(); 		
}

void RobotClass::readRobotMessages(){
  float pwmLeft;
  float pwmRight;
  float distance;
  float angle;
  float speed;  
  float duration;
  
    char ch = ROBOTMSG.read();    
    switch (ch){     
      case '?':               
        int cmd = ROBOTMSG.parseInt();                
        //DEBUG(ch);
        //DEBUGLN(cmd);
        switch (cmd) {          
          case 70: configureBluetooth(); break;          
          case 6:  distance = ROBOTMSG.parseFloat();
                   angle = ROBOTMSG.parseFloat();
                   speed = ROBOTMSG.parseFloat();
                   Motor.travelLineDistance(distance, angle, speed); 
                   break;                     
          case 7: duration = ROBOTMSG.parseFloat();
                  angle = ROBOTMSG.parseFloat();
                  speed = ROBOTMSG.parseFloat();
                  Motor.travelLineTime(duration, angle, speed); 
                  break;
          case 8: angle = ROBOTMSG.parseFloat();
                  speed = ROBOTMSG.parseFloat();
                  Motor.rotateAngle(scalePI(Motor.angleRadCurr+angle), speed); 
                  break;
          case 9: duration = ROBOTMSG.parseFloat();                  
                  speed = ROBOTMSG.parseFloat();
                  Motor.rotateTime(duration, speed); 
                  break;                    
          case 10: IMU.startGyroCalibration(); break;
          case 71: //ADCMan.calibrate(); 
                   break;
          //case 72: IMU.startCompassCalibration(); break;
          case 11: trackClockwise = ROBOTMSG.parseInt(); 
								   startTrackingForEver(); break;
          case 12: startMapping(); break;
          case 13: startLaneMowing(); break;
          case 14: startRandomMowing(); break;
          case 73: IMU.verboseOutput = ROBOTMSG.parseInt(); 
									 Motor.verboseOutput = ROBOTMSG.parseInt(); 
									 Map.verboseOutput = ROBOTMSG.parseInt(); 
					         break;      
          case 74: Motor.setMowerPWM(ROBOTMSG.parseFloat()); break;
          case 0: setIdle(); break;
          case 2: pwmLeft = ROBOTMSG.parseFloat();
                    pwmRight = ROBOTMSG.parseFloat();                   
                    Motor.setSpeedPWM(pwmLeft, pwmRight);                    
                    /*DEBUG(pwmLeft);
                    DEBUG(",");
                    DEBUGLN(pwmRight);*/
                    break;
          case 3: sendMap(); break;         
          case 15: sendParticles(); break;
          case 16: Map.distributeParticlesOutline(); break;
          case 5: sendPerimeterOutline(); break;       
          case 78: /*IMU.comCentre.x = ROBOTMSG.parseFloat();
                  IMU.comCentre.y = ROBOTMSG.parseFloat();
                  IMU.comCentre.z = ROBOTMSG.parseFloat();
                  IMU.comRadius.x = ROBOTMSG.parseFloat();
                  IMU.comRadius.y = ROBOTMSG.parseFloat();
                  IMU.comRadius.z = ROBOTMSG.parseFloat();                  */
                  IMU.comCalA_1[0] = ROBOTMSG.parseFloat();
                  IMU.comCalA_1[1] = ROBOTMSG.parseFloat();
                  IMU.comCalA_1[2] = ROBOTMSG.parseFloat();
                  IMU.comCalA_1[3] = ROBOTMSG.parseFloat();
                  IMU.comCalA_1[4] = ROBOTMSG.parseFloat();
                  IMU.comCalA_1[5] = ROBOTMSG.parseFloat();
                  IMU.comCalA_1[6] = ROBOTMSG.parseFloat();
                  IMU.comCalA_1[7] = ROBOTMSG.parseFloat();
                  IMU.comCalA_1[8] = ROBOTMSG.parseFloat();
                  IMU.comCalB[0] = ROBOTMSG.parseFloat();
                  IMU.comCalB[1] = ROBOTMSG.parseFloat();
                  IMU.comCalB[2] = ROBOTMSG.parseFloat();
                  IMU.saveCalib();    
                  break;
          case 79: IMU.runSelfTest(); break;
          case 80: IMU.startCompassCalibration(); break;
          case 81: IMU.stopCompassCalibration(); break;
          case 82: IMU.useGyro = ROBOTMSG.parseInt(); 
                   IMU.gyroBiasDpsMax = ROBOTMSG.parseFloat();
                   IMU.mode = ((IMUMode)ROBOTMSG.parseInt());
                   DEBUGLN(F("received IMU settings"));
                   break;
          case 83: Motor.rpmMax = ROBOTMSG.parseFloat();
									 Robot.reverseSpeedPerc = ROBOTMSG.parseFloat();
									 Robot.rotationSpeedPerc = ROBOTMSG.parseFloat();
									 Robot.trackSpeedPerc = ROBOTMSG.parseFloat();
									 Robot.trackRotationSpeedPerc = ROBOTMSG.parseFloat();
									 Motor.robotMass = ROBOTMSG.parseFloat();
									 Motor.motorFrictionMin = ROBOTMSG.parseFloat();
									 Motor.motorFrictionMax = ROBOTMSG.parseFloat();
									 Motor.mowSenseMax = ROBOTMSG.parseFloat();
									 Motor.imuPID.Kp = ROBOTMSG.parseFloat();
                   Motor.imuPID.Ki = ROBOTMSG.parseFloat();
                   Motor.imuPID.Kd = ROBOTMSG.parseFloat();
                   Motor.motorLeftPID.Kp = ROBOTMSG.parseFloat();
                   Motor.motorLeftPID.Ki = ROBOTMSG.parseFloat();
                   Motor.motorLeftPID.Kd = ROBOTMSG.parseFloat();
                   Motor.motorRightPID.Kp = Motor.motorLeftPID.Kp;
                   Motor.motorRightPID.Ki = Motor.motorLeftPID.Ki;
                   Motor.motorRightPID.Kd = Motor.motorLeftPID.Kd;									 
									 Motor.stuckMaxDiffOdometryIMU = ROBOTMSG.parseFloat();
									 Motor.stuckMaxIMUerror = ROBOTMSG.parseFloat();
                   DEBUGLN(F("received motor settings"));
                   break;          
		  case 84: Perimeter.timedOutIfBelowSmag = ROBOTMSG.parseInt();
		           Perimeter.timeOutSecIfNotInside = ROBOTMSG.parseInt();
				       Perimeter.swapCoilPolarity = ROBOTMSG.parseInt();
				       DEBUGLN(F("received perimeter settings"));
				       break;
          case 85:  distance = ROBOTMSG.parseFloat();
                   angle = ROBOTMSG.parseFloat();
                   speed = ROBOTMSG.parseFloat();
                   Motor.travelAngleDistance(distance, scalePI(Motor.angleRadCurr+angle), speed); 
                   break;                     
              
        }
        break;      
    }               
}

void RobotClass::run(){
  unsigned long time;
  int addr;
  float power;
  float distance;
   
  if (millis() >= nextInfoTime){
    nextInfoTime = millis() + 1000;
    loopsPerSec = loopCounter;
    loopCounter=0;

    //DEBUG("DIST=");
    //DEBUGLN(Motor.distToLine);
    //ADCMan.printInfo();
    //DEBUG("ADCconvs=");
    //DEBUGLN(ADCMan.getConvCounter());
    
    printSensorData();    
		sensorTriggerStatus = 0;
    
	  if ( IMU_USE && (!RC.enable) && (IMU.needGyroCal()) ) {     
	    Motor.setPaused(true);
      lastState = state;
	    IMU.startGyroCalibration();
      state = STAT_CAL_GYRO;
    }		
		if (Motor.paused){
			if (IMU.state != IMU_CAL_GYRO){
		      state = lastState;
          Motor.setPaused(false);
          Perimeter.resetTimedOut();
		    }
		}
  }
  if (millis() >= nextIMUTime){    
    nextIMUTime = millis() + 10; // 100 Hz
    IMU.run();
  }

  Buzzer.run();
	ADCMan.run();
	Sonar.run();

  if (millis() >= nextControlTime){    
    nextControlTime = millis() + 200; // 5 Hz

    if ( (state != STAT_CHG) && (Battery.chargerConnected()) ){
      Motor.stopImmediately();
      state = STAT_CHG;      
    }
    Bumper.run();
    RC.run();
    Motor.run();        
    Perimeter.run();
    if (IMU_USE) IMU.run();
	  Map.run();
    //RemoteCtl.run();    
    Battery.run();    

    stateMachine();
				
    if ( (!RC.enable) && ((state != STAT_IDLE) && (state != STAT_CAL_GYRO)) && (state != STAT_CHG) ) 
    {
	    if (Perimeter.signalTimedOut()){
        setIdle();
	      DEBUG(F("PERIMETER TIMEOUT: smag="));
        DEBUG(Perimeter.getSmoothMagnitude(IDX_LEFT));            
        DEBUG(F(","));
        DEBUGLN(Perimeter.getSmoothMagnitude(IDX_RIGHT));            
	      Buzzer.sound(SND_PERIMETER_TIMEOUT, true); 	   
      }
		
	    if ( (fabs(IMU.ypr.roll/PI*180)>30) || (fabs(IMU.ypr.pitch/PI*180)>30) ){
  	    setIdle();
	      DEBUGLN(F("TILT"));
	      Buzzer.sound(SND_TILT, true); 	   
	    }
	  }		

    /*ROBOTMSG.print(F("!04"));     
    ROBOTMSG.print(F(","));       
    ROBOTMSG.print(Map.robotState.x);
    ROBOTMSG.print(F(","));       
    ROBOTMSG.print(Map.robotState.x);
    ROBOTMSG.print(F(","));       
    ROBOTMSG.println(Map.robotState.orientation);  */
    
    loopCounter++;
  }

  if ( ROBOTMSG.available()  ){        	     
    readRobotMessages();    
  }
  //serveHTTP();
  
  if ( (RANGING.available())  ){               
    char ch = RANGING.read();    
    switch (ch){     
      case '!':               
        int cmd = RANGING.parseInt();                
        //DEBUG(ch);
        //DEBUGLN(cmd);
        switch (cmd) {          
          case 77:  time = RANGING.parseInt();
                   addr = RANGING.parseInt();
                   distance = RANGING.parseFloat();
                   power = RANGING.parseFloat();
                   ROBOTMSG.print(F("!77,"));
                   ROBOTMSG.print(time);
                   ROBOTMSG.print(F(","));
                   ROBOTMSG.print(addr);
                   ROBOTMSG.print(F(","));
                   ROBOTMSG.print(distance);
                   ROBOTMSG.print(F(","));
                   ROBOTMSG.println(power);
                   break;                     
        }
    }
  }   
}

void RobotClass::setIdle(){
  state = STAT_IDLE;
  Motor.stopImmediately(); 
}

void RobotClass::startMapping(){
  Map.clearOutline();
  state = STAT_CREATE_MAP;
  trackState = TRK_FIND;  
  mowState = MOW_LINE;  
  Motor.travelLineDistance(10000, IMU.getYaw(), trackSpeedPerc);
}

void RobotClass::startLaneMowing(){
  mowPattern = PATTERN_LANES;
  state = STAT_MOW;
  mowState = MOW_LINE;
  mowingAngle = IMU.getYaw();
  mowingDirection = mowingAngle-PI/2;
  Motor.travelLineDistance(3000, mowingAngle, 1.0);
}

void RobotClass::startRandomMowing(){
  mowPattern = PATTERN_RANDOM;
  state = STAT_MOW;
  mowState = MOW_LINE;
  mowingAngle = IMU.getYaw();
  mowingDirection = mowingAngle-PI/2;
  Motor.travelLineDistance(100000, mowingAngle, 1.0);
}

void RobotClass::startTrackingForEver(){  
  mowPattern = PATTERN_NONE;
  state = STAT_TRACK;  
  trackState = TRK_FIND;  
  Motor.travelLineDistance(10000, IMU.getYaw(), trackSpeedPerc);
}

void RobotClass::track(){
  float leftMag = Perimeter.getMagnitude(IDX_LEFT);
  float rightMag = Perimeter.getMagnitude(IDX_RIGHT);  	
	boolean leftIn = (leftMag < 0);
  boolean rightIn = (rightMag < 0);  	
	float rotationSign = 1;
	if (trackClockwise) {
		leftIn = 	(rightMag < 0);
		rightIn = 	(leftMag < 0);
		rotationSign = -1;
	}	  
	float startDist;       	
  switch (trackState){
    case TRK_FIND:      
	    if  ((!leftIn) || (!rightIn)){
        Motor.stopImmediately();
        trackState = TRK_RUN;
        trackAngle = IMU.getYaw();
        Map.distributeParticlesOutline();
      }
      break;
    case TRK_RUN:	    
	    if ((leftIn) && (!rightIn)){        
        Motor.travelLineTime(300, trackAngle, trackSpeedPerc);
        //Motor.travelLineDistance(5, IMU.getYaw(), speed);
        //Motor.setSpeedPWM(speed, speed);
      }
      else {       
       if (!leftIn) {          
          //Motor.setSpeedPWM(-speedRot, speedRot);
          //Motor.rotateAngle(IMU.getYaw()+PI/180.0*3, speedRot);          
          Motor.rotateTime(300, rotationSign * trackRotationSpeedPerc);
          trackAngle = IMU.getYaw();
        } else {          
          //Motor.setSpeedPWM(speedRot, -speedRot);
          //Motor.rotateAngle(IMU.getYaw()-PI/180.0*3, speedRot);          
          Motor.rotateTime(300, rotationSign * (-trackRotationSpeedPerc) );
          trackAngle = IMU.getYaw();
        }                    
      }      
      
      if (state == STAT_CREATE_MAP){
        startDist = Map.distanceToStart(Map.robotState.x,Map.robotState.y);
        //DEBUGLN(startDist);
		    if (startDist < 0.3) {
          Motor.stopImmediately();
          Buzzer.sound(SND_READY, true);
          state = STAT_IDLE;
          Map.correctOutline();
          Map.transferOutlineToMap();
          //Map.distributeParticlesOutline();
		      Map.saveMap();
          if (mowPattern != PATTERN_NONE){
            trackState = TRK_ROTATE;
            Motor.rotateAngle(IMU.getYaw()+PI/180.0*90, fabs(trackRotationSpeedPerc));
          }
        }
      }	  
	    break;
    case TRK_ROTATE:
      if (Motor.motion == MOT_STOP){
        mowingAngle = IMU.getYaw();
        mowingDirection = mowingAngle-PI/2;
        state = STAT_IDLE;
        /*state = STAT_MOW;
        mowState = MOW_LINE;
        Motor.travelLineDistance(300, mowingAngle, speed);*/
      }
      break;	  
  }
  /*if (sign(mag) != sign(lastPerimeterMag)){
    // inside/outside transition
	  trackLineTimeout = millis() + 1500;
  }
  lastPerimeterMag = mag;*/
}


void RobotClass::mow(){  
  if (!Perimeter.isInside(IDX_LEFT)) sensorTriggered(SEN_PERIMETER_LEFT);
  if (!Perimeter.isInside(IDX_RIGHT)) sensorTriggered(SEN_PERIMETER_RIGHT);
	switch (mowState){
    case MOW_ROTATE:
      if (Motor.motion == MOT_STOP){
        if (mowPattern == PATTERN_LANES){
          Motor.travelLineDistance(15, rotateAngle, 1.0);
          mowState = MOW_ENTER_LINE;
          //DEBUGLN(F("MOW_ENTER_LINE"));
        } else {
          mowState = MOW_LINE;
          Motor.travelLineDistance(100000, mowingAngle, 1.0);
          //DEBUGLN(F("MOW_LINE"));
        }
      }
      break;
    case MOW_REV:
      if (Motor.motion == MOT_STOP){
        if (mowPattern == PATTERN_LANES){
          unsigned long duration = millis()-lastStartLineTime;
          DEBUG(F("duration="));
          DEBUGLN(duration);
          if (duration < 5000){
            DEBUGLN(F("new lane direction"));
            mowingAngle = scalePI( mowingDirection + PI );
            mowingDirection = mowingAngle-PI/2;
          } else {
            mowingAngle = scalePI( mowingAngle + PI );
          }
          float enterDelta = PI/4;
          float yaw = IMU.getYaw();
          float deltaAngle = distancePI(mowingAngle, mowingDirection); // w-x
          if (deltaAngle > 0) rotateAngle = mowingAngle + enterDelta;
            else rotateAngle = mowingAngle - enterDelta;
        } else {
          mowingAngle = scalePI( mowingAngle + PI + ((float)random(-90, 90)/180.0*PI ));
          rotateAngle = mowingAngle;
        }
        Motor.rotateAngle(rotateAngle, rotationSpeedPerc);
        mowState = MOW_ROTATE;
        //DEBUGLN(F("MOW_ROTATE"));
      }
      break;
    case MOW_ENTER_LINE:
      if (Motor.motion == MOT_STOP){
        Motor.travelLineDistance(100000, mowingAngle, 1.0);
        mowState = MOW_LINE;
        //DEBUGLN(F("MOW_LINE"));
        lastStartLineTime = millis();
      }
      break;
    case MOW_LINE:
      //if (!Perimeter.isInside()) Motor.stopSlowly();
      if ( (!Perimeter.isInside()) || (Motor.motion == MOT_STOP)  ){              
				Motor.stopImmediately();
        Motor.travelLineDistance(50, mowingAngle, -reverseSpeedPerc);
        mowState = MOW_REV;
        //DEBUGLN(F("MOW_REV"));
      }
      break;
  }
}


void RobotClass::stateMachine(){    
		switch (state){
      case STAT_CAL_GYRO:	      
	      break;
      case STAT_IDLE:
        break;
      case STAT_MOW:
        mow();
        break;      
      case STAT_CREATE_MAP:
	    case STAT_TRACK:
        track();
        break;
    }
}

String RobotClass::getStateName(){
  return String(robotStateNames[state]);
}

void RobotClass::sendPerimeterOutline(){
  ROBOTMSG.print(F("!05"));        
  for (int i=0; i < Map.perimeterOutlineSize; i++){
    robot_state_t pt = Map.outlineParticles[i];
    ROBOTMSG.print(F(","));  
    ROBOTMSG.print(pt.x);
    ROBOTMSG.print(F(","));  
    ROBOTMSG.print(pt.y);    
  }  
  ROBOTMSG.println();
}

void RobotClass::sendParticles(){
  ROBOTMSG.print(F("!15,"));  
  for (int i=0; i < OUTLINE_PARTICLES; i++){    
    ROBOTMSG.print(Map.outlineParticles[i].x);      
    ROBOTMSG.print(F(","));                     
    ROBOTMSG.print(Map.outlineParticles[i].y);      
    if (i < OUTLINE_PARTICLES-1) ROBOTMSG.print(",");                     
  }  
  ROBOTMSG.println();  
}

void RobotClass::sendMap(){
  ROBOTMSG.print(F("!03,"));    
  ROBOTMSG.print(Map.mapScaleX);  
  ROBOTMSG.print(F(","));  
  ROBOTMSG.print(Map.mapScaleY);  
  ROBOTMSG.print(F(","));  
  ROBOTMSG.print(MAP_SIZE_X);
  ROBOTMSG.print(F(","));
  ROBOTMSG.print(MAP_SIZE_Y);  
  ROBOTMSG.print(F(","));
  map_data_t d;
  byte r,g,b,col;
  for (int y=0; y < MAP_SIZE_Y; y++){    
    for (int x=0; x < MAP_SIZE_X; x++){        
	    d = Map.mapData[y][x];		
		  if (d.s.state == MAP_DATA_STATE_MOWED){
          r=0; g=255; b=0;		  
      } else {
          col = 255.0*1.0/((float)(32-d.s.signal));         
          if (d.s.side == 1) {
            r=255; g=255-col; b=255-col;
			      //r=255; g=0; b=0;
          } else {
            r=255-col; g=255-col; b=255;
			      //r=0; g=0; b=255;
		      }
      }
		  //ROBOTMSG.write(Map.mapData[y][x].v);      
		  ROBOTMSG.print(r);      
		  ROBOTMSG.print(F(","));      		
		  ROBOTMSG.print(g);      
		  ROBOTMSG.print(F(","));      		
		  ROBOTMSG.print(b);      		
		  ROBOTMSG.print(F(","));      				
    }   
	  //ROBOTMSG.flush();
    //delay(100); 
  }  
  ROBOTMSG.println();
}


void RobotClass::receiveEEPROM_or_ERASE(){
  boolean received = false;
  int addr;
  byte data;
  int cmd;
  char ch;
  
  // wait for serial data (Arduino IDE ERASE command or PC EEPROM data) 
  while (millis() < 1000);  
  
  Flash.verboseOutput=false;    
  while (ROBOTMSG.available()){                           
    char ch = ROBOTMSG.read();    
    switch (ch){     
      // Arduino Due IDE sends:  €€#N#w00000000,4#  ( https://sourceforge.net/p/lejos/wiki-nxt/SAM-BA%20Protocol/ )
      case 'N': 
        ch = ROBOTMSG.read();
        if (ch == '#'){          
            #ifndef __AVR__                       
              initiateReset(1);   // Arduino Due ERASE trigger			  
			  tickReset();  // https://forums.adafruit.com/viewtopic.php?f=19&t=47844&start=30
            #endif
            while(true);
        }
        break;
      case '?':               
        cmd = ROBOTMSG.parseInt();                
        switch (cmd) {                    
          case 76: addr  = ROBOTMSG.parseInt();
                   data  = ROBOTMSG.parseInt();                   
                   Flash.write(addr, data);                   
                   /*DEBUG(addr);
                   DEBUG(F("="));
                   DEBUG(data);
                   DEBUGLN(F(", "));*/
                   received = true;
                   break;          
        }
        break;      
    }                 
    if (!ROBOTMSG.available()) delay(200);
  }
  Flash.verboseOutput=true;
  if (received) DEBUGLN(F("EEPROM received"));  
    else DEBUGLN(F("ERROR receiving EEPROM"));  
  //Flash.dump();
}


void RobotClass::sensorTriggered(uint16_t sensorID){
	sensorTriggerStatus |= sensorID;	
}



