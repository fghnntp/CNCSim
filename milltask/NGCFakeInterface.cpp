#include "NGCFakeInterface.h"

void SET_G5X_OFFSET(int origin, double x, double y, double z, double a, double b, double c, double u, double v, double w)
{

}

void SET_G92_OFFSET(double x, double y, double z, double a, double b, double c, double u, double v, double w)
{

}

void SET_XY_ROTATION(double t)
{

}

void CANON_UPDATE_END_POINT(double x, double y, double z, double a, double b, double c, double u, double v, double w)
{

}

void USE_LENGTH_UNITS(CANON_UNITS u)
{

}

void SELECT_PLANE(CANON_PLANE pl)
{

}

void SET_TRAVERSE_RATE(double rate)
{

}

void STRAIGHT_TRAVERSE(int lineno, double x, double y, double z, double a, double b, double c, double u, double v, double w)
{

}

void SET_FEED_RATE(double rate)
{

}

void SET_FEED_REFERENCE(CANON_FEED_REFERENCE reference)
{

}

void SET_FEED_MODE(int spindle, int mode)
{

}

void SET_MOTION_CONTROL_MODE(CANON_MOTION_MODE mode, double tolerance)
{

}

void SET_NAIVECAM_TOLERANCE(double tolerance)
{

}

void SET_CUTTER_RADIUS_COMPENSATION(double radius)
{

}

void START_CUTTER_RADIUS_COMPENSATION(int direction)
{

}

void STOP_CUTTER_RADIUS_COMPENSATION()
{

}

void START_SPEED_FEED_SYNCH(int spindle, double feed_per_revolution, bool velocity_mode)
{

}

void STOP_SPEED_FEED_SYNCH()
{

}

void ARC_FEED(int lineno, double first_end, double second_end, double first_axis, double second_axis, int rotation, double axis_end_point, double a, double b, double c, double u, double v, double w)
{

}

void STRAIGHT_FEED(int lineno, double x, double y, double z, double a, double b, double c, double u, double v, double w)
{

}

std::vector<unsigned int> nurbs_G5_knot_vector_creator(unsigned int n, unsigned int k)
{
    return std::vector<unsigned int>();
}

double nurbs_G5_Nmix(unsigned int i, unsigned int k, double u, const std::vector<unsigned int> &knot_vector)
{
    return 0.0;
}

double nurbs_G5_Rden(double u, unsigned int k, const std::vector<NURBS_CONTROL_POINT> &nurbs_control_points, const std::vector<unsigned int> &knot_vector)
{
    return 0.0;
}

NURBS_PLANE_POINT nurbs_G5_point(double u, unsigned int k, const std::vector<NURBS_CONTROL_POINT> &nurbs_control_points, const std::vector<unsigned int> &knot_vector)
{
    return NURBS_PLANE_POINT {0.0, 0.0};
}


NURBS_PLANE_POINT nurbs_G5_tangent(double u, unsigned int k, const std::vector<NURBS_CONTROL_POINT> &nurbs_control_points, const std::vector<unsigned int> &knot_vector)
{
    return NURBS_PLANE_POINT {0.0, 0.0};
}

std::vector<double> nurbs_g6_knot_vector_creator(unsigned int n, unsigned int k, const std::vector<NURBS_G6_CONTROL_POINT> &nurbs_control_points)
{

}

std::vector<double> nurbs_interval_span_knot_vector_creator(unsigned int n, unsigned int k, const std::vector<double> &knot_vector_)
{

}

double nurbs_lderv(double u, unsigned int k, const std::vector<NURBS_G6_CONTROL_POINT> &nurbs_control_points, const std::vector<double> &knot_vector_)
{

}

double nurbs_Sa1_b1_length_(double a1, double b1, unsigned int k, const std::vector<NURBS_G6_CONTROL_POINT> &nurbs_control_points, const std::vector<double> &knot_vector_)
{

}

std::vector<double> nurbs_lenght_vector_creator(unsigned int k, const std::vector<NURBS_G6_CONTROL_POINT> &nurbs_control_points, const std::vector<double> &knot_vector_, const std::vector<double> &span_knot_vector)
{

}

double nurbs_lenght_tot(int j, const std::vector<double> &span_knot_vector, const std::vector<double> &lenght_vector)
{

}

double nurbs_lenght_l_u(double u, unsigned int k, const std::vector<NURBS_G6_CONTROL_POINT> &nurbs_control_points, const std::vector<double> &knot_vector_, const std::vector<double> &span_knot_vector, const std::vector<double> &lenght_vector)
{

}

// Add default implementations for all missing functions from NGCFakeInterface.h

void INIT_CANON() {}

