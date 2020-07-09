#!/usr/bin/env python
# -*- coding: utf-8 -*-
# This is an extension of the example provided by bitcraze:
# - added raw format
# - added color
# - added pipe as transport

import rospy
from cv_bridge import CvBridge
from sensor_msgs.msg import Image
from client import Client


def main() -> None:
    rospy.init_node('himax_driver', anonymous=True)
    image_pub = rospy.Publisher("image_raw", Image, queue_size=1)
    bridge = CvBridge()
    pipe = rospy.get_param('~pipe', '')
    host = rospy.get_param('~host', '')
    port = rospy.get_param('~port', 5000)
    client = Client(host=host, port=port, pipe=pipe, keep_track_of_fps=False)
    for seq, frame in enumerate(client.run()):
        msg = bridge.cv2_to_imgmsg(frame)
        msg.header.stamp = rospy.Time.now()
        msg.header.seq = seq
        image_pub.publish(msg)
        rospy.sleep(0)


if __name__ == '__main__':
    main()
