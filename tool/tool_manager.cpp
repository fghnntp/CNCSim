#include "tool_manager.h"
#include <iostream>
#include <sstream>
#include <algorithm>

ToolManager::ToolManager(const std::string& table_file) 
    : table_file_path_(table_file) {
    if (!table_file.empty()) {
        load_tool_table(table_file);
    }
}

ToolManager::~ToolManager() {
    // 可选：自动保存
    if (!table_file_path_.empty()) {
        save_tool_table();
    }
}

bool ToolManager::load_tool_table(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open tool table file: " << filename << std::endl;
        return false;
    }
    
    tools_.clear();
    std::string line;
    int line_number = 0;
    
    while (std::getline(file, line)) {
        line_number++;
        
        // 跳过空行和注释
        if (line.empty() || line[0] == '#' || line[0] == ';') {
            continue;
        }
        
        // 解析刀具数据: T10 P10 Z10 D10
        std::istringstream iss(line);
        std::string token;
        ToolData tool;
        
        bool valid_line = true;
        while (iss >> token && valid_line) {
            if (token[0] == 'T' && token.length() > 1) {
                tool.tool_number = std::stoi(token.substr(1));
            } else if (token[0] == 'P' && token.length() > 1) {
                tool.pocket_number = std::stoi(token.substr(1));
            } else if (token[0] == 'Z' && token.length() > 1) {
                tool.length_offset = std::stod(token.substr(1));
            } else if (token[0] == 'D' && token.length() > 1) {
                tool.diameter_offset = std::stod(token.substr(1));
            } else {
                std::cerr << "Warning: Invalid token '" << token 
                         << "' at line " << line_number << std::endl;
                valid_line = false;
            }
        }
        
        if (valid_line && tool.tool_number > 0) {
            tools_[tool.tool_number] = tool;
        }
    }
    
    file.close();
    table_file_path_ = filename;
    return true;
}

bool ToolManager::save_tool_table(const std::string& filename) {
    std::string output_file = filename.empty() ? table_file_path_ : filename;
    if (output_file.empty()) {
        std::cerr << "Error: No filename specified for saving" << std::endl;
        return false;
    }
    
    std::ofstream file(output_file);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot write to file: " << output_file << std::endl;
        return false;
    }
    
    // 按刀具号排序输出
    std::vector<std::pair<int, ToolData>> sorted_tools(tools_.begin(), tools_.end());
    std::sort(sorted_tools.begin(), sorted_tools.end());
    
    for (const auto& pair : sorted_tools) {
        const ToolData& tool = pair.second;
        file << "T" << tool.tool_number 
             << "  P" << tool.pocket_number
             << "  Z" << tool.length_offset
             << "  D" << tool.diameter_offset;
        
        if (!tool.description.empty()) {
            file << "  ; " << tool.description;
        }
        file << std::endl;
    }
    
    file.close();
    return true;
}

bool ToolManager::add_tool(const ToolData& tool) {
    if (tool.tool_number <= 0) {
        std::cerr << "Error: Invalid tool number: " << tool.tool_number << std::endl;
        return false;
    }
    
    tools_[tool.tool_number] = tool;
    return true;
}

bool ToolManager::remove_tool(int tool_number) {
    auto it = tools_.find(tool_number);
    if (it != tools_.end()) {
        tools_.erase(it);
        return true;
    }
    return false;
}

const ToolData* ToolManager::get_tool(int tool_number) const {
    auto it = tools_.find(tool_number);
    return (it != tools_.end()) ? &(it->second) : nullptr;
}

std::vector<ToolData> ToolManager::get_all_tools() const {
    std::vector<ToolData> result;
    for (const auto& pair : tools_) {
        result.push_back(pair.second);
    }
    return result;
}

bool ToolManager::tool_exists(int tool_number) const {
    return tools_.find(tool_number) != tools_.end();
}

void ToolManager::print_tool_table() const {
    std::cout << "Tool Table (" << tools_.size() << " tools):" << std::endl;
    std::cout << "T#\tP#\tZ-Offset\tD-Offset\tDescription" << std::endl;
    std::cout << "---\t---\t--------\t--------\t-----------" << std::endl;
    
    for (const auto& pair : tools_) {
        const ToolData& tool = pair.second;
        std::cout << "T" << tool.tool_number 
                  << "\tP" << tool.pocket_number
                  << "\t" << tool.length_offset
                  << "\t\t" << tool.diameter_offset
                  << "\t\t" << tool.description << std::endl;
    }
}

bool ToolManager::update_tool(int tool_number, const ToolData& tool) {
    auto it = tools_.find(tool_number);
    if (it != tools_.end()) {
        it->second = tool;
        return true;
    }
    return false;
}

int ToolManager::get_tool_count() const {
    return static_cast<int>(tools_.size());
}

void ToolManager::clear_all_tools() {
    tools_.clear();
}

bool ToolManager::is_pocket_occupied(int pocket_number) const {
    for (const auto& pair : tools_) {
        if (pair.second.pocket_number == pocket_number) {
            return true;
        }
    }
    return false;
}