#pragma once

#include <map>
#include <deque>
#include <memory>
#include <functional>
#include <unordered_map>

#include "RuntimeLog.h"

namespace mpengine {

template<typename NodeIndex, typename Event>
requires requires(std::ostream& stream, const NodeIndex& index, const Event& event) {
    { stream << index } -> std::same_as<std::ostream&>;
    { stream << event } -> std::same_as<std::ostream&>;
}
struct FSMNode {
    using NodePtr = FSMNode*;
    struct edge {
        NodePtr source, target;
        std::function<void()> action;
    };
    bool ending;
    NodeIndex index;
    std::unordered_map<Event, edge> transition_chart;
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
        if (it->second.target == nullptr) {
            throw PUTILS_GENERAL_EXCEPTION("Error state encountered, parse terminated.", "FSM error");
        }
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
    typename Node,
    typename NodeAllocator,
    typename EventList
> concept ValidFSMTemplateParams = 
std::copy_constructible<NodeIndex> &&
std::equality_comparable<NodeIndex> &&
std::copy_constructible<Event> &&
std::equality_comparable<Event> &&
requires(NodeIndex index, Event event, NodeAllocator allocator) {
    requires std::is_base_of_v<FSMNode<NodeIndex, Event>, Node>;
    { std::hash<NodeIndex>{}(index) } -> std::convertible_to<std::size_t>;
    { std::hash<Event>{}(event) } -> std::convertible_to<std::size_t>;
    { std::allocate_shared<Node>(allocator, index) } -> std::same_as<std::shared_ptr<Node>>;
};

template<
    typename NodeIndex, 
    typename Event, 
    typename Node = FSMNode<NodeIndex, Event>,
    typename NodeAllocator = std::allocator<Node>,
    typename EventList = std::basic_string<Event>
> requires ValidFSMTemplateParams<NodeIndex, Event, Node, NodeAllocator, EventList>
class FiniteStateMachine {
public:
    using NodeSPtr = std::shared_ptr<Node>;
    using NodePtr = Node*;
private:
    NodePtr p;
    NodeIndex starting_index;
    std::unordered_map<NodeIndex, NodeSPtr> nodes;
    NodeAllocator node_allocator;
public:
    FiniteStateMachine(): p(nullptr), starting_index(), nodes(), node_allocator() {};
    virtual ~FiniteStateMachine() = default;
    FiniteStateMachine(const FiniteStateMachine&) = default;
    FiniteStateMachine& operator = (const FiniteStateMachine&) = default;
    FiniteStateMachine(FiniteStateMachine&&) = default;
    FiniteStateMachine& operator = (FiniteStateMachine&&) = default;
protected:
    Event current_event;
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
    template<typename... Args>
    bool add_error_transition(const NodeIndex& source, Args&&... args) noexcept {
        auto it_source = nodes.find(source);
        if (it_source == nodes.end()) {
            return false;
        }
        it_source->second->add_transition(nullptr, std::forward<Args>(args)...);
        return true;
    }
    template<typename... Args>
    bool add_error_transitions(const NodeIndex& source, const EventList& events, Args&&... args) noexcept {
        bool flag = true;
        for (auto it = events.begin(); it != events.end(); it++) {
            flag &= add_error_transition(source, *it, std::forward<Args>(args)...);
        }
        return flag;
    }
public:
    void reset() {
        auto it = nodes.find(starting_index);
        if (it == nodes.end()) {
            throw PUTILS_GENERAL_EXCEPTION("Starting state is not set.", "FSM error");
        }
        p = it->second.get();
    }
    bool step(const Event& event) {
        if (!p) {
            throw PUTILS_GENERAL_EXCEPTION("Initial state is not set.", "FSM error");
        }
        try {
            current_event = event;
            p = p->step(event);
            return p->ending;
        } PUTILS_CATCH_THROW_GENERAL
    }
    bool steps(const EventList& events) {
        if (!p) {
            throw PUTILS_GENERAL_EXCEPTION("Initial state is not set.", "FSM error");
        }
        try {
            for (auto it = events.begin(); it != events.end(); it++) {
                current_event = *it;
                p = p->step(*it);
            }
            return p->ending;
        } PUTILS_CATCH_THROW_GENERAL
    }
    const NodeIndex& get_current_state() const {
        if (!p) {
            throw PUTILS_GENERAL_EXCEPTION("Initial state is not set.", "FSM error");
        }
        return p->index;
    }
    bool accepted() const {
        if (!p) {
            throw PUTILS_GENERAL_EXCEPTION("Initial state is not set.", "FSM error");
        }
        return p->ending;
    }
};

using Automaton = FiniteStateMachine<std::string, char, FSMNode<std::string, char>, std::allocator<FSMNode<std::string, char>>, std::string>;

}
