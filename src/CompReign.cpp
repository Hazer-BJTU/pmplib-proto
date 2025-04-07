#include "CompReign.h"

namespace rpc1k {

CompReign::CompReign() {}

CompReign::~CompReign() {}

const std::shared_ptr<ThreadPool>& CompReign::get_task_handler() const{
    return task_handler;
}

}
