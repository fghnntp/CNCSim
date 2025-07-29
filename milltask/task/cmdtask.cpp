#include "cmdtask.h"
#include <chrono>
#include <iostream>
#include <libintl.h>
#include <rtapi_string.h>
#include "emcLog.h"
#include "Version.h"
#include "emcParas.h"
#include "emcChannel.h"
#include "iomanip"

CmdTask::CmdTask() : running(false) {
    init();
    EMCLog::SetLog("CmdTask start work");
}

CmdTask::~CmdTask() {
    stopWork(); // Ensure thread is stopped on destruction
}

void CmdTask::doWork() {
    if (running) return; // Already running

    running = true;
    workerThread = std::thread([this]() {
        EMCLog::SetLog("CmdTask start work");

        while (running) {
            // Process work
            process();

            // Small delay to prevent busy waiting
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Notify that we're finished
        if (finishedCallback) {
            finishedCallback();
        }
    });
}

void CmdTask::stopWork() {
    if (!running) return;

    running = false;
    cv.notify_all(); // Notify any waiting threads

    if (workerThread.joinable()) {
        workerThread.join();
    }
}

void CmdTask::process() {

    // Emit result through callback

    while (!cmdQueue.empty()) {
        auto cmdStr = cmdQueue.pop();

        if (cmdStr != "") {
            ExecuteCommand(cmdStr);
        }
    }

    if (resultCallback) {
        std::string result;
        resultCallback(result);
    }

}

void CmdTask::setResultCallback(std::function<void(const std::string&)> callback) {
    resultCallback = callback;
}

void CmdTask::setFinishedCallback(std::function<void()> callback) {
    finishedCallback = callback;
}


//These struct is crated by motion module, and these
//point is used to drive motion module
//motion module is quickly scanned, and alawys run
extern struct emcmot_struct_t *emcmotStruct;
/* ptrs to either buffered copies or direct memory for command and status */
extern struct emcmot_command_t *emcmotCommand;
extern struct emcmot_status_t *emcmotStatus;
extern struct emcmot_config_t *emcmotConfig;
extern struct emcmot_internal_t *emcmotInternal;
extern struct emcmot_error_t *emcmotError;	/* unused for RT_FIFO */

std::string getEmcPose(EmcPose pose)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(6);
    ss << "X:" << pose.tran.x << "\n" <<
          "Y:" << pose.tran.y << "\n" <<
          "Z:" << pose.tran.z << "\n" <<
          "A:" << pose.a << "\n" <<
          "B:" << pose.b << "\n" <<
          "C:" << pose.c << "\n" <<
          "U:" << pose.u << "\n" <<
          "V:" << pose.v << "\n" <<
          "W:" << pose.w;
    return ss.str();
}

