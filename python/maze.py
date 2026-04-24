import csv
import logging
import math
from enum import IntEnum
from typing import List, Dict, Tuple

import numpy as np
import pandas
import itertools


from node import Direction, Node

log = logging.getLogger(__name__)


class Action(IntEnum):
    ADVANCE = 1
    U_TURN = 2
    TURN_RIGHT = 3
    TURN_LEFT = 4
    HALT = 5


class Maze:
    def __init__(self, filepath: str):
        # TODO : read file and implement a data structure you like
        # For example, when parsing raw_data, you may create several Node objects.
        # Then you can store these objects into self.nodes.
        # Finally, add to nd_dict by {key(index): value(corresponding node)}
        self.raw_data = pandas.read_csv(filepath).values
        self.nodes = []
        self.node_dict = dict()  # key: index, value: the correspond node
        self.deadends = []  # List to store deadend nodes
        self.initial_dir = Direction.NORTH
        
        # Create nodes from the raw data
        row, _ = self.raw_data.shape
        for i in range(row):
            node_index = int(self.raw_data[i][0])
            node = Node(node_index)
            self.nodes.append(node)
            self.node_dict[node_index] = node
        
        # Set successors for each node
        for i in range(row):
            current_node = self.nodes[i]
            for j in range(1, 5):  # Directions 1-4 (NORTH, SOUTH, WEST, EAST)
                if j < len(self.raw_data[i]) and not math.isnan(self.raw_data[i][j]):
                    successor_idx = int(self.raw_data[i][j])
                    if successor_idx in self.node_dict:
                        current_node.set_successor(self.node_dict[successor_idx], j)
        
        # Find all deadends in the maze
        self.find_deadends()

    def find_deadends(self):
        """Identify all deadends in the maze and store them in self.deadends"""
        self.deadends = []
        for node_idx, maze_node in self.node_dict.items():
            if len(maze_node.get_successors()) == 1:
                self.deadends.append(maze_node)
        
        log.info(f"Found {len(self.deadends)} deadends in the maze")
        return self.deadends

    def get_start_point(self):
        if len(self.node_dict) < 2:
            log.error("Error: the start point is not included.")
            return 0
        return self.node_dict[1]

    def get_node_dict(self):
        return self.node_dict

    def BFS(self, node: Node):
        # TODO : design your data structure here for your algorithm
        # Tips : return a sequence of nodes from the node to the nearest unexplored deadend
        start_idx = node.get_index()
        
        # Use the precomputed deadends list
        if not self.deadends:
            log.error("No deadends found in the maze")
            return []
        
        # Use BFS to find the shortest path to each deadend
        shortest_path = None
        shortest_length = float('inf')
        
        for deadend in self.deadends:
            path = self.BFS_2(node, deadend)
            if path and (shortest_path is None or len(path) < shortest_length):
                shortest_path = path
                shortest_length = len(path)
        
        return shortest_path

    def BFS_2(self, node_from: Node, node_to: Node):
        # TODO : similar to BFS but with fixed start point and end point
        # Tips : return a sequence of nodes of the shortest path
        start_idx = node_from.get_index()
        end_idx = node_to.get_index()
        
        row = len(self.nodes)
        level = 0
        
        # Initialize arrays for BFS
        q = [math.nan for _ in range(row)]  # Store previous node index
        d = [math.nan for _ in range(row)]  # Store distance/level
        dd = [math.nan for _ in range(row)]  # Store direction
        
        # Map node indices to array indices
        node_to_array = {node.get_index(): i for i, node in enumerate(self.nodes)}
        
        # Set starting point
        d[node_to_array[start_idx]] = level
        
        # BFS algorithm
        while True:
            if all(not math.isnan(d[i]) for i in range(row)):
                break
                
            for i in range(row):
                if d[i] == level:
                    current_node = self.nodes[i]
                    for successor, direction, _ in current_node.get_successors():
                        successor_array_idx = node_to_array[successor.get_index()]
                        if math.isnan(d[successor_array_idx]):
                            d[successor_array_idx] = level + 1
                            dd[successor_array_idx] = direction
                            q[successor_array_idx] = current_node.get_index()
            
            level += 1
            
            # Break if end node is found
            if not math.isnan(d[node_to_array[end_idx]]):
                break
        
        # Reconstruct path
        path = []
        current = end_idx
        
        while current != start_idx:
            current_array_idx = node_to_array[current]
            previous = q[current_array_idx]
            if math.isnan(previous):
                log.error(f"No path found from node {start_idx} to node {end_idx}")
                return []
            path.append(self.node_dict[current])
            current = previous
            
        path.append(node_from)  # Add the start node
        path.reverse()  # Reverse to get path from start to end
        
        return path

    def getAction(self, car_dir, node_from: Node, node_to: Node):
        # TODO : get the car action
        # Tips : return an action and the next direction of the car if the node_to is the Successor of node_to
        # If not, print error message and return 0
        if not node_from.is_successor(node_to):
            log.error(f"Error: Node {node_to.get_index()} is not a successor of Node {node_from.get_index()}")
            return 0, car_dir
        
        next_dir = node_from.get_direction(node_to)
        
        # Determine action based on current direction and next direction
        if car_dir == Direction.NORTH:
            if next_dir == Direction.NORTH: 
                return Action.ADVANCE, next_dir
            elif next_dir == Direction.WEST:  
                return Action.TURN_LEFT, next_dir
            elif next_dir == Direction.EAST:  
                return Action.TURN_RIGHT, next_dir
            elif next_dir == Direction.SOUTH: 
                return Action.U_TURN, next_dir
        elif car_dir == Direction.SOUTH:
            if next_dir == Direction.SOUTH: 
                return Action.ADVANCE, next_dir
            elif next_dir == Direction.WEST:  
                return Action.TURN_RIGHT, next_dir
            elif next_dir == Direction.EAST:  
                return Action.TURN_LEFT, next_dir
            elif next_dir == Direction.NORTH: 
                return Action.U_TURN, next_dir
        elif car_dir == Direction.WEST:
            if next_dir == Direction.WEST:  
                return Action.ADVANCE, next_dir
            elif next_dir == Direction.SOUTH: 
                return Action.TURN_LEFT, next_dir
            elif next_dir == Direction.NORTH: 
                return Action.TURN_RIGHT, next_dir
            elif next_dir == Direction.EAST:  
                return Action.U_TURN, next_dir
        elif car_dir == Direction.EAST:
            if next_dir == Direction.EAST:  
                return Action.ADVANCE, next_dir
            elif next_dir == Direction.SOUTH: 
                return Action.TURN_RIGHT, next_dir
            elif next_dir == Direction.NORTH: 
                return Action.TURN_LEFT, next_dir
            elif next_dir == Direction.WEST:  
                return Action.U_TURN, next_dir
                
        return 0, car_dir

    def getActions(self, nodes: List[Node]):
        # TODO : given a sequence of nodes, return the corresponding action sequence
        # Tips : iterate through the nodes and use getAction() in each iteration
        if not nodes or len(nodes) < 2:
            return []
            
        actions = []
        car_dir = self.initial_dir  # Assuming initial direction is NORTH
        
        for i in range(len(nodes) - 1):
            action, car_dir = self.getAction(car_dir, nodes[i], nodes[i + 1])
            if action == 0:
                return []
            actions.append(action)
            
        return actions

    def actions_to_str(self, actions):
        # cmds should be a string sequence like "fbrl....", use it as the input of BFS checklist #1
        cmd = "fbrlh"
        cmds = ""
        for action in actions:
            cmds += cmd[action - 1]
        log.info(cmds)
        return cmds

    # strategy: find a path that visits all deadends
    def strategy(self, node: Node):
        # Use the precomputed deadend list and find shortest path through all deadends
        if not self.deadends:
            log.warning("No deadends found in the maze")
            return []
        
        # Create a 2D paths list to store paths between each pair of deadends
        # We'll also store the path length for easier access
        num_deadends = len(self.deadends)
        paths = [[None for _ in range(num_deadends)] for _ in range(num_deadends)]
        distances = [[float('inf') for _ in range(num_deadends)] for _ in range(num_deadends)]
        
        log.info(f"Calculating paths between {num_deadends} deadends...")
        # Calculate paths between each pair of deadends
        for i in range(num_deadends):
            for j in range(num_deadends):
                if i != j:  # Skip calculating path from a deadend to itself
                    paths[i][j] = self.BFS_2(self.deadends[i], self.deadends[j])
                    if paths[i][j]:
                        distances[i][j] = len(paths[i][j]) - 1  # -1 because the path includes both endpoints
                    log.debug(f"Path from deadend {self.deadends[i].get_index()} to {self.deadends[j].get_index()}: {len(paths[i][j]) if paths[i][j] else 'No path'}")
        
        # Calculate paths from current node to each deadend
        start_to_deadend_paths = [None] * num_deadends
        start_to_deadend_distances = [float('inf')] * num_deadends
        
        for i in range(num_deadends):
            start_to_deadend_paths[i] = self.BFS_2(node, self.deadends[i])
            if start_to_deadend_paths[i]:
                start_to_deadend_distances[i] = len(start_to_deadend_paths[i]) - 1
        
        # Now we'll use a simple nearest neighbor algorithm to find a path through all deadends
        # This is a greedy approach and might not give the optimal solution, but it's a good starting point
        
        # We'll keep track of the unvisited deadends
        unvisited = set(range(num_deadends))
        
        # Start with the closest deadend to the current node
        current_idx = min(range(num_deadends), key=lambda i: start_to_deadend_distances[i])
        unvisited.remove(current_idx)
        
        # Initialize the complete path with the path to the first deadend
        complete_path = start_to_deadend_paths[current_idx]
        
        # Keep track of the current position (the last deadend we visited)
        current_position = self.deadends[current_idx]
        
        # Visit all remaining deadends
        while unvisited:
            # Find the nearest unvisited deadend
            next_idx = min(unvisited, key=lambda j: distances[current_idx][j])
            unvisited.remove(next_idx)
            
            # Add the path to this deadend to the complete path (excluding the first node which is already in the path)
            next_segment = paths[current_idx][next_idx]
            if next_segment:
                complete_path.extend(next_segment[1:])  # Skip the first node which is already in the path
            else:
                log.error(f"No path found from deadend {self.deadends[current_idx].get_index()} to {self.deadends[next_idx].get_index()}")
                # If there's no path to a deadend, we can't complete the full tour
                # You might want to handle this differently based on your requirements
                continue
            
            # Update the current position
            current_idx = next_idx
            current_position = self.deadends[current_idx]
        
        return complete_path

    def strategy_2(self, node: Node):
        """
        Brute-force all permutations of deadends to find the globally shortest route.
        Return: a complete node path starting from `node` and visiting all deadends.
        """
        if not self.deadends:
            log.warning("No deadends found in the maze")
            return []

        num_deadends = len(self.deadends)

        # --- 1) Precompute shortest paths/distances between all deadend pairs ---
        paths_dd = [[None for _ in range(num_deadends)] for _ in range(num_deadends)]
        dist_dd = [[float("inf") for _ in range(num_deadends)] for _ in range(num_deadends)]

        for i in range(num_deadends):
            for j in range(num_deadends):
                if i == j:
                    # deadend to itself
                    paths_dd[i][j] = [self.deadends[i]]
                    dist_dd[i][j] = 0
                else:
                    p = self.BFS_2(self.deadends[i], self.deadends[j])
                    if p:
                        paths_dd[i][j] = p
                        dist_dd[i][j] = len(p) - 1  # edge count

        # --- 2) Precompute shortest paths/distances from start node to each deadend ---
        paths_start = [None] * num_deadends
        dist_start = [float("inf")] * num_deadends
        for i in range(num_deadends):
            p = self.BFS_2(node, self.deadends[i])
            if p:
                paths_start[i] = p
                dist_start[i] = len(p) - 1

        # 若有 deadend 從起點不可達，直接失敗
        unreachable = [i for i in range(num_deadends) if dist_start[i] == float("inf")]
        if unreachable:
            log.error(f"Some deadends are unreachable from start: {unreachable}")
            return []

        # --- 3) Brute-force permutations ---
        best_order = None
        best_dist = float("inf")

        all_idx = list(range(num_deadends))
        for order in itertools.permutations(all_idx):
            # distance = start -> first + sum(pairwise)
            total = dist_start[order[0]]

            feasible = True
            for k in range(len(order) - 1):
                d = dist_dd[order[k]][order[k + 1]]
                if d == float("inf"):
                    feasible = False
                    break
                total += d

                # 小剪枝：已經不可能更好就跳
                if total >= best_dist:
                    feasible = False
                    break

            if feasible and total < best_dist:
                best_dist = total
                best_order = order

        if best_order is None:
            log.error("No feasible route that visits all deadends.")
            return []

        # --- 4) Reconstruct full node path from best order ---
        complete_path = list(paths_start[best_order[0]])  # copy

        for k in range(len(best_order) - 1):
            seg = paths_dd[best_order[k]][best_order[k + 1]]
            if not seg:
                log.error("Unexpected missing segment during reconstruction.")
                return []
            complete_path.extend(seg[1:])  # skip duplicated joint node

        log.info(
            f"strategy_2 brute-force done. deadends={num_deadends}, best_dist={best_dist}, order="
            f"{[self.deadends[i].get_index() for i in best_order]}"
        )
        return complete_path
