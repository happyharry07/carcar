import argparse
import logging
import sys
import time

from maze import Action, Maze
from score import ScoreboardServer, ScoreboardFake
from hm10_esp32 import HM10ESP32Bridge

EXPECTED_NAME = "sixcar"

logging.basicConfig(
    format="%(asctime)s - %(name)s - %(levelname)s - %(message)s", level=logging.INFO
)
log = logging.getLogger(__name__)

TEAM_NAME = "carcarcarcar"
SERVER_URL = "http://carcar.ntuee.org/scoreboard"
MAZE_FILE = "data/medium_maze.csv"
BT_PORT = "COM7"


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", help="0: treasure-hunting, 1: self-testing", type=str)
    parser.add_argument("--maze-file", default=MAZE_FILE, help="Maze file", type=str)
    parser.add_argument("--bt-port", default=BT_PORT, help="Bluetooth port", type=str)
    parser.add_argument("--team-name", default=TEAM_NAME, help="Your team name", type=str)
    parser.add_argument("--server-url", default=SERVER_URL, help="Server URL", type=str)
    return parser.parse_args()


def setup_bridge(port: str, expected_name: str):
    bridge = HM10ESP32Bridge(port=port)

    current_name = bridge.get_hm10_name()
    if current_name != expected_name:
        print(f"Target mismatch. Current: {current_name}, Expected: {expected_name}")
        print(f"Updating target name to {expected_name}...")

        if bridge.set_hm10_name(expected_name):
            print("✅ Name updated successfully. Resetting ESP32...")
            bridge.reset()
            bridge = HM10ESP32Bridge(port=port)
        else:
            print("❌ Failed to set name. Exiting.")
            sys.exit(1)

    status = bridge.get_status()
    if status != "CONNECTED":
        print(f"⚠️ ESP32 is {status}. Please ensure HM-10 is advertising. Exiting.")
        sys.exit(0)

    print(f"✨ Ready! Connected to {expected_name}")
    return bridge


def wait_msg(bridge, timeout=None):
    """
    Blocking wait for one message from Arduino.
    timeout=None -> wait forever
    return: str or None (if timeout)
    """
    t0 = time.time()
    while True:
        msg = bridge.listen()
        if msg:
            msg = msg.strip()
            print(f"[HM10]: {msg}")
            return msg

        if timeout is not None and (time.time() - t0) > timeout:
            return None

        time.sleep(0.05)


def main(mode: str, bt_port: str, team_name: str, server_url: str, maze_file: str):
    maze = Maze(maze_file)
    point = ScoreboardServer(team_name, server_url)
    # point = ScoreboardFake(team_name, "data/fakeUID.csv")

    bridge = setup_bridge(port=bt_port, expected_name=EXPECTED_NAME)
    bridge.send("s")

    if mode == "0":
        log.info("Mode 0: For treasure-hunting")

        node_dict = maze.get_node_dict()
        if 1 not in node_dict or 12 not in node_dict:
            log.error("Node 1 or Node 12 not found in maze.")
            sys.exit(1)

        start_node = node_dict[1]
        goal_node = node_dict[12]

        # 1) BFS shortest node path
        nodes_path = maze.BFS_2(start_node, goal_node)
        if not nodes_path or len(nodes_path) < 2:
            log.error("No valid path from node 1 to node 12.")
            sys.exit(1)

        # 2) node path -> actions -> command string (f/b/r/l/s)
        actions = maze.getActions(nodes_path)
        if not actions:
            log.error("Failed to convert path to actions.")
            sys.exit(1)

        cmds = maze.actions_to_str(actions)
        log.info(f"Planned commands: {cmds}")

        # 3) preload first 3 commands
        preload = min(3, len(cmds))
        for i in range(preload):
            bridge.send(cmds[i])
            log.info(f"preload sent: {cmds[i]}")

        # 4) send one command each time 'n' is received
        next_idx = preload
        while next_idx < len(cmds):
            msg = wait_msg(bridge, timeout=None)  # blocking wait
            if msg == "n":
                bridge.send(cmds[next_idx])
                log.info(f"sent: {cmds[next_idx]}")
                next_idx += 1

        log.info("arrive")

                
        

    elif mode == "1":
        log.info("Mode 1: Self-testing mode.")
        bridge.send("f")
        bridge.send("f")
        bridge.send("f")
        log.info("Sent f,f,f. Waiting for events... (Ctrl+C to stop)")

        try:
            while True:
                msg = bridge.listen()
                if msg == "n":
                    bridge.send("l")
                    log.info("sent 'l'")
        except KeyboardInterrupt:
            log.info("Stopped by user.")

    else:
        log.error("Invalid mode")
        sys.exit(1)


if __name__ == "__main__":
    args = parse_args()
    main(**vars(args))
