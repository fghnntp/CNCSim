/*    This is a component of LinuxCNC
 *    Copyright 2011 Michael Haberler <git@mah.priv.at>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#ifndef TASKCLASS_HH
#define TASKCLASS_HH

#include "emc.hh"
#include "inifile.hh"
#include "hal.hh"
#include "tooldata.hh"


class Task {
public:
    Task(EMC_IO_STAT &emcioStatus_in);
    virtual ~Task();

    // Don't copy or assign
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    virtual int emcIoInit();
    virtual int emcIoAbort(EMC_ABORT reason);
    virtual int emcAuxEstopOn();
    virtual int emcAuxEstopOff();
    virtual int emcCoolantMistOn();
    virtual int emcCoolantMistOff();
    virtual int emcCoolantFloodOn();
    virtual int emcCoolantFloodOff();
    virtual int emcToolSetOffset(int pocket, int toolno, const EmcPose& offset, double diameter,
				 double frontangle, double backangle, int orientation);
    virtual int emcToolPrepare(int tool);
    virtual int emcToolLoad();
    virtual int emcToolLoadToolTable(const char *file);
    virtual int emcToolUnload();
    virtual int emcToolSetNumber(int number);

    int iocontrol_hal_init(void);
    void reload_tool_number(int toolno);
    void load_tool(int idx);
    void run();
    int read_tool_inputs(void);
    void hal_init_pins(void);

    EMC_IO_STAT &emcioStatus;
    int random_toolchanger {0};
    iocontrol_str iocontrol_data;
    hal_comp iocontrol;
    const char *ini_filename;
    const char *tooltable_filename {};
    char db_program[LINELEN] {};
    tooldb_t db_mode {tooldb_t::DB_NOTUSED};
    int tool_status;
};

extern Task *task_methods;

#endif
