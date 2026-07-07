import json

import paho.mqtt.client as mqtt
import rclpy
from rclpy.node import Node
from std_msgs.msg import Float64


class MqttToRos(Node):
    def __init__(self):
        super().__init__('mqtt_to_ros_telemetry')
        
        # Match the broker and topic from your working Twist script
        self.broker_host = "broker.hivemq.com"
        self.broker_port = 1883
        self.telemetry_topic = "/pikes/servo/telemetry"
        
        # ROS 2 Publishers
        self.angle_pub = self.create_publisher(Float64, '/pikes/servo/angle', 10)
        self.torque_pub = self.create_publisher(Float64, '/pikes/servo/torque_sim', 10)
        
        # Initialize MQTT Client using the v2 API (Standard TCP, no WebSockets)
        self.mqtt_client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
        self.mqtt_client.on_connect = self.on_connect
        self.mqtt_client.on_message = self.on_message
        
        self.get_logger().info(f"Connecting to MQTT Broker at {self.broker_host}:{self.broker_port}...")
        
        # Standard TCP connection
        self.mqtt_client.connect(self.broker_host, self.broker_port, 60)
        self.mqtt_client.loop_start()

    def on_connect(self, client, userdata, flags, reason_code, properties):
        if reason_code == 0:
            self.get_logger().info("Successfully connected to MQTT broker!")
            self.get_logger().info(f"Subscribing to MQTT '{self.telemetry_topic}'. Forwarding to ROS 2...")
            self.mqtt_client.subscribe(self.telemetry_topic)
        else:
            self.get_logger().error(f"MQTT connection failed with reason code: {reason_code}")

    def on_message(self, client, userdata, msg):
        try:
            payload = msg.payload.decode('utf-8')
            data = json.loads(payload)
            
            # Publish Angle
            if "angle" in data:
                angle_msg = Float64()
                angle_msg.data = float(data["angle"])
                self.angle_pub.publish(angle_msg)
                
            # Publish Simulated Torque
            if "torque" in data:
                torque_msg = Float64()
                torque_msg.data = float(data["torque"])
                self.torque_pub.publish(torque_msg)
                
        except json.JSONDecodeError:
            self.get_logger().warn(f"Failed to parse incoming JSON telemetry: {payload}")
        except Exception as e:
            self.get_logger().error(f"Error processing message: {e}")

    def destroy_node(self):
        self.get_logger().info("Tearing down MQTT client...")
        self.mqtt_client.loop_stop()
        self.mqtt_client.disconnect()
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args)
    node = MqttToRos()
    
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        node.get_logger().info("Node stopped by user.")
    finally:
        node.destroy_node()
        rclpy.try_shutdown()

if __name__ == '__main__':
    main()