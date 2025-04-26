#ifndef SENSORS_H
#define SENSORS_H

typedef struct {
    float voltage1;
    float voltage2;
    float voltage3;
    float voltage4;
    float current1;
    float current2;
    float current3;
    float current4;
    float velocity;
} mesurements_t;

void sensors_init(void);
mesurements_t sensors_read_all(void);
char* measurements_to_json(const mesurements_t *m);

#endif // SENSORS_H