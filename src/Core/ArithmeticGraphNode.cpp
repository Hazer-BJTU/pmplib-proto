#include "ArithmeticGraphNode.h"

namespace rpc1k {

ConstantNode::ConstantNode() {}

ConstantNode::ConstantNode(const std::string num) {
    RealParser parser;
    parser(out_domain, num);
}

ConstantNode::~ConstantNode() {}

ConstantNode::ConstantNode(const std::shared_ptr<GraphNode>& node) {
    out_domain = node->out_domain;
}

ConstantNode& ConstantNode::operator = (const std::shared_ptr<GraphNode>& node) {
    if (this == node.get()) {
        return *this;
    }
    out_domain = node->out_domain;
    return *this;
}

AddNode::AddTask::~AddTask() {}

void AddNode::AddTask::run() {
    bool overflow_flag = 0;
    if (*signA == *signB) {
        //Same sign, add up the absolute values.
        *signC = *signA;
        overflow_flag |= arithmetic_numerical_add_carry(dataA, dataB, dataC);
    } else if (*signA == 0 && *signB == 1) {
        // A < 0, B >= 0; A + B = -|A| + |B|
        if (arithmetic_numerical_comp(dataA, dataB) >= 0) {
            // |A| >= |B| -> A + B = -(|A| - |B|)
            *signC = 0;
            overflow_flag |= arithmetic_numerical_sub_carry(dataA, dataB, dataC);
        } else {
            // |A| < |B| -> A + B = |B| - |A|
            *signC = 1;
            overflow_flag |= arithmetic_numerical_sub_carry(dataB, dataA, dataC);
        }
    } else if (*signA == 1 && *signB == 0) {
        // A >= 0, B < 0; A + B = |A| - |B|
        if (arithmetic_numerical_comp(dataA, dataB) >= 0) {
            // |A| >= |B| -> A + B = |A| - |B|
            *signC = 1;
            overflow_flag |= arithmetic_numerical_sub_carry(dataA, dataB, dataC);
        } else {
            // |A| < |B| -> A + B = -(|B| - |A|)
            *signC = 0;
            overflow_flag |= arithmetic_numerical_sub_carry(dataB, dataA, dataC);
        }
    }
    if (overflow_flag) {
        FREELOG("Arithmetic overflow during computation process!", errlevel::WARNING);
    }
    callback();
    return;
}

int MultNode::get_split_size() {
    const size_t max_concurrency = std::thread::hardware_concurrency();
    for (int candidate = std::min(LENGTH, max_concurrency); candidate >= 1; candidate--) {
        if (LENGTH % candidate == 0) {
            return LENGTH / candidate;
        }
    }
    return LENGTH;
}

int MultNode::SPLIT_SIZE = MultNode::get_split_size();
int MultNode::FIRST_INPUT = 0;
int MultNode::SECOND_INPUT = 1;

AddNode::AddNode() {}

AddNode::~AddNode() {}

std::shared_ptr<AddNode> AddNode::construct_add_node_from_nodes(
    const std::shared_ptr<GraphNode>& nodeA,
    const std::shared_ptr<GraphNode>& nodeB
) {
    auto new_node = std::make_shared<AddNode>();
    //Initialize data field.
    new_node->precursors.emplace_back(nodeA);
    new_node->precursors.emplace_back(nodeB);
    new_node->input_domains.emplace_back(nodeA->out_domain);
    new_node->input_domains.emplace_back(nodeB->out_domain);
    new_node->inp_dept_counter = new_node->input_domains.size();
    //Backward connection.
    nodeA->successors.emplace_back(new_node);
    nodeB->successors.emplace_back(new_node);
    //Initialize work field.
    new_node->workload.emplace_back(
        std::make_shared<AddTask>(
            nodeA->out_domain,
            nodeB->out_domain,
            new_node->out_domain,
            [new_node]() {
                //Call back function. Reference guaranteed.
                new_node->red_count_down();
            }
        )
    );
    new_node->red_dept_counter = new_node->workload.size();
    return new_node;
}

MultNode::MultTaskSplit::~MultTaskSplit() {}

void MultNode::MultTaskSplit::run() {
    //Do the share.
    arithmetic_numerical_multiply(dataA, dataB, dataC, left_bound, right_bound);
    //Don't forget to call back!
    callback();
    return;
}

MultNode::MultNode() {}

MultNode::~MultNode() {}

std::shared_ptr<MultNode> MultNode::construct_mult_node_from_nodes(
    const std::shared_ptr<GraphNode>& nodeA,
    const std::shared_ptr<GraphNode>& nodeB
) {
    auto new_node = std::make_shared<MultNode>();
    //Initialize data field.
    new_node->precursors.emplace_back(nodeA);
    new_node->precursors.emplace_back(nodeB);
    new_node->input_domains.emplace_back(nodeA->out_domain);
    new_node->input_domains.emplace_back(nodeB->out_domain);
    new_node->inp_dept_counter = new_node->input_domains.size();
    //Backward connection.
    nodeA->successors.emplace_back(new_node);
    nodeB->successors.emplace_back(new_node);
    //Initialize work field.
    for (int i = 0; i < LENGTH / SPLIT_SIZE; i++) {
        new_node->workload.emplace_back(
            std::make_shared<MultTaskSplit>(
                i * SPLIT_SIZE,
                (i + 1) * SPLIT_SIZE,
                nodeA->out_domain,
                nodeB->out_domain,
                new_node->out_domain,
                [new_node]() {
                    //Call back function. Reference guaranteed.
                    new_node->red_count_down();
                }
            )
        );
    }
    new_node->red_dept_counter = new_node->workload.size();
    return new_node;
}

void MultNode::reduce() {
    //Complete carry and sign calculation.
    out_domain->sign = !(input_domains[FIRST_INPUT]->sign ^ input_domains[SECOND_INPUT]->sign);
    bool overflow_flag = arithmetic_numerical_carry(out_domain->data);
    if (overflow_flag) {
        FREELOG("Arithmetic overflow during computation process!", errlevel::WARNING);
    }
    //All done!
    return;
}

}
