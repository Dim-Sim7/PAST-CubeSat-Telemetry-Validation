#include "telemetry.h"

extern const BatteryData_t batteryData[];
extern const BarometerData_t baroData[];
extern const IMUData_t imuData[];
extern const GNSSData_t gnssData[];

#define DATA_COUNT 50 /* 50 points of data in each data set */