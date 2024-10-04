#include "tcp_client.h"
#include <io.h>

int main()
{
    Client client{ "192.168.1.10", 8081 };
    client.connectToServer();
}
