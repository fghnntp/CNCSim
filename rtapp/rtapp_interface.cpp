#include "rtapp_interface.h"
#include "rtapp.h"
#include <cstring>
#include <stdexcept>

// Concrete implementation of the interface
class RtAppImplementation : public IRtAppInterface {
public:
    RtAppImplementation(char* emcfile) {
        // Initialization code
        rtApp_ = new RTAPP(emcfile);
    }

    ~RtAppImplementation() override {
        // Cleanup code
    }

    void initialize() override {
        // Initialization logic
        rtApp_->doWork();
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
private:
    RTAPP *rtApp_;
};

// Factory function
IRtAppInterface* IRtAppInterface::create(char* emcFile) {
    return new RtAppImplementation(emcFile);
}

// C interface implementation
struct RtAppInterface {
    IRtAppInterface* impl;
};

extern "C" {
    RtAppInterface* api_interface_create() {
        RtAppInterface* wrapper = new RtAppInterface;
        wrapper->impl = IRtAppInterface::create();
        return wrapper;
    }

    void api_interface_initialize(RtAppInterface* handle) {
        if (handle && handle->impl) {
            handle->impl->initialize();
        }
    }

    void api_interface_process_data(RtAppInterface* handle, const char* input, char* output, int size) {
        if (handle && handle->impl) {
            try {
                handle->impl->processData(input, output, size);
            } catch (...) {
                // Handle or log error
                memset(output, 0, size);
            }
        }
    }

    void api_interface_shutdown(RtAppInterface* handle) {
        if (handle && handle->impl) {
            handle->impl->shutdown();
        }
    }

    void api_interface_destroy(RtAppInterface* handle) {
        if (handle) {
            delete handle->impl;
            delete handle;
        }
    }
}
