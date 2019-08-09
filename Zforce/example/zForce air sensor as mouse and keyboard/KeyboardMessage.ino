#include <Keyboard.h>
#include <Mouse.h>
#include <Arduino.h>
#include "Zforce.h"
//#include "Ticker.h"
#include "Settings.h"

int previousButtonState = HIGH; // for checking the state of a pushButton
Zforce touch = Zforce();
const int DATA_READY = 5;
TouchPoint tp;
Location previousLoc;
Location initialLoc;

void touchInit()
{
  touch.Start(DATA_READY);
  Serial.println("Starting touch initialization...");

  Message *msg = touch.GetMessage();
  if (msg != NULL)
  {
    Serial.println(F("zForce touch sensor detected"));
  }
  touch.DestroyMessage(msg);

//  touch.ReverseX(true); // Send and read ReverseX
//  do
//  {
//    msg = touch.GetMessage();
//  } while (msg == NULL);
//
//  if (msg->type == MessageType::REVERSEXTYPE)
//  {
//    // Sprintln(F("zForce touch sensor reverse X axis"));
//  }
//  touch.DestroyMessage(msg);
//
//  uint8_t changeFreq[] = {0xEE, 0x0B, 0xEE, 0x09, 0x40, 0x02, 0x02, 0x00, 0x68, 0x03, 0x80, 0x01, 0x32};
//  touch.Write(changeFreq);
//  do
//  {
//    msg = touch.GetMessage();
//  } while (msg == NULL);
//  touch.DestroyMessage(msg);
//  // Sprintln(F("Changed frequency"));

  touch.Enable(true); // Send and read Enable
  do
  {
    msg = touch.GetMessage();
  } while (msg == NULL);

  if (msg->type == MessageType::ENABLETYPE)
  {
    Serial.println(F("zForce touch sensor is ready"));
  }
  touch.DestroyMessage(msg);
}

void loopMouse()
{
  if (tp.state == 0)
  {
    previousLoc.x = initialLoc.x = tp.loc.x;
    previousLoc.y = initialLoc.y = tp.loc.y;
  }
  else
  {
    if (previousLoc.x == tp.loc.x && previousLoc.y == tp.loc.y)
    {
    }
    else if (tp.state == 1 && tp.loc.x <= 1100)
    {
      Mouse.move((tp.loc.x-previousLoc.x)/1, (tp.loc.y-previousLoc.y)/1);
      previousLoc.x = tp.loc.x;
      previousLoc.y = tp.loc.y;
    }
  }
}

void loopKeyboard()
{
  int buttonState = tp.state < 2; // read the pushbutton:
  // if the button state has changed,
  if ((buttonState != previousButtonState)
      // and it's currently pressed:
      && (buttonState == HIGH))
  {
    // type out a message
    if(tp.loc.x > 1100)
    {
      if(tp.loc.y < 250)
      {
        Keyboard.print('1');
      }
      else if(tp.loc.y < 500)
      {
        Keyboard.print('2');
      }
      else if(tp.loc.y < 750)
      {
        Keyboard.print('3');
      }
      else
      {
        Keyboard.print('4');
      }
    }
    else
    {
//      Keyboard.print('E');
    }
  }
  // save the current button state for comparison next time:
  previousButtonState = buttonState;
}

void getTouch()
{
  Message *msg = touch.GetMessage();
  if (msg != NULL)
  {
    if (msg->type == MessageType::TOUCHTYPE)
    {
      // do something with the data touchData
      tp.loc.x = ((TouchMessage *)msg)->touchData[0].x;
      tp.loc.y = ((TouchMessage *)msg)->touchData[0].y;
      tp.state = ((TouchMessage *)msg)->touchData[0].event;

//      #ifdef DEBUG
      Serial.print(((TouchMessage *)msg)->touchData[0].x);
      Serial.print("\t");
      Serial.print(((TouchMessage *)msg)->touchData[0].y);
      Serial.print("\t");
      Serial.println(((TouchMessage *)msg)->touchData[0].event);
//      #endif
    }
  }
  touch.DestroyMessage(msg);
}

void setup()
{
  Serial.begin(115200);
  
#ifdef DEBUG
  for(int i = 0; i < 100; i++)
  {
    Serial.print("Program starts ");
    Serial.print(i);
    Serial.println(" ...");
    delay(50);
  }
#endif
  
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  touchInit();
  
  Keyboard.begin(); // initialize control over the keyboard
  Mouse.begin();
}

void loop()
{
  digitalWrite(LED_BUILTIN, LOW);
  getTouch();
  digitalWrite(LED_BUILTIN, HIGH);
  loopKeyboard();
  loopMouse();
}

