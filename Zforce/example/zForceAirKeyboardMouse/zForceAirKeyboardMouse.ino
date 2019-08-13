#include <Arduino.h>
#include <Keyboard.h>
#include <Mouse.h>
#include "Zforce.h"
#include "Settings.h"

// #define Serial SerialUSB

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

long globalMillis = 0;
const int holdTime = 80;
const int keyboardBoundary = 1100;

void loopMouse()
{
    if(tp.loc.x > keyboardBoundary)
    {
        return;
    }

    if (tp.state == 0)
    {
        previousLoc.x = initialLoc.x = tp.loc.x;
        previousLoc.y = initialLoc.y = tp.loc.y;
        globalMillis = millis();
    }
    else
    {
        if (tp.state == 1)
        {
            if(previousLoc.x == tp.loc.x && previousLoc.y == tp.loc.y)
            {

            }
            else if (tp.state == 1 && tp.loc.x <= keyboardBoundary)
            {
                if((millis() - globalMillis) >= holdTime)
                {
                    Mouse.move((tp.loc.x - previousLoc.x) / 1, (tp.loc.y - previousLoc.y) / 1);
                }
                previousLoc.x = tp.loc.x;
                previousLoc.y = tp.loc.y;
            }
        }
        else if (tp.state == 2)
        {
            if(millis() - globalMillis < holdTime)
            {
                Mouse.click(MOUSE_LEFT);
            }
            // globalMillis = millis();
            tp.state = 0xFF;
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
        if (tp.loc.x > keyboardBoundary)
        {
            if (tp.loc.y < 250)
            {
                Keyboard.print('A');
            }
            else if (tp.loc.y < 500)
            {
                Keyboard.print('B');
            }
            else if (tp.loc.y < 750)
            {
                Keyboard.print('C');
            }
            else if (tp.loc.y < 1000)
            {
                Keyboard.print('D');
            }
            else
            {
                Keyboard.print('E');
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

#ifdef DEBUG
            Serial.print(((TouchMessage *)msg)->touchData[0].x);
            Serial.print("\t");
            Serial.print(((TouchMessage *)msg)->touchData[0].y);
            Serial.print("\t");
            Serial.println(((TouchMessage *)msg)->touchData[0].event);
#endif
        }
    }
    touch.DestroyMessage(msg);
}

void setup()
{
    Serial.begin(115200);

#ifdef DEBUG
    for (int i = 0; i < 100; i++)
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
