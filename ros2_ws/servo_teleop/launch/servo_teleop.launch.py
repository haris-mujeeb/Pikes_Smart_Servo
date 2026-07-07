from launch_ros.actions import Node

from launch import LaunchDescription


def generate_launch_description():
  return LaunchDescription([
    Node(
      package='servo_teleop',
      executable='twist_to_mqtt',
      name='twist_to_mqtt_bridge',
      output='screen'
    ),
    
    Node(
      package='teleop_twist_keyboard',
      executable='teleop_twist_keyboard',
      name='teleop_twist_keyboard',
      output='screen',
      prefix='xterm -e'
    )
  ])