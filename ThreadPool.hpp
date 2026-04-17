#pragma once

#include "RedCppLib.hpp"

BEGIN_RED_NAMESPACE;

class ThreadPool
{
    protected:
    public:

    using SimpleDispatchFunction = std::function<void()>;
    using SimpleReturnDispatchFunction = std::function<std::any()>;

    using VariableDispatchFunction = std::function<void(std::any&)>;
    using VariableReturnDispatchFunction = std::function<std::any(std::any&)>;

    struct DispatchHandle
    {
        std::mutex m;
        std::condition_variable cv;
        std::any result;
        std::atomic<bool> has_result = false;
        std::atomic<bool> done = false;
    };

    protected:

    class DispatchRunner
    {
        public:
        virtual bool CanReturn() = 0;
        virtual std::optional<std::any> Run() = 0;
        std::shared_ptr<DispatchHandle> handle;
        DispatchRunner()
        {
            handle = std::make_shared<DispatchHandle>();
        }
    };
    class DispatchSimpleDispatchFunction : public DispatchRunner
    {
        public:
        SimpleDispatchFunction f;
        DispatchSimpleDispatchFunction(SimpleDispatchFunction f)
        {
            this->f = f;
        }
        virtual bool CanReturn() override
        {
            return false;
        }
        virtual std::optional<std::any> Run() override
        {
            f();
            return std::nullopt;
        }
    };

    class DispatchSimpleReturnDispatchFunction : public DispatchRunner
    {
        public:
        SimpleReturnDispatchFunction f;
        DispatchSimpleReturnDispatchFunction(SimpleReturnDispatchFunction f)
        {
            this->f = f;
        }
        virtual bool CanReturn() override
        {
            return true;
        }
        virtual std::optional<std::any> Run() override
        {
            return f();
        }
    };

    class DispatchVariableDispatchFunction : public DispatchRunner
    {
        public:
        std::any data;
        VariableDispatchFunction f;
        DispatchVariableDispatchFunction(VariableDispatchFunction f, std::any& data)
        {
            this->f = f;
            this->data = data;
        }
        virtual bool CanReturn() override
        {
            return false;
        }
        virtual std::optional<std::any> Run() override
        {
            f(data);
            return std::nullopt;
        }
    };

    class DispatchVariableReturnDispatchFunction : public DispatchRunner
    {
        public:
        std::any data;
        VariableReturnDispatchFunction f;
        DispatchVariableReturnDispatchFunction(VariableReturnDispatchFunction f, std::any& data)
        {
            this->f = f;
            this->data = data;
        }
        virtual bool CanReturn() override
        {
            return true;
        }
        virtual std::optional<std::any> Run() override
        {
            return f(data);
        }
    };
    struct ThreadState
    {
        std::atomic<bool> ready;
        std::mutex m;
        std::thread thread;
        std::condition_variable v;

    };
    std::vector<std::shared_ptr<ThreadState>> threads;
    std::queue<std::shared_ptr<DispatchRunner>> queue;
    std::thread management_thread;
    std::mutex queue_mutex;
    bool kill = false;

    void InitThreads(int n)
    {

    }
    void WorkingThread(std::shared_ptr<ThreadState> state)
    {
        std::unique_lock<std::mutex> lock{state->m};
        std::shared_ptr<DispatchRunner> dispatch;
        while(!kill)
        {
            state->ready = true;
            state->v.wait(lock);
            if(kill)
                return;
            state->ready = false;
            while(queue_mutex.try_lock() && queue.size() > 0)
            {
                dispatch = queue.back();
                queue.pop();
                queue_mutex.unlock();
                dispatch->Run();
                dispatch->handle->done = true;
                dispatch->handle->has_result= dispatch->CanReturn();
                dispatch->handle->m.unlock();
                dispatch->handle->cv.notify_one();
                dispatch = nullptr;
            }
        }
    }


    std::shared_ptr<DispatchHandle> DispatchOne(std::shared_ptr<DispatchRunner> runner)
    {
        Assert(runner != nullptr);

        auto h = std::make_shared<DispatchHandle>();
        h->m.lock();
        runner->handle = h;
        queue_mutex.lock();
        queue.push(runner);
        queue_mutex.unlock();
    }
    std::vector<std::shared_ptr<DispatchHandle>> DispatchMany(std::vector<std::shared_ptr<DispatchRunner>> runners)
    {
        Assert(runners.size() > 0);

        std::vector<std::shared_ptr<DispatchHandle>> ret;
        ret.reserve(runners.size());

        for(auto r : runners)
        {
            Assert(r != nullptr);

            auto h = std::make_shared<DispatchHandle>();
            h->m.lock();
            r->handle = h;
        }
        queue_mutex.lock();
        for(auto r : runners)
        {
            queue.push(r);
        }
        queue_mutex.unlock();
    }

