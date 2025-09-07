import sys
import json

import numpy as np
import networkx as nx

from typing import *
from .graphv_factory import graphv_method_factory

def parse_json(file_path: str) -> Tuple[Dict, Dict]:
    content_str = '\{\}'
    try:
        with open(file_path, 'r', encoding='utf-8') as file:
            content_str = file.read()
    except Exception as e:
        print(f'An error has occurred when reading from {file_path}: {e}.')
        sys.exit(1)
    content: Dict = {}
    try:
        content = json.loads(content_str)
    except Exception as e:
        print(f'An error has occurred when parsing the json file: {e}.')
        sys.exit(1)
    assert 'nodes_groups' in content and 'edges_groups' in content, 'Incomplete graph informations.'
    nodes_groups: Dict = content['nodes_groups']
    edges_groups: Dict = content['edges_groups']
    return nodes_groups, edges_groups

def DAG_layout(
        rlist: List[int], 
        nlist: List[int], 
        dlist: List[int], 
        e_r_n: List[Tuple[int, int]], 
        e_n_d: List[Tuple[int, int]], 
        e_n_n: List[Tuple[int, int]]
) -> Tuple[nx.DiGraph, Dict[int, np.ndarray]]:
    graph = nx.DiGraph()
    all_nodes = rlist + nlist + dlist
    all_edges = e_r_n + e_n_d + e_n_n
    for node in all_nodes:
        graph.add_node(node)
    for edge in all_edges:
        graph.add_edge(*edge)
    pos = nx.nx_agraph.graphviz_layout(graph, prog='dot')
    pos = np.stack([np.array(pos[n], dtype=np.float32) for n in all_nodes])
    pos = (pos - np.mean(pos)) / np.std(pos)
    pos = { n: pos[i] for i, n in enumerate(all_nodes) }
    return graph, pos

def graphv_dag_with_references_and_datas(nodes_groups: Dict, edges_groups: Dict) -> Tuple[nx.DiGraph, Dict, Dict, Dict]:
    assert 'references' in nodes_groups and 'dag_nodes' in nodes_groups and 'datas' in nodes_groups, 'Incomplete node groups.'
    assert 'references_nodes' in edges_groups and 'nodes_datas' in edges_groups and 'nodes_nodes' in edges_groups, 'Incomplete edge groups.'
    references: Dict = nodes_groups['references']
    dag_nodes: Dict = nodes_groups['dag_nodes']
    datas: Dict = nodes_groups['datas']
    references_nodes: Dict = edges_groups['references_nodes']
    nodes_datas: Dict = edges_groups['nodes_datas']
    nodes_nodes: Dict = edges_groups['nodes_nodes']
    try:
        rlist: List[int] = [r['index'] for r in references['node_list']]
        nlist: List[int] = [n['index'] for n in dag_nodes['node_list']]
        dlist: List[int] = [d['index'] for d in datas['node_list']]
        e_r_n: List[Tuple[int, int]] = [(e['source'], e['target']) for e in references_nodes['edge_list']]
        e_n_d: List[Tuple[int, int]] = [(e['source'], e['target']) for e in nodes_datas['edge_list']]
        e_n_n: List[Tuple[int, int]] = [(e['source'], e['target']) for e in nodes_nodes['edge_list']]
    except Exception as e:
        print(f'An error has occurred when traversing the graph: {e}.')
        sys.exit(1)
    graph, pos = DAG_layout(rlist, nlist, dlist, e_r_n, e_n_d, e_n_n)
    
    def generate_node_instructions(list: List[int], group: Dict) -> Dict:
        result: Dict = {}
        result['list'] = list
        if 'display_configs' in group:
            result['display_configs'] = group['display_configs']
        if 'label_configs' in group:
            result['label'] = { node['index']: (node['label'] if 'label' in node else node['index']) for node in group['node_list'] }
            result['label_configs'] = group['label_configs']
        return result
    
    def generate_edge_instructions(list: List[Tuple[int, int]], group: Dict) -> Dict:
        result: Dict = {}
        result['list'] = list
        if 'display_configs' in group:
            result['display_configs'] = group['display_configs']
        return result
    
    nodes_instructions = [
        generate_node_instructions(rlist, references),
        generate_node_instructions(nlist, dag_nodes),
        generate_node_instructions(dlist, datas)
    ]

    edges_instructions = [
        generate_edge_instructions(e_r_n, references_nodes),
        generate_edge_instructions(e_n_d, nodes_datas),
        generate_edge_instructions(e_n_n, nodes_nodes)
    ]

    return graph, nodes_instructions, edges_instructions, pos

class GraphvDAG:
    def __init__(self, file_path, args):
        self.file_path = file_path

    def get_graph_instructions(self):
        return graphv_dag_with_references_and_datas(*parse_json(self.file_path))
    
graphv_method_factory.register('graphv_dag', GraphvDAG)

if __name__ == '__main__':
    # test_file_path = '/mnt/d/pmplib/bin/core_tests/dag.json'
    # print(graphv_dag_with_references_and_datas(*parse_json(test_file_path)))
    pass
