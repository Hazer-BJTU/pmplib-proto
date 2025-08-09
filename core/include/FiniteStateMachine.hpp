#pragma once

#include <map>
#include <tuple>
#include <deque>
#include <memory>
#include <functional>
#include <unordered_map>

#include "RuntimeLog.h"

namespace mpengine {

/**
 * @class FSMNode
 * @brief Template class representing a node in a Finite State Machine
 * 
 * @tparam NodeIndex Type used to identify nodes, must be streamable
 * @tparam Event Type representing events/transitions, must be streamable
 * 
 * This class represents a single state in a finite state machine with 
 * transitions triggered by events. Each transition can have an associated action.
 * 
 * @author Hazer
 * @date 2025/8/6
 */

template<typename NodeIndex, typename Event>
requires requires(std::ostream& stream, const NodeIndex& index, const Event& event) {
    { stream << index } -> std::same_as<std::ostream&>;
    { stream << event } -> std::same_as<std::ostream&>;
}
struct FSMNode {
    using NodePtr = FSMNode*;
    struct edge {
        bool wait;
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
    std::tuple<NodePtr, bool> step(const Event& event) {
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
            #ifdef MPENGINE_FSM_IMPLICIT_OVERLOAD
                throw PUTILS_GENERAL_EXCEPTION("Error state encountered, parse terminated.", "FSM error");
            #else
                std::stringstream ss;
                ss << "Transition overloaded: " << index << " gets " << event << "!";
                throw PUTILS_GENERAL_EXCEPTION(ss.str(), "FSM error");
            #endif
        }
        return {it->second.target, it->second.wait};
    }
    template<typename Callable>
    void add_transition(NodePtr target, const Event& event, Callable&& action, bool wait = false) noexcept {
        auto it = transition_chart.find(event);
        if (it == transition_chart.end()) {
            transition_chart.insert(std::make_pair(event, edge(wait, this, target, std::forward<Callable>(action))));
        } else {
            it->second = edge(wait, this, target, std::forward<Callable>(action));
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
std::same_as<typename EventList::value_type, Event> &&
requires(NodeIndex index, Event event, NodeAllocator allocator, EventList list) {
    requires std::is_base_of_v<FSMNode<NodeIndex, Event>, Node>;
    { std::hash<NodeIndex>{}(index) } -> std::convertible_to<std::size_t>;
    { std::hash<Event>{}(event) } -> std::convertible_to<std::size_t>;
    { list.begin() } -> std::same_as<typename EventList::iterator>;
    { list.end() } -> std::same_as<typename EventList::iterator>;
    { std::allocate_shared<Node>(allocator, index) } -> std::same_as<std::shared_ptr<Node>>;
};

/**
 * @class FiniteStateMachine
 * @brief Template class implementing a Finite State Machine
 * 
 * @tparam NodeIndex Type used to identify nodes
 * @tparam Event Type representing events/transitions 
 * @tparam Node Node type (defaults to FSMNode<NodeIndex, Event>)
 * @tparam NodeAllocator Allocator type for nodes (default std::allocator<Node>)
 * @tparam EventList Container type for event sequences (default std::basic_string<Event>)
 * 
 * This class provides a complete FSM implementation with state management,
 * transitions, and event processing capabilities.
 * 
 * @author Hazer
 * @date 2025/8/6
 */

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
    Event _current_event;
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
    bool add_transition(const NodeIndex& source, const NodeIndex& target, const Event& event, Args&&... args) noexcept {
        auto it_source = nodes.find(source), it_target = nodes.find(target);
        if (it_source == nodes.end() || it_target == nodes.end()) {
            return false;
        }
        it_source->second->add_transition(it_target->second.get(), event, std::forward<Args>(args)...);
        return true;
    }
    template<typename... Args>
    bool add_transition(const NodeIndex& source, const NodeIndex& target, const EventList& events, Args&&... args) noexcept {
        bool flag = true;
        for (auto it = events.begin(); it != events.end(); it++) {
            auto it_source = nodes.find(source), it_target = nodes.find(target);
            if (it_source == nodes.end() || it_target == nodes.end()) {
                flag = false;
                continue;
            }
            it_source->second->add_transition(it_target->second.get(), *it, std::forward<Args>(args)...);
        }
        return flag;
    }
    template<typename... Args>
    bool add_error_transition(const NodeIndex& source, const Event& event, Args&&... args) noexcept {
        auto it_source = nodes.find(source);
        if (it_source == nodes.end()) {
            return false;
        }
        it_source->second->add_transition(nullptr, event, std::forward<Args>(args)...);
        return true;
    }
    template<typename... Args>
    bool add_error_transition(const NodeIndex& source, const EventList& events, Args&&... args) noexcept {
        bool flag = true;
        for (auto it = events.begin(); it != events.end(); it++) {
            auto it_source = nodes.find(source);
            if (it_source == nodes.end()) {
                flag = false;
                continue;
            }
            it_source->second->add_transition(nullptr, *it, std::forward<Args>(args)...);
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
            _current_event = event;
            auto [p, wait] = p->step(event);
            return wait;
        } PUTILS_CATCH_THROW_GENERAL
    }
    void steps(const EventList& events) {
        if (!p) {
            throw PUTILS_GENERAL_EXCEPTION("Initial state is not set.", "FSM error");
        }
        try {
            auto it = events.begin();
            while(it != events.end()) {
                _current_event = *it;
                auto [new_p, wait] = p->step(*it);
                p = new_p;
                if (!wait) {
                    it++;
                }
            }
            return;
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

struct cs {
    static const std::string digits;
    static const std::string lowercase;
    static const std::string uppercase;
    static const std::string punctuation;
    static const std::string alphanumeric;
    static const std::string whitespace;
    static const std::string control;
    static const std::string invisible;
    static const std::string visible;
    static const std::string text;
    static const std::string any;
    static bool in(std::string_view charset1, std::string_view charset2) noexcept {
        for (const char& c: charset1) {
            if (charset2.find(c) == std::string_view::npos) {
                return false;
            }
        }
        return true;
    }
    static const std::string except(std::string_view charset1, std::string_view charset2) noexcept {
        std::string result;
        for (const char& c: charset1) {
            if (charset2.find(c) == std::string_view::npos) {
                result.push_back(c);
            }
        }
        return result;
    }
    static const std::string concate(std::string_view charset1, std::string_view charset2) noexcept {
        return std::string(charset1) + std::string(charset2);
    }
};

const std::string cs::digits = "0123456789";
const std::string cs::lowercase = "abcdefghijklmnopqrstuvwxyz";
const std::string cs::uppercase = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::string cs::punctuation = "!\"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}~";
const std::string cs::alphanumeric = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::string cs::whitespace = " \t\n\r\v\f";
const std::string cs::control = "\0\a\b\t\n\v\f\r\e";
const std::string cs::invisible = " \t\n\r\v\f\0\a\b\t\n\v\f\r\e";
const std::string cs::visible = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}~";
const std::string cs::text = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}~ ";
const std::string cs::any = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#$%&\'()*+,-./:;<=>?@[\\]^_`{|}~ \t\n\r\v\f\0\a\b\t\n\v\f\r\e";

}
