#ifndef TOOL_MANAGER_H
#define TOOL_MANAGER_H

#include <map>
#include <vector>
#include <string>
#include <fstream>

struct ToolData {
    int tool_number;
    int pocket_number;
    double length_offset;
    double diameter_offset;
    std::string description;  // 可选：刀具描述
    
    ToolData() : tool_number(0), pocket_number(0), 
                 length_offset(0.0), diameter_offset(0.0) {}
    
    ToolData(int t, int p, double z, double d, const std::string& desc = "")
        : tool_number(t), pocket_number(p), length_offset(z), 
          diameter_offset(d), description(desc) {}

    bool operator<(const ToolData& other) const {
           // Example: Compare based on tool ID or another meaningful field
           return this->tool_number < other.tool_number;
    }
};

class ToolManager {
private:
    std::map<int, ToolData> tools_;  // key: tool_number
    std::string table_file_path_;
    
public:
    ToolManager(const std::string& table_file = "");
    ~ToolManager();
    
    // 基本操作
    bool load_tool_table(const std::string& filename);
    bool save_tool_table(const std::string& filename = "");
    
    // 刀具管理
    bool add_tool(const ToolData& tool);
    bool remove_tool(int tool_number);
    bool update_tool(int tool_number, const ToolData& tool);
    
    // 查询操作
    const ToolData* get_tool(int tool_number) const;
    std::vector<ToolData> get_all_tools() const;
    bool tool_exists(int tool_number) const;
    
    // 实用函数
    int get_tool_count() const;
    void clear_all_tools();
    bool is_pocket_occupied(int pocket_number) const;
    
    // 调试输出
    void print_tool_table() const;
};

#endif // TOOL_MANAGER_H
