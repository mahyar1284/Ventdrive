#define SERIAL_RX_BUFFER_SIZE 4096
#define SERIAL_TX_BUFFER_SIZE 4096
#include "SystemFacade.hpp"

#define ID1 123456789
#define ID2 987654321

void setup() 
{
    delay(500); // startup delay
    SystemFacade system(ID2);
    system.begin();
    while(true) 
        system.loop();
}
void loop() {}