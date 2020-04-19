/*  Neonode zForce v7 interface library for Arduino

    Copyright (C) 2019 Neonode Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include <Zforce.h>

#define DATA_READY PIN_NN_DR
uint8_t counter = 10;   // print header every 10 lines to improve read-ability

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.begin(115200);
    while (!Serial) // 10Hz blink to indicate USB connection is waiting to establish
    {
        digitalWrite(LED_BUILTIN, LOW);
        delay(50);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(50);
    }
    digitalWrite(LED_BUILTIN, HIGH);
    
    zforce.Start(DATA_READY);

    Message *msg = zforce.GetMessage();
    
    if (msg != NULL)
    {
        Serial.println("Received Boot Complete Notification");
        Serial.print("Message type is: ");
        Serial.println((int)msg->type);
        zforce.DestroyMessage(msg);
    }

    // Send and read ReverseX
    zforce.ReverseX(false);

    do
    {
        msg = zforce.GetMessage();
    } while (msg == NULL);

    if (msg->type == MessageType::REVERSEXTYPE)
    {
        Serial.println("Received ReverseX Response");
        Serial.print("Message type is: ");
        Serial.println((int)msg->type);
    }

    zforce.DestroyMessage(msg);

    // Send and read ReverseY
    zforce.ReverseY(false);

    do
    {
        msg = zforce.GetMessage();
    } while (msg == NULL);

    if (msg->type == MessageType::REVERSEYTYPE)
    {
        Serial.println("Received ReverseY Response");
        Serial.print("Message type is: ");
        Serial.println((int)msg->type);
    }

    zforce.DestroyMessage(msg);

    // Send and read Touch Active Area
    zforce.TouchActiveArea(0, 0, 4000, 4000);

    do
    {
        msg = zforce.GetMessage();
    } while (msg == NULL);

    if (msg->type == MessageType::TOUCHACTIVEAREATYPE)
    {
        Serial.print("minX is: ");
        Serial.println(((TouchActiveAreaMessage *)msg)->minX);
        Serial.print("minY is: ");
        Serial.println(((TouchActiveAreaMessage *)msg)->minY);
        Serial.print("maxX is: ");
        Serial.println(((TouchActiveAreaMessage *)msg)->maxX);
        Serial.print("maxY is: ");
        Serial.println(((TouchActiveAreaMessage *)msg)->maxY);
    }

    zforce.DestroyMessage(msg);

    // Send and read Enable
    zforce.Enable(true);

    msg = zforce.GetMessage();

    do
    {
        msg = zforce.GetMessage();
    } while (msg == NULL);

    if (msg->type == MessageType::ENABLETYPE)
    {
        Serial.print("Message type is: ");
        Serial.println((int)msg->type);
        Serial.println("Sensor is now enabled and will report touches.");
    }

    zforce.DestroyMessage(msg);
}

void loop()
{
    Message *touch = zforce.GetMessage();
    if (touch != NULL)
    {
        if (touch->type == MessageType::TOUCHTYPE)
        {
            if (++counter > 9)
            {
                counter = 0;
                Serial.println("\nID ---\tEvent -\tX ---\tY ---");
            }
            for (uint8_t i = 0; i < ((TouchMessage *)touch)->touchCount; i++)
            {
                Serial.print(((TouchMessage *)touch)->touchData[i].id);
                Serial.print("\t");
                Serial.print(((TouchMessage *)touch)->touchData[i].event);
                Serial.print("\t");
                Serial.print(((TouchMessage *)touch)->touchData[i].x);
                Serial.print("\t");
                Serial.println(((TouchMessage *)touch)->touchData[i].y);
            }
        }
        zforce.DestroyMessage(touch);
    }
}
