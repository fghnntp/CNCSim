#ifndef MILL_TASK_INTERFACE_H
#define MILL_TASK_INTERFACE_H
#ifdef __cplusplus
#include <vector>
#include <string>
// C++ interface
class IMillTaskInterface {
public:
    struct ToolPath {
        double x, y, z;
        double a, b, c;
        double u, v, w;
    };
    virtual ~IMillTaskInterface() = default;

    virtual void initialize() = 0;
    virtual void processData(const char* input, char* output, int size) = 0;
    virtual void shutdown() = 0;

    // load the file and get the tool path with nc, one line nc
    virtual int loadfile(const char *filename, std::vector<ToolPath> &toolPath, std::string &err) = 0;
    // generator motion profile for configured tool machine
    virtual int simulate(const char *filename, std::string &res, std::string &err) = 0;

    // Factory function
    static IMillTaskInterface* create(const char* emcfile = nullptr);
};

extern "C" {
#endif

// C interface
typedef struct MillTaskInterface MillTaskInterface;

// C function pointers matching the C++ interface
MillTaskInterface* api_interface_create_mill();
void api_interface_initialize_mill(MillTaskInterface* handle);
void api_interface_process_data_mill(MillTaskInterface* handle, const char* input, char* output, int size);
void api_interface_shutdown_mill(MillTaskInterface* handle);
void api_interface_destroy_mill(MillTaskInterface* handle);

#ifdef __cplusplus
}
#endif

#endif // MILL_TASK_INTERFACE_H

