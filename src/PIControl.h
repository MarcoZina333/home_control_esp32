#include "Arduino.h"

#define MIN_PWM 0 
#define MAX_PWM 255

class PIControl {
   double* output;
   double* input;
   double* desired;
   unsigned long lastTime;
   double errSum, lastErr;
   double kp, ki;
   
   public:

   PIControl(double* _input, double* _output, double* _desired,
        double _kp, double _ki, int sampleTime);

   public:

   void compute();

   public:
    
   void setTunings(double _kp, double _ki);

};