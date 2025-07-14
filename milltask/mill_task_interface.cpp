#include "mill_task_interface.h"
#include "milltask.h"
#include <cstring>
#include <stdexcept>

// Concrete implementation of the interface
class MillTaskImplementation : public IMillTaskInterface {
public:
    MillTaskImplementation(const char* emcfile) {
        // Initialization code
        millTask_ = new MillTask(emcfile);
    }

    ~MillTaskImplementation() override {
        // Cleanup code
    }

    void initialize() override {
        // Initialization logic
        millTask_->doWork();
    }

    void processData(const char* input, char* output, int size) override {
        if (!input || !output || size <= 0) {
            throw std::invalid_argument("Invalid input parameters");
        }

        // Example processing - reverse the string
        for (int i = 0; i < size; ++i) {
            output[i] = input[size - 1 - i];
        }
    }

    void shutdown() override {
        // Shutdown logic
    }

    int loadfile(const char *filename, std::vector<ToolPath> &toolPath, std::string &err) override {
        std::string name(filename);
        int retval = 0;

        retval = millTask_->taskMethods->load_file(filename, &toolPath, err);

        return retval;
    }
private:
    MillTask *millTask_;
};

// Factory function
IMillTaskInterface* IMillTaskInterface::create(const char* emcFile) {
    return new MillTaskImplementation(emcFile);
}

// C interface implementation
struct MillTaskInterface {
    IMillTaskInterface* impl;
};

extern "C" {
    MillTaskInterface* api_interface_create_mill() {
        MillTaskInterface* wrapper = new MillTaskInterface;
        wrapper->impl = IMillTaskInterface::create();
        return wrapper;
    }

    void api_interface_initialize_mill(MillTaskInterface* handle) {
        if (handle && handle->impl) {
            handle->impl->initialize();
        }
    }

    void api_interface_process_data_mill(MillTaskInterface* handle, const char* input, char* output, int size) {
        if (handle && handle->impl) {
            try {
                handle->impl->processData(input, output, size);
            } catch (...) {
                // Handle or log error
                memset(output, 0, size);
            }
        }
    }

    void api_interface_shutdown_mill(MillTaskInterface* handle) {
        if (handle && handle->impl) {
            handle->impl->shutdown();
        }
    }

    void api_interface_destroy_mill(MillTaskInterface* handle) {
        if (handle) {
            delete handle->impl;
            delete handle;
        }
    }
}
