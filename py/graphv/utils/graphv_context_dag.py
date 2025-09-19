import sys
import json

import numpy as np
import networkx as nx

from typing import *
from .graphv_factory import as_graphv_methods

@as_graphv_methods('context_dag')
class GraphvContextDAG: 
    def __init__(self, input_file_path: str, **kwargs):
        self.input_file_path = input_file_path

    def get_graph_instructions(self) -> Tuple[nx.DiGraph, Dict, List, List]:
        try:
            with open(self.input_file_path, 'r', encoding='utf-8') as file:
                content = json.loads(file.read())
        except Exception as e:
            print(f'An error has occurred when parsing the json file: {e}.')
            sys.exit(1)
        
        assert 'nodes_groups' in content and 'edges_groups' in content, 'Incomplete graph informations.'
        nodes_groups: Dict = content['nodes_groups']
        edges_groups: Dict = content['edges_groups']

        nodes_instructions: List = []
        edges_instructions: List = []

        graph = nx.DiGraph()

        for group_name, group in nodes_groups.items():
            assert 'node_list' in group, 'Incomplete node group informations.'
            
            node_list: List = group['node_list']
            index_list: List = []
            label_list: Dict = {}
            
            for node in node_list:
                assert 'index' in node, 'Missing node index!'
                graph.add_node(node['index'])
                index_list.append(node['index'])
                label_list[node['index']] = node['label'] if 'label' in node else node['index']

            instruction: Dict = {}
            instruction['index_list'] = index_list
            instruction['label_list'] = label_list
            
            if 'display_configs' in group:
                instruction['display_configs'] = group['display_configs']
            if 'label_configs' in group:
                instruction['label_configs'] = group['label_configs']

            nodes_instructions.append(instruction)

        for group_name, group in edges_groups.items():
            assert 'edge_list' in group, 'Incomplete edge group informations.'

            edge_list: List = group['edge_list']
            pair_list: List = []
            label_list: Dict = {}

            for edge in edge_list:
                assert 'source' in edge and 'target' in edge, 'Incomplete edge informations.'
                graph.add_edge(edge['source'], edge['target'])
                pair_list.append((edge['source'], edge['target']))
                label_list[(edge['source'], edge['target'])] = edge['label'] if 'label' in edge else f"({edge['source']}, {edge['target']})"

            instruction: Dict = {}
            instruction['pair_list'] = pair_list
            instruction['label_list'] = label_list

            if 'display_configs' in group:
                instruction['display_configs'] = group['display_configs']
            if 'label_configs' in group:
                instruction['label_configs'] = group['label_configs']

            edges_instructions.append(instruction)
        
        try:
            pos = nx.nx_agraph.graphviz_layout(graph, prog='dot')
        except Exception:
            print('Use networkx built-in layout: spring_layout.')
            pos = nx.spring_layout(graph)

        return graph, pos, nodes_instructions, edges_instructions

if __name__ == '__main__':
    # test_file_path = '/mnt/d/pmplib/bin/core_tests/dag.json'
    # print(graphv_dag_with_references_and_datas(*parse_json(test_file_path)))
    pass