void NURBS_G5_FEED(int, const std::vector<NURBS_CONTROL_POINT>&, unsigned int, CANON_PLANE) {}
void NURBS_G6_FEED(int, const std::vector<NURBS_G6_CONTROL_POINT>&, unsigned int, double, int, CANON_PLANE) {}
double alpha_finder(double, double) { return 0.0; }
void RIGID_TAP(int, double, double, double, double) {}
void STRAIGHT_PROBE(int, double, double, double, double, double, double, double, double, double, unsigned char) {}
void STOP() {}
void DWELL(double) {}
void SET_SPINDLE_MODE(int, double) {}
void SPINDLE_RETRACT_TRAVERSE() {}
void START_SPINDLE_CLOCKWISE(int, int) {}
void START_SPINDLE_COUNTERCLOCKWISE(int, int) {}
void SET_SPINDLE_SPEED(int, double) {}
void STOP_SPINDLE_TURNING(int) {}
void SPINDLE_RETRACT() {}
void ORIENT_SPINDLE(int, double, int) {}
void WAIT_SPINDLE_ORIENT_COMPLETE(int, double) {}
void LOCK_SPINDLE_Z() {}
void USE_SPINDLE_FORCE() {}
void USE_NO_SPINDLE_FORCE() {}
void SET_TOOL_TABLE_ENTRY(int, int, const EmcPose&, double, double, double, int) {}
void USE_TOOL_LENGTH_OFFSET(const EmcPose&) {}
void CHANGE_TOOL() {}
void SELECT_TOOL(int) {}
void CHANGE_TOOL_NUMBER(int) {}
void RELOAD_TOOLDATA() {}
void CLAMP_AXIS(CANON_AXIS) {}
void COMMENT(const char*) {}
void DISABLE_ADAPTIVE_FEED() {}
void ENABLE_ADAPTIVE_FEED() {}
void DISABLE_FEED_OVERRIDE() {}
void ENABLE_FEED_OVERRIDE() {}
void DISABLE_SPEED_OVERRIDE(int) {}
void ENABLE_SPEED_OVERRIDE(int) {}
void DISABLE_FEED_HOLD() {}
void ENABLE_FEED_HOLD() {}
void FLOOD_OFF() {}
void FLOOD_ON() {}
void MESSAGE(char*) {}
void LOG(char*) {}
void LOGOPEN(char*) {}
void LOGAPPEND(char*) {}
void LOGCLOSE() {}
void MIST_OFF() {}
void MIST_ON() {}
void PALLET_SHUTTLE() {}
void TURN_PROBE_OFF() {}
void TURN_PROBE_ON() {}
void UNCLAMP_AXIS(CANON_AXIS) {}
void NURB_KNOT_VECTOR() {}
void NURB_CONTROL_POINT(int, double, double, double, double) {}
void NURB_FEED(double, double) {}
void SET_BLOCK_DELETE(bool) {}
bool GET_BLOCK_DELETE(void) { return false; }
void OPTIONAL_PROGRAM_STOP() {}
void SET_OPTIONAL_PROGRAM_STOP(bool) {}
bool GET_OPTIONAL_PROGRAM_STOP() { return false; }
void PROGRAM_END() {}
void PROGRAM_STOP() {}
void SET_MOTION_OUTPUT_BIT(int) {}
void CLEAR_MOTION_OUTPUT_BIT(int) {}
void SET_AUX_OUTPUT_BIT(int) {}
void CLEAR_AUX_OUTPUT_BIT(int) {}
void SET_MOTION_OUTPUT_VALUE(int, double) {}
void SET_AUX_OUTPUT_VALUE(int, double) {}
int WAIT(int, int, int, double) { return -1; }
int UNLOCK_ROTARY(int, int) { return 0; }
int LOCK_ROTARY(int, int) { return 0; }
double GET_EXTERNAL_FEED_RATE() { return 0.0; }
int GET_EXTERNAL_FLOOD() { return 0; }
CANON_UNITS GET_EXTERNAL_LENGTH_UNIT_TYPE() { return CANON_UNITS_MM; }
double GET_EXTERNAL_LENGTH_UNITS() { return 1.0; }
double GET_EXTERNAL_ANGLE_UNITS() { return 1.0; }
int GET_EXTERNAL_MIST() { return 0; }
CANON_MOTION_MODE GET_EXTERNAL_MOTION_CONTROL_MODE() { return CANON_EXACT_STOP; }
double GET_EXTERNAL_MOTION_CONTROL_TOLERANCE() { return 0.0; }
double GET_EXTERNAL_MOTION_CONTROL_NAIVECAM_TOLERANCE() { return 0.0; }
void GET_EXTERNAL_PARAMETER_FILE_NAME(char* filename, int max_size) { if (filename && max_size > 0) filename[0] = '\0'; }
void SET_PARAMETER_FILE_NAME(const char*) {}
CANON_PLANE GET_EXTERNAL_PLANE() { return CANON_PLANE::XY; }
double GET_EXTERNAL_POSITION_A() { return 0.0; }
double GET_EXTERNAL_POSITION_B() { return 0.0; }
double GET_EXTERNAL_POSITION_C() { return 0.0; }
double GET_EXTERNAL_POSITION_X() { return 0.0; }
double GET_EXTERNAL_POSITION_Y() { return 0.0; }
double GET_EXTERNAL_POSITION_Z() { return 0.0; }
double GET_EXTERNAL_POSITION_U() { return 0.0; }
double GET_EXTERNAL_POSITION_V() { return 0.0; }
double GET_EXTERNAL_POSITION_W() { return 0.0; }
double GET_EXTERNAL_PROBE_POSITION_A() { return 0.0; }
double GET_EXTERNAL_PROBE_POSITION_B() { return 0.0; }
double GET_EXTERNAL_PROBE_POSITION_C() { return 0.0; }
double GET_EXTERNAL_PROBE_POSITION_X() { return 0.0; }
double GET_EXTERNAL_PROBE_POSITION_Y() { return 0.0; }
double GET_EXTERNAL_PROBE_POSITION_Z() { return 0.0; }
double GET_EXTERNAL_PROBE_POSITION_U() { return 0.0; }
double GET_EXTERNAL_PROBE_POSITION_V() { return 0.0; }
double GET_EXTERNAL_PROBE_POSITION_W() { return 0.0; }
double GET_EXTERNAL_PROBE_VALUE() { return 0.0; }
int GET_EXTERNAL_PROBE_TRIPPED_VALUE() { return 0; }
int GET_EXTERNAL_QUEUE_EMPTY() { return 1; }
double GET_EXTERNAL_SPEED(int) { return 0.0; }
CANON_DIRECTION GET_EXTERNAL_SPINDLE(int) { return CANON_STOPPED; }
double GET_EXTERNAL_TOOL_LENGTH_XOFFSET() { return 0.0; }
double GET_EXTERNAL_TOOL_LENGTH_YOFFSET() { return 0.0; }
double GET_EXTERNAL_TOOL_LENGTH_ZOFFSET() { return 0.0; }
double GET_EXTERNAL_TOOL_LENGTH_AOFFSET() { return 0.0; }
double GET_EXTERNAL_TOOL_LENGTH_BOFFSET() { return 0.0; }
double GET_EXTERNAL_TOOL_LENGTH_COFFSET() { return 0.0; }
double GET_EXTERNAL_TOOL_LENGTH_UOFFSET() { return 0.0; }
double GET_EXTERNAL_TOOL_LENGTH_VOFFSET() { return 0.0; }
double GET_EXTERNAL_TOOL_LENGTH_WOFFSET() { return 0.0; }
int GET_EXTERNAL_TOOL_SLOT() { return 0; }
int GET_EXTERNAL_SELECTED_TOOL_SLOT() { return -1; }
CANON_TOOL_TABLE GET_EXTERNAL_TOOL_TABLE(int) { return CANON_TOOL_TABLE(); }
int GET_EXTERNAL_TC_FAULT() { return 0; }
int GET_EXTERNAL_TC_REASON() { return 0; }
double GET_EXTERNAL_TRAVERSE_RATE() { return 0.0; }
int GET_EXTERNAL_FEED_OVERRIDE_ENABLE() { return 0; }
int GET_EXTERNAL_SPINDLE_OVERRIDE_ENABLE(int) { return 0; }
int GET_EXTERNAL_ADAPTIVE_FEED_ENABLE() { return 0; }
int GET_EXTERNAL_FEED_HOLD_ENABLE() { return 0; }
int GET_EXTERNAL_DIGITAL_INPUT(int, int def) { return def; }
double GET_EXTERNAL_ANALOG_INPUT(int, double def) { return def; }
int GET_EXTERNAL_AXIS_MASK() { return 0; }
int USER_DEFINED_FUNCTION_ADD(USER_DEFINED_FUNCTION_TYPE, int) { return 0; }
void FINISH(void) {}
void ON_RESET(void) {}
void CANON_ERROR(const char *fmt, ...) {}
int GET_EXTERNAL_OFFSET_APPLIED() { return 0; }
EmcPose GET_EXTERNAL_OFFSETS() { return EmcPose(); }
void UPDATE_TAG(const StateTag&) {}

#include "python_plugin.hh"

int _task = 0; // control preview behaviour when remapping
extern "C" PyObject* PyInit_interpreter(void);
extern "C" PyObject* PyInit_emccanon(void);
struct _inittab builtin_modules[] = {
    { "interpreter", PyInit_interpreter },
    { "emccanon", PyInit_emccanon },
    // any others...
    { NULL, NULL }
};


USER_DEFINED_FUNCTION_TYPE USER_DEFINED_FUNCTION[USER_DEFINED_FUNCTION_NUM]
    = { 0 };
