import csv
import logging
import math
from enum import IntEnum
from typing import List

import numpy as np
import pandas

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
        # Design data structure for finding path to nearest unexplored deadend
        # A deadend is a node with only one connection
        start_idx = node.get_index()
        
        # Find all deadends in the maze
        deadends = []
        for node_idx, maze_node in self.node_dict.items():
            if len(maze_node.get_successors()) == 1:
                deadends.append(maze_node)
        
        if not deadends:
            log.error("No deadends found in the maze")
            return []
        
        # Use BFS to find the shortest path to each deadend
        shortest_path = None
        shortest_length = float('inf')
        
        for deadend in deadends:
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
        car_dir = Direction.NORTH  # Assuming initial direction is NORTH ########################################
        
        for i in range(len(nodes) - 1):
            action, car_dir = self.getAction(car_dir, nodes[i], nodes[i + 1])
            if action == 0:
                return []
            actions.append(action)
            
        return actions

    def actions_to_str(self, actions):
        # cmds should be a string sequence like "fbrl....", use it as the input of BFS checklist #1
        cmd = "fbrls"
        cmds = ""
        for action in actions:
            cmds += cmd[action - 1]
        log.info(cmds)
        return cmds

    def strategy(self, node: Node):
        return self.BFS(node)

    def strategy_2(self, node_from: Node, node_to: Node):
        return self.BFS_2(node_from, node_to)