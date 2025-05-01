#pragma once

#include "ArithmeticFunctions.hpp"
#include <functional>

namespace rpc1k {

class ConstantNode: public TriggerableNode{
public:
    ConstantNode();
    ConstantNode(const std::string num);
    ~ConstantNode();
    ConstantNode(const GraphNode&) = delete;
    ConstantNode& operator = (const GraphNode&) = delete;
    ConstantNode(const std::shared_ptr<GraphNode>& node);
    ConstantNode& operator = (const std::shared_ptr<GraphNode>& node);
};

class AddNode: public GraphNode {
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
        const std::shared_ptr<GraphNode>& nodeA,
        const std::shared_ptr<GraphNode>& nodeB
    );
    AddNode(const AddNode&) = delete;
    AddNode& operator = (const AddNode&) = delete;
};

class MultNode: public GraphNode {
private:
    static int get_split_size();
    static int SPLIT_SIZE, FIRST_INPUT, SECOND_INPUT;
    class MultTaskSplit: public Task {
    private:
        int left_bound, right_bound;
        int64 *dataA, *dataB, *dataC;
        bool *signA, *signB, *signC;
        std::function<void()> callback;
    public:
        template<class F>
        MultTaskSplit(
            int left, int right,
            const std::shared_ptr<BaseNum>& operandA,
            const std::shared_ptr<BaseNum>& operandB,
            const std::shared_ptr<BaseNum>& operandC,
            F&& callback
        ): left_bound(left),
           right_bound(right),
           dataA(operandA->get_data()),
           dataB(operandB->get_data()),
           dataC(operandC->get_data()),
           signA(operandA->get_sign_ptr()),
           signB(operandB->get_sign_ptr()),
           signC(operandC->get_sign_ptr()),
           callback(std::forward<F>(callback))
        {}
        ~MultTaskSplit();
        MultTaskSplit(const MultTaskSplit&) = delete;
        MultTaskSplit& operator = (const MultTaskSplit&) = delete;
        void run() override;
    };
public:
    MultNode();
    ~MultNode();
    static std::shared_ptr<MultNode> construct_mult_node_from_nodes(
        const std::shared_ptr<GraphNode>& nodeA,
        const std::shared_ptr<GraphNode>& nodeB
    );
    MultNode(const MultNode&) = delete;
    MultNode& operator = (const MultNode&) = delete;
    void reduce() override;
};

}
