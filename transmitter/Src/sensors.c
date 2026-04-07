#include "sensors.h"

int readDataFile(QueueHandle_t queue, uint16_t* cur_seq, const char* fileName, 
                PacketType_e type, ParseLineFn parseLine, void* data) {
    char line[128]; //buffer to hold a row in the CSV file
    int count = 0;
    TelemetryPacket_t packet;

    FILE* f = fopen(fileName, "r");
    if (f == NULL) {
        fprintf(stderr, "Error: could not open %s\n", fileName);
        return -1;
    } 

    fgets(line, sizeof(line), f); //Skip header
    while (fgets(line, sizeof(line), f)) {
        if (parseLine(line, data) == 0) {
            createPacket(&packet, &data, type, cur_seq);
            //copy the packet onto the queue
            xQueueSend(queue, &packet, portMAX_DELAY);
            count++;
        }
    }
    
    if (ferror(f) || count == 0) {
        perror("Error reading from file ");
        fclose(f);
        return -1;
    }

    fclose(f);
    return count;
}


int parseGNSSData(const char* line, void* data) {
    GNSSData_t* d = (GNSSData_t*)data;
    return sscanf(line, "%f,%lf,%lf,%f",   // %lf for double
                  &d->time, &d->latitude, &d->longitude, &d->altitude) == 4 ? 0 : -1;
}

int parseBarometerData(const char* line, void* data) {
    BarometerData_t* d = (BarometerData_t*)data;
    return sscanf(line, "%lu,%hd", &d->pressure, &d->temperature) == 2 ? 0 : -1;
}

int parseIMUData(const char* line, void* data) {
    IMUData_t* d = (IMUData_t*)data;
    return sscanf(line, "%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd", 
            &d->accelX, &d->accelY, &d->accelZ, 
            &d->angularVelX, &d->angularVelY, &d->angularVelZ, 
            &d->magX, &d->magY, &d->magZ) == 9 ? 0 : -1;
}

int parseBatteryData(const char* line, void* data) {
    BatteryData_t* d = (BatteryData_t*)data;
    return sscanf(line, "%hhu:%hhu,%hhu,%7s", &d->hours, &d->minutes, &d->percent, d->status) == 4 ? 0 : -1;
}
