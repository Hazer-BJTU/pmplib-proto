#pragma once

#include <map>
#include <deque>
#include <memory>
#include <functional>

#include "RuntimeLog.h"

namespace mpengine {

template<typename, typename, typename, typename, typename>
class FiniteStateMachine;

template<typename NodeIndex, typename Event>
class FSMNode {
public:
    using NodePtr = FSMNode*;
    struct edge {
        NodePtr source, target;
        std::function<void()> action;
    };
private:
    bool ending;
    NodeIndex index;
    std::map<Event, edge> transition_chart;
    template<
        typename FSMNodeIndexType, 
        typename FSMEventType, 
        typename FSMNodeType, 
        typename FSMNodeAllocatorType,
        typename FSMEventListType
    >
    requires std::is_same_v<FSMNodeIndexType, NodeIndex> && 
             std::is_same_v<FSMEventType, Event>
    friend class FiniteStateMachine;
public:
    FSMNode(const NodeIndex& input_index, bool ending = false): index(input_index), transition_chart(), ending(ending) {}
    virtual ~FSMNode() {}
    FSMNode(const FSMNode&) = default;
    FSMNode& operator = (const FSMNode&) = default;
    FSMNode(FSMNode&&) = default;
    FSMNode& operator = (FSMNode&&) = default;
    NodePtr step(const Event& event) {
        auto it = transition_chart.find(event);
        if (it == transition_chart.end()) {
            std::stringstream ss;
            ss << "Undefined state transition: " << index << " gets " << event << "!";
            throw PUTILS_GENERAL_EXCEPTION(ss.str(), "FSM error");
        }
        try {
            it->second.action();
        } PUTILS_CATCH_THROW_GENERAL
        return it->second.target;
    }
    template<typename Callable>
    void add_transition(NodePtr target, const Event& event, Callable&& action) noexcept {
        auto it = transition_chart.find(event);
        if (it == transition_chart.end()) {
            transition_chart.insert(std::make_pair(event, edge(this, target, std::forward<Callable>(action))));
        } else {
            it->second = edge(this, target, std::forward<Callable>(action));
        }
        return;
    }
    void add_transition(NodePtr target, const Event& event) noexcept {
        add_transition(target, event, []{});
        return;
    }
};

template<
    typename NodeIndex, 
    typename Event, 
    typename Node = FSMNode<NodeIndex, Event>,
    typename NodeAllocator = std::allocator<Node>,
    typename EventList = std::deque<Event>
>
class FiniteStateMachine {
public:
    using NodeSPtr = std::shared_ptr<Node>;
    using NodePtr = Node*;
private:
    NodePtr p;
    NodeIndex starting_index;
    std::map<NodeIndex, NodeSPtr> nodes;
    NodeAllocator node_allocator;
public:
    FiniteStateMachine(): p(nullptr), starting_index(), nodes(), node_allocator() {};
    virtual ~FiniteStateMachine() = default;
    FiniteStateMachine(const FiniteStateMachine&) = default;
    FiniteStateMachine& operator = (const FiniteStateMachine&) = default;
    FiniteStateMachine(FiniteStateMachine&&) = default;
    FiniteStateMachine& operator = (FiniteStateMachine&&) = default;
    template<typename... Args>
    bool add_node(const NodeIndex& index, Args&&... args) noexcept {
        if (nodes.find(index) != nodes.end()) {
            return false;
        }
        try {
            nodes.insert(std::make_pair(index, std::allocate_shared<Node>(node_allocator, index, std::forward<Args>(args)...)));
        } catch(...) {
            return false;
        }
        return true;
    }
    bool set_starting(const NodeIndex& starting) noexcept {
        auto it = nodes.find(starting);
        if (it == nodes.end()) {
            return false;
        }
        starting_index = starting;
        p = it->second.get();
        return true;
    }
    void reset() noexcept {
        p = nodes.find(starting_index)->second.get();
    }
    template<typename... Args>
    bool add_transition(const NodeIndex& source, const NodeIndex& target, Args&&... args) noexcept {
        auto it_source = nodes.find(source), it_target = nodes.find(target);
        if (it_source == nodes.end() || it_target == nodes.end()) {
            return false;
        }
        it_source->second->add_transition(it_target->second.get(), std::forward<Args>(args)...);
        return true;
    }
    template<typename... Args>
    bool add_transitions(const NodeIndex& source, const NodeIndex& target, const EventList& events, Args&&... args) noexcept {
        bool flag = true;
        for (auto it = events.begin(); it != events.end(); it++) {
            flag &= add_transition(source, target, *it, std::forward<Args>(args)...);
        }
        return flag;
    }
    bool step(const Event& event) {
        try {
            p = p->step(event);
            return p->ending;
        } PUTILS_CATCH_THROW_GENERAL
    }
    bool steps(const EventList& events) {
        try {
            for (auto it = events.begin(); it != events.end(); it++) {
                p = p->step(*it);
            }
            return p->ending;
        } PUTILS_CATCH_THROW_GENERAL
    }
};

using Automaton = FiniteStateMachine<std::string, char, FSMNode<std::string, char>, std::allocator<FSMNode<std::string, char>>, std::string>;

}
