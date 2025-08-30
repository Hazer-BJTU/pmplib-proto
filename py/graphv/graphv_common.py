import json
import argparse

import numpy as np
import networkx as nx
import matplotlib.pyplot as plt

from typing import *

def graph_visualization(input_path, output_path, random_seed):
    content_str = '\{\}'
    with open(input_path, 'r', encoding='utf-8') as file:
        content_str = file.read()
    try:
        content: Dict = json.loads(content_str)
    except Exception as e:
        print(f'An error occurred during json parsing: {e}')
    else:
        graph = nx.DiGraph()
        assert 'nodes_groups' in content and 'edges_groups' in content, 'Incomplete graph informations.'
        nodes_groups: List = content['nodes_groups']
        edges_groups: List = content['edges_groups']
        for group in nodes_groups:
            assert 'node_list' in group, 'Node list missing.'
            for node in group['node_list']:
                assert 'index' in node, 'Node index missing.'
                graph.add_node(node['index'])
        for group in edges_groups:
            assert 'edge_list' in group, 'Edge list missing.'
            for edge in group['edge_list']:
                assert 'source' in edge and 'target' in edge, 'Incomplete edge informations.'
                graph.add_edge(edge['source'], edge['target'])
        pos = nx.spring_layout(graph, seed=random_seed)
        for group in nodes_groups:
            nlist = [node['index'] for node in group['node_list']]
            if 'display_configs' in group:
                nx.draw_networkx_nodes(graph, pos, nlist, **group['display_configs'])
            else:
                nx.draw_networkx_nodes(graph, pos, nlist)
            if 'label_configs' in group:
                llist = { node['index']: (node['label'] if 'label' in node else node['index']) for node in group['node_list'] }
                nx.draw_networkx_labels(graph, pos, llist)
        for group in edges_groups:
            elist = [(edge['source'], edge['target']) for edge in group['edge_list']]
            if 'display_configs' in group:
                nx.draw_networkx_edges(graph, pos, elist, **group['display_configs'])
            else:
                nx.draw_networkx_edges(graph, pos, elist)
        plt.savefig(output_path)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='graphv_common',
        description='A python script that helps with graph / tree data structure visualization.'
    )
    parser.add_argument('-i', '--input_path', type=str, required=True, help='Input json file path.')
    parser.add_argument('-o', '--output_path', type=str, nargs='?', default='output.png', help='Output picture name.')
    parser.add_argument('-r', '--random_seed', type=int, nargs='?', default=42, help='Random seed for graph layout.')
    args = parser.parse_args()

    graph_visualization(args.input_path, args.output_path, args.random_seed)
