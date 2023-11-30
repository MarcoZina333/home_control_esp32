#include "Arduino.h"
#include "PIControl.h"

#define MIN_PWM 0 
#define MAX_PWM 255

   //classe che funge da controllore prop. e integrale per sistemi SISO retroazionati
   //implementati su framework arduino

PIControl::PIControl(double* _input, double* _output, double* _desired,
      double _kp, double _ki, int sampleTime)
{
   output = _output;
   input = _input;
   desired = _desired;		//default output limit corresponds to
   
   errSum = *desired - *input;

   kp = _kp;
   ki = _ki;
   lastTime = millis();

}

void PIControl::compute()
{
   /*How long since we last calculated*/
   unsigned long now = millis();
   double timeChange = (now - lastTime)/1000.0;

   /*Compute all the working error variables*/
   double error = *desired - *input;
   errSum += (error * timeChange);

   /*Compute PI output*/
   double tempOut = kp * error + ki * errSum;

   //Serial.printf("Dimming value pre: %.2f \n", tempOut);
   if (tempOut < MIN_PWM){
      tempOut = MIN_PWM;
      errSum -= (error * timeChange);
   }
   else if (tempOut > MAX_PWM) {
      tempOut = MAX_PWM;
      errSum -= (error * timeChange);
   }
   /*Remember some variables for next time*/
   *output = tempOut;
   lastTime = now;
}
   
void PIControl::setTunings(double _kp, double _ki)
{
   kp = _kp;
   ki = _ki;
}
