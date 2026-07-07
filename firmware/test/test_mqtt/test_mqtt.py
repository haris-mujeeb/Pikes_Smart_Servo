import json
import time

import paho.mqtt.client as mqtt

BROKER_HOST = "broker.hivemq.com"
BROKER_PORT = 1883
CMD_TOPIC = "/pikes/servo/cmd_vel"

TEST_PAYLOAD = json.dumps({"velocity": 15.0})

def on_connect(client, userdata, flags, reason_code, properties):
    if reason_code == 0:
        print(f"[INFO] Connected to broker at {BROKER_HOST}:{BROKER_PORT}")
    else:
        print(f"[ERROR] Connection failed with reason code: {reason_code}")

def on_publish(client, userdata, mid, reason_code, properties):
    print(f"[INFO] Payload transmitted successfully (Message ID: {mid})")

def run_test_transmission():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = on_connect
    client.on_publish = on_publish

    print("[INFO] Initializing MQTT client sequence...")
    client.connect(BROKER_HOST, BROKER_PORT, 60)
    
    client.loop_start()
    time.sleep(1)

    print(f"[INFO] Beginning teleop transmission sequence to '{CMD_TOPIC}'...")
    
    try:
      sequence = 1
      while True:
        print(f"[INFO] Dispatching payload #{sequence}: {TEST_PAYLOAD}")
        client.publish(CMD_TOPIC, TEST_PAYLOAD, qos=1)
        time.sleep(1)
                  
    except KeyboardInterrupt:
        print("\n[WARN] Test interrupted by local user.")
        
    finally:
        print("[INFO] Tearing down MQTT client.")
        client.loop_stop()
        client.disconnect()
        print("[INFO] Test routine exited cleanly.")

if __name__ == "__main__":
    run_test_transmission()