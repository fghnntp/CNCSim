#include "mill_task_interface.h"
#include "milltask.h"
#include "mottask.h"
#include <cstring>
#include <stdexcept>
#include "emcLog.h"
#include "cmdtask.h"
#include "motionTask.h"

// Concrete implementation of the interface
class MillTaskImplementation : public IMillTaskInterface {
public:
    MillTaskImplementation(const char* emcfile) {
        // Initialization code
        millTask_ = new MillTask(emcfile);
        motTask_ = new MotTask;
        cmdTask_ = new CmdTask;
    }

    ~MillTaskImplementation() override {
        // Cleanup code
    }

    void initialize() override {
        // Initialization logic
        millTask_->doWork();
        motTask_->doWork();
        cmdTask_->doWork();
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

        retval = millTask_->load_file(filename, &toolPath, err);

        return retval;
    }


    int simulate(const char *filename, std::string &res, std::string &err) override {

        return 0;
    }

    int getlog(std::string &log, int &level) override {
        return EMCLog::GetLog(log, level);
    }

    void setCmd(std::string &cmd) override {
        cmdTask_->SetCmd(cmd);
    }

    ToolPath getCarteCmdPos() override {
        return MotionTask::getCarteCmdPos();
    }

    void active_g_codes(int active_gcodes[]) override {
        millTask_->active_gcodes(active_gcodes);
    }

    void active_m_codes(int active_mcodes[]) override {
        millTask_->active_mcodes(active_mcodes);
    }

    void active_settings(double settings[]) override {
        millTask_->active_settings(settings);
    }

    void getFeedrateSacle(double &rapid, double &feed) override {
        MotionTask::getFeedrateSacle(rapid, feed);
    }

    void getMotionState(int &state) override {
        MotionTask::getMotionState(state);
    }

    void getMotionFlag(int &flag) override {
        MotionTask::getMotionFlag(flag);
    }

    void getCmdFbPosFlag(int &carte_pos_cmd_ok, int &carte_pos_fb_ok) override {
        MotionTask::getMotCmdFb(carte_pos_cmd_ok, carte_pos_fb_ok);
    }

    int isSoftLimit() override {
        return MotionTask::isSoftLimit();
    }

    double getDtg() override {
        return MotionTask::getMotDtg();
    }

    void getStateTag(state_tag_t &tag) override {
        MotionTask::getStateTag(tag);
    }

    void runInfo(double &curVel, double &reqVel) override {
        MotionTask::getRunInfo(curVel, reqVel);
    }

    int isJoggingActive() override {
        return MotionTask::isJogging();
    }


    int getKineType(void) override {
        return 0;
    }

    void setKineType(int type) override {

    }

private:
    MillTask *millTask_;
    MotTask *motTask_;
    CmdTask *cmdTask_;
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
