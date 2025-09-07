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

if __name__ == '__main__':
    pass