    public:

    int GetNThreads()
    {
        return threads.size();
    }

    int ThreadsRunning()
    {
        int n = 0;
        for(auto& t : threads)
        {
            n += !t->ready;
        }
        return n;
    }
    int QueuedTasks()
    {
        int n = 0;
        queue_mutex.lock();
        n = queue.size();
        queue_mutex.unlock();
        return n;
    }

    int TotalTasks()
    {
        return ThreadsRunning() + QueuedTasks();
    }

    //DispatchSimpleDispatchFunction
    std::shared_ptr<DispatchHandle> Dispatch(SimpleDispatchFunction function)
    {
        return DispatchOne(std::make_shared<DispatchSimpleDispatchFunction>(function));
    }
    std::vector<std::shared_ptr<DispatchHandle>> Dispatch(SimpleDispatchFunction function, int n)
    {
        std::vector<std::shared_ptr<DispatchRunner>> fs;
        fs.reserve(n);
        for(int i = 0; i < n; i++)
        {
            fs.push_back(std::make_shared<DispatchSimpleDispatchFunction>(function));
        }
        return DispatchMany(fs);
    }
    std::vector<std::shared_ptr<DispatchHandle>> Dispatch(SimpleDispatchFunction* function, int n)
    {
        std::vector<std::shared_ptr<DispatchRunner>> fs;
        fs.reserve(n);
        for(int i = 0; i < n; i++)
        {
            fs.push_back(std::make_shared<DispatchSimpleDispatchFunction>(function[i]));
        }
        return DispatchMany(fs);
    }
    std::vector<std::shared_ptr<DispatchHandle>> Dispatch(const std::vector<SimpleDispatchFunction>& function)
    {
        std::vector<std::shared_ptr<DispatchRunner>> fs;
        fs.reserve(function.size());
        for(int i = 0; i < function.size(); i++)
        {
            fs.push_back(std::make_shared<DispatchSimpleDispatchFunction>(function[i]));
        }
        return DispatchMany(fs);
    }

    // VariableDispatchFunction
    std::shared_ptr<DispatchHandle> Dispatch(VariableDispatchFunction function, std::any data)
    {
        return DispatchOne(std::make_shared<DispatchVariableDispatchFunction>(function, data));
    }
    std::vector<std::shared_ptr<DispatchHandle>> Dispatch(VariableDispatchFunction function, int n, std::any& data)
    {
        std::vector<std::shared_ptr<DispatchRunner>> fs;
        fs.reserve(n);
        for(int i = 0; i < n; i++)
        {
            fs.push_back(std::make_shared<DispatchVariableDispatchFunction>(function, data));
        }
        return DispatchMany(fs);
    }
    std::vector<std::shared_ptr<DispatchHandle>> Dispatch(const std::vector<VariableDispatchFunction>& function, std::any& data)
    {
        std::vector<std::shared_ptr<DispatchRunner>> fs;
        fs.reserve(function.size());
        for(int i = 0; i < function.size(); i++)
        {
            fs.push_back(std::make_shared<DispatchVariableDispatchFunction>(function[i], data));
        }
        return DispatchMany(fs);
    }

    std::vector<std::shared_ptr<DispatchHandle>> Dispatch(VariableDispatchFunction function, int n, std::vector<std::any> data)
    {
        std::vector<std::shared_ptr<DispatchRunner>> fs;
        fs.reserve(n);
        for(int i = 0; i < n; i++)
        {
            fs.push_back(std::make_shared<DispatchVariableDispatchFunction>(function, data[i]));
        }
        return DispatchMany(fs);
    }
    std::vector<std::shared_ptr<DispatchHandle>> DispatchDataArray(VariableDispatchFunction* function, int n, std::vector<std::any> data)
    {
        std::vector<std::shared_ptr<DispatchRunner>> fs;
        fs.reserve(n);
        for(int i = 0; i < n; i++)
        {
            fs.push_back(std::make_shared<DispatchVariableDispatchFunction>(function[i], data[i]));
        }
        return DispatchMany(fs);
    }
    std::vector<std::shared_ptr<DispatchHandle>> DispatchDataArray(const std::vector<VariableDispatchFunction>& function, std::vector<std::any> data)
    {
        return DispatchDataArray((VariableDispatchFunction*)&function[0], (int)function.size(), (std::vector<std::any>)data);
    }

