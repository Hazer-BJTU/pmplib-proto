#include "Basics.h"

#define make_mono_mono std::make_unique<mpengine::MonoUnit<mpengine::MonoSynchronizer>>()
#define make_para_mono std::make_unique<mpengine::ParallelizableUnit<mpengine::MonoSynchronizer>>()

int main() {
    putils::ThreadPool::set_global_threadpool();
    auto& logger = putils::RuntimeLog::get_global_log();
    auto unit_0 = make_mono_mono;
    unit_0->add_task_from_outer([&logger] { logger.add("unit_0"); });
    auto unit_1 = make_mono_mono;
    unit_1->add_task_from_outer([&logger] { logger.add("unit_1"); });
    auto unit_2 = make_mono_mono;
    unit_2->add_task_from_outer([&logger] { logger.add("unit_2"); });
    auto unit_3 = make_para_mono;
    unit_3->add_task_from_outer([&logger] { logger.add("unit_3 task_0"); });
    unit_3->add_task_from_outer([&logger] { logger.add("unit_3 task_1"); });
    unit_3->add_task_from_outer([&logger] { logger.add("unit_3 task_2"); });
    unit_3->add_task_from_outer([&logger] { logger.add("unit_3 task_3"); });
    auto unit_4 = make_mono_mono;
    unit_4->add_task_from_outer([&logger] { logger.add("unit_4"); });
    auto unit_5 = make_mono_mono;
    unit_5->add_task_from_outer([&logger] { logger.add("unit_5"); });
    auto unit_6 = make_mono_mono;
    unit_6->add_task_from_outer([&logger] { logger.add("unit_6"); });
    std::latch final_synchronizer{1};
    unit_1->add_dependency(*unit_0);
    unit_2->add_dependency(*unit_1);
    unit_3->add_dependency(*unit_2);
    unit_4->add_dependency(*unit_3);
    unit_5->add_dependency(*unit_4);
    unit_6->add_dependency(*unit_5);
    std::latch synchronizer{1};
    mpengine::add_dependency(synchronizer, *unit_6);
    unit_0->dependency_notice();
    synchronizer.wait();
    return 0;
}
