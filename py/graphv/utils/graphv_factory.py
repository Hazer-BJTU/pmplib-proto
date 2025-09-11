from typing import *

class GraphvFactory:
    def __init__(self):
        self.creators: Dict[str, Callable] = {}

    def get(self, method_name: str, **kwargs) -> Any:
        if method_name not in self.creators:
            raise KeyError(f'Undefined graphv method: {method_name}.')
        else:
            return self.creators[method_name](**kwargs)

    def register(self, method_name: str, creator: Callable) -> bool:
        if method_name in self.creators:
            return False
        self.creators[method_name] = creator
        return True

graphv_method_factory = GraphvFactory()

def as_graphv_methods(method_name: str) -> Callable:
    def graphv_methods_decorator(cls):
        graphv_method_factory.register(method_name, cls)
        return cls
    return graphv_methods_decorator

def get_graphv_methods(method_name: str, **kwargs) -> Any:
    try:
        return graphv_method_factory.get(method_name, **kwargs)
    except Exception as e:
        raise e

if __name__ == '__main__':
    pass
