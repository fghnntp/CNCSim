#ifndef MILL_TASK_INTERFACE_H
#define MILL_TASK_INTERFACE_H
#ifdef __cplusplus
#include <vector>
#include <string>
// C++ interface
#define ACTIVE_G_CODES 17
#define ACTIVE_M_CODES 10
#define ACTIVE_SETTINGS 5
class IMillTaskInterface {
public:
    struct ToolPath {
        double x, y, z;
        double a, b, c;
        double u, v, w;
    };

    virtual ~IMillTaskInterface() = default;

    //Call to start start 3 thread
    //cmd function can be use
    //milltask function can be use
    //mottask function can be use
    virtual void initialize() = 0;
    //This is useless now
    virtual void processData(const char* input, char* output, int size) = 0;
    //This is uesless now
    virtual void shutdown() = 0;

    //load the file and get the tool path with nc, one line nc,
    //this will let called thread to interpter NC, it's fast
    virtual int loadfile(const char *filename, std::vector<ToolPath> &toolPath, std::string &err) = 0;
    // generator motion profile for configured tool machine,
    // this will send task to milltask, and milltask will let mottask do fastest simulation
    virtual int simulate(const char *filename, std::string &res, std::string &err) = 0;
    //level 0: message 1: warning 2:error 3:cmdline echo
    virtual int getlog(std::string &log, int &level) = 0;
    //do some command have been reigisted
    virtual void setCmd(std::string &cmd) = 0;

    virtual ToolPath getCarteCmdPos() = 0;
    virtual void active_g_codes(int active_gcodes[ACTIVE_G_CODES]) = 0;
    virtual void active_m_codes(int active_gcodes[ACTIVE_M_CODES]) = 0;
    virtual void active_settings(double settings[ACTIVE_SETTINGS]) = 0;

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

