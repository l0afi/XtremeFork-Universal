#pragma once

#include <fstream>
#include <memory>
#include <string>

class Logger {
public:
    static void Init(const std::string& filename = "XtremeTool.log");
    static void Log(const std::string& message);

private:
    static std::unique_ptr<std::ofstream> s_File;
};
