# 初步验证能否跑通

import glob
import os
import sys

try:
    sys.path.append(glob.glob("PythonAPI/carla/dist/carla-*%d.%d-%s.egg" % (
        sys.version_info.major,
        sys.version_info.minor,
        "linux-x86_64"))[0])
except IndexError:
    pass

import carla

client = carla.Client("127.0.0.1", 2000)
client.set_timeout(10.0)

world = client.get_world()
bps = world.get_blueprint_library().filter("sensor.other.safe_distance")

print("Found:", len(bps))
for bp in bps:
    print(bp.id)
    for attr in bp:
        print(" ", attr.id, attr.type, list(attr.recommended_values))