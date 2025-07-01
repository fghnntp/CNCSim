#ifndef MILL_TASK_INTERFACE_H
#define MILL_TASK_INTERFACE_H
#ifdef __cplusplus
// C++ interface
class IMillTaskInterface {
public:
    virtual ~IMillTaskInterface() = default;

    virtual void initialize() = 0;
    virtual void processData(const char* input, char* output, int size) = 0;
    virtual void shutdown() = 0;

    // Factory function
    static IMillTaskInterface* create();
};

extern "C" {
#endif

// C interface
typedef struct MillTaskInterface MillTaskInterface;

// C function pointers matching the C++ interface
MillTaskInterface* api_interface_create();
void api_interface_initialize(MillTaskInterface* handle);
void api_interface_process_data(MillTaskInterface* handle, const char* input, char* output, int size);
void api_interface_shutdown(MillTaskInterface* handle);
void api_interface_destroy(MillTaskInterface* handle);

#ifdef __cplusplus
}
#endif

#endif // MILL_TASK_INTERFACE_H

