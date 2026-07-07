import json

import paho.mqtt.client as mqtt
import rclpy
from geometry_msgs.msg import Twist
from rclpy.node import Node


class TwistToMqttBridge(Node):
    def __init__(self):
        super().__init__('twist_to_mqtt_bridge')
        
        # match the broker and topic from your ESP32 / working script
        self.broker_host = "broker.hivemq.com"
        self.broker_port = 1883
        self.cmd_topic = "/pikes/servo/cmd_vel"
        
        # initialize MQTT Client using the v2 API
        self.mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        self.mqtt_client.on_connect = self.on_connect
        
        self.get_logger().info(f"Connecting to MQTT Broker at {self.broker_host}:{self.broker_port}...")
        
        # TCP connection
        self.mqtt_client.connect(self.broker_host, self.broker_port, 60)
        self.mqtt_client.loop_start()

        # ROS 2 Subscription to /cmd_vel
        self.subscription = self.create_subscription(
            Twist,
            '/cmd_vel',
            self.cmd_vel_callback,
            10
        )
        self.get_logger().info(f"Subscribed to ROS 2 '/cmd_vel'. Forwarding to MQTT '{self.cmd_topic}'...")

    def on_connect(self, client, userdata, flags, reason_code, properties):
        if reason_code == 0:
            self.get_logger().info("Successfully connected to MQTT broker!")
        else:
            self.get_logger().error(f"MQTT connection failed with reason code: {reason_code}")

    def cmd_vel_callback(self, msg):
        # get velocity from the Twist message.        
        velocity = float(msg.angular.z * 15.0) # rotation keys: J/L
        
        # send the JSON payload
        payload_dict = {"velocity": velocity}
        payload_json = json.dumps(payload_dict)
        
        self.mqtt_client.publish(self.cmd_topic, payload_json, qos=1)
        
        # use `ros2 run --ros-args --log-level debug` to see it
        self.get_logger().debug(f"Transmitted: {payload_json}")

    def destroy_node(self):
        self.get_logger().info("Tearing down MQTT client...")
        self.mqtt_client.loop_stop()
        self.mqtt_client.disconnect()
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args)
    node = TwistToMqttBridge()
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info("Node stopped by user.")
    finally:
        node.destroy_node()
        rclpy.try_shutdown()

if __name__ == '__main__':
    main()