FROM osrf/ros:humble-desktop

RUN apt-get update && apt-get install -y \
    python3-pip \
    xterm \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install paho-mqtt

ENV ROS_WS=/ros2_ws
WORKDIR $ROS_WS

COPY src/ $ROS_WS/src/

# build the ROS 2 workspace
RUN /bin/bash -c "source /opt/ros/humble/setup.bash && colcon build"

# to source ros2 env on start
COPY entrypoint.sh /
ENTRYPOINT ["/entrypoint.sh"]

# run launch file bv default
CMD ["ros2", "launch", "servo_teleop", "servo_teleop.launch.py"]