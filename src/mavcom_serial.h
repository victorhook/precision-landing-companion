#include "mavcom.h"


class MavComSerial : public MavCom
{
    public:
        void doInit() override;
};