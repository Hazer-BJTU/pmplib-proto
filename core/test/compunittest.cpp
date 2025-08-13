#include "Basics.h"

int main() {
    auto& logger = putils::RuntimeLog::get_global_log();
    auto unit_0 = mpengine::make_mono<mpengine::MonoSynchronizer>();
    unit_0->add_task_from_outer([&logger] { logger.add("unit_0"); });
    auto unit_1 = mpengine::make_mono<mpengine::MonoSynchronizer>();
    unit_1->add_task_from_outer([&logger] { logger.add("unit_1"); });
    auto unit_2 = mpengine::make_mono<mpengine::MonoSynchronizer>();
    unit_2->add_task_from_outer([&logger] { logger.add("unit_2"); });
    auto unit_3 = mpengine::make_parallelizable<mpengine::MonoSynchronizer>();
    unit_3->add_task_from_outer([&logger] { logger.add("unit_3 task_0"); });
    unit_3->add_task_from_outer([&logger] { logger.add("unit_3 task_1"); });
    unit_3->add_task_from_outer([&logger] { logger.add("unit_3 task_2"); });
    unit_3->add_task_from_outer([&logger] { logger.add("unit_3 task_3"); });
    auto unit_4 = mpengine::make_mono<mpengine::MonoSynchronizer>();
    unit_4->add_task_from_outer([&logger] { logger.add("unit_4"); });
    auto unit_5 = mpengine::make_mono<mpengine::MonoSynchronizer>();
    unit_5->add_task_from_outer([&logger] { logger.add("unit_5"); });
    auto unit_6 = mpengine::make_mono<mpengine::MonoSynchronizer>();
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
