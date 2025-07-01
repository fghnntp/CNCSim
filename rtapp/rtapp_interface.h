#ifndef MILL_TASK_INTERFACE_H
#define MILL_TASK_INTERFACE_H
#ifdef __cplusplus
// C++ interface
class IRtAppInterface {
public:
    virtual ~IRtAppInterface() = default;

    virtual void initialize() = 0;
    virtual void processData(const char* input, char* output, int size) = 0;
    virtual void shutdown() = 0;

    // Factory function
    static IRtAppInterface* create(char*emcfile = nullptr);
};

extern "C" {
#endif

// C interface
typedef struct RtAppInterface RtAppInterface;

// C function pointers matching the C++ interface
RtAppInterface* api_interface_create();
void api_interface_initialize(RtAppInterface* handle);
void api_interface_process_data(RtAppInterface* handle, const char* input, char* output, int size);
void api_interface_shutdown(RtAppInterface* handle);
void api_interface_destroy(RtAppInterface* handle);

#ifdef __cplusplus
}
#endif

#endif // MILL_TASK_INTERFACE_H

