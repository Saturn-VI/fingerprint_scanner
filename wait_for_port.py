Import("env")
import os
import subprocess
import time


def wait_for_serial_permissions(source, target, env):
    port = env.subst("$UPLOAD_PORT") or "/dev/ttyACM0"
    print(f"Waiting for {port} to re-enumerate with user permissions...")
    deadline = time.time() + 5.0
    while time.time() < deadline:
        if os.path.exists(port):
            subprocess.run(["udevadm", "settle", "-t", "2"], check=False)
            if os.access(port, os.R_OK | os.W_OK):
                return
        time.sleep(0.1)


env.AddPostAction("upload", wait_for_serial_permissions)