std::string getModesStr(struct state_tag_t &tag)
{
    std::stringstream ss;
    if (tag.packed_flags & GM_FLAG_UNITS) {
        ss << "GM_FLAG_UNITS\n";
    }
    if (tag.packed_flags & GM_FLAG_DISTANCE_MODE) {
        ss << "GM_FLAG_DISTANCE_MODE\n";
    }
    if (tag.packed_flags & GM_FLAG_TOOL_OFFSETS_ON) {
        ss << "GM_FLAG_TOOL_OFFSETS_ON\n";
    }
    if (tag.packed_flags & GM_FLAG_RETRACT_OLDZ) {
        ss << "GM_FLAG_RETRACT_OLDZ\n";
    }
    if (tag.packed_flags & GM_FLAG_BLEND) {
        ss << "GM_FLAG_BLEND\n";
    }
    if (tag.packed_flags & GM_FLAG_EXACT_STOP) {
        ss << "GM_FLAG_EXACT_STOP\n";
    }
    if (tag.packed_flags & GM_FLAG_FEED_INVERSE_TIME) {
        ss << "GM_FLAG_FEED_INVERSE_TIME\n";
    }
    if (tag.packed_flags & GM_FLAG_FEED_UPM) {
        ss << "GM_FLAG_FEED_UPM\n";
    }
    if (tag.packed_flags & GM_FLAG_CSS_MODE) {
        ss << "GM_FLAG_CSS_MODE\n";
    }
    if (tag.packed_flags & GM_FLAG_IJK_ABS) {
        ss << "GM_FLAG_IJK_ABS\n";
    }
    if (tag.packed_flags & GM_FLAG_DIAMETER_MODE) {
        ss << "GM_FLAG_DIAMETER_MODE\n";
    }
    if (tag.packed_flags & GM_FLAG_G92_IS_APPLIED) {
        ss << "GM_FLAG_G92_IS_APPLIED\n";
    }
    if (tag.packed_flags & GM_FLAG_SPINDLE_ON) {
        ss << "GM_FLAG_SPINDLE_ON\n";
    }
    if (tag.packed_flags & GM_FLAG_SPINDLE_CW) {
        ss << "GM_FLAG_SPINDLE_CW\n";
    }
    if (tag.packed_flags & GM_FLAG_MIST) {
        ss << "GM_FLAG_MIST\n";
    }
    if (tag.packed_flags & GM_FLAG_FLOOD) {
        ss << "GM_FLAG_FLOOD\n";
    }
    if (tag.packed_flags & GM_FLAG_FEED_OVERRIDE) {
        ss << "GM_FLAG_FEED_OVERRIDE\n";
    }
    if (tag.packed_flags & GM_FLAG_SPEED_OVERRIDE) {
        ss << "GM_FLAG_SPEED_OVERRIDE\n";
    }
    if (tag.packed_flags & GM_FLAG_ADAPTIVE_FEED) {
        ss << "GM_FLAG_ADAPTIVE_FEED\n";
    }
    if (tag.packed_flags & GM_FLAG_FEED_HOLD) {
        ss << "GM_FLAG_FEED_HOLD\n";
    }
    if (tag.packed_flags & GM_FLAG_RESTORABLE) {
        ss << "GM_FLAG_RESTORABLE\n";
    }
    if (tag.packed_flags & GM_FLAG_IN_REMAP) {
        ss << "GM_FLAG_IN_REMAP\n";
    }
    if (tag.packed_flags & GM_FLAG_IN_SUB) {
        ss << "GM_FLAG_IN_SUB\n";
    }
    if (tag.packed_flags & GM_FLAG_EXTERNAL_FILE) {
        ss << "GM_FLAG_EXTERNAL_FILE\n";
    }
    return ss.str();
}

std::string getModesDetialStr(struct state_tag_t &tag)
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(6);

    ss << "LINE_NUMBER: " << tag.fields[GM_FIELD_LINE_NUMBER] << "\n" <<
          "G_MODE_0: "  << tag.fields[GM_FIELD_G_MODE_0] << "\n" <<
          "CUTTER_COMP: " << tag.fields[GM_FIELD_CUTTER_COMP] << "\n" <<
          "MOTION_MODE: " << tag.fields[GM_FIELD_MOTION_MODE] << "\n" <<
          "PLANE: " << tag.fields[GM_FIELD_PLANE] << "\n" <<
          "M_MODES_4: " << tag.fields[GM_FIELD_M_MODES_4] << "\n" <<
          "ORIGIN: " << tag.fields[GM_FIELD_ORIGIN] << "\n" <<
          "TOOLCHANGE: " << tag.fields[GM_FIELD_TOOLCHANGE] << "\n" <<
          "FLOAT_LINE_NUMBER: " << tag.fields_float[GM_FIELD_FLOAT_LINE_NUMBER] << "\n" <<
          "FLOAT_FEED: " << tag.fields_float[GM_FIELD_FLOAT_FEED] << "\n" <<
          "FLOAT_SPEED: " << tag.fields_float[GM_FIELD_FLOAT_SPEED] << "\n" <<
          "FLOAT_PATH_TOLERANCE: " << tag.fields_float[GM_FIELD_FLOAT_PATH_TOLERANCE] << "\n" <<
          "FLOAT_NAIVE_CAM_TOLERANCE: " << tag.fields_float[GM_FIELD_FLOAT_NAIVE_CAM_TOLERANCE];

    return ss.str();
}

#include "motionhalctrl.h"

