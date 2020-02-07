#pragma once

#include <cstdint>


class Usb {
public:
    static void init();

    static bool checkConnect();

    static bool send(const void * data, uint32_t dataLen);

    static uint32_t receive(const void * data, uint32_t bufLen);

    enum Endpoints {
        epOutCdc = 0x01,
        epInCdc = 0x82,
        epCdcNotify = 0x83,
    };

};
