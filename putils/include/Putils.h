#pragma once

namespace putils {

class Task {
public:
    Task();
    virtual ~Task() = 0;
    Task(const Task&) = default;
    Task& operator = (const Task&) = default;
    Task(Task&&) = default;
    Task& operator = (Task&&) = default;
    virtual void run() = 0;
};

}
