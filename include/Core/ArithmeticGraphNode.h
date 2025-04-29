#pragma once

#include "ArithmeticFunctions.hpp"
#include <functional>

namespace rpc1k {

class ConstantNode: public TriggerableNode{
public:
    ConstantNode(const std::string num);
    ~ConstantNode();
    ConstantNode(const GraphNode& node);
    ConstantNode& operator = (const GraphNode& node);
};

class AddNode: public GraphNode{
private:
    class AddTask: public Task {
    private:
        int64 *dataA, *dataB, *dataC;
        bool *signA, *signB, *signC;
        std::function<void()> callback;
    public:
        template<class F>
        AddTask(
            const std::shared_ptr<BaseNum>& operandA, 
            const std::shared_ptr<BaseNum>& operandB,
            const std::shared_ptr<BaseNum>& operandC,
            F&& callback
        ): dataA(operandA->get_data()), 
           dataB(operandB->get_data()), 
           dataC(operandC->get_data()), 
           signA(operandA->get_sign_ptr()), 
           signB(operandB->get_sign_ptr()), 
           signC(operandC->get_sign_ptr()),
           callback(std::forward<F>(callback)) 
        {}
        ~AddTask();
        AddTask(const AddTask&) = delete;
        AddTask& operator = (const AddTask&) = delete;
        void run() override;
    };
    friend BaseNum;
public:
    AddNode();
    ~AddNode();
    static std::shared_ptr<AddNode> construct_add_node_from_nodes(
        std::shared_ptr<GraphNode>& nodeA,
        std::shared_ptr<GraphNode>& nodeB
    );
    AddNode(const AddNode&) = delete;
    AddNode& operator = (const AddNode&) = delete;
};

}
