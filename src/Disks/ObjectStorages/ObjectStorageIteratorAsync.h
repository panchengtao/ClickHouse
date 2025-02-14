#pragma once

#include <Disks/ObjectStorages/ObjectStorageIterator.h>
#include <Common/ThreadPool.h>
#include <Interpreters/threadPoolCallbackRunner.h>
#include <mutex>
#include <Common/CurrentMetrics.h>

namespace DB
{

class IObjectStorageIteratorAsync : public IObjectStorageIterator
{
public:
    IObjectStorageIteratorAsync(
        CurrentMetrics::Metric threads_metric,
        CurrentMetrics::Metric threads_active_metric,
        const std::string & thread_name)
        : list_objects_pool(threads_metric, threads_active_metric, 1)
        , list_objects_scheduler(threadPoolCallbackRunner<BatchAndHasNext>(list_objects_pool, thread_name))
    {
    }

    void next() override;
    bool isValid() const override;
    RelativePathWithMetadata current() const override;
    size_t getAccumulatedSize() const override;

    ~IObjectStorageIteratorAsync() override
    {
        list_objects_pool.wait();
    }

protected:

    virtual bool getBatchAndCheckNext(RelativePathsWithMetadata & batch) = 0;

    struct BatchAndHasNext
    {
        RelativePathsWithMetadata batch;
        bool has_next;
    };

    std::future<BatchAndHasNext> scheduleBatch();

    bool is_finished{false};

    std::mutex mutex;
    ThreadPool list_objects_pool;
    ThreadPoolCallbackRunner<BatchAndHasNext> list_objects_scheduler;
    std::future<BatchAndHasNext> outcome_future;
    RelativePathsWithMetadata current_batch;
    RelativePathsWithMetadata::iterator current_batch_iterator;
    std::atomic<size_t> accumulated_size = 0;
};


}
