#include <Keyboard.h>
#include <Mouse.h>
#include <Zforce.h>

#define TOUCH_BUFFER_SIZE 1 // must not exeed 4

typedef struct Location
{
    uint16_t x;
    uint16_t y;
} Location;

typedef struct TouchPoint
{
    Location loc;
    uint8_t state;
} TouchPoint;

Zforce zf = Zforce();
uint8_t nTouches = 0;
TouchPoint tp;
Location previousLoc;
Location initialLoc;
int previousButtonState = HIGH; // for checking the state of a pushButton
volatile bool newTouchDataFlag = false;

void dataReadyISR() { newTouchDataFlag = true; }

bool isDataReady() { return digitalRead(PIN_NN_DR) == HIGH; }

bool touchInit()
{
    zf.Start(PIN_NN_DR);
    pinMode(PIN_NN_DR, INPUT_PULLDOWN);
    if (isDataReady())
    {
        Message *msg = zf.GetMessage();
        if (msg->type == MessageType::BOOTCOMPLETETYPE)
            Serial.println(F("Sensor connected"));
        else
            Serial.println(F("Unexpected senosr message"));
        zf.DestroyMessage(msg);
    }

    zf.Enable(true); // Send and read Enable
    uint16_t timeout = 500;
    while (!isDataReady() && timeout--)
        ;
    if(timeout == 0)
        return false;
    Message *msg = zf.GetMessage();
    if (msg == NULL)
        return false;
    zf.DestroyMessage(msg);
    newTouchDataFlag = false;   // clear flag
    return true;
}

long globalMillis = 0;
const int holdTime = 80;
const int keyboardBoundary = 1100;

void loopMouse()
{
    if(tp.loc.x > keyboardBoundary)
        return;

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
            if (tp.state == 1 && tp.loc.x <= keyboardBoundary)
            {
                if((millis() - globalMillis) >= holdTime)
                    Mouse.move((tp.loc.x - previousLoc.x) / 1, (tp.loc.y - previousLoc.y) / 1);
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
            tp.state = 0xFF;
        }
    }
}

void loopKeyboard()
{
    int buttonState = tp.state < 2; // read the pushbutton:

    if ((buttonState != previousButtonState) && (buttonState == HIGH))
    {   // if the button state has changed, and it's currently pressed
        if (tp.loc.x > keyboardBoundary)
        {
            if (tp.loc.y < 250)
                Keyboard.print('A');
            else if (tp.loc.y < 500)
                Keyboard.print('B');
            else if (tp.loc.y < 750)
                Keyboard.print('C');
            else if (tp.loc.y < 1000)
                Keyboard.print('D');
            else
                Keyboard.print('E');
        }
        else
        {
            // May do something to catch the rest of the cases
        }
    }
    previousButtonState = buttonState;  // save the current button state for comparison next time
}

uint8_t updateTouch()
{
    if (newTouchDataFlag == false)
        return 0;

    newTouchDataFlag = false;
    Message *touch = zf.GetMessage();
    if (touch != NULL)
    {
        if (touch->type == MessageType::TOUCHTYPE)
        {
            auto size = ((TouchMessage *)touch)->touchCount;
            nTouches = size >= TOUCH_BUFFER_SIZE ? TOUCH_BUFFER_SIZE : size;
            for (uint8_t i = 0; i < nTouches; i++)
            {
                tp.loc.x = ((TouchMessage *)touch)->touchData[0].x;
                tp.loc.y = ((TouchMessage *)touch)->touchData[0].y;
                tp.state = ((TouchMessage *)touch)->touchData[0].event;
            }
            zf.DestroyMessage(touch);
            return nTouches;
        }
        zf.DestroyMessage(touch);
    }
    return -1;
}

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    
    Keyboard.begin(); // initialize control over the keyboard
    Mouse.begin();
    if(!touchInit())
        Serial.println(F("Touch init failed"));
    
    attachInterrupt(digitalPinToInterrupt(PIN_NN_DR), dataReadyISR, RISING);
}

void loop()
{
    digitalWrite(LED_BUILTIN, LOW);
    updateTouch();
    digitalWrite(LED_BUILTIN, HIGH);
    loopKeyboard();
    loopMouse();
}
