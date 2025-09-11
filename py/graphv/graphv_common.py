import json
import argparse

import networkx as nx
import matplotlib.pyplot as plt

from utils import graphv_method_factory

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='graphv_common',
        description='A python script that helps with graph / tree data structure visualization.'
    )
    parser.add_argument('-i', '--input_path', type=str, required=True, help='Input json file path.')
    parser.add_argument('-o', '--output_path', type=str, nargs='?', default='output.pdf', help='Output picture path.')
    parser.add_argument('-m', '--method', type=str, nargs='?', default='context_dag', help='Method to visualize a graph.')
    parser.add_argument('-a', '--arguments', type=str, nargs='?', default='{ \"empty\": true }', help='Method configurations.')
    args = parser.parse_args()
    
    try:
        method_args = json.loads(args.arguments)
    except Exception as e:
        print(f"An error has occurred when parsing arguments: '{args.arguments}': {e}.")
        method_args = {}
    method = graphv_method_factory.get(args.method, input_file_path=args.input_path, **method_args)

    try:
        graph, pos, nodes_instructions, edges_instructions = method.get_graph_instructions()
        
        for inst in nodes_instructions:
            if 'display_configs' in inst:
                nx.draw_networkx_nodes(graph, pos, inst['index_list'], **inst['display_configs'])
            else:
                nx.draw_networkx_nodes(graph, pos, inst['index_list'])
            if 'label_configs' in inst:
                nx.draw_networkx_labels(graph, pos, inst['label_list'], **inst['label_configs'])
        
        for inst in edges_instructions:
            if 'display_configs' in inst:
                nx.draw_networkx_edges(graph, pos, inst['pair_list'], **inst['display_configs'])
            else:
                nx.draw_networkx_edges(graph, pos, inst['pair_list'])
            if 'label_configs' in inst:
                nx.draw_networkx_edge_labels(graph, pos, inst['label_list'], **inst['label_configs'])

        plt.savefig(args.output_path)
    except Exception as e:
        print(f'Failed to generate instructions for graph visualization: {e}.')
