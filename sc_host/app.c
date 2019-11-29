#include <app.h>
#include <host_gecko.h>


bool sc_host_bootup = false;

void ble_system_reset(int r)
{
    sc_host_bootup = false;
    gecko_cmd_system_reset(r);
}

bool ble_is_bootup()
{
    return sc_host_bootup;
}

void ble_bootup_done()
{
    sc_host_bootup = true;
}
