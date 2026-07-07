#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

void mqtt_start(const char *broker_uri); // MQTT broker: mqtt://127.0.0.1

float mqtt_get_servo_rot_vel(void);

void mqtt_pub_telemetry(float angle, float torque);

#endif // MQTT_MANAGER_H