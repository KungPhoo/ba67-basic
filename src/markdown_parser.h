#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <regex>
#include <filesystem>

class MarkdownParser {
    public:
    MarkdownParser(const std::string& filename);

    private:
    std::string filename;
    std::vector<std::pair<std::string, int>> headings;
    std::vector<std::pair<std::string, std::string>> usageSections;
    std::string fileContent;

    void readMarkdown();

    void extractHeadingsAndUsage();

    void updateTOC();

    void writeHelpInclude(const std::string& path);

    public:
    static void ParseAndApplyManual();
};