void CmdTask::init()
{

    RegisterCommand("KIN", [this](const std::vector<std::string>& args) -> std::string {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(6);
        if (args.size() == 0) {
            ss << "KineType = "<< MotHalCtrl::get_kine_type();
        }
        else if (args.size() == 1) {
            int type = std::stoi(args[0]);
            MotHalCtrl::set_kine_type(type);
            ss << "Set KineType";
        }
        else {
            ss << "Wrong KIN";
        }

        return ss.str();

        });

    RegisterCommand("JINC", [this](const std::vector<std::string>& args) -> std::string {
        int joint = 0, axis = -1;
        double vel = 0.0, offset = 0.0;

        if (args.size() == 2) {
            //use max velocity to jog
            joint = std::stod(args[0]);
            offset = std::stof(args[1]);
            vel = EMCParas::GetJointConfig(joint)->MaxVel;
        }
        else if (args.size() == 3) {
            //use specify velocity to jog
            joint = std::stod(args[0]);
            offset = std::stof(args[1]);
            vel = std::stof(args[2]);
        }
        else {
            return "wrong";
        }

        EMCChannel::emcMotJogInc(joint, axis, vel, offset);

        return "done";
        });

    RegisterCommand("JABS", [this](const std::vector<std::string>& args) -> std::string {
        int joint = 0, axis = -1;
        double vel = 0.0, offset = 0.0;

        if (args.size() == 2) {
            //use max velocity to jog
            joint = std::stod(args[0]);
            offset = std::stof(args[1]);
            vel = EMCParas::GetJointConfig(joint)->MaxVel;
        }
        else if (args.size() == 3) {
            //use specify velocity to jog
            joint = std::stod(args[0]);
            offset = std::stof(args[1]);
            vel = std::stof(args[2]);
        }
        else {
            return "wrong";
        }

        EMCChannel::emcMotJogAbs(joint, axis, vel, offset);

        return "done";
        });

    RegisterCommand("AINC", [this](const std::vector<std::string>& args) -> std::string {
        int joint = -1, axis = 0;
        double vel = 0.0, offset = 0.0;

        if (args.size() == 2) {
            //use max velocity to jog
            axis = std::stod(args[0]);
            offset = std::stof(args[1]);
            vel = EMCParas::GetAxisConfig(axis)->MaxVel;
        }
        else if (args.size() == 3) {
            //use specify velocity to jog
            axis = std::stod(args[0]);
            offset = std::stof(args[1]);
            vel = std::stof(args[2]);
        }
        else {
            return "wrong";
        }

        EMCChannel::emcMotJogInc(joint, axis, vel, offset);

        return "done";
        });

    RegisterCommand("AABS", [this](const std::vector<std::string>& args) -> std::string {
        int joint = -1, axis = 0;
        double vel = 0.0, offset = 0.0;

        if (args.size() == 2) {
            //use max velocity to jog
            axis = std::stod(args[0]);
            offset = std::stof(args[1]);
            vel = EMCParas::GetAxisConfig(axis)->MaxVel;
        }
        else if (args.size() == 3) {
            //use specify velocity to jog
            axis = std::stod(args[0]);
            offset = std::stof(args[1]);
            vel = std::stof(args[2]);
        }
        else {
            return "wrong";
        }

        EMCChannel::emcMotJogAbs(joint, axis, vel, offset);

        return "done";
        });

    RegisterCommand("MOTMODE", [this](const std::vector<std::string>& args) -> std::string {
        std::stringstream ss;

        struct state_tag_t &tag = emcmotStatus->tag;

        ss << "In file: " << tag.filename << "\n" <<
              "Modes: " << getModesStr(tag) << "\n" <<
              "Modes detial info:\n" << getModesDetialStr(tag) << "\n";

        return ss.str();
        });


    RegisterCommand("MOTPOS", [this](const std::vector<std::string>& args) -> std::string {
        std::stringstream ss;

        ss << std::fixed << std::setprecision(6);
        ss << "carte_pos_cmd:\n" << getEmcPose(emcmotStatus->carte_pos_cmd) << "\n" <<
              "carte_pos_fb:\n" << getEmcPose(emcmotStatus->carte_pos_fb) << "\n" <<
              "world_home:\n" << getEmcPose(emcmotStatus->world_home);

        return ss.str();
        });

    RegisterCommand("MOTSTS", [this](const std::vector<std::string>& args) -> std::string {
        std::stringstream ss;
        std::string motionStateStr;
        switch (emcmotStatus->motion_state) {
        case EMCMOT_MOTION_DISABLED:
            motionStateStr = "EMCMOT_MOTION_DISABLED";
            break;
        case EMCMOT_MOTION_FREE:
            motionStateStr = "EMCMOT_MOTION_FREE";
            break;
        case EMCMOT_MOTION_TELEOP:
            motionStateStr = "EMCMOT_MOTION_TELEOP";
            break;
        case EMCMOT_MOTION_COORD:
            motionStateStr = "EMCMOT_MOTION_COORD";
            break;
        default:
            motionStateStr = "EMCMOT_MOTION_UNKNOW";
            break;
        }

        std::string motionStsStr = "MotionSts:";
        if (emcmotStatus->motionFlag & EMCMOT_MOTION_ENABLE_BIT)
            motionStsStr += " enable ";
        if (emcmotStatus->motionFlag & EMCMOT_MOTION_INPOS_BIT)
            motionStsStr += " inpos ";
        if (emcmotStatus->motionFlag & EMCMOT_MOTION_COORD_BIT)
            motionStsStr += " coord ";
        if (emcmotStatus->motionFlag & EMCMOT_MOTION_ERROR_BIT)
            motionStsStr += " error ";
        if (emcmotStatus->motionFlag & EMCMOT_MOTION_TELEOP_BIT)
            motionStsStr += " teleop ";

        std::string softLimitStr;
        if (emcmotStatus->on_soft_limit)
            softLimitStr = "on_soft_limit";
        else
            softLimitStr = "no_soft_limit";


        ss << "emcmot_status_t\n" <<
              "feed_scale " << emcmotStatus->feed_scale << "\n" <<
              "rapid_scale " << emcmotStatus->rapid_scale << "\n" <<
              "motion_state " << motionStateStr << "\n" <<
              "motionFlag " << motionStsStr << "\n" <<
              "soft_limit " << softLimitStr;


        return ss.str();
        });


    RegisterCommand("RUN", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;

        EMCChannel::emitMillCmd(EMCChannel::kMillAuto);

        return res;
        });

    RegisterCommand("STOP", [this](const std::vector<std::string>& args) -> std::string {
       std::string res;
       std::stringstream ss;

       return res;
       });

    RegisterCommand("$$", [this](const std::vector<std::string>& args) -> std::string {
       std::string res;
       std::stringstream ss;

       return res;
       });

    RegisterCommand("VER", [this](const std::vector<std::string>& args) -> std::string {
       std::string res;
       std::stringstream ss;
       ss << "CNCSIMU Version: " << CNCSIMU::version_str << std::endl;
       ss << "MILL Version: " << CNCSIMU::MILLVersion::str << std::endl;
       ss << "TOOL Version: " << CNCSIMU::TOOLVersion::str << std::endl;
       ss << "LinuxCNC Version: " << CNCSIMU::linuxcnc_version << std::endl;
       ss << "Build Time: " << CNCSIMU::build_date << " " << CNCSIMU::build_time <<  std::endl;
       ss << "Git Hash: " << CNCSIMU::git_commit_hash;

       res = ss.str();
       return res;
       });

    RegisterCommand(".", [this](const std::vector<std::string>& args) -> std::string {
       std::string res;
       std::stringstream ss;


       res = ss.str();
       return res;
       });


    RegisterCommand(".=", [this](const std::vector<std::string>& args) -> std::string {
       std::string res;
       std::stringstream ss;

       return res;
       });

    //These cmds are important, it can directly control the emcs paras
    //It means, it can change emc paras in anytime.
    RegisterCommand("TRAJ", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;
        if (args.size() == 0) {
            res = EMCParas::showTrajParas();
        }
        else if (args.size() == 2) {
            if (args[0] == "MAXACC") {
                EMCParas::emcTrajSetMaxAcceleration_(std::stod(args[1]));
            }
            else if (args[0] == "MAXVEL") {
                EMCParas::emcTrajSetMaxVelocity_(std::stod(args[1]));
            }
            else if (args[0] == "VEL") {
                EMCParas::emcTrajSetVelocity_(std::stod(args[1]), -1.0);
            }
            else if (args[0] == "ACC") {
                EMCParas::emcTrajSetAcceleration_(std::stod(args[1]));
            }
            else;
        }
        else;

        return res;
        });

    RegisterCommand("JOINT", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;
        if (args.size() == 1) {
           res = EMCParas::showJoint(std::stoi(args[0]));
        }
        else if (args.size() == 2) {

        }
        return res;
        });

    RegisterCommand("AXIS", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;
        if (args.size() == 1) {
            res = EMCParas::showAxis(std::stoi(args[0]));
        }
            return res;
        });

    RegisterCommand("SPINDLE", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;
        if (args.size() == 1) {
           res = EMCParas::showSpindle(std::stoi(args[0]));
        }
        return res;
        });

    //EMC Cmd Direct Set
    RegisterCommand("ABORT", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;
       
        EMCChannel::emcMotAbort(EMCChannel::kCmdChannel);

        return res;
        });

    RegisterCommand("ENABLE", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;

        EMCChannel::emcMotEna();

        return res;
        });

    RegisterCommand("DISABLE", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;

        EMCChannel::emcMotDisable();

        return res;
        });

    RegisterCommand("FREE", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;

        EMCChannel::emcMotFree();

        return res;
        });

    RegisterCommand("COOR", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;

        EMCChannel::emcMotCoord();

        return res;
        });

    RegisterCommand("TELEOP", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;

        EMCChannel::emcMotTeleop();

        return res;
        });

    RegisterCommand("PAUSE", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;

        EMCChannel::emcMotPause();

        return res;
        });

    RegisterCommand("REVERSE", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;

        EMCChannel::emcMotReverse();

        return res;
        });

    RegisterCommand("FORWARD", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;

        EMCChannel::emcMotForward();

        return res;
        });

    RegisterCommand("RESUME", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;

        EMCChannel::emcMotResume();

        return res;
        });

    RegisterCommand("STEP", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;

        EMCChannel::emcMotStep();

        return res;
        });

    RegisterCommand("FSENA", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;

        EMCChannel::emcMotFSEna();

        return res;
        });

    RegisterCommand("FHENA", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;

        EMCChannel::emcMotFHEna();

        return res;
        });

    RegisterCommand("FHENA", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;

        EMCChannel::emcMotFHEna();

        return res;
        });

    RegisterCommand("HOME", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;

        if (args.size() == 0) {
            EMCChannel::emcMotJointHome(-1);
        }
        else if (args.size() == 1) {
            int joint = std::stoi(args[0]);
            EMCChannel::emcMotJointHome(joint);
        }
        else {

        }


        EMCChannel::emcMotFHEna();

        return res;
        });

    RegisterCommand("RST", [this](const std::vector<std::string>& args) -> std::string {
        std::string res;
        std::stringstream ss;

        EMCChannel::emitMillCmd(EMCChannel::kMillRest);
        EMCChannel::emitMotCmd(EMCChannel::kMotRest);

        return res;
        });

}

