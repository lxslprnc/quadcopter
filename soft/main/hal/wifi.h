#pragma once

#include <string>
#include <esp_err.h>
#include <esp_event.h>

#include <data_model/data_ressources_registry.h>

using namespace std;

class Wifi
{

  private:

    typedef enum
    {
        OFF,
        STARTED,
        CONNECTED
    }State;

    State                    _state;
    bool                     _connection_request;
    bool                     _disconnection_request;
    DataRessourcesRegistry * _registry;

    static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

public :

    Wifi(DataRessourcesRegistry * registry);

    void connect();

    void disconnect();
};