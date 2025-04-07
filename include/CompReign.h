#pragma once

#include "Real.h"
#include "ThreadPool.h"
#include "CompGraphNodes.h"
#include <iostream>
#include <memory>
#include <vector>

namespace rpc1k {

class CompReign {
public:
    std::vector<std::shared_ptr<GraphNode>> graph_nodes;
    std::vector<std::weak_ptr<Real>> references;
    std::shared_ptr<ThreadPool> task_handler;
    CompReign();
    ~CompReign();
    CompReign(const CompReign&) = delete;
    CompReign& operator = (const CompReign&) = delete;
    const std::shared_ptr<ThreadPool>& get_task_handler() const;
};

}