void CmdTask::RegisterCommand(const std::string &name, CommandFunc func)
{
    std::lock_guard<std::mutex> lock(mutex_reg);
    _commandTable[name] = func;
}

std::pair<std::string, std::vector<std::string>> CmdTask::ParseCommand(std::string s)
{
    // 使用 transform 和 toupper
    std::transform(s.begin(), s.end(), s.begin(),
    [](unsigned char c){ return std::toupper(c); });

    std::istringstream iss(s);
    std::string name;

    iss >> name;

    std::vector<std::string> args;
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }

    return std::make_pair(name, args);
}


std::string CmdTask::ExecuteCommand(const std::string &rawCmd)
{
    typedef std::unordered_map<std::string, std::function<std::string(const std::vector<std::string>&)>> CommandTable;

    // 解析命令
    std::pair<std::string, std::vector<std::string>> parsed = ParseCommand(rawCmd);
    std::string& cmd = parsed.first;
    std::vector<std::string>& args = parsed.second;
    std::string result{"Cmd Not Found!"};

    EMCLog::SetLog("Do Cmd: " + cmd);

    // 执行注册函数
    CommandTable::const_iterator it = _commandTable.find(cmd);//Best match
    if (it != _commandTable.end()) {
        result = it->second(args);  // 调用注册的函数
    }

//    resQueue.push(result);
    EMCLog::SetLog("Cmd Res: " + result, 3);

    return result;
}

void CmdTask::SetCmd(const std::string &str)
{
    std::lock_guard<std::mutex> lock(mutex_cmd);
    cmdQueue.push(str);
}