    //DispatchSimpleDispatchFunction
    std::shared_ptr<DispatchHandle> Dispatch(SimpleReturnDispatchFunction function)
    {
        return DispatchOne(std::make_shared<DispatchSimpleReturnDispatchFunction>(function));
    }
    std::vector<std::shared_ptr<DispatchHandle>> Dispatch(SimpleReturnDispatchFunction function, int n)
    {
        std::vector<std::shared_ptr<DispatchRunner>> fs;
        fs.reserve(n);
        for(int i = 0; i < n; i++)
        {
            fs.push_back(std::make_shared<DispatchSimpleReturnDispatchFunction>(function));
        }
        return DispatchMany(fs);
    }
    std::vector<std::shared_ptr<DispatchHandle>> Dispatch(SimpleReturnDispatchFunction* function, int n)
    {
        std::vector<std::shared_ptr<DispatchRunner>> fs;
        fs.reserve(n);
        for(int i = 0; i < n; i++)
        {
            fs.push_back(std::make_shared<DispatchSimpleReturnDispatchFunction>(function[i]));
        }
        return DispatchMany(fs);
    }
    std::vector<std::shared_ptr<DispatchHandle>> Dispatch(const std::vector<SimpleReturnDispatchFunction>& function)
    {
        return Dispatch((SimpleReturnDispatchFunction*)&function[0], (int)function.size());
    }
    
    // VariableReturnDispatchFunction
    std::vector<std::shared_ptr<DispatchHandle>> Dispatch(VariableReturnDispatchFunction function, int n, std::any data)
    {
        std::vector<std::shared_ptr<DispatchRunner>> fs;
        fs.reserve(n);
        for(int i = 0; i < n; i++)
        {
            fs.push_back(std::make_shared<DispatchVariableReturnDispatchFunction>(function, data));
        }
        return DispatchMany(fs);
    }
    std::vector<std::shared_ptr<DispatchHandle>> Dispatch(VariableReturnDispatchFunction* function, int n, std::any data)
    {
        std::vector<std::shared_ptr<DispatchRunner>> fs;
        fs.reserve(n);
        for(int i = 0; i < n; i++)
        {
            fs.push_back(std::make_shared<DispatchVariableReturnDispatchFunction>(function[i], data));
        }
        return DispatchMany(fs);
    }
    std::vector<std::shared_ptr<DispatchHandle>> Dispatch(const std::vector<VariableReturnDispatchFunction>& function, std::any data)
    {
        return Dispatch((VariableReturnDispatchFunction*)&function[0], (int)function.size(), data);
    }

    std::vector<std::shared_ptr<DispatchHandle>> Dispatch(VariableReturnDispatchFunction function, int n, std::vector<std::any> data)
    {
        std::vector<std::shared_ptr<DispatchRunner>> fs;
        fs.reserve(n);
        for(int i = 0; i < n; i++)
        {
            fs.push_back(std::make_shared<DispatchVariableReturnDispatchFunction>(function, data[i]));
        }
        return DispatchMany(fs);
    }
    std::vector<std::shared_ptr<DispatchHandle>> Dispatch(VariableReturnDispatchFunction* function, int n, std::vector<std::any> data)
    {
        std::vector<std::shared_ptr<DispatchRunner>> fs;
        fs.reserve(n);
        for(int i = 0; i < n; i++)
        {
            fs.push_back(std::make_shared<DispatchVariableReturnDispatchFunction>(function[n], data[i]));
        }
        return DispatchMany(fs);
    }
    std::vector<std::shared_ptr<DispatchHandle>> Dispatch(const std::vector<VariableReturnDispatchFunction>& function, std::vector<std::any> data)
    {
        return Dispatch((VariableReturnDispatchFunction*)&function[0], (int)function.size(), data);
    }

    ThreadPool(int nthreads)
    {
        threads.reserve(nthreads);
        for(int i = 0; i < nthreads; i++)
        {
            auto ts =std::make_shared<ThreadState>();
            ts->thread = std::thread(this, ThreadPool::WorkingThread, ts);
            threads.push_back(ts);
        }
    }
    ~ThreadPool()
    {
        kill = true;
        for(auto thread : threads)
        {
            thread->v.notify_one();
            thread->thread.join();
        }
    }
};

END_RED_NAMESPACE;